#include "../Source/BoomDSP.h"
#ifdef NDEBUG
#undef NDEBUG // Assertions must execute in Release and CI.
#endif
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

using boom::Parameters;

static Parameters transparent(){
 Parameters p;
 p.color=0; p.deEssAmount=0;
 p.compRatio=1; p.compThreshold=0;
 p.limiterThreshold=0; p.ceilingDb=0;
 p.push=0;
 return p;
}
struct Stereo {
 std::vector<float> l,r;
 explicit Stereo(int n) : l(static_cast<std::size_t>(n)),r(static_cast<std::size_t>(n)) {}
};
static Stereo inputSine(int n,double sr,float amplitude=0.025f){
 Stereo s(n);
 for(int i=0;i<n;++i){
  auto phase=2.0*boom::pi*500.0*i/sr;
  s.l[static_cast<std::size_t>(i)]=amplitude*std::sin(phase);
  s.r[static_cast<std::size_t>(i)]=amplitude*std::sin(phase+0.4);
 }
 return s;
}
static float tailRms(const std::vector<float>& values){
 const std::size_t n=values.size(),start=n/2;
 double sum=0;
 for(std::size_t i=start;i<n;++i)sum+=double(values[i])*values[i];
 return static_cast<float>(std::sqrt(sum/double(n-start)));
}
static Stereo render(const Parameters& p,int n=96000,double rate=48000){
 auto signal=inputSine(n,rate);boom::Engine engine;
 engine.prepare(rate,512);engine.setParameters(p);
 engine.process(signal.l.data(),signal.r.data(),n);
 return signal;
}
int main(){
 // The independently adjustable channel trims are actual DSP controls, not
 // decorative UI parameters. A +6 dB left trim must not boost the right.
 auto p=transparent();
 const auto base=render(p);
 const float baselineLeft=tailRms(base.l),baselineRight=tailRms(base.r);
 p.inputTrimLDb=6;
 const auto leftHot=render(p);
 const float ratioLeft=tailRms(leftHot.l)/baselineLeft;
 const float ratioRight=tailRms(leftHot.r)/baselineRight;
 assert(ratioLeft>1.95f&&ratioLeft<2.05f);
 assert(ratioRight>0.98f&&ratioRight<1.02f);

 // Analog output compensation operates on the WET path only.
 p=transparent();p.color=1;p.colorDrive=.3f;p.colorMix=1.0f;
 const auto originalColor=render(p);
 p.colorOutputDb=6;
 const auto boostedColor=render(p);
 assert(tailRms(boostedColor.l)/tailRms(originalColor.l)>1.94f);
 p.colorMix=0;
 const auto bypassedColor=render(p);
 assert(std::abs(tailRms(bypassedColor.l)-baselineLeft)<0.0001f);

 // Buffer splitting must not change the sample stream. This also catches
 // accidental state reset or callback-size-dependent coefficient changes.
 p=transparent();p.color=2;p.colorDrive=.4f;p.colorMix=.6f;
 p.deEssAmount=.7f;p.deEssThreshold=-32;p.deEssAttackMs=1.5f;p.deEssReleaseMs=250;
 p.push=38;p.eqDb={1.0f,-2.0f,0.7f,-0.5f,1.3f};
 const int n=8192;
 auto whole=inputSine(n,48000,.25f),split=inputSine(n,48000,.25f);
 boom::Engine a,b;a.prepare(48000,512);b.prepare(48000,512);
 a.setParameters(p);b.setParameters(p);
 a.process(whole.l.data(),whole.r.data(),n);
 const int steps[]={1,13,64,7,257,32,2,511};
 int offset=0,k=0;
 while(offset<n){int count=std::min(steps[k++%8],n-offset);
  b.process(split.l.data()+offset,split.r.data()+offset,count);offset+=count;
 }
 for(int i=0;i<n;++i){
  assert(std::abs(whole.l[static_cast<std::size_t>(i)]-split.l[static_cast<std::size_t>(i)])<2e-6f);
  assert(std::abs(whole.r[static_cast<std::size_t>(i)]-split.r[static_cast<std::size_t>(i)])<2e-6f);
 }

 // Automation stress: abrupt changes must not produce NaNs, Inf, or sudden
 // output bursts above the limiter's advertised SAMPLE-PEAK ceiling.
 boom::Engine stressing;stressing.prepare(96000,32);
 for(int block=0;block<1600;++block){
  Parameters x=transparent();
  x.inputDb=block%2?18.f:-18.f;
  x.inputTrimLDb=block%3?12.f:-12.f;
  x.inputTrimRDb=block%3?-12.f:12.f;
  x.outputDb=block%2?6.f:-18.f;
  x.color=1+block%2;x.colorMix=(block%7)/6.f;
  x.colorOutputDb=block%2?9.f:-9.f;
  x.colorDrive=(block%11)/10.f;
  x.deEssAttackMs=block%2?0.1f:20.f;
  x.deEssReleaseMs=block%2?20.f:500.f;
  x.deEssAmount=block%2?1.f:0.f;
  x.compAttackMs=block%2?1.f:100.f;
  x.compReleaseMs=block%2?50.f:1000.f;
  x.compRatio=block%2?4.f:1.f;
  x.push=block%2?100.f:0.f;
  x.limiterThreshold=-15.f;x.ceilingDb=-1.0f;
  stressing.setParameters(x);
  float l[32],r[32];
  for(int i=0;i<32;++i){const auto t=(block*32+i)/96000.0;
   l[i]=.6f*std::sin(2*boom::pi*1700*t);
   r[i]=.6f*std::sin(2*boom::pi*1200*t);
  }
  const auto m=stressing.process(l,r,32);
  assert(std::isfinite(m.truePeak));
  for(int i=0;i<32;++i){
   assert(std::isfinite(l[i])&&std::isfinite(r[i]));
   assert(std::abs(l[i])<=boom::dbToGain(-1)+1e-4f);
   assert(std::abs(r[i])<=boom::dbToGain(-1)+1e-4f);
  }
 }
 std::cout<<"BOOM v0.3 regression: PASS; stereo trim, color compensation, block invariance, automation stress\n";
}
