#include "../Source/BoomDSP.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

namespace {
constexpr double sr=48000.0;
std::vector<float> sine(int length,double fs,float amplitude=0.15f) {
 std::vector<float> signal(static_cast<std::size_t>(length));
 for(int i=0;i<length;++i)
  signal[static_cast<std::size_t>(i)]=amplitude*std::sin(boom::pi*2*440*i/fs);
 return signal;
}
boom::Parameters neutral(){
 boom::Parameters p;
 p.color=0;p.colorDrive=0;p.push=0;p.deEssAmount=0;
 p.compRatio=1;p.compThreshold=0;p.compMix=0;
 p.ceilingDb=0;p.limiterThreshold=0;
 p.limiterBypass=true;
 return p;
}
float rmsAfter(const std::vector<float>& x,std::size_t start){
 double energy=0;
 for(std::size_t i=start;i<x.size();++i)energy+=double(x[i])*x[i];
 return static_cast<float>(std::sqrt(energy/double(x.size()-start)));
}
}
int main(){
 // Regression: the limiter's internal predrive must be bypassed, not just its
 // envelope gain reduction. Compare a -18 dB and 0 dB threshold with bypass ON.
 auto baseline=neutral();baseline.limiterThreshold=0;
 auto thresholdHot=neutral();thresholdHot.limiterThreshold=-18;
 auto l=sine(96000,sr),r=l,reference=l,referenceR=r;
 boom::Engine a,b;a.prepare(sr,1024);b.prepare(sr,1024);
 a.setParameters(baseline);b.setParameters(thresholdHot);
 a.process(reference.data(),referenceR.data(),int(reference.size()));
 b.process(l.data(),r.data(),int(l.size()));
 const auto first=rmsAfter(reference,48000),second=rmsAfter(l,48000);
 assert(std::abs(second/first-1.0f)<0.0001f);

 // On-the-fly character switches must fade instead of jumping directly
 // between transfer functions. The test uses a continuous 440 Hz sine.
 auto p=neutral();p.color=1;p.colorMix=1;p.colorDrive=0.8f;
 auto mono=sine(96000,sr,0.36f);auto stereo=mono;
 boom::Engine morph;morph.prepare(sr,256);
 morph.setParameters(p);
 for(int offset=0;offset<96000;offset+=64){
  if(offset==48000){p.color=2;morph.setParameters(p);}
  morph.process(mono.data()+offset,stereo.data()+offset,64);
 }
 assert(std::abs(mono[48000]-mono[47999])<0.05f);
 for(auto v:mono)assert(std::isfinite(v));

 // Invalid host sample input is turned into silence before entering recursive
 // DSP state; a single NaN should not poison future output.
 auto valid=sine(8192,sr);auto validR=valid;
 valid[1024]=std::numeric_limits<float>::quiet_NaN();
 validR[2048]=std::numeric_limits<float>::infinity();
 boom::Engine defensive;defensive.prepare(sr,64);defensive.setParameters(neutral());
 for(int i=0;i<8192;i+=64)defensive.process(valid.data()+i,validR.data()+i,64);
 for(auto v:valid)assert(std::isfinite(v));
 for(auto v:validR)assert(std::isfinite(v));

 // Engine must report sample-rate-derived lookahead consistently (5ms), and
 // cope with variable host buffer sizes at high sample rates.
 for(double rate:{44100.0,48000.0,96000.0,192000.0}){
  boom::Engine e;e.prepare(rate,7);e.setParameters(neutral());
  assert(e.latencySamples()==int(std::ceil(rate*0.005))+boom::OversampledTube4x::latency);
  for(int i=0;i<150;++i){
   float ll[7]{0,0,0.1f,0,0,0,0};float rr[7]{0,0,0.1f,0,0,0,0};
   const auto m=e.process(ll,rr,7);
   assert(std::isfinite(m.outputPeakL)&&std::isfinite(m.integratedLufs));
  }
 }
 std::cout<<"BOOM v0.4 DSP regressions: PASS; limiter bypass, analog morph, invalid samples, latency\n";
}
