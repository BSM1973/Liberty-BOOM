#pragma once
// BOOM — 4x polyphase FIR oversampling for the original TUBE waveshaper.
// Blackman-windowed 65-tap interpolation and reconstruction FIR stages.
// Fixed storage, no allocations in the audio callback. Exactly 16 samples of
// additional fixed algorithmic latency (8 in each linear-phase stage).
// This is a prototype anti-aliasing path, not a published hardware model.
#include <algorithm>
#include <array>
#include <cmath>

namespace boom {
class OversampledTube4x {
public:
 static constexpr int factor=4;
 static constexpr int taps=65;
 static constexpr int latency=16;
 void prepare() noexcept {
  constexpr double pi=3.1415926535897932384626433832795;
  constexpr double fc=0.1125; // fraction of oversampled sample rate
  double sum=0;
  for(int i=0;i<taps;++i){
   const double d=i-(taps-1)/2;
   const double ideal=std::abs(d)<1.e-12?2.*fc:std::sin(2.*pi*fc*d)/(pi*d);
   const double w=0.42-0.50*std::cos(2.*pi*i/(taps-1))
                 +0.08*std::cos(4.*pi*i/(taps-1));
   reconstruction[i]=ideal*w;
   sum+=reconstruction[i];
  }
  for(int i=0;i<taps;++i){
   reconstruction[i]/=sum;
   interpolation[i]=4.*reconstruction[i];
  }
  inputHistory.fill(0.f);outputHistory.fill(0.f);
  inputIndex=outputIndex=0;
 }
 float process(float sample,float amount) noexcept {
  inputHistory[inputIndex]=sample;
  const float k=0.6f+3.8f*std::clamp(amount,0.f,1.5f);
  const double norm=std::tanh(double(k));
  float output=0;
  for(int phase=0;phase<factor;++phase){
   double interpolated=0;
   for(int tap=phase;tap<taps;tap+=factor){
    const int lag=(tap-phase)/factor;
    const int slot=(inputIndex+int(inputHistory.size())-lag)%int(inputHistory.size());
    interpolated+=interpolation[tap]*inputHistory[static_cast<std::size_t>(slot)];
   }
   const double nonlinear=std::tanh((interpolated+0.035*interpolated*interpolated)*k)/norm;
   outputHistory[outputIndex]=static_cast<float>(nonlinear);
   if(phase==0){
    double reconstructed=0;
    for(int tap=0;tap<taps;++tap){
     const int slot=(outputIndex+taps-tap)%taps;
     reconstructed+=reconstruction[tap]*outputHistory[static_cast<std::size_t>(slot)];
    }
    output=static_cast<float>(reconstructed);
   }
   outputIndex=(outputIndex+1)%taps;
  }
  inputIndex=(inputIndex+1)%int(inputHistory.size());
  return output;
 }
private:
 std::array<double,taps> interpolation{},reconstruction{};
 std::array<float,17> inputHistory{};
 std::array<float,taps> outputHistory{};
 int inputIndex=0,outputIndex=0;
};
} // namespace boom
