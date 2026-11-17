#include "../Source/BoomDSP.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

static float render(boom::Engine& e,const boom::Parameters& p,int n=32768){
 e.setParameters(p);
 float energy=0.f;
 std::vector<float> left(256),right(256);
 int count=0;
 while(count<n){
  const int block=std::min(256,n-count);
  for(int i=0;i<block;++i){
   const float v=.031f*std::sin(2.f*float(boom::pi)*1000.f*
                                float((count+i)%48000)/48000.f);
   left[static_cast<size_t>(i)]=v;right[static_cast<size_t>(i)]=v;
  }
  e.process(left.data(),right.data(),block);
  if(count>n/2)
   for(int i=0;i<block;++i){
    const float a=left[static_cast<size_t>(i)];
    const float b=right[static_cast<size_t>(i)];
    assert(std::isfinite(a)&&std::isfinite(b));
    energy+=a*a+b*b;
   }
  count+=block;
 }
 return std::sqrt(energy/std::max(1,n));
}
int main(){
 boom::Engine engine;
 engine.prepare(48000,512);
 boom::Parameters p;
 p.colorBypass=true;p.eqBypass=true;p.deEssBypass=true;
 p.compBypass=true;p.limiterBypass=true;
 p.outputPeakGuard=false;p.outputLevelComp=false;
 p.outputDb=6.f;p.outputBypass=false;
 const float amplified=render(engine,p);
 p.outputBypass=true;
 const float bypassed=render(engine,p);
 assert(amplified>bypassed*1.55f);
 // Toggle the parameter back and verify it can be automated without NaN.
 p.outputBypass=false;
 const float returnGain=render(engine,p);
 assert(returnGain>bypassed*1.55f);
 // The PUSH character is a real, finite and bounded DSP input.
 p.colorBypass=false;p.color=1;p.colorMix=1.f;p.colorDrive=.3f;p.push=85.f;
 for(int character : {0,25,50,100}){
  p.colorPushCharacter=float(character);
  const float e=render(engine,p,8192);
  assert(std::isfinite(e)&&e>=0.f);
 }
 std::cout<<"BOOM UI DSP contract PASS: output activation and PUSH character\n";
}
