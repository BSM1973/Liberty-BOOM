#pragma once
// BOOM / Liberty Music Software
// ITU-R BS.1770 style K-weighted loudness / EBU R128 style gated integrated metering.
// Four-times oversampled, windowed-sinc true-peak ESTIMATE (not conformance certified).
// All memory is allocated in prepare(); processSample() is allocation- and lock-free.
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace boom {
namespace meters {
constexpr double PI=3.141592653589793238462643383279502884;
inline float toDb(float x) noexcept {return 20.f*std::log10(std::max(1.e-12f,x));}
inline float loudness(double meanSquare) noexcept {
 return meanSquare>1.e-13 ? static_cast<float>(-0.691+10.0*std::log10(meanSquare)) : -100.f;
}
struct Filter {
 double b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
 void reset() noexcept {z1=z2=0;}
 double tick(double x) noexcept {
  const double y=b0*x+z1;
  z1=b1*x-a1*y+z2;
  z2=b2*x-a2*y;
  if(std::abs(z1)<1.e-24)z1=0;
  if(std::abs(z2)<1.e-24)z2=0;
  return y;
 }
 static Filter kShelf(double fs) noexcept {
  // K-weighting prefilter based on the BS.1770 reference filter parameters.
  constexpr double G=3.999843853973347, Q=0.7071752369554196, f=1681.974450955533;
  const double V=std::pow(10.0,G/20), Vb=std::pow(V,0.4996667741545416);
  const double K=std::tan(PI*f/fs),d=1+K/Q+K*K;
  Filter out;
  out.b0=(V+Vb*K/Q+K*K)/d;
  out.b1=2*(K*K-V)/d;
  out.b2=(V-Vb*K/Q+K*K)/d;
  out.a1=2*(K*K-1)/d;
  out.a2=(1-K/Q+K*K)/d;
  return out;
 }
 static Filter kHighPass(double fs) noexcept {
  constexpr double Q=0.5003270373238773, f=38.13547087602444;
  const double K=std::tan(PI*f/fs),d=1+K/Q+K*K;
  Filter out;
  out.b0=1/d;out.b1=-2/d;out.b2=1/d;
  out.a1=2*(K*K-1)/d;out.a2=(1-K/Q+K*K)/d;
  return out;
 }
};

// Fixed-size 16-tap fractional-delay interpolator. These values are only an
// indication of dBTP; final limiting compliance needs external tests.
class TruePeakEstimator {
public:
 void prepare() noexcept {
  for(int phase=0;phase<4;++phase){
   double total=0;
   for(int tap=0;tap<16;++tap){
    const double x=double(tap)-7.0-double(phase)*0.25;
    const double sinc=std::abs(x)<1.e-12?1.0:std::sin(PI*x)/(PI*x);
    const double window=0.42+0.50*std::cos(PI*x/8.0)+0.08*std::cos(PI*x/4.0);
    weights[phase][tap]=sinc*std::max(0.0,window);
    total+=weights[phase][tap];
   }
   for(auto& weight:weights[phase])weight/=total;
  }
  history={};index=0;
 }
 float tick(float l,float r) noexcept {
  history[index]={l,r};
  const int newest=static_cast<int>(index);
  index=(index+1u)&15u;
  float maximum=std::max(std::abs(l),std::abs(r));
  for(const auto& phase:weights){
   double sl=0,sr=0;
   for(int k=0;k<16;++k){
    const auto& sample=history[static_cast<std::size_t>((newest-k)&15)];
    sl+=phase[k]*sample[0];sr+=phase[k]*sample[1];
   }
   maximum=std::max(maximum,static_cast<float>(std::max(std::abs(sl),std::abs(sr))));
  }
  return maximum;
 }
private:
 std::array<std::array<double,16>,4> weights{};
 std::array<std::array<float,2>,16> history{};
 std::size_t index=0;
};

struct Snapshot {
 float momentary=-100.f,shortTerm=-100.f,integrated=-100.f,loudnessRange=0.f;
 float truePeak=0.f,correlation=0.f;
};
class LoudnessMeter {
public:
 void prepare(double sampleRate){
  fs=std::max(8000.0,sampleRate);
  momentaryN=std::max(1,int(std::round(fs*0.4)));
  shortN=std::max(momentaryN+1,int(std::round(fs*3.0)));
  blockN=std::max(1,int(std::round(fs*0.1)));
  k1={Filter::kShelf(fs),Filter::kShelf(fs)};
  k2={Filter::kHighPass(fs),Filter::kHighPass(fs)};
  ring.assign(static_cast<std::size_t>(shortN),0.0);
  pos=0;seen=0;momentarySum=shortSum=0;
  blockSamples=0;blockNumber=0;secondCounter=0;
  corrLR=corrL=corrR=0;
  peak.prepare();peakMax=0;resetIntegrated();
  s={};
 }
 void resetIntegrated() noexcept{
  histCount.fill(0);histEnergy.fill(0);shortHist.fill(0);
  s.integrated=-100.f;s.loudnessRange=0.f;blockNumber=0;
 }
 void tick(float l,float r) noexcept {
  const double xL=k2[0].tick(k1[0].tick(l));
  const double xR=k2[1].tick(k1[1].tick(r));
  const double energy=xL*xL+xR*xR;
  const double oldShort=seen>=static_cast<std::uint64_t>(shortN)?ring[pos]:0.0;
  const std::size_t oldMomentary=(pos+ring.size()-static_cast<std::size_t>(momentaryN))%ring.size();
  const double oldM=seen>=static_cast<std::uint64_t>(momentaryN)?ring[oldMomentary]:0.0;
  ring[pos]=energy;pos=(pos+1u)%ring.size();++seen;
  momentarySum=std::max(0.0,momentarySum+energy-oldM);
  shortSum=std::max(0.0,shortSum+energy-oldShort);
  const double corrFactor=1-std::exp(-1.0/(0.3*fs));
  corrLR+=corrFactor*(double(l)*r-corrLR);
  corrL+=corrFactor*(double(l)*l-corrL);
  corrR+=corrFactor*(double(r)*r-corrR);
  s.correlation=(corrL>1.e-12 && corrR>1.e-12)?
      static_cast<float>(std::clamp(corrLR/std::sqrt(corrL*corrR),-1.0,1.0)):0.f;
  peakMax=std::max(peakMax,peak.tick(l,r));
  if(++blockSamples>=blockN){
   blockSamples=0;
   s.momentary=seen>=static_cast<std::uint64_t>(momentaryN)?loudness(momentarySum/momentaryN):-100.f;
   s.shortTerm=seen>=static_cast<std::uint64_t>(shortN)?loudness(shortSum/shortN):-100.f;
   if(s.momentary>-70.f){
    const int bin=toBin(s.momentary);
    ++histCount[bin];histEnergy[bin]+=momentarySum/momentaryN;
   }
   computeIntegrated();
   // 100ms LRA histogram cadence tracks the independent FFmpeg EBU R128
   // example more closely than 1s samples. Not a compliance declaration.
   if(++secondCounter>=1){
    secondCounter=0;
    if(s.shortTerm>-70.f)++shortHist[toBin(s.shortTerm)];
    computeRange();
   }
  }
 }
 Snapshot snapshotAndResetPeak() noexcept {s.truePeak=peakMax;peakMax=0;return s;}
private:
 static constexpr int bins=1001; // -70.0 to +30.0 LUFS in 0.1 LU steps
 static int toBin(float lufs) noexcept {
  return std::clamp(int(std::lround((lufs+70.f)*10.f)),0,bins-1);
 }
 void computeIntegrated() noexcept{
  double energy=0;std::uint64_t count=0;
  for(int i=0;i<bins;++i){energy+=histEnergy[i];count+=histCount[i];}
  if(count==0){s.integrated=-100;return;}
  const float relativeGate=loudness(energy/double(count))-10.f;
  const int threshold=toBin(std::max(-70.f,relativeGate));
  energy=0;count=0;
  for(int i=threshold;i<bins;++i){energy+=histEnergy[i];count+=histCount[i];}
  s.integrated=count?loudness(energy/double(count)):-100.f;
 }
 void computeRange() noexcept{
  // EBU Tech 3342 concept: distribution of 3s blocks, relative gate at I-20 LU,
  // 10th to 95th percentile. A formal R128 conformance suite is still required.
  const int threshold=toBin(std::max(-70.f,s.integrated-20.f));
  std::uint64_t count=0;
  for(int i=threshold;i<bins;++i)count+=shortHist[i];
  if(count<3){s.loudnessRange=0;return;}
  const auto lower=static_cast<std::uint64_t>(std::ceil(count*.1));
  const auto upper=static_cast<std::uint64_t>(std::ceil(count*.95));
  std::uint64_t running=0;int lo=threshold,hi=threshold;bool foundLo=false;
  for(int i=threshold;i<bins;++i){running+=shortHist[i];
   if(!foundLo && running>=lower){lo=i;foundLo=true;}
   if(running>=upper){hi=i;break;}
  }
  s.loudnessRange=std::max(0.f,float(hi-lo)*0.1f);
 }
 double fs=48000.0;
 int momentaryN=19200,shortN=144000,blockN=4800,blockSamples=0,blockNumber=0,secondCounter=0;
 std::array<Filter,2> k1{},k2{};
 std::vector<double> ring;
 std::size_t pos=0;std::uint64_t seen=0;
 double momentarySum=0,shortSum=0;
 double corrLR=0,corrL=0,corrR=0;
 TruePeakEstimator peak;
 float peakMax=0;
 std::array<std::uint64_t,bins> histCount{},shortHist{};
 std::array<double,bins> histEnergy{};
 Snapshot s;
};
} // namespace meters
} // namespace boom
