#include "../Source/BoomDSP.h"
#ifdef NDEBUG
#undef NDEBUG // Never compile out validation checks in Release/CI.
#endif
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

static float rms(const std::vector<float>& v) {
 double s=0; for(float x:v) s+=double(x)*x;
 return float(std::sqrt(s/v.size()));
}
int main(){
 constexpr int N=48000;
 boom::Engine e; e.prepare(48000,512);
 boom::Parameters p; p.color=0;p.deEssAmount=0;p.compRatio=1;p.compThreshold=0;
 p.limiterThreshold=0;p.ceilingDb=0;
 std::vector<float> l(N),r(N);
 for(int i=0;i<N;++i)l[i]=r[i]=0.1f*std::sin(2*boom::pi*440*i/48000);
 const auto before=rms(l);e.setParameters(p); auto m=e.process(l.data(),r.data(),N);
 assert(m.outputPeakL>0.01f && std::isfinite(m.vuL));
 assert(std::abs(rms(l)-before)<0.015f);
 p.inputDb=12; e.prepare(48000,512);e.setParameters(p);
 for(int i=0;i<N;++i)l[i]=r[i]=0.02f*std::sin(2*boom::pi*440*i/48000);
 e.process(l.data(),r.data(),N);assert(rms(l)>0.055f);
 p.inputDb=0;p.eqDb[2]=8; e.prepare(48000,512);e.setParameters(p);
 for(int i=0;i<N;++i)l[i]=r[i]=0.02f*std::sin(2*boom::pi*3000*i/48000);
 e.process(l.data(),r.data(),N);assert(rms(l)>0.025f);
 p.eqDb[2]=0;p.push=80;p.color=1;p.colorDrive=0.7f;p.compThreshold=-24;
 e.prepare(48000,512);e.setParameters(p);
 for(int i=0;i<N;++i)l[i]=r[i]=0.4f*std::sin(2*boom::pi*440*i/48000);
 m=e.process(l.data(),r.data(),N);
 assert(m.compressorGainReductionDb>0);
 for(float x:l) assert(std::isfinite(x));
 std::cout << "DSP smoke tests: PASS; latency=" << e.latencySamples() << " samples; comp GR=" << m.compressorGainReductionDb << " dB\n";
}
