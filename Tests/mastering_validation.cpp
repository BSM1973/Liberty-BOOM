#include "../Source/BoomDSP.h"
#include <algorithm>
#ifdef NDEBUG
#undef NDEBUG // Never compile out validation checks in Release/CI.
#endif
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

constexpr int fs=48000;
constexpr double PI=boom::pi;

struct Signal {
 std::vector<float> l,r;
 explicit Signal(int count):l(count),r(count){}
 float peak()const {float out=0;for(float x:l)out=std::max(out,std::abs(x));return out;}
};
Signal sine(float hz,float amplitude,int seconds,float phase=0){
 Signal out(fs*seconds);
 for(int i=0;i<fs*seconds;++i)
  out.l[i]=out.r[i]=amplitude*std::sin(2*PI*hz*i/fs+phase);
 return out;
}
float rmsTail(const std::vector<float>& values){
 double s=0;const auto start=values.size()/2;
 for(std::size_t i=start;i<values.size();++i)s+=double(values[i])*values[i];
 return static_cast<float>(std::sqrt(s/(values.size()-start)));
}
boom::Parameters neutral(){
 boom::Parameters p;p.compRatio=1;p.compThreshold=0;
 p.deEssAmount=0;p.color=0;p.colorDrive=0;p.push=0;
 p.limiterThreshold=0;p.ceilingDb=0;p.outputDb=0;
 return p;
}
int main(){
 // A normal stereo sine has a well-defined integrated K-weighted loudness.
 boom::meters::LoudnessMeter loudness;loudness.prepare(fs);
 for(int i=0;i<fs*5;++i){
  const float x=0.1f*std::sin(2*PI*1000*i/fs);
  loudness.tick(x,x);
 }
 auto s=loudness.snapshotAndResetPeak();
 assert(s.momentary>-22&&s.momentary<-19);
 assert(std::abs(s.integrated-s.momentary)<0.4f);
 assert(s.shortTerm>-22&&s.shortTerm<-19);
 assert(s.correlation>0.99f);
 assert(s.truePeak>0.09f&&s.truePeak<0.12f);
 const float beforeSilence=s.integrated;
 for(int i=0;i<fs*6;++i)loudness.tick(0,0);
 s=loudness.snapshotAndResetPeak();
 // Silence is excluded by the absolute and relative gates.
 assert(std::abs(s.integrated-beforeSilence)<0.5f);
 loudness.resetIntegrated();
 s=loudness.snapshotAndResetPeak();assert(s.integrated==-100.f);

 // Anti-correlated stereo input registers approximately -1.
 loudness.prepare(fs);
 for(int i=0;i<fs;++i){const float x=0.2f*std::sin(2*PI*300*i/fs);loudness.tick(x,-x);}
 s=loudness.snapshotAndResetPeak();assert(s.correlation<-0.99f);

 // Windowed-sinc 4x estimator must detect peaks between actual samples.
 boom::meters::TruePeakEstimator truePeak;truePeak.prepare();
 float samplePeak=0,oversampledPeak=0;
 for(int i=0;i<4000;++i){
  const float x=std::sin(2*PI*6000*i/fs+PI/8.0);
  samplePeak=std::max(samplePeak,std::abs(x));
  if(i>30)oversampledPeak=std::max(oversampledPeak,truePeak.tick(x,x));
  else truePeak.tick(x,x);
 }
 assert(oversampledPeak>samplePeak+0.02f);

 // EQ bypass must measurably remove EQ processing, even at 3 kHz.
 auto p=neutral();p.eqDb[2]=8;
 boom::Engine engine;engine.prepare(fs,512);engine.setParameters(p);
 auto equalized=sine(3000,.025f,2);engine.process(equalized.l.data(),equalized.r.data(),static_cast<int>(equalized.l.size()));
 const float boosted=rmsTail(equalized.l);
 p.eqBypass=true;engine.prepare(fs,512);engine.setParameters(p);
 auto unEqualized=sine(3000,.025f,2);engine.process(unEqualized.l.data(),unEqualized.r.data(),static_cast<int>(unEqualized.l.size()));
 assert(boosted>rmsTail(unEqualized.l)*1.5f);

 // Limiter obeys the sample peak ceiling, but no claim of TP certification.
 p=neutral();p.inputDb=12;p.ceilingDb=-1;p.limiterThreshold=-8;
 engine.prepare(fs,512);engine.setParameters(p);
 auto limited=sine(440,.8f,2);
 auto meters=engine.process(limited.l.data(),limited.r.data(),static_cast<int>(limited.l.size()));
 assert(limited.peak()<=boom::dbToGain(-1.f)+1e-5f);
 assert(meters.limiterGainReductionDb>1.0f);
 assert(meters.momentaryLufs>-100);

 // Module bypass changes actual limiter behavior rather than its appearance.
 p.limiterBypass=true;engine.prepare(fs,512);engine.setParameters(p);
 auto noLimiter=sine(440,.8f,2);
 engine.process(noLimiter.l.data(),noLimiter.r.data(),static_cast<int>(noLimiter.l.size()));
 assert(noLimiter.peak()>boom::dbToGain(-1.f)*1.5f);

 // DSP outputs must stay finite at high rates and small host blocks.
 for(double rate:{44100.,48000.,96000.,192000.}){
  boom::Engine current;current.prepare(rate,32);
  p=neutral();p.push=95;p.color=2;p.colorDrive=.9f;p.compThreshold=-25;
  current.setParameters(p);
  std::vector<float> l(32),r(32);
  for(int block=0;block<400;++block){
   for(int i=0;i<32;++i){l[i]=.95f*std::sin(2*PI*float(block*32+i)*1700/rate);r[i]=-l[i];}
   auto result=current.process(l.data(),r.data(),32);
   for(float value:l)assert(std::isfinite(value));
   assert(std::isfinite(result.outputPeakR));
  }
  assert(current.latencySamples()==static_cast<int>(std::ceil(rate*.005))+boom::OversampledTube4x::latency);
 }
 std::cout<<"Mastering validation: PASS; I="<<beforeSilence
  <<" LUFS; oversampled peak="<<oversampledPeak<<"; unlinked module bypass and limiter tested\n";
}
