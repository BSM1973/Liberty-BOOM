#include "../Source/LoudnessMeter.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>

int main(){
 constexpr double pi=3.14159265358979323846;
 for(const int fs:{44100,48000,96000,192000}){
  boom::meters::LoudnessMeter meter;meter.prepare(fs);
  for(int i=0;i<fs*8;++i){
   const float x=.1f*std::sin(2.*pi*1000*i/fs);
   meter.tick(x,x);
  }
  const auto constant=meter.snapshotAndResetPeak();
  assert(constant.momentary>-22 && constant.momentary<-19);
  assert(std::abs(constant.integrated-constant.momentary)<0.25f);
  for(int i=0;i<fs*3;++i)meter.tick(0,0);
  const auto withSilence=meter.snapshotAndResetPeak();
  assert(std::abs(withSilence.integrated-constant.integrated)<0.3f);
  // FFmpeg ebur128=peak=true reference on stereo 1 kHz / 0.1 peak / 8 s
  // followed by 3 s of silence (48 kHz): I=-20.1 LUFS, LRA=5.7 LU.
  // 44.1/96/192 kHz are checked for stable behavior, not external certification.
  assert(std::abs(withSilence.integrated-(-20.1f))<0.25f);
  assert(std::abs(withSilence.loudnessRange-5.7f)<0.35f);
  std::cout<<fs<<" Hz: BOOM integrated="<<constant.integrated
     <<" LUFS, momentary="<<constant.momentary
     <<" LUFS, short="<<constant.shortTerm
     <<", gated with silence="<<withSilence.integrated<<" LUFS, LRA="<<withSilence.loudnessRange<<" LU\n";
 }
}
