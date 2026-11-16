#include "../Source/BoomDSP.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

static double goertzel(const std::vector<float>& audio,int start,double frequency,double sampleRate) {
 const double omega=2.*boom::pi*frequency/sampleRate, coefficient=2*std::cos(omega);
 double z0=0,z1=0,z2=0;
 for(std::size_t i=static_cast<std::size_t>(start);i<audio.size();++i){
  z0=audio[i]+coefficient*z1-z2;z2=z1;z1=z0;
 }
 return std::hypot(z1-z2*std::cos(omega),z2*std::sin(omega));
}
int main(){
 constexpr double fs=48000.;
 constexpr int n=96000;
 boom::OversampledTube4x oversampled;
 oversampled.prepare();
 assert(boom::OversampledTube4x::latency==16);
 std::vector<float> unfiltered(n),filtered(n);
 const float k=0.6f+3.8f*1.0f;
 for(int i=0;i<n;++i){
  const float x=0.7f*std::sin(2*boom::pi*17000*i/fs);
  unfiltered[i]=std::tanh((x+0.035f*x*x)*k)/std::tanh(k);
  filtered[i]=oversampled.process(x,1.0f);
  assert(std::isfinite(filtered[i]));
 }
 // 17 kHz third harmonic (51 kHz) folds to 3 kHz at 48 kHz without
 // anti-aliasing. The 4x path must reject it measurably.
 const double aliasNaive=goertzel(unfiltered,n/2,3000.,fs);
 const double aliasFiltered=goertzel(filtered,n/2,3000.,fs);
 const double attenuationDb=20*std::log10(std::max(1e-12,aliasFiltered/aliasNaive));
 assert(attenuationDb < -20.0);
 // A clean path always introduces exactly the same fixed latency as TUBE:
 // the host-reported PDC must account for 16 samples plus limiter lookahead.
 boom::Engine e;e.prepare(fs,256);
 assert(e.latencySamples()==int(std::ceil(fs*.005))+16);
 boom::Parameters p; p.color=0;p.colorMix=0;p.compMix=0;p.limiterBypass=true;
 p.eqBypass=true;p.deEssBypass=true;p.limiterThreshold=0;p.ceilingDb=0;
 e.setParameters(p);
 std::vector<float> impulse(4096,0),copy(4096,0);
 impulse[0]=copy[0]=.5f;
 e.process(impulse.data(),copy.data(),4096);
 // Smoothed default gains are unity, so a unit impulse is preserved after PDC.
 for(int i=0;i<4096;++i){
  if(i==e.latencySamples())assert(impulse[static_cast<std::size_t>(i)]>.45f);
  else assert(std::abs(impulse[static_cast<std::size_t>(i)])<1e-4f);
 }
 std::cout << "BOOM v0.5 4x TUBE alias attenuation " << attenuationDb
           << " dB, latency " << e.latencySamples() << " samples: PASS\n";
}
