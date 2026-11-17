#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace {
using Layout=juce::AudioProcessorValueTreeState::ParameterLayout;
void addFloat(Layout& l, const char* id,const char* name,float lo,float hi,float def,float step=0.01f){
 l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>{lo,hi,step},def));
}
void addBool(Layout& l,const char* id,const char* name,bool def=false){
 l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{id,1},name,def));
}
void addChoice(Layout& l,const char* id,const char* name,juce::StringArray opts,int initial=0){
 l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id,1},name,opts,initial));
}
}
BoomAudioProcessor::BoomAudioProcessor():
 AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true)
  .withOutput("Output",juce::AudioChannelSet::stereo(),true)),
 apvts(*this,nullptr,"BOOM_STATE",parameterLayout()){}

juce::AudioProcessorValueTreeState::ParameterLayout BoomAudioProcessor::parameterLayout(){
 Layout l;
 addFloat(l,"inputDb","Input Drive",-18,18,0);
 addFloat(l,"inputLTrim","Left Input Trim",-12,12,0);
 addFloat(l,"inputRTrim","Right Input Trim",-12,12,0);
 addFloat(l,"eq60","EQ 60 Hz",-8,8,0);
 addFloat(l,"eq120","EQ 120 Hz",-10,10,0);
 addFloat(l,"eq3k","EQ 3 kHz",-10,10,0);
 addFloat(l,"eq6k","EQ 6 kHz",-10,10,0);
 addFloat(l,"eq10k","EQ 10 kHz",-8,8,0);
 addFloat(l,"push","PUSH",0,100,0,0.1f);
 addChoice(l,"color","Analog Color",{"CLEAN","TUBE","TAPE"});
 addFloat(l,"colorDrive","Analog Drive",0,1,0,0.001f);
 addFloat(l,"colorMix","Analog Mix",0,1,0.4f,0.001f);
 addFloat(l,"colorOut","Analog Output Compensation",-12,12,0);
 addFloat(l,"deFreq","De-esser Frequency",3000,12000,6500,1);
 addFloat(l,"deTh","De-esser Threshold",-40,0,-18);
 addFloat(l,"deAmt","De-esser Amount",0,1,0,0.001f);
 addFloat(l,"deAtt","De-esser Attack",0.1f,20.0f,4.0f,0.1f);
 addFloat(l,"deRel","De-esser Release",20.0f,500.0f,100.0f,1.0f);
 addFloat(l,"compTh","Compressor Threshold",-40,0,-12);
 addFloat(l,"compRatio","Compressor Ratio",1,4,1.5f);
 addFloat(l,"compAttack","Compressor Attack",1,100,30);
 addFloat(l,"compRelease","Compressor Release",50,1000,220);
 addFloat(l,"compMakeup","Compressor Makeup",-12,12,0);
 addFloat(l,"compMix","Parallel Mix",0,1,1,0.001f);
 addFloat(l,"limitTh","Limiter Threshold",-18,0,-1);
 addFloat(l,"ceiling","Output Ceiling",-2,0,-1);
 addFloat(l,"limitRelease","Limiter Release",20,1000,120);
 addChoice(l,"limitMode","Limiter Mode",{"TRANSPARENT","PUNCH","LOUD"});
 addFloat(l,"outDb","Output Trim",-18,6,0);
 addBool(l,"colorBypass","Analog Color Bypass");
 addBool(l,"eqBypass","Master EQ Bypass");
 addBool(l,"deEssBypass","De-esser Bypass");
 addBool(l,"compBypass","Compressor Bypass");
 addBool(l,"limiterBypass","Limiter Bypass");
 addBool(l,"compStereoLink","Compressor Stereo Link",true);
 addBool(l,"limiterStereoLink","Limiter Stereo Link",true);
 addBool(l,"limiterAutoRelease","Limiter Auto Release",false);
 addBool(l,"outPeakGuard","Output Peak Protection",true);
 addBool(l,"outputLevelComp","Output Level Compensation",false);
 addBool(l,"inputLink","Input L/R Trim Link",true);
 addBool(l,"mono","Mono Monitoring");
 addBool(l,"bypass","Global Bypass");
 addBool(l,"meterInput","VU Meter Input");
 addChoice(l,"language","Language",{"Français","English"});
 return l;
}

