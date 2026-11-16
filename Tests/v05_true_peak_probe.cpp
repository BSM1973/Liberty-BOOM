// Run: boom_v05_true_peak_probe /path/to/probe.f32
// The generated file is 48 kHz / 2-channel / interleaved float32LE.
// Compare it with an INDEPENDENT BS.1770 true-peak meter (e.g. FFmpeg ebur128).
#include "../Source/BoomDSP.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
int main(int argc,char** argv){
 if(argc!=2){std::cerr << "Usage: boom_v05_true_peak_probe output.f32\n";return 2;}
 std::ofstream file(argv[1],std::ios::binary);
 if(!file){std::cerr<<"Cannot open output file\n";return 2;}
 constexpr int fs=48000;
 boom::Engine engine;engine.prepare(fs,512);
 boom::Parameters p;p.inputDb=10.f;p.push=75.f;
 p.color=1;p.colorMix=.55f;p.colorDrive=.8f;
 p.compThreshold=-20.f;p.compRatio=2.5f;
 p.limiterThreshold=-8.f;p.ceilingDb=-1.f;p.outputDb=0.f;
 engine.setParameters(p);
 float peak=0,estimated=0;
 for(int offset=0;offset<fs*4;offset+=256){
  float l[256],r[256];
  for(int i=0;i<256;++i){
   const int t=offset+i;
   const double s=double(t)/fs;
   const float burst=(s>1.0&&s<1.2)?0.4f:0.f;
   l[i]=0.68f*std::sin(2*boom::pi*13001*s+0.19)+0.18f*std::sin(2*boom::pi*5939*s)+burst;
   r[i]=0.66f*std::sin(2*boom::pi*15289*s+0.61)+0.21f*std::sin(2*boom::pi*5091*s)-burst;
  }
  auto m=engine.process(l,r,256);
  estimated=std::max(estimated,m.truePeak);
  for(int i=0;i<256;++i){
   peak=std::max(peak,std::max(std::abs(l[i]),std::abs(r[i])));
   file.write(reinterpret_cast<const char*>(&l[i]),sizeof(float));
   file.write(reinterpret_cast<const char*>(&r[i]),sizeof(float));
  }
 }
 std::cout<<"Sample peak "<<boom::gainToDb(peak)<<" dBFS; prototype 4x TP estimate "
          <<boom::gainToDb(estimated)<<" dBTP (NOT certified); output "<<argv[1]<<"\n";
}
