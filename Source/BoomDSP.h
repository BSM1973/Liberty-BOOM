#pragma once
// BOOM — Liberty Music Software: dependency-free stereo DSP prototype.
// No locks, heap allocations, UI calls or file I/O in process().
// Internal state is owned by the audio thread; prepare() is called outside it.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>
#include "LoudnessMeter.h"
#include "OversampledTube.h"

namespace boom {
constexpr double pi = 3.1415926535897932384626433832795;
inline float clamp(float x,float lo,float hi) noexcept {return std::max(lo,std::min(hi,x));}
inline float dbToGain(float db) noexcept {return std::pow(10.0f,db/20.0f);}
inline float gainToDb(float v) noexcept {return 20.0f*std::log10(std::max(1e-10f,v));}

struct Parameters {
 float inputDb=0.0f, inputTrimLDb=0.0f, inputTrimRDb=0.0f; std::array<float,5> eqDb{{0,0,0,0,0}}; float push=0.0f;
 bool meterInput=false;
 int color=0; // 0 CLEAN, 1 TUBE, 2 TAPE
 float colorDrive=0.0f, colorMix=0.4f, colorOutputDb=0.0f;
 float colorPushCharacter=50.0f;
 float deEssFreq=6500.0f,deEssThreshold=-18.0f,deEssAmount=0.0f;
 float deEssAttackMs=4.0f,deEssReleaseMs=100.0f;
 float compThreshold=-12.0f,compRatio=1.5f,compAttackMs=30.0f,compReleaseMs=220.0f;
 float compMakeupDb=0.0f,compMix=1.0f;
 float limiterThreshold=-1.0f,ceilingDb=-1.0f,limiterReleaseMs=120.0f;
 int limiterMode=0;
 float outputDb=0.0f;
 bool limiterAutoRelease=false,limiterStereoLink=true;
 bool outputBypass=false;
 bool outputPeakGuard=true,outputLevelComp=false;
 bool colorBypass=false,eqBypass=false,deEssBypass=false,compBypass=false,limiterBypass=false;
 bool compStereoLink=true;
 bool bypass=false,monoMonitor=false;
};
struct Metering { float inputPeakL=0,inputPeakR=0,outputPeakL=0,outputPeakR=0;
 float vuL=0,vuR=0,compressorGainReductionDb=0,limiterGainReductionDb=0;
 float outputRmsL=0,outputRmsR=0;
 float momentaryLufs=-100,shortTermLufs=-100,integratedLufs=-100,lra=0;
 float truePeak=0,correlation=0; };

struct Biquad {
 struct Coefs{double b0=1,b1=0,b2=0,a1=0,a2=0;};
 Coefs c; double s1=0,s2=0;
 void reset() noexcept{s1=s2=0;}
 void set(Coefs coeff) noexcept{c=coeff;}
 float run(float x) noexcept {
  // Transposed direct form II.
  const double y=c.b0*x+s1;
  s1=c.b1*x-c.a1*y+s2; s2=c.b2*x-c.a2*y;
  if(std::abs(s1)<1.e-20) s1=0;
  if(std::abs(s2)<1.e-20) s2=0;
  return static_cast<float>(y);
 }
 static Coefs coefficients(int type, double freq, double q, double gainDb, double fs) noexcept {
  freq=std::clamp(freq,12.0,fs*0.449);
  const double w=2.0*pi*freq/fs, sn=std::sin(w), cs=std::cos(w);
  const double A=std::pow(10.0,gainDb/40.0), alpha=sn/(2.0*q);
  double b0=1,b1=0,b2=0,a0=1,a1=0,a2=0;
  switch(type){
   case 0: // peaking EQ
    b0=1+alpha*A;b1=-2*cs;b2=1-alpha*A;a0=1+alpha/A;a1=-2*cs;a2=1-alpha/A;break;
   case 1: // low shelf, slope=1
    {const double a=sn/2.0*std::sqrt((A+1/A)*0.0+2.0),b=2*std::sqrt(A)*a;
    b0=A*((A+1)-(A-1)*cs+b);b1=2*A*((A-1)-(A+1)*cs);b2=A*((A+1)-(A-1)*cs-b);
    a0=(A+1)+(A-1)*cs+b;a1=-2*((A-1)+(A+1)*cs);a2=(A+1)+(A-1)*cs-b;}break;
   case 2: // high shelf, slope=1
    {const double a=sn/2.0*std::sqrt(2.0),b=2*std::sqrt(A)*a;
    b0=A*((A+1)+(A-1)*cs+b);b1=-2*A*((A-1)+(A+1)*cs);b2=A*((A+1)+(A-1)*cs-b);
    a0=(A+1)-(A-1)*cs+b;a1=2*((A-1)-(A+1)*cs);a2=(A+1)-(A-1)*cs-b;}break;
   case 3: // high pass
    b0=(1+cs)*0.5;b1=-(1+cs);b2=b0;a0=1+alpha;a1=-2*cs;a2=1-alpha;break;
  }
  if(a0<=0 || !std::isfinite(a0)) return {};
  return {b0/a0,b1/a0,b2/a0,a1/a0,a2/a0};
 }
};

class Engine {
public:
 void prepare(double newSampleRate,int /*maxBlockSize*/) {
  fs=std::max(8000.0,newSampleRate);
  lookaheadSamples=std::max(1,static_cast<int>(std::ceil(fs*0.005)));
  delay.assign(static_cast<std::size_t>(lookaheadSamples+1),std::array<float,2>{0,0});
  dryDelay.assign(static_cast<std::size_t>(lookaheadSamples+OversampledTube4x::latency+1),std::array<float,2>{0,0});
  colorDelay.fill({0,0});colorPos=0;dryPos=0;
  for(auto& tube:tube4x)tube.prepare();
  pos=0; releaseGain={1.f,1.f};wetBlend=1;monoBlend=0; compEnvelope={0,0};deessEnvelope=0;
  levelMatchInputPower=levelMatchOutputPower=0.f;levelMatchGain=1.f;
  eqBlend=colorBlend=deEssBlend=compBlend=limiterBlend=1;
  limiterHold={0,0};inputTruePeak.prepare();for(auto& detector:independentTruePeak)detector.prepare();loudness.prepare(fs);
  inGain=outGain=inTrimL=inTrimR=colorOutGain=1; drive=0;push=0;smoothedDeEssFreq=6500;
  smoothedColorMix=0.4f;smoothedCompMix=1.0f;
  smoothedCompThreshold=-12.f;smoothedCompRatio=1.5f;smoothedCompMakeup=0.f;
  smoothedDeEssThreshold=-18.f;smoothedDeEssAmount=0.f;
  smoothedLimitThreshold=-1.f;currentColorMode=previousColorMode=0;modeCrossfade=1.f;
  for(auto& channel:eq) for(auto& band:channel) band.reset();
  for(auto& f:deEssHP) f.reset();
  tapeMemory[0]=tapeMemory[1]=0;
  vuPower={0,0};rmsPower={0,0};eqSmoothed={0,0,0,0,0};
  p={}; internalCounter=0;
  refreshFilters();
 }
 int latencySamples() const noexcept {return lookaheadSamples+OversampledTube4x::latency;}
 void setParameters(const Parameters& next) noexcept { p=next; }
 void resetIntegratedLoudness() noexcept {loudness.resetIntegrated();}
 Metering process(float* left,float* right,int count) noexcept {
  Metering m{};
  if(!left||!right||count<=0||delay.empty())return m;
  const float aFast=static_cast<float>(1.0-std::exp(-1.0/(fs*0.012)));
  const float aEq=static_cast<float>(1.0-std::exp(-1.0/(fs*0.018)));
  const float aDynamics=static_cast<float>(1.0-std::exp(-1.0/(fs*0.012)));
  const float aVu=static_cast<float>(1.0-std::exp(-1.0/(fs*0.300)));
  const float aRms=static_cast<float>(1.0-std::exp(-1.0/(fs*0.400)));
  const float aLevel=aRms;
  const float targetInput=dbToGain(clamp(p.inputDb,-18,18));
  const float targetLTrim=dbToGain(clamp(p.inputTrimLDb,-12,12));
  const float targetRTrim=dbToGain(clamp(p.inputTrimRDb,-12,12));
  const float targetColorOutput=dbToGain(clamp(p.colorOutputDb,-12,12));
  const float targetOutput=p.outputBypass?1.0f:dbToGain(clamp(p.outputDb,-18,6));
  const float pushTarget=clamp(p.push*0.01f,0,1);
  float gainRedComp=0,gainRedLimit=0;
  const int requestedColor=std::clamp(p.color,0,2);
  if(requestedColor!=currentColorMode){
   previousColorMode=currentColorMode;currentColorMode=requestedColor;modeCrossfade=0;
  }
  const float colorFadeStep=static_cast<float>(1.0/(fs*0.025));
  // Coefficients that depend only on the block's parameter snapshot are
  // computed once; avoid costly exp() calls in the per-sample inner loop.
  const float deessAtt=expCoeff(clamp(p.deEssAttackMs,0.1f,20.0f)*0.001f);
  const float deessRel=expCoeff(clamp(p.deEssReleaseMs,20.0f,500.0f)*0.001f);
  const float compAttack=expCoeff(std::max(1.0f,p.compAttackMs)*0.001f);
  const float compRelease=expCoeff(std::max(50.0f,p.compReleaseMs)*0.001f);
  const float modeRelease=p.limiterMode==1?1.55f:p.limiterMode==2?0.65f:1.0f;
  const float release=expCoeff(std::max(20.0f,p.limiterReleaseMs*modeRelease)*0.001f);
  const float quickRelease=expCoeff(std::max(20.0f,p.limiterReleaseMs*modeRelease*.55f)*0.001f);
  const float slowRelease=expCoeff(std::max(20.0f,p.limiterReleaseMs*modeRelease*1.8f)*0.001f);
  // This initial limiter is lookahead/sample-peak, NOT a validated true-peak limiter.
  for(int i=0;i<count;++i){
   inGain+=aFast*(targetInput-inGain); outGain+=aFast*(targetOutput-outGain);
   inTrimL+=aFast*(targetLTrim-inTrimL); inTrimR+=aFast*(targetRTrim-inTrimR);
   colorOutGain+=aFast*(targetColorOutput-colorOutGain);
   push+=aFast*(pushTarget-push);drive+=aFast*(clamp(p.colorDrive,0,1)-drive);
   for(int b=0;b<5;++b)eqSmoothed[b]+=aEq*(p.eqDb[b]-eqSmoothed[b]);
   smoothedDeEssFreq+=aEq*(clamp(p.deEssFreq,3000,12000)-smoothedDeEssFreq);
   if((++internalCounter&31)==0)refreshFilters();
   smoothedColorMix+=aDynamics*(clamp(p.colorMix,0,1)-smoothedColorMix);
   smoothedCompMix+=aDynamics*(clamp(p.compMix,0,1)-smoothedCompMix);
   smoothedCompThreshold+=aDynamics*(clamp(p.compThreshold,-40,0)-smoothedCompThreshold);
   smoothedCompRatio+=aDynamics*(clamp(p.compRatio,1,4)-smoothedCompRatio);
   smoothedCompMakeup+=aDynamics*(clamp(p.compMakeupDb,-12,12)-smoothedCompMakeup);
   smoothedDeEssThreshold+=aDynamics*(clamp(p.deEssThreshold,-40,0)-smoothedDeEssThreshold);
   smoothedDeEssAmount+=aDynamics*(clamp(p.deEssAmount,0,1)-smoothedDeEssAmount);
   smoothedLimitThreshold+=aDynamics*(clamp(p.limiterThreshold,-18,0)-smoothedLimitThreshold);
   modeCrossfade=std::min(1.0f,modeCrossfade+colorFadeStep);
   float l=std::isfinite(left[i])?left[i]:0.f,r=std::isfinite(right[i])?right[i]:0.f;
   const float lInputForMeter=l,rInputForMeter=r;
   m.inputPeakL=std::max(m.inputPeakL,std::abs(l));m.inputPeakR=std::max(m.inputPeakR,std::abs(r));
   // Bypass and mono are crossfaded, with both dry and wet time-aligned.
   const float bypassTarget=p.bypass?0.0f:1.0f;
   wetBlend+=aFast*(bypassTarget-wetBlend);
   monoBlend+=aFast*((p.monoMonitor?1.0f:0.0f)-monoBlend);
   dryDelay[dryPos]={l,r};
   const auto dryOld=dryDelay[(dryPos+1)%dryDelay.size()];
   dryPos=(dryPos+1)%dryDelay.size();
   l*=inGain*inTrimL;r*=inGain*inTrimR;
   colorBlend+=aFast*((p.colorBypass?0.0f:1.0f)-colorBlend);
   eqBlend+=aFast*((p.eqBypass?0.0f:1.0f)-eqBlend);
   deEssBlend+=aFast*((p.deEssBypass?0.0f:1.0f)-deEssBlend);
   compBlend+=aFast*((p.compBypass?0.0f:1.0f)-compBlend);
   limiterBlend+=aFast*((p.limiterBypass?0.0f:1.0f)-limiterBlend);
   const float character=clamp(p.colorPushCharacter*.01f,0.f,1.f);
   const float effectiveDrive=clamp(drive+(0.15f+0.62f*character)*push,0,1.5f);
   // A fixed 16-sample alignment is present for ALL modes, including CLEAN
   // and color bypass. This keeps mode changes and global bypass time aligned.
   colorDelay[colorPos]={l,r};
   const auto aligned=colorDelay[(colorPos+1)%colorDelay.size()];
   colorPos=(colorPos+1)%colorDelay.size();
   const float tubeL=(currentColorMode==1||previousColorMode==1)?tube4x[0].process(l,effectiveDrive):0.f;
   const float tubeR=(currentColorMode==1||previousColorMode==1)?tube4x[1].process(r,effectiveDrive):0.f;
   l=aligned[0];r=aligned[1];
   // A 25 ms color-mode crossfade avoids instantaneous TUBE/TAPE/CLEAN
   // transfer-function jumps when changing Styles or recalling presets.
   const float mix=smoothedColorMix*colorBlend;
   if(mix>0.0000001f && (currentColorMode!=0||previousColorMode!=0)){
    const float previousL=previousColorMode==1?tubeL*colorOutGain:previousColorMode==2?analog(l,effectiveDrive,previousColorMode,0)*colorOutGain:l;
    const float previousR=previousColorMode==1?tubeR*colorOutGain:previousColorMode==2?analog(r,effectiveDrive,previousColorMode,1)*colorOutGain:r;
    const float nextL=currentColorMode?(currentColorMode==previousColorMode?previousL:
      (currentColorMode==1?tubeL:analog(l,effectiveDrive,currentColorMode,0))*colorOutGain):l;
    const float nextR=currentColorMode?(currentColorMode==previousColorMode?previousR:
      (currentColorMode==1?tubeR:analog(r,effectiveDrive,currentColorMode,1))*colorOutGain):r;
    const float wetL=previousL+(nextL-previousL)*modeCrossfade;
    const float wetR=previousR+(nextR-previousR)*modeCrossfade;
    l+=(wetL-l)*mix;r+=(wetR-r)*mix;
   }
   if(modeCrossfade>=1.0f)previousColorMode=currentColorMode;
   const float eqInL=l,eqInR=r;
   for(int band=0;band<5;++band){l=eq[0][band].run(l);r=eq[1][band].run(r);}
   l=eqInL+(l-eqInL)*eqBlend;r=eqInR+(r-eqInR)*eqBlend;
   // De-esser removes only the dynamically attenuated high-frequency component.
   const float hfL=deEssHP[0].run(l),hfR=deEssHP[1].run(r);
   const float detector=std::max(std::abs(hfL),std::abs(hfR));
   deessEnvelope=detector>deessEnvelope?deessAtt*deessEnvelope+(1-deessAtt)*detector
       :deessRel*deessEnvelope+(1-deessRel)*detector;
   const float excess=std::max(0.0f,gainToDb(deessEnvelope)-smoothedDeEssThreshold);
   const float reduction=std::min(18.0f,excess*0.55f)*smoothedDeEssAmount;
   const float deessGain=dbToGain(-reduction*deEssBlend);
   l+=(deessGain-1)*hfL;r+=(deessGain-1)*hfR;
   const float peak=std::max(std::abs(l),std::abs(r));
   const float levels[2]={p.compStereoLink?peak:std::abs(l),p.compStereoLink?peak:std::abs(r)};
   for(int c=0;c<2;++c){const float v=levels[c];
    compEnvelope[c]=v>compEnvelope[c]?compAttack*compEnvelope[c]+(1-compAttack)*v
       :compRelease*compEnvelope[c]+(1-compRelease)*v;
   }
   const float dynamicThreshold=clamp(smoothedCompThreshold-7.0f*push,-45.0f,0.0f);
   const float ratio=clamp(smoothedCompRatio+0.9f*push,1.0f,5.0f);
   float cg[2]={1,1};
   for(int c=0;c<2;++c){
    const float overdB=gainToDb(compEnvelope[c])-dynamicThreshold;
    float compressed=0;
    if(overdB>3)compressed=(1.0f-1.0f/ratio)*overdB;
    else if(overdB>-3)compressed=(1.0f-1.0f/ratio)*(overdB+3)*(overdB+3)/12.0f;
    gainRedComp=std::max(gainRedComp,compressed);
    cg[c]=dbToGain(-compressed+smoothedCompMakeup+push*1.2f);
   }
   const float compMix=smoothedCompMix*compBlend;
   l*=1.0f+compMix*(cg[0]-1.0f);r*=1.0f+compMix*(cg[1]-1.0f);
   // Limiter modes change predrive and release envelope (not cosmetic).
   const float modeDrive=p.limiterMode==1?0.8f:p.limiterMode==2?2.4f:0.0f;
   const float limiterGain=dbToGain(-clamp(smoothedLimitThreshold-3.5f*push-modeDrive,-18,0));
   // A bypassed limiter must not continue to apply its predrive gain.
   const float effectiveLimiterGain=1.f+limiterBlend*(limiterGain-1.f);
   l*=effectiveLimiterGain;r*=effectiveLimiterGain;
   const float ceiling=dbToGain(clamp(p.ceilingDb,-2.0f,0.0f));
   // Reserve output trim headroom before computing the limiting detector.
   // Conservative 0.10 dBTP margin: one external probe at -1.0 dBTP
   // measured -0.99 dBTP before this margin. Still NOT a guarantee of
   // conformance on arbitrary material or every sample rate.
   const float guardedCeiling=ceiling*dbToGain(-0.10f);
   const float preCeiling=guardedCeiling/std::max(1.0f,outGain);
   // Oversampled interpolation estimates inter-sample peaks with a 7-sample
   // measurement lag. 5 ms lookahead leaves enough lead for this observation.
   const float estimatedPeak=inputTruePeak.tick(l,r);
   const float independent[2]={independentTruePeak[0].tick(l,0.f),
                               independentTruePeak[1].tick(0.f,r)};
   float desired[2]={1.f,1.f};
   for(int c=0;c<2;++c){
    const float detector=p.limiterStereoLink?estimatedPeak:independent[c];
    desired[c]=std::min(1.f,preCeiling/std::max(1.e-9f,detector));
    if(desired[c]<releaseGain[c]){
     releaseGain[c]=desired[c];
     limiterHold[c]=lookaheadSamples+16;
    }else if(limiterHold[c]>0){
     --limiterHold[c];
    }else{
     const float autoBlend=clamp((1.f-releaseGain[c])*2.f,0.f,1.f);
     const float coeff=p.limiterAutoRelease?
      quickRelease+(slowRelease-quickRelease)*autoBlend:release;
     releaseGain[c]=coeff*releaseGain[c]+(1.f-coeff)*desired[c];
    }
    gainRedLimit=std::max(gainRedLimit,-gainToDb(releaseGain[c])*limiterBlend);
   }
   delay[pos]={l,r};const auto old=delay[(pos+1)%delay.size()];pos=(pos+1)%delay.size();
   const float limitedL=old[0]*releaseGain[0],limitedR=old[1]*releaseGain[1];
   l=old[0]+limiterBlend*(limitedL-old[0]);
   r=old[1]+limiterBlend*(limitedR-old[1]);
   l*=outGain;r*=outGain;
   // Gentle RMS-linked matching for level-compensated A/B monitoring.
   const float inputPower=(lInputForMeter*lInputForMeter+rInputForMeter*rInputForMeter)*.5f;
   const float outputPower=(l*l+r*r)*.5f;
   levelMatchInputPower+=aLevel*(inputPower-levelMatchInputPower);
   levelMatchOutputPower+=aLevel*(outputPower-levelMatchOutputPower);
   const float wantedMatch=p.outputLevelComp?
      clamp(std::sqrt((levelMatchInputPower+1.e-9f)/(levelMatchOutputPower+1.e-9f)),.50f,2.f):1.f;
   levelMatchGain+=aDynamics*(wantedMatch-levelMatchGain);
   l*=levelMatchGain;r*=levelMatchGain;
   // Sample-peak failsafe only when the limiter is engaged. It is not
   // sufficient to claim normative true-peak ceiling compliance.
   const float postPeak=std::max(std::abs(l),std::abs(r));
   const float safety=std::min(1.0f,guardedCeiling/std::max(1.e-9f,postPeak));
   const float guard=1.0f+(p.outputPeakGuard?limiterBlend:0.f)*(safety-1.0f);
   l*=guard;r*=guard;
   l=l*wetBlend+dryOld[0]*(1.0f-wetBlend);
   r=r*wetBlend+dryOld[1]*(1.0f-wetBlend);
   const float mid=0.5f*(l+r);
   l+=(mid-l)*monoBlend;r+=(mid-r)*monoBlend;
   left[i]=l;right[i]=r;
   m.outputPeakL=std::max(m.outputPeakL,std::abs(left[i]));
   m.outputPeakR=std::max(m.outputPeakR,std::abs(right[i]));
   const float meterL=p.meterInput?lInputForMeter:left[i];
   const float meterR=p.meterInput?rInputForMeter:right[i];
   vuPower[0]+=aVu*(meterL*meterL-vuPower[0]);
   vuPower[1]+=aVu*(meterR*meterR-vuPower[1]);
   rmsPower[0]+=aRms*(left[i]*left[i]-rmsPower[0]);
   rmsPower[1]+=aRms*(right[i]*right[i]-rmsPower[1]);
   loudness.tick(left[i],right[i]);
  }
  m.vuL=std::sqrt(std::max(0.f,vuPower[0]));m.vuR=std::sqrt(std::max(0.f,vuPower[1]));
  m.outputRmsL=std::sqrt(std::max(0.f,rmsPower[0]));m.outputRmsR=std::sqrt(std::max(0.f,rmsPower[1]));
  m.compressorGainReductionDb=gainRedComp;m.limiterGainReductionDb=gainRedLimit;
  const auto extra=loudness.snapshotAndResetPeak();
  m.momentaryLufs=extra.momentary;m.shortTermLufs=extra.shortTerm;
  m.integratedLufs=extra.integrated;m.lra=extra.loudnessRange;
  m.truePeak=extra.truePeak;m.correlation=extra.correlation;
  return m;
 }
private:
 double fs=48000.0;
 Parameters p;
 std::array<std::array<Biquad,5>,2> eq{};
 std::array<Biquad,2> deEssHP{};
 std::array<float,5> eqSmoothed{{0,0,0,0,0}};
 std::array<float,2> vuPower{{0,0}},rmsPower{{0,0}};
 meters::LoudnessMeter loudness;
 meters::TruePeakEstimator inputTruePeak;
 std::array<meters::TruePeakEstimator,2> independentTruePeak;
 std::vector<std::array<float,2>> delay,dryDelay;
 std::array<std::array<float,2>,OversampledTube4x::latency+1> colorDelay{};
 std::array<OversampledTube4x,2> tube4x;
 std::size_t dryPos=0,colorPos=0;
 std::size_t pos=0;int lookaheadSamples=240,internalCounter=0;
 float inGain=1,outGain=1,inTrimL=1,inTrimR=1,colorOutGain=1;
 float drive=0,push=0,deessEnvelope=0;
 std::array<float,2> releaseGain{{1.f,1.f}};
 std::array<float,2> compEnvelope{{0,0}};
 float eqBlend=1,colorBlend=1,deEssBlend=1,compBlend=1,limiterBlend=1;
 std::array<int,2> limiterHold{{0,0}};
 float levelMatchInputPower=0.f,levelMatchOutputPower=0.f,levelMatchGain=1.f;
 float wetBlend=1,monoBlend=0,smoothedDeEssFreq=6500;
 float tapeMemory[2]{0,0};
 float smoothedColorMix=0.4f,smoothedCompMix=1.f;
 float smoothedCompThreshold=-12.f,smoothedCompRatio=1.5f,smoothedCompMakeup=0.f;
 float smoothedDeEssThreshold=-18.f,smoothedDeEssAmount=0.f,smoothedLimitThreshold=-1.f;
 int currentColorMode=0,previousColorMode=0;
 float modeCrossfade=1.f;
 float expCoeff(float seconds) const noexcept{return static_cast<float>(std::exp(-1.0/(fs*std::max(seconds,0.0001f))));}
 float analog(float x,float amount,int mode,int channel) noexcept{
  if(mode==1){ // tube-like smoothly saturated transfer with asymmetric even harmonics
   const float k=0.6f+3.8f*amount;
   const float wet=(std::tanh((x+0.035f*x*x)*k)-std::tanh(0.0f))/std::tanh(k);
   return wet;
  }
  // tape-like frequency-dependent memory and rounded peaks (not a tape emulation model)
  float& prev=tapeMemory[channel];
  const float hf=x-prev;prev+=0.07f*hf;
  const float wet=std::tanh((x-hf*0.08f*amount)*(1.0f+2.8f*amount)) / std::tanh(1.0f+2.8f*amount);
  return wet;
 }
 void refreshFilters() noexcept{
  static constexpr double freq[5]={60.0,120.0,3000.0,6000.0,10000.0};
  static constexpr double qs[5]={0.707,0.83,0.75,0.76,0.707};
  for(int c=0;c<2;++c){
   for(int band=0;band<5;++band){
    const int type=band==0?1:band==4?2:0;
    eq[c][band].set(Biquad::coefficients(type,freq[band],qs[band],eqSmoothed[band],fs));
   }
   deEssHP[c].set(Biquad::coefficients(3,smoothedDeEssFreq,0.707,0,fs));
  }
 }
};
}