bool BoomAudioProcessor::isBusesLayoutSupported(const BusesLayout& layout) const{
 return layout.getMainInputChannelSet()==juce::AudioChannelSet::stereo() &&
  layout.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();
}
void BoomAudioProcessor::prepareToPlay(double sr,int samplesPerBlock){
 engine.prepare(sr,samplesPerBlock);
 setLatencySamples(engine.latencySamples());
}
boom::Parameters BoomAudioProcessor::readParameters() const noexcept{
 boom::Parameters p;
 auto load=[this](const char* id)->float{auto* a=apvts.getRawParameterValue(id);return a?a->load():0.0f;};
 p.inputDb=load("inputDb");p.inputTrimLDb=load("inputLTrim");p.inputTrimRDb=load("inputRTrim");p.eqDb={load("eq60"),load("eq120"),load("eq3k"),load("eq6k"),load("eq10k")};
 p.push=load("push");p.color=int(load("color"));p.colorDrive=load("colorDrive");p.colorMix=load("colorMix");p.colorOutputDb=load("colorOut");
 p.deEssFreq=load("deFreq");p.deEssThreshold=load("deTh");p.deEssAmount=load("deAmt");
 p.deEssAttackMs=load("deAtt");p.deEssReleaseMs=load("deRel");
 p.compThreshold=load("compTh");p.compRatio=load("compRatio");p.compAttackMs=load("compAttack");
 p.compReleaseMs=load("compRelease");p.compMakeupDb=load("compMakeup");p.compMix=load("compMix");
 p.limiterThreshold=load("limitTh");p.ceilingDb=load("ceiling");p.limiterReleaseMs=load("limitRelease");
 p.limiterMode=int(load("limitMode"));p.outputDb=load("outDb");
 p.limiterAutoRelease=load("limiterAutoRelease")>0.5f;
 p.limiterStereoLink=load("limiterStereoLink")>0.5f;
 p.outputPeakGuard=load("outPeakGuard")>0.5f;
 p.outputLevelComp=load("outputLevelComp")>0.5f;
 p.colorBypass=load("colorBypass")>0.5f;p.eqBypass=load("eqBypass")>0.5f;
 p.deEssBypass=load("deEssBypass")>0.5f;p.compBypass=load("compBypass")>0.5f;
 p.limiterBypass=load("limiterBypass")>0.5f;p.compStereoLink=load("compStereoLink")>0.5f;
 p.monoMonitor=load("mono")>0.5f;p.bypass=load("bypass")>0.5f;p.meterInput=load("meterInput")>0.5f;
 return p;
}
void BoomAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&){
 juce::ScopedNoDenormals noDenormals;
 if(buffer.getNumChannels()<2)return;
 if(loudnessResetRequested.exchange(false))engine.resetIntegratedLoudness();
 auto params=readParameters();
 // Hosts generally report true for offline renders. Monitoring-only MONO is
 // suppressed there; real-time capture still requires the user's attention.
 if(isNonRealtime())params.monoMonitor=false;
 engine.setParameters(params);
 auto meters=engine.process(buffer.getWritePointer(0),buffer.getWritePointer(1),buffer.getNumSamples());
 inPeakL.store(meters.inputPeakL);inPeakR.store(meters.inputPeakR);
 outPeakL.store(meters.outputPeakL);outPeakR.store(meters.outputPeakR);
 vuL.store(meters.vuL);vuR.store(meters.vuR);
 grComp.store(meters.compressorGainReductionDb);grLimit.store(meters.limiterGainReductionDb);
 rmsL.store(meters.outputRmsL);rmsR.store(meters.outputRmsR);
 momentaryLufs.store(meters.momentaryLufs);shortTermLufs.store(meters.shortTermLufs);
 integratedLufs.store(meters.integratedLufs);lra.store(meters.lra);
 truePeak.store(meters.truePeak);correlation.store(meters.correlation);
}
juce::AudioProcessorEditor* BoomAudioProcessor::createEditor(){return new BoomAudioProcessorEditor(*this);}
boom::Metering BoomAudioProcessor::lastMeters() const noexcept{
 boom::Metering m; m.inputPeakL=inPeakL.load();m.inputPeakR=inPeakR.load();
 m.outputPeakL=outPeakL.load();m.outputPeakR=outPeakR.load();m.vuL=vuL.load();m.vuR=vuR.load();
 m.compressorGainReductionDb=grComp.load();m.limiterGainReductionDb=grLimit.load();
 m.outputRmsL=rmsL.load();m.outputRmsR=rmsR.load();
 m.momentaryLufs=momentaryLufs.load();m.shortTermLufs=shortTermLufs.load();
 m.integratedLufs=integratedLufs.load();m.lra=lra.load();
 m.truePeak=truePeak.load();m.correlation=correlation.load();return m;
}
void BoomAudioProcessor::writeParameters(const boom::Parameters& p){
 auto set=[this](const char* id,float value){if(auto* param=apvts.getParameter(id))
   param->setValueNotifyingHost(param->convertTo0to1(value));};
 set("inputDb",p.inputDb);set("inputLTrim",p.inputTrimLDb);set("inputRTrim",p.inputTrimRDb);
 set("eq60",p.eqDb[0]);set("eq120",p.eqDb[1]);
 set("eq3k",p.eqDb[2]);set("eq6k",p.eqDb[3]);set("eq10k",p.eqDb[4]);
 set("push",p.push);set("color",float(p.color));set("colorDrive",p.colorDrive);set("colorMix",p.colorMix);set("colorOut",p.colorOutputDb);
 set("deFreq",p.deEssFreq);set("deTh",p.deEssThreshold);set("deAmt",p.deEssAmount);
 set("deAtt",p.deEssAttackMs);set("deRel",p.deEssReleaseMs);
 set("compTh",p.compThreshold);set("compRatio",p.compRatio);set("compAttack",p.compAttackMs);
 set("compRelease",p.compReleaseMs);set("compMakeup",p.compMakeupDb);set("compMix",p.compMix);
 set("limitTh",p.limiterThreshold);set("ceiling",p.ceilingDb);set("limitRelease",p.limiterReleaseMs);
 set("limitMode",float(p.limiterMode));set("outDb",p.outputDb);
 set("limiterStereoLink",p.limiterStereoLink?1.f:0.f);
 set("limiterAutoRelease",p.limiterAutoRelease?1.f:0.f);
 set("outPeakGuard",p.outputPeakGuard?1.f:0.f);
 set("outputLevelComp",p.outputLevelComp?1.f:0.f);
 set("colorBypass",p.colorBypass?1.f:0.f);set("eqBypass",p.eqBypass?1.f:0.f);
 set("deEssBypass",p.deEssBypass?1.f:0.f);set("compBypass",p.compBypass?1.f:0.f);
 set("limiterBypass",p.limiterBypass?1.f:0.f);set("compStereoLink",p.compStereoLink?1.f:0.f);
 // Operational monitoring controls are not overwritten when changing a Style.
}
void BoomAudioProcessor::loadStyle(int idx){
 idx=juce::jlimit(0,39,idx);
 selectedStyle.store(idx);selectedPreset.store(-1);
 writeParameters(boom::makeStyle(idx));
}
void BoomAudioProcessor::loadFactoryPreset(int idx){
 idx=juce::jlimit(0,99,idx);const auto pr=boom::makePreset(idx);
 selectedStyle.store(pr.style);selectedPreset.store(idx);
 writeParameters(pr.params);
}
void BoomAudioProcessor::storeComparisonState(int slot){
 if(slot<0||slot>=2)return;
 auto snapshot=apvts.copyState();
 snapshot.setProperty("presetIndex",selectedPreset.load(),nullptr);
 snapshot.setProperty("styleIndex",selectedStyle.load(),nullptr);
 const juce::ScopedLock lock(metadataLock);
 comparisonSlots[static_cast<std::size_t>(slot)]=snapshot.createCopy();
}
void BoomAudioProcessor::applySnapshot(const juce::ValueTree& snapshot){
 for(auto* parameter:getParameters()){
  if(auto* ranged=dynamic_cast<juce::RangedAudioParameter*>(parameter)){
   // Operational monitoring and interface language are never recalled with
   // A/B or mastering presets. They remain user/host preferences.
   const auto parameterId=ranged->getParameterID();
   if(parameterId=="language"||parameterId=="mono"||parameterId=="meterInput")continue;
   const auto child=snapshot.getChildWithProperty("id",parameterId);
   if(child.isValid()){
    const float raw=static_cast<float>(child.getProperty("value"));
    ranged->setValueNotifyingHost(ranged->convertTo0to1(raw));
   }
  }
 }
}
void BoomAudioProcessor::recallComparisonState(int slot){
 if(slot<0||slot>=2)return;
 juce::ValueTree snapshot;
 {
  const juce::ScopedLock lock(metadataLock);
  if(!comparisonSlots[static_cast<std::size_t>(slot)].isValid())return;
  snapshot=comparisonSlots[static_cast<std::size_t>(slot)].createCopy();
 }
 applySnapshot(snapshot);
 selectedPreset.store(int(snapshot.getProperty("presetIndex",-1)));
 selectedStyle.store(int(snapshot.getProperty("styleIndex",0)));
}
void BoomAudioProcessor::copyComparisonState(int sourceSlot,int destinationSlot){
 if(sourceSlot<0||sourceSlot>=2||destinationSlot<0||destinationSlot>=2||sourceSlot==destinationSlot)return;
 const juce::ScopedLock lock(metadataLock);
 const auto& original=comparisonSlots[static_cast<std::size_t>(sourceSlot)];
 if(original.isValid())comparisonSlots[static_cast<std::size_t>(destinationSlot)]=original.createCopy();
}
bool BoomAudioProcessor::isFavorite(int index) const{
 if(index<0||index>=100)return false;
 const juce::ScopedLock lock(metadataLock);
 return favorites[static_cast<std::size_t>(index)];
}
void BoomAudioProcessor::toggleFavorite(int index){
 if(index<0||index>=100)return;
 const juce::ScopedLock lock(metadataLock);
 auto& flag=favorites[static_cast<std::size_t>(index)];flag=!flag;
}
// BOOM project-portable user library. Files use the same plain XML snapshot
// as host serialization, with a versioned BOOM_USER wrapper.
int BoomAudioProcessor::saveUserPreset(const juce::String& requestedName){
 const auto name=requestedName.trim().substring(0,80);
 if(name.isEmpty())return -1;
 auto snapshot=apvts.copyState();
 snapshot.setProperty("presetIndex",selectedPreset.load(),nullptr);
 snapshot.setProperty("styleIndex",selectedStyle.load(),nullptr);
 const juce::ScopedLock lock(metadataLock);
 if(userPresets.size()>=512)return -1;
 userPresets.push_back({name,snapshot.createCopy()});
 return static_cast<int>(userPresets.size()-1);
}
bool BoomAudioProcessor::renameUserPreset(int index,const juce::String& requestedName){
 const auto name=requestedName.trim().substring(0,80);
 if(name.isEmpty())return false;
 const juce::ScopedLock lock(metadataLock);
 if(index<0||static_cast<std::size_t>(index)>=userPresets.size())return false;
 userPresets[static_cast<std::size_t>(index)].name=name;
 return true;
}
bool BoomAudioProcessor::deleteUserPreset(int index){
 const juce::ScopedLock lock(metadataLock);
 if(index<0||static_cast<std::size_t>(index)>=userPresets.size())return false;
 userPresets.erase(userPresets.begin()+index);
 return true;
}
int BoomAudioProcessor::userPresetCount() const{
 const juce::ScopedLock lock(metadataLock);
 return static_cast<int>(userPresets.size());
}
juce::String BoomAudioProcessor::userPresetName(int index) const{
 const juce::ScopedLock lock(metadataLock);
 if(index<0||static_cast<std::size_t>(index)>=userPresets.size())return {};
 return userPresets[static_cast<std::size_t>(index)].name;
}
bool BoomAudioProcessor::loadUserPreset(int index){
 juce::ValueTree snapshot;
 {
  const juce::ScopedLock lock(metadataLock);
  if(index<0||static_cast<std::size_t>(index)>=userPresets.size())return false;
  snapshot=userPresets[static_cast<std::size_t>(index)].snapshot.createCopy();
 }
 if(!snapshot.isValid()||!snapshot.hasType("BOOM_STATE"))return false;
 applySnapshot(snapshot);
 selectedPreset.store(-1);
 selectedStyle.store(int(snapshot.getProperty("styleIndex",-1)));
 return true;
}
bool BoomAudioProcessor::exportUserPreset(int index,const juce::File& destination) const{
 juce::ValueTree pack("BOOM_USER");
 {
  const juce::ScopedLock lock(metadataLock);
  if(index<0||static_cast<std::size_t>(index)>=userPresets.size())return false;
  const auto& item=userPresets[static_cast<std::size_t>(index)];
  pack.setProperty("name",item.name,nullptr);
  pack.setProperty("version",1,nullptr);
  pack.addChild(item.snapshot.createCopy(),-1,nullptr);
 }
 if(auto xml=pack.createXml())return xml->writeTo(destination);
 return false;
}
bool BoomAudioProcessor::importUserPreset(const juce::File& source){
 if(!source.existsAsFile()||source.getSize()<=0||source.getSize()>1024*1024)return false;
 const auto xml=juce::XmlDocument::parse(source);
 if(!xml)return false;
 const auto pack=juce::ValueTree::fromXml(*xml);
 if(!pack.isValid()||!pack.hasType("BOOM_USER")||int(pack.getProperty("version",0))!=1)return false;
 const auto name=pack.getProperty("name").toString().trim().substring(0,80);
 const auto snapshot=pack.getChildWithName("BOOM_STATE");
 if(name.isEmpty()||!snapshot.isValid()||!snapshot.getChildWithProperty("id","inputDb").isValid())return false;
 const juce::ScopedLock lock(metadataLock);
 if(userPresets.size()>=512)return false;
 userPresets.push_back({name,snapshot.createCopy()});
 return true;
}

void BoomAudioProcessor::getStateInformation(juce::MemoryBlock& target){
 auto stateCopy=apvts.copyState();
 stateCopy.setProperty("selectedPreset",selectedPreset.load(),nullptr);
 stateCopy.setProperty("selectedStyle",selectedStyle.load(),nullptr);
 juce::ValueTree comparisons("BOOM_AB_SLOTS");
 juce::String favoritesString;
 {
  const juce::ScopedLock lock(metadataLock);
  for(int i=0;i<100;++i)favoritesString+=favorites[static_cast<std::size_t>(i)]?'1':'0';
  for(int i=0;i<2;++i){
   if(comparisonSlots[static_cast<std::size_t>(i)].isValid()){
    auto snapshot=comparisonSlots[static_cast<std::size_t>(i)].createCopy();
    snapshot.setProperty("AB_SLOT_NUMBER",i,nullptr);
    comparisons.addChild(snapshot,-1,nullptr);
   }
  }
 }
 stateCopy.setProperty("favorites",favoritesString,nullptr);
 stateCopy.addChild(comparisons,-1,nullptr);
 juce::ValueTree userLibrary("BOOM_USER_LIBRARY");
 {
  const juce::ScopedLock lock(metadataLock);
  for(const auto& preset:userPresets){
   juce::ValueTree entry("BOOM_USER");
   entry.setProperty("name",preset.name,nullptr);
   entry.setProperty("version",1,nullptr);
   entry.addChild(preset.snapshot.createCopy(),-1,nullptr);
   userLibrary.addChild(entry,-1,nullptr);
  }
 }
 stateCopy.addChild(userLibrary,-1,nullptr);
 if(auto xml=stateCopy.createXml())copyXmlToBinary(*xml,target);
}
void BoomAudioProcessor::setStateInformation(const void* data,int bytes){
 if(auto xml=getXmlFromBinary(data,bytes)){
  auto tree=juce::ValueTree::fromXml(*xml);
  if(tree.isValid()&&tree.hasType("BOOM_STATE")){
   selectedPreset.store(int(tree.getProperty("selectedPreset",-1)));
   selectedStyle.store(int(tree.getProperty("selectedStyle",-1)));
   const juce::String favoritesString=tree.getProperty("favorites",juce::String()).toString();
   const auto comparisons=tree.getChildWithName("BOOM_AB_SLOTS");
   const auto userLibrary=tree.getChildWithName("BOOM_USER_LIBRARY");
   {
    const juce::ScopedLock lock(metadataLock);
    for(int i=0;i<100;++i)favorites[static_cast<std::size_t>(i)]=favoritesString.length()>i&&favoritesString[i]=='1';
    comparisonSlots={};
    userPresets.clear();
    if(userLibrary.isValid())for(int i=0;i<userLibrary.getNumChildren() && userPresets.size()<512;++i){
     const auto entry=userLibrary.getChild(i);
     const auto snapshot=entry.getChildWithName("BOOM_STATE");
     const auto name=entry.getProperty("name").toString().trim().substring(0,80);
     if(entry.hasType("BOOM_USER")&&snapshot.isValid()&&!name.isEmpty())
      userPresets.push_back({name,snapshot.createCopy()});
    }
    for(int i=0;i<comparisons.getNumChildren();++i){
     auto child=comparisons.getChild(i);
     const int slot=int(child.getProperty("AB_SLOT_NUMBER",-1));
     if(slot>=0&&slot<2)comparisonSlots[static_cast<std::size_t>(slot)]=child.createCopy();
    }
   }
   if(comparisons.isValid())tree.removeChild(comparisons,nullptr);
   if(userLibrary.isValid())tree.removeChild(userLibrary,nullptr);
   apvts.replaceState(tree);
  }
 }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new BoomAudioProcessor();}
