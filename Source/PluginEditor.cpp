#include "PluginEditor.h"
#include <BinaryData.h>
#include <array>
#include <algorithm>
#include <cmath>

namespace {
const auto gold=juce::Colour::fromRGB(229,201,148);
const auto copper=juce::Colour::fromRGB(195,112,78);
const auto crimson=juce::Colour::fromRGB(123,22,36);
const auto velvet=juce::Colour::fromRGB(24,9,12);
const auto ivory=juce::Colour::fromRGB(247,226,186);
const auto muted=juce::Colour::fromRGB(181,152,110);
const char* mainIds[]={"inputDb","eq60","eq120","eq3k","eq6k","eq10k","push"};

juce::Font makeFont(float sz,bool bold=false){
 return juce::Font(juce::FontOptions(sz,bold?juce::Font::bold:juce::Font::plain));
}
bool hit(juce::Point<float> p,float x,float y,float w,float h){
 return juce::Rectangle<float>(x,y,w,h).contains(p);
}
}

BoomAudioProcessorEditor::BoomAudioProcessorEditor(BoomAudioProcessor& p)
 :juce::AudioProcessorEditor(&p),proc(p){
 // Map resources through JUCE's original filenames; no assumptions about sanitized C identifiers.
 for(int index=0;index<BinaryData::namedResourceListSize;++index){
  const auto* key=BinaryData::namedResourceList[index];
  int byteCount=0;
  const auto* bytes=BinaryData::getNamedResource(key,byteCount);
  if(bytes==nullptr||byteCount<=0)continue;
  const juce::String original(BinaryData::getNamedResourceOriginalFilename(key));
  if(original.containsIgnoreCase("console-reference"))
   chainSkin=juce::ImageFileFormat::loadFrom(bytes,static_cast<size_t>(byteCount));
  if(original.containsIgnoreCase("browser-reference"))
   browserSkin=juce::ImageFileFormat::loadFrom(bytes,static_cast<size_t>(byteCount));
 }
 english=raw("language")>0.5f;
 addAndMakeVisible(search);
 search.setTextToShowWhenEmpty(english?"Search the 100 factory presets...":"Rechercher parmi les 100 presets...",muted);
 search.setColour(juce::TextEditor::backgroundColourId,juce::Colour::fromRGB(27,13,15));
 search.setColour(juce::TextEditor::textColourId,ivory);
 search.setColour(juce::TextEditor::outlineColourId,copper);
 search.setColour(juce::TextEditor::focusedOutlineColourId,gold);
 search.setFont(makeFont(15));
 search.onTextChange=[this]{refreshRows();};
 addAndMakeVisible(numericEditor);
 numericEditor.setVisible(false);
 numericEditor.setColour(juce::TextEditor::backgroundColourId,juce::Colour::fromRGB(20,10,13));
 numericEditor.setColour(juce::TextEditor::textColourId,ivory);
 numericEditor.setColour(juce::TextEditor::outlineColourId,gold);
 numericEditor.setColour(juce::TextEditor::focusedOutlineColourId,copper);
 numericEditor.setJustification(juce::Justification::centredRight);
 numericEditor.setInputRestrictions(16,"0123456789.,-+");
 numericEditor.onReturnKey=[this]{commitNumeric();};
 numericEditor.onEscapeKey=[this]{cancelNumeric();};
 numericEditor.onFocusLost=[this]{if(editingId.isNotEmpty())commitNumeric();};
 addAndMakeVisible(saveName);
 saveName.setTextToShowWhenEmpty(english?"Save current preset...":"Sauvegarder le preset actuel...",muted);
 saveName.setColour(juce::TextEditor::backgroundColourId,juce::Colour::fromRGB(26,11,13));
 saveName.setColour(juce::TextEditor::textColourId,ivory);
 saveName.setColour(juce::TextEditor::outlineColourId,copper);
 saveName.setColour(juce::TextEditor::focusedOutlineColourId,gold);
 saveName.setFont(makeFont(13));
 saveName.onReturnKey=[this]{saveCurrent();};
 saveName.onEscapeKey=[this]{saveName.clear();giveAwayKeyboardFocus();};
 refreshRows();
 setResizable(true,false);
 setResizeLimits(1100,620,2200,1400);
 setSize(chainW,chainH);
 startTimerHz(30);
}
BoomAudioProcessorEditor::~BoomAudioProcessorEditor(){
 finishDrag();stopTimer();
}
int BoomAudioProcessorEditor::artW()const noexcept {return page==Page::browser?browserW:chainW;}
int BoomAudioProcessorEditor::artH()const noexcept {return page==Page::browser?browserH:chainH;}
juce::Point<float> BoomAudioProcessorEditor::artPoint(const juce::MouseEvent& e)const{
 const auto p=e.position;
 return {p.x*float(artW())/float(juce::jmax(1,getWidth())),
         p.y*float(artH())/float(juce::jmax(1,getHeight()))};
}
juce::Rectangle<int> BoomAudioProcessorEditor::artRect(int x,int y,int w,int h)const{
 return {juce::roundToInt(float(x)*getWidth()/artW()),
         juce::roundToInt(float(y)*getHeight()/artH()),
         juce::roundToInt(float(w)*getWidth()/artW()),
         juce::roundToInt(float(h)*getHeight()/artH())};
}
void BoomAudioProcessorEditor::resized(){
 search.setBounds(artRect(393,138,445,30));
 saveName.setBounds(artRect(940,396,205,27));
 const bool onBrowser=page==Page::browser;
 search.setVisible(onBrowser);saveName.setVisible(onBrowser);
}
void BoomAudioProcessorEditor::showPage(Page newPage){
 finishDrag();cancelNumeric();
 page=newPage;
 if(page==Page::browser){
  refreshRows();
  setSize(browserW,browserH);
 }else{
  setSize(chainW,chainH);
 }
 resized();repaint();
}
float BoomAudioProcessorEditor::raw(const char* id)const{
 if(const auto* v=proc.state().getRawParameterValue(id))return v->load();
 return 0.f;
}
void BoomAudioProcessorEditor::setParam(const char* id,float rawValue){
 if(auto* p=proc.state().getParameter(id))
  p->setValueNotifyingHost(p->convertTo0to1(rawValue));
 repaint();
}
void BoomAudioProcessorEditor::toggle(const char* id){
 if(auto* p=proc.state().getParameter(id)){
  p->beginChangeGesture();
  p->setValueNotifyingHost(p->convertTo0to1(raw(id)>0.5f?0.f:1.f));
  p->endChangeGesture();
 }
 repaint();
}
void BoomAudioProcessorEditor::cancelNumeric(){
 editingId.clear();
 numericEditor.setVisible(false);
}
void BoomAudioProcessorEditor::openFieldEditor(const Field& field){
 cancelNumeric();
 editingId=juce::String(field.id);
 editingScale=field.scale;
 numericEditor.setBounds(artRect(int(field.area.getX()),int(field.area.getY()),
                                 int(field.area.getWidth()+24),int(field.area.getHeight()+4)));
 numericEditor.setText(juce::String(raw(field.id)*field.scale,field.decimals),false);
 numericEditor.setVisible(true);
 numericEditor.toFront(false);
 numericEditor.grabKeyboardFocus();
 numericEditor.selectAll();
}
void BoomAudioProcessorEditor::commitNumeric(){
 if(editingId.isEmpty())return;
 const auto id=editingId;
 const float scale=editingScale;
 const auto entered=numericEditor.getText().trim().replaceCharacter(',', '.');
 editingId.clear();
 numericEditor.setVisible(false);
 if(entered.isEmpty())return;
 if(auto* param=proc.state().getParameter(id)){
  const auto range=param->getNormalisableRange();
  const float value=entered.getFloatValue()/std::max(.00001f,scale);
  if(std::isfinite(value)){
   const float limited=juce::jlimit(range.start,range.end,value);
   param->beginChangeGesture();
   param->setValueNotifyingHost(param->convertTo0to1(limited));
   param->endChangeGesture();
   if(raw("inputLink")>.5f && (id=="inputLTrim"||id=="inputRTrim")){
    const auto other=(id=="inputLTrim"?"inputRTrim":"inputLTrim");
    if(auto* linked=proc.state().getParameter(other)){
     linked->beginChangeGesture();
     linked->setValueNotifyingHost(linked->convertTo0to1(limited));
     linked->endChangeGesture();
    }
   }
  }
 }
 repaint();
}
void BoomAudioProcessorEditor::cyclePreset(int delta){
 const int current=proc.presetIndex();
 const int next=(current<0?(delta<0?99:0):(current+delta+100)%100);
 choosePreset(next);
}
void BoomAudioProcessorEditor::showTrimsMenu(){
 juce::PopupMenu menu;
 menu.addItem(1,"Trim gauche : saisir la valeur en dB");
 menu.addItem(2,"Trim droit : saisir la valeur en dB");
 menu.addItem(3,"Réinitialiser les deux trims");
 menu.addItem(4,raw("inputLink")>.5f?"Délier G/D":"Lier G/D");
 juce::Component::SafePointer<BoomAudioProcessorEditor> safe(this);
 menu.showMenuAsync(juce::PopupMenu::Options(),[safe](int n){
  if(safe==nullptr)return;
  if(n==1)safe->openFieldEditor({{200.f,852.f,104.f,25.f},"inputLTrim",1.f," dB",1});
  if(n==2)safe->openFieldEditor({{200.f,852.f,104.f,25.f},"inputRTrim",1.f," dB",1});
  if(n==3){safe->setParam("inputLTrim",0.f);safe->setParam("inputRTrim",0.f);}
  if(n==4)safe->toggle("inputLink");
 });
}
void BoomAudioProcessorEditor::beginDrag(const char* id,const juce::MouseEvent& e){
 finishDrag();
 if(auto* p=proc.state().getParameter(id)){
  dragged=id;startPosition=artPoint(e);startValue=raw(id);dragChanged=false;pendingNumeric.reset();
  p->beginChangeGesture();isGesture=true;
 }
}
void BoomAudioProcessorEditor::finishDrag(){
 if(isGesture){
  if(auto* p=proc.state().getParameter(dragged))p->endChangeGesture();
 }
 isGesture=false;dragged.clear();
}
const BoomAudioProcessorEditor::Knob* BoomAudioProcessorEditor::activeKnob(juce::Point<float> point)const{
 static const Knob chain[]={{233,624,122,"inputDb",233,767},
  {503,742,62,"eq60",503,809},{663,742,62,"eq120",663,809},
  {822,742,62,"eq3k",822,809},{981,742,62,"eq6k",981,809},
  {1141,742,62,"eq10k",1141,809},{1425,624,123,"push",1425,767}};
 static const Knob browser[]={{218,680,112,"inputDb",218,811},
  {497,778,55,"eq60",497,846},{633,778,55,"eq120",633,846},
  {775,778,55,"eq3k",775,846},{914,778,55,"eq6k",914,846},
  {1052,778,55,"eq10k",1052,846},{1331,680,113,"push",1331,811}};
 const auto& knobs=(page==Page::browser)?browser:chain;
 for(const auto& k:knobs){
  const float dx=point.x-k.x,dy=point.y-k.y;
  if(dx*dx+dy*dy<=k.r*k.r)return &k;
 }
 return nullptr;
}
const std::array<BoomAudioProcessorEditor::Field,21>& BoomAudioProcessorEditor::chainFields()const{
 static const std::array<Field,21> fields{{
  {{248,205,55,22},"colorMix",100.f," %",1},
   {{248,229,55,22},"colorDrive",100.f," %",0},
   {{484,181,57,20},"eq60",1.f," dB",1},
   {{484,204,57,20},"eq120",1.f," dB",1},
   {{484,226,57,20},"eq3k",1.f," dB",1},
   {{484,249,57,20},"eq6k",1.f," dB",1},
   {{484,270,57,20},"eq10k",1.f," dB",1},
   {{730,181,57,21},"deFreq",.001f," kHz",2},
   {{730,205,57,20},"deTh",1.f," dB",1},
   {{730,228,57,21},"deAmt",100.f," %",1},
   {{730,250,57,20},"deRel",1.f," ms",0},
   {{988,180,57,21},"compTh",1.f," dB",1},
   {{988,201,57,21},"compRatio",1.f," :1",2},
   {{988,224,57,21},"compAttack",1.f," ms",0},
   {{988,246,57,21},"compRelease",1.f," ms",0},
   {{988,270,57,20},"compMakeup",1.f," dB",1},
   {{988,292,57,20},"compMix",100.f," %",0},
   {{1240,180,55,21},"limitTh",1.f," dB",1},
   {{1240,202,55,22},"ceiling",1.f," dBFS",1},
   {{1240,226,55,21},"limitRelease",1.f," ms",0},
   {{1510,180,52,21},"outDb",1.f," dB",1}
 }};
 return fields;
}
const BoomAudioProcessorEditor::Field* BoomAudioProcessorEditor::chainField(juce::Point<float> p)const{
 for(const auto& field:chainFields())if(field.area.contains(p))return &field;
 return nullptr;
}
void BoomAudioProcessorEditor::renderChainControls(juce::Graphics& g){
 // Editable APVTS values mask the static numbers in the reference photograph.
 // A value never becomes cosmetic: it always reflects its current host/DSP parameter.
 for(const auto& f:chainFields()){
  const auto value=raw(f.id)*f.scale;
  const auto label=juce::String(value,f.decimals);
  drawValue(g,label,f.area.getX(),f.area.getY(),
            f.area.getWidth(),f.area.getHeight(),f.decimals);
 }
 const int circuit=juce::jlimit(0,2,int(raw("color")+.5f));
 const char* mode[]={"CLEAN","TUBE","TAPE"};
 drawValue(g,mode[circuit],218.f,179.f,88.f,24.f,0);
 struct Switch {float x,y,w,h;const char* id;const char* on;const char* off;bool inverse;};
 const std::array<Switch,12> switches{{
  {228.f,154.f,81.f,24.f,"colorBypass","ACTIF","OFF",true},
  {470.f,154.f,83.f,24.f,"eqBypass","ACTIF","OFF",true},
  {719.f,154.f,82.f,24.f,"deEssBypass","ACTIF","OFF",true},
  {969.f,154.f,85.f,24.f,"compBypass","ACTIF","OFF",true},
  {1227.f,154.f,86.f,24.f,"limiterBypass","ACTIF","OFF",true},
  {846.f,290.f,129.f,25.f,"compStereoLink","LIEN STÉRÉO","DÉLIÉ",false},
  {1095.f,250.f,122.f,27.f,"limiterAutoRelease","AUTO RELEASE","MANUEL",false},
  {1095.f,272.f,122.f,27.f,"limiterStereoLink","LIEN STÉRÉO","DÉLIÉ",false},
  {1351.f,207.f,143.f,23.f,"outPeakGuard","PROTECTION CRÊTES","PROTECTION OFF",false},
  {1351.f,233.f,143.f,23.f,"outputLevelComp","NIVEAU COMPENSÉ","NIVEAU LIBRE",false},
  {146.f,832.f,97.f,28.f,"inputLink","LIEN G/D","G/D DÉLIÉS",false},
  {1511.f,355.f,97.f,29.f,"outputLevelComp","◉ NIVEAU","○ NIVEAU",false}
 }};
 for(const auto& sw:switches){
  const bool activated=(raw(sw.id)>.5f)!=sw.inverse;
  g.setColour(activated?juce::Colour::fromRGB(71,27,28):juce::Colour::fromRGB(20,15,16));
  g.fillRoundedRectangle(sw.x,sw.y,sw.w,sw.h,3.f);
  g.setColour(activated?gold:copper.withAlpha(.65f));
  g.drawRoundedRectangle(sw.x,sw.y,sw.w,sw.h,3.f,1.f);
  g.setColour(activated?ivory:muted);
  g.setFont(makeFont(sw.w>110?10.f:11.f,true));
  g.drawFittedText(activated?sw.on:sw.off,int(sw.x+3),int(sw.y),
                   int(sw.w-6),int(sw.h),juce::Justification::centred,1);
 }
}

void BoomAudioProcessorEditor::drawValue(juce::Graphics& g,const juce::String& value,
  float x,float y,float w,float h,int)const{
 g.setColour(juce::Colour::fromRGB(25,13,15).withAlpha(.97f));
 g.fillRoundedRectangle(x,y,w,h,2.5f);
 g.setColour(copper.withAlpha(.78f));
 g.drawRoundedRectangle(x,y,w,h,2.5f,1.f);
 g.setColour(ivory);g.setFont(makeFont(h>22?16.f:12.f,true));
 g.drawFittedText(value,juce::Rectangle<int>(int(x+3),int(y),int(w-6),int(h)),
                  juce::Justification::centredRight,1);
}
void BoomAudioProcessorEditor::renderKnobs(juce::Graphics& g){
 const bool browse=page==Page::browser;
 for(int i=0;i<7;++i){
  const char* id=mainIds[i];
  const float value=raw(id);
  const bool big=i==0||i==6;
  const float x=big?(i==0?(browse?218.f:233.f):(browse?1331.f:1425.f)):0.f;
  float cx=x,cy=big?(browse?680.f:624.f):(browse?778.f:742.f);
  if(!big){
   const float bx[]={497,633,775,914,1052},ch[]={503,663,822,981,1141};
   cx=browse?bx[i-1]:ch[i-1];
  }
  const float radius=big?(browse?95.f:107.f):(browse?46.f:52.f);
  float mn=0.f,mx=100.f;
  if(auto* p=proc.state().getParameter(id)){
   const auto r=p->getNormalisableRange();mn=r.start;mx=r.end;
  }
  const float norm=juce::jlimit(0.f,1.f,(value-mn)/juce::jmax(.001f,mx-mn));
  const float a=-2.3f+4.6f*norm;
  const float px=cx+std::sin(a)*radius,py=cy-std::cos(a)*radius;
  g.setColour(juce::Colours::black.withAlpha(.65f));g.fillEllipse(px-5,py-4,11,11);
  g.setColour(gold);g.fillEllipse(px-3,py-3,6,6);
  const float textY=big?(browse?807.f:764.f):(browse?840.f:803.f);
  const float textW=big?85.f:76.f;
  juce::String label=juce::String(value,1)+(i==6?" %":" dB");
  g.setColour(juce::Colour::fromRGB(28,14,17).withAlpha(.98f));
  g.fillRect(cx-textW*.5f,textY,textW,24.f);
  g.setColour(ivory);g.setFont(makeFont(big?21.f:16.f));
  g.drawText(label,int(cx-textW*.5f),int(textY),int(textW),23,juce::Justification::centred);
 }
}
void BoomAudioProcessorEditor::renderMeterNeedles(juce::Graphics& g){
 if(!showMeterNeedles)return;
 const bool b=page==Page::browser;
 const float level[]={smoothVuL,smoothVuR};
 const float originX[]={b?594.f:618.f,b?953.f:1034.f};
 const float originY[]={b?695.f:631.f,b?695.f:631.f};
 const float length=b?110.f:121.f;
 for(int c=0;c<2;++c){
  if(level[c]<.001f)continue; // the reference photograph already shows the idle needle
  const float db=juce::jlimit(-20.f,3.f,boom::gainToDb(level[c]/boom::dbToGain(-18.f)));
  const float angle=juce::jmap(db,-20.f,3.f,-1.11f,1.11f);
  const float x=originX[c],y=originY[c];
  const float tipX=x+std::sin(angle)*length,tipY=y-std::cos(angle)*length;
  g.setColour(juce::Colours::black.withAlpha(.45f));g.drawLine(x+1,y+2,tipX+1,tipY+2,4.f);
  g.setColour(juce::Colour::fromRGB(49,31,20));g.drawLine(x,y,tipX,tipY,2.1f);
  g.setColour(copper);g.fillEllipse(x-3,y-3,6,6);
 }
}
void BoomAudioProcessorEditor::renderDynamicText(juce::Graphics& g){
 // Keep the supplied photographed chrome; only redraw state-dependent typography.
 const bool b=page==Page::browser;
 const int si=proc.styleIndex(),pi=proc.presetIndex();
 if(si>=0&&si<40){
  const int x=b?123:130,y=b?34:40,w=b?235:280;
  g.setColour(juce::Colour::fromRGB(24,9,12).withAlpha(.95f));g.fillRect(x,y,w,27);
  g.setColour(ivory);g.setFont(makeFont(22,true));
  g.drawFittedText(juce::String(boom::styles()[static_cast<size_t>(si)].name),x,y,w,27,juce::Justification::centredLeft,1);
 }
 if(pi>=0&&pi<100){
  const int x=b?1204:1250,y=b?34:40,w=b?260:265;
  g.setColour(juce::Colour::fromRGB(24,9,12).withAlpha(.96f));g.fillRect(x,y,w,27);
  g.setColour(ivory);g.setFont(makeFont(19,true));
  g.drawFittedText(juce::String(boom::makePreset(pi).name),x,y,w,27,juce::Justification::centredLeft,1);
 }
 renderKnobs(g);
 renderMeterNeedles(g);
 if(page==Page::chain)renderChainControls(g);
 if(page==Page::browser){
  const bool linked=raw("inputLink")>.5f;
  g.setColour(linked?gold:copper);
  g.drawRoundedRectangle(123.f,862.f,98.f,30.f,3.f,linked?2.f:1.f);
 }
 // Status LEDs for DSP toggles and monitoring.
 const int y=b?524:448;
 const float xMeter=b?566.f:580.f,xBypass=b?790.f:828.f,xMono=b?1030.f:1076.f;
 for(const auto& led:std::array<std::pair<float,const char*>,3>{{{xMeter,"meterInput"},{xBypass,"bypass"},{xMono,"mono"}}}){
  if(raw(led.second)>0.5f){
   g.setColour(copper.withAlpha(.8f));g.fillEllipse(led.first-13,y-5.f,11.f,11.f);
   g.setColour(ivory);g.fillEllipse(led.first-10,y-2.f,5.f,5.f);
  }
 }
 if(page==Page::chain){
  const std::array<std::pair<juce::Rectangle<float>,const char*>,5> bypasses{{
   {{228,157,67,21},"colorBypass"},{{471,157,69,21},"eqBypass"},
   {{720,157,64,21},"deEssBypass"},{{970,157,69,21},"compBypass"},
   {{1227,157,68,21},"limiterBypass"}
  }};
  for(const auto& s:bypasses)if(raw(s.second)>.5f){
    g.setColour(crimson.withAlpha(.85f));g.drawRoundedRectangle(s.first.reduced(1.f),3.f,2.f);
  }
 }
}
void BoomAudioProcessorEditor::renderBrowser(juce::Graphics& g){
 // Redraw only changeable data. The frame, metallic hardware, shadows and
 // photorealistic texture come directly from the user's source image.
 const int rowTop=211,rowH=34,rowW=744,rowLeft=368;
 g.setColour(juce::Colour::fromRGB(13,11,12));g.fillRect(rowLeft,rowTop,rowW,179);
 g.setColour(copper.withAlpha(.65f));g.drawRect(rowLeft,rowTop,rowW,179,1);
 if(gridMode){
  for(int i=0;i<6;++i){
   const int idx=firstVisible+i;
   if(idx>=int(rows.size()))break;
   const auto row=rows[static_cast<size_t>(idx)];
   const bool selected=idx==selectedRow;
   const int x=rowLeft+(i%2)*372,y=rowTop+(i/2)*60;
   const auto p=row.user?boom::makePreset(0):boom::makePreset(row.index);
   const juce::String name=row.user?proc.userPresetName(row.index):juce::String(p.name);
   const juce::String family=row.user?"Utilisateur":juce::String(boom::categories[static_cast<size_t>(p.category)]);
   g.setColour(selected?juce::Colour::fromRGB(102,21,38):juce::Colour::fromRGB(26,16,18));
   g.fillRoundedRectangle(float(x+2),float(y+2),367.f,55.f,4.f);
   g.setColour(selected?gold:copper.withAlpha(.50f));
   g.drawRoundedRectangle(float(x+2),float(y+2),367.f,55.f,4.f,1.f);
   g.setColour(ivory);g.setFont(makeFont(15,selected));
   g.drawFittedText(name,x+13,y+6,308,23,juce::Justification::centredLeft,1);
   g.setColour(muted);g.setFont(makeFont(11));
   g.drawFittedText(family,x+14,y+30,310,20,juce::Justification::centredLeft,1);
   g.setColour(row.user?copper:proc.isFavorite(row.index)?gold:muted);
   g.setFont(makeFont(22));
   g.drawText(row.user?"◆":proc.isFavorite(row.index)?"★":"☆",
              x+329,y+13,35,32,juce::Justification::centred);
  }
 }else{
  for(int i=0;i<5;++i){
   const int idx=firstVisible+i;const int y=rowTop+i*rowH;
   if(idx>=static_cast<int>(rows.size()))break;
   const bool selected=idx==selectedRow;
   const auto r=rows[static_cast<size_t>(idx)];
   g.setColour(selected?juce::Colour::fromRGB(122,18,41):juce::Colour::fromRGB(i%2?17:23,15,15));
   g.fillRect(rowLeft+2,y+1,rowW-4,rowH-1);
   if(selected){g.setColour(copper);g.drawRoundedRectangle(float(rowLeft+2),float(y+1),float(rowW-4),float(rowH-2),4.f,1.5f);}
   juce::String name,cat,style;
   if(r.user){name=proc.userPresetName(r.index);cat="Utilisateur";style="—";}
   else{
    const auto p=boom::makePreset(r.index);
    name=juce::String(p.name);
    cat=juce::String(boom::categories[static_cast<size_t>(p.category)]);
    style=juce::String(boom::styles()[static_cast<size_t>(p.style)].name);
   }
   g.setColour(ivory);g.setFont(makeFont(15,selected));
   g.drawFittedText(name,407,y+1,260,rowH-3,juce::Justification::centredLeft,1);
   g.setFont(makeFont(14));
   g.drawFittedText(cat,680,y+1,211,rowH-3,juce::Justification::centredLeft,1);
   g.drawFittedText(style,896,y+1,173,rowH-3,juce::Justification::centredLeft,1);
   g.setColour((!r.user&&proc.isFavorite(r.index))?gold:muted);g.setFont(makeFont(22));
   g.drawText(r.user?"◆":(!r.user&&proc.isFavorite(r.index)?"★":"☆"),1068,y+1,40,rowH-3,juce::Justification::centred);
  }
 }
 // Active category and changing counts (the photo contains the fixed category names).
 if(category>=0&&category<=11){
  const int cy=136+category*22;
  g.setColour(crimson.withAlpha(.40f));g.fillRoundedRectangle(39.f,float(cy),304.f,22.f,3.f);
  g.setColour(copper.withAlpha(.9f));g.drawRoundedRectangle(39.f,float(cy),304.f,22.f,3.f,1.f);
 }
 const int userCount=proc.userPresetCount();
 int favCount=0;for(int i=0;i<100;++i)if(proc.isFavorite(i))++favCount;
 for(const auto& value:std::array<std::pair<int,int>,2>{{{361,favCount},{384,userCount}}}){
  g.setColour(juce::Colour::fromRGB(54,14,22));g.fillRoundedRectangle(300.f,float(value.first),25.f,19.f,4.f);
  g.setColour(gold);g.setFont(makeFont(12,true));
  g.drawText(juce::String(value.second),302,value.first,22,19,juce::Justification::centred);
 }
 // Right panel: a real parameter preview, calculated from the selected preset.
 g.setColour(juce::Colour::fromRGB(27,11,14));g.fillRect(1155,143,370,207);
 if(selectedRow<0||selectedRow>=static_cast<int>(rows.size()))return;
 const auto r=rows[static_cast<size_t>(selectedRow)];
 juce::String name,cat,style,description;
 boom::Parameters params;
 if(!r.user){
  const auto p=boom::makePreset(r.index);name=juce::String(p.name);params=p.params;
  cat=juce::String(boom::categories[static_cast<size_t>(p.category)]);
  style=juce::String(boom::styles()[static_cast<size_t>(p.style)].name);
  const juce::String phrases[]={
   "Master musical, équilibré et dynamique pour une diffusion moderne.",
   "Guitares précises, dynamique préservée et présence naturelle.",
   "Impact et graves contrôlés avec une présence incisive.",
   "Basses généreuses, voix distinctes et transitoires solides.",
   "Puissance électronique, bas du spectre net et impact maîtrisé.",
   "Traitement subtil préservant le caractère des instruments.",
   "Profondeur, nuances et ampleur pour les bandes-son.",
   "Chaleur analogique et texture harmonique vintage.",
   "Définition adaptée aux usages et plateformes modernes.",
   "Réglage ciblé pour corriger et affiner le master."};
  description=phrases[juce::jlimit(0,9,p.category)];
 }else{
  name=proc.userPresetName(r.index);cat="Utilisateur";style="Preset personnalisé";
  description="Réglages complets enregistrés dans le projet DAW.";
 }
 g.setColour(gold);g.setFont(makeFont(25,true));
 g.drawFittedText(name,1166,145,316,34,juce::Justification::centredLeft,1);
 g.setColour(ivory);g.setFont(makeFont(13));
 g.drawFittedText(cat+"  |  "+style,1169,181,319,23,juce::Justification::centredLeft,1);
 g.drawFittedText(description,1168,208,335,44,juce::Justification::topLeft,2);
 if(!r.user){
  const juce::String labels[]={"DRIVE  "+juce::String(params.inputDb,1)+" dB",
   "PUSH  "+juce::String(params.push,0)+" %",
   "CIRCUIT  "+juce::String(params.color==0?"CLEAN":params.color==1?"TUBE":"TAPE"),
   "GLUE  "+juce::String(params.compRatio,2)+" : 1",
   "PLAFOND  "+juce::String(params.ceilingDb,1)+" dBFS",
   "STYLE  "+style};
  g.setColour(muted);g.drawLine(1168.f,252.f,1512.f,252.f,1.f);
  g.setColour(gold);g.setFont(makeFont(12,true));
  for(int i=0;i<6;++i){const int x=1168+(i%2)*174,y=255+(i/2)*28;
   g.drawFittedText(labels[i],x,y,170,24,juce::Justification::centredLeft,1);
  }
 }
}
void BoomAudioProcessorEditor::renderExtra(juce::Graphics& g){
 const auto area=juce::Rectangle<float>(48.f,106.f,1543.f,229.f);
 juce::ColourGradient bg(juce::Colour::fromRGB(54,17,25),area.getTopLeft(),velvet,area.getBottomRight(),false);
 g.setGradientFill(bg);g.fillRoundedRectangle(area,7.f);
 g.setColour(copper);g.drawRoundedRectangle(area,7.f,1.5f);
 g.setColour(gold);g.setFont(makeFont(18,true));
 auto title=page==Page::meters?"MESURES DE PRÉCISION":page==Page::settings?"RÉGLAGES DE LA CONSOLE":"AIDE / WORKFLOW BOOM";
 g.drawText(title,72,114,700,32,juce::Justification::centredLeft);
 g.drawText("×",1540,113,29,29,juce::Justification::centred);
 g.setColour(muted);g.drawLine(70.f,154.f,1572.f,154.f,1.f);
 const auto m=proc.lastMeters();
 if(page==Page::meters){
  const juce::String labels[]={"LUFS MOMENTANÉ","LUFS COURT TERME","LUFS INTÉGRÉ","LRA",
   "CRÊTE VRAIE EST.","CORRÉLATION","GR COMP.","GR LIMITEUR"};
  const float vals[]={m.momentaryLufs,m.shortTermLufs,m.integratedLufs,m.lra,
   boom::gainToDb(m.truePeak),m.correlation,m.compressorGainReductionDb,m.limiterGainReductionDb};
  for(int i=0;i<8;++i){const int x=70+(i%4)*372,y=165+(i/4)*78;
   g.setColour(gold);g.setFont(makeFont(12,true));g.drawText(labels[i],x,y,350,21,juce::Justification::centredLeft);
   g.setColour(ivory);g.setFont(makeFont(23,true));
   g.drawText(juce::String(vals[i],1)+(i<3?" LUFS":i==3?" LU":i==4?" dBTP*":i>=6?" dB":""),x,y+20,330,40,juce::Justification::centredLeft);
  }
 }else if(page==Page::settings){
  struct Setting {const char* title;const char* id;int x;float scale;int digits;};
  const Setting values[]={
   {"TRIM GAUCHE","inputLTrim",92,1.f,1},
   {"TRIM DROIT","inputRTrim",464,1.f,1},
   {"COMPENSATION COULEUR","colorOut",838,1.f,1},
   {"ATTAQUE DE-ESSER","deAtt",1204,1.f,1}
  };
  for(const auto& f:values){
   g.setColour(gold);g.setFont(makeFont(13,true));
   g.drawText(f.title,f.x,173,300,22,juce::Justification::centredLeft);
   const juce::String unit=juce::String(f.id)=="deAtt"?" ms":" dB";
   drawValue(g,juce::String(raw(f.id)*f.scale,f.digits)+unit,
             float(f.x),202.f,170.f,33.f,f.digits);
  }
  const auto modes=std::array<const char*,3>{{"TRANSPARENT","PUNCH","LOUD"}};
  const auto index=juce::jlimit(0,2,int(raw("limitMode")+.5f));
  g.setColour(gold);g.setFont(makeFont(13,true));
  g.drawText("MODE LIMITEUR",92,254,280,21,juce::Justification::centredLeft);
  drawValue(g,modes[static_cast<size_t>(index)],92.f,277.f,224.f,30.f,0);
  g.setColour(gold);
  g.drawText("LANGUE",464,254,250,21,juce::Justification::centredLeft);
  drawValue(g,english?"ENGLISH":"FRANÇAIS",464.f,277.f,205.f,30.f,0);
  g.setColour(gold);g.drawText("LIEN TRIMS G/D",838,254,270,21,juce::Justification::centredLeft);
  drawValue(g,raw("inputLink")>.5f?"LIÉS":"INDÉPENDANTS",838.f,277.f,205.f,30.f,0);
  g.setColour(gold);g.drawText("AUTO RELEASE",1204,254,270,21,juce::Justification::centredLeft);
  drawValue(g,raw("limiterAutoRelease")>.5f?"ACTIF":"MANUEL",1204.f,277.f,205.f,30.f,0);
 }else{
  g.setColour(ivory);g.setFont(makeFont(18));
  g.drawFittedText("1. Choisissez votre Style musical parmi les 40 Styles.\n"
   "2. Ouvrez le navigateur de 100 presets depuis le menu PRESET DE MASTERING.\n"
   "3. Réglez INPUT DRIVE, les cinq bandes EQ et PUSH pour affiner le mastering.\n"
   "4. Comparez A et B ; contrôlez les mesures LUFS avant export.\n"
   "Attention : plafond true-peak et conformité EBU R128 à valider indépendamment.",
   75,167,1455,143,juce::Justification::topLeft,6);
 }
}
void BoomAudioProcessorEditor::paint(juce::Graphics& g){
 const juce::Image& img=page==Page::browser?browserSkin:chainSkin;
 g.fillAll(velvet);
 if(img.isValid())g.drawImageWithin(img,0,0,getWidth(),getHeight(),juce::RectanglePlacement::stretchToFit);
 juce::Graphics::ScopedSaveState state(g);
 g.addTransform(juce::AffineTransform::scale(float(getWidth())/artW(),float(getHeight())/artH()));
 renderDynamicText(g);
 if(page==Page::browser)renderBrowser(g);
 if(page==Page::meters||page==Page::settings||page==Page::help)renderExtra(g);
 if(notice.isNotEmpty()){
  const int y=page==Page::browser?423:335;
  g.setColour(velvet.withAlpha(.96f));g.fillRoundedRectangle(603.f,float(y),462.f,24.f,4.f);
  g.setColour(gold);g.setFont(makeFont(12,true));
  g.drawFittedText(notice,607,y+2,454,21,juce::Justification::centred,1);
 }
}
void BoomAudioProcessorEditor::timerCallback(){
 const auto m=proc.lastMeters();
 smoothVuL+=.22f*(m.vuL-smoothVuL);
 smoothVuR+=.22f*(m.vuR-smoothVuR);
 const bool next=raw("language")>.5f;
 if(next!=english){english=next;
  search.setTextToShowWhenEmpty(english?"Search the 100 factory presets...":"Rechercher parmi les 100 presets...",muted);
  saveName.setTextToShowWhenEmpty(english?"Save current preset...":"Sauvegarder le preset actuel...",muted);
 }
 repaint();
}
void BoomAudioProcessorEditor::refreshRows(){
 PresetRow previously{false,-1};
 if(selectedRow>=0&&selectedRow<int(rows.size()))previously=rows[static_cast<size_t>(selectedRow)];
 rows.clear();
 const auto query=search.getText().trim();
 const bool showFactory=category!=11;
 if(showFactory)for(int i=0;i<100;++i){
  const auto p=boom::makePreset(i);
  if(category>=0&&category<=9&&p.category!=category)continue;
  if(category==10&&!proc.isFavorite(i))continue;
  if(styleFilter>=0&&p.style!=styleFilter)continue;
  if(query.isNotEmpty()&&!juce::String(p.name).containsIgnoreCase(query)&&
    !juce::String(boom::styles()[static_cast<size_t>(p.style)].name).containsIgnoreCase(query))continue;
  rows.push_back({false,i});
 }
 if((category==11||category==-1)&&styleFilter<0)
  for(int i=0;i<proc.userPresetCount();++i)
   if(proc.userPresetName(i).containsIgnoreCase(query))rows.push_back({true,i});
 selectedRow=rows.empty()?-1:0;
 for(int i=0;i<int(rows.size());++i)
  if(rows[static_cast<size_t>(i)].user==previously.user&&rows[static_cast<size_t>(i)].index==previously.index)
   {selectedRow=i;break;}
 const int visible=gridMode?6:5;
 firstVisible=juce::jlimit(0,juce::jmax(0,int(rows.size())-visible),firstVisible);
 if(selectedRow>=0&&selectedRow<firstVisible)firstVisible=selectedRow;
 if(selectedRow>=firstVisible+visible)firstVisible=selectedRow-visible+1;
 repaint();
}
void BoomAudioProcessorEditor::selectRow(int idx){
 if(idx<0||idx>=int(rows.size()))return;
 selectedRow=idx;
 if(idx<firstVisible)firstVisible=idx;
 if(idx>=firstVisible+(gridMode?6:5))firstVisible=idx-(gridMode?5:4);
 repaint();
}
void BoomAudioProcessorEditor::loadRow(){
 if(selectedRow<0||selectedRow>=int(rows.size()))return;
 const auto r=rows[static_cast<size_t>(selectedRow)];
 if(r.user){
  if(proc.loadUserPreset(r.index))notice="Preset utilisateur chargé";
 }else choosePreset(r.index);
 repaint();
}
void BoomAudioProcessorEditor::choosePreset(int idx){
 if(idx<0||idx>=100)return;
 proc.loadFactoryPreset(idx);
 notice="Preset chargé : "+juce::String(boom::makePreset(idx).name);
 repaint();
}
void BoomAudioProcessorEditor::chooseStyle(int idx){
 if(idx<0||idx>=40)return;
 proc.loadStyle(idx);
 notice="Style chargé : "+juce::String(boom::styles()[static_cast<size_t>(idx)].name);
 repaint();
}
void BoomAudioProcessorEditor::showPresetsMenu(){
 juce::PopupMenu root;
 root.addItem(1,english?"Open preset browser":"Ouvrir le navigateur de presets");
 root.addSeparator();
 for(int c=0;c<10;++c){
  juce::PopupMenu sub;
  for(int j=0;j<10;++j){const int i=c*10+j;
   sub.addItem(100+i,juce::String(boom::makePreset(i).name));
  }
  root.addSubMenu(juce::String(boom::categories[static_cast<size_t>(c)]),sub);
 }
 juce::Component::SafePointer<BoomAudioProcessorEditor> safe(this);
 root.showMenuAsync(juce::PopupMenu::Options(),[safe](int result){
  if(safe==nullptr)return;
  if(result==1)safe->showPage(Page::browser);
  else if(result>=100&&result<200)safe->choosePreset(result-100);
 });
}
void BoomAudioProcessorEditor::showStyleMenu(){
 juce::PopupMenu m;
 m.addItem(1,english?"Show all styles":"Afficher tous les Styles");
 m.addSeparator();
 for(int i=0;i<40;++i)m.addItem(i+100,juce::String(boom::styles()[static_cast<size_t>(i)].name));
 juce::Component::SafePointer<BoomAudioProcessorEditor> safe(this);
 m.showMenuAsync(juce::PopupMenu::Options(),[safe](int id){
  if(safe==nullptr)return;
  if(id==1){safe->showPage(Page::browser);safe->styleFilter=-1;safe->category=-1;safe->refreshRows();}
  else if(id>=100&&id<140)safe->chooseStyle(id-100);
 });
}
void BoomAudioProcessorEditor::saveCurrent(){
 const auto name=saveName.getText().trim();
 if(name.isEmpty()){notice="Entrez un nom de preset, puis validez avec Entrée";repaint();return;}
 const int newIndex=proc.saveUserPreset(name);
 notice=newIndex>=0?"Preset utilisateur enregistré dans le projet DAW":"Impossible d'enregistrer ce preset";
 if(newIndex>=0){category=11;search.clear();styleFilter=-1;refreshRows();
  for(int i=0;i<int(rows.size());++i)if(rows[static_cast<size_t>(i)].user&&rows[static_cast<size_t>(i)].index==newIndex){selectRow(i);break;}
 }
 saveName.clear();repaint();
}
void BoomAudioProcessorEditor::showUserMenu(){
 juce::PopupMenu menu;
 menu.addItem(1,"Enregistrer les réglages actuels");
 menu.addItem(2,"Renommer le preset utilisateur");
 menu.addItem(3,"Supprimer le preset utilisateur");
 menu.addSeparator();
 menu.addItem(4,"Importer .boom");
 menu.addItem(5,"Exporter .boom");
 juce::Component::SafePointer<BoomAudioProcessorEditor> safe(this);
 menu.showMenuAsync(juce::PopupMenu::Options(),[safe](int command){
  if(safe==nullptr)return;
  auto& e=*safe;
  if(command==1){e.saveName.grabKeyboardFocus();e.notice="Saisissez un nom et validez avec Entrée";}
  else if(command==4)e.importUser();
  else if(command>=2&&command<=5){
   if(e.selectedRow<0||e.selectedRow>=int(e.rows.size())||!e.rows[static_cast<size_t>(e.selectedRow)].user){
    e.notice="Sélectionnez un preset utilisateur";e.repaint();return;
   }
   const int idx=e.rows[static_cast<size_t>(e.selectedRow)].index;
   if(command==2){
    const auto s=e.saveName.getText().trim();
    if(!s.isEmpty()){e.proc.renameUserPreset(idx,s);e.notice="Preset renommé";e.refreshRows();}
    else{e.saveName.grabKeyboardFocus();e.notice="Entrez le nouveau nom avant de choisir Renommer";}
   }else if(command==3){
    if(!e.deleteArmed){e.deleteArmed=true;e.notice="Choisissez Supprimer une seconde fois pour confirmer";}
    else{e.proc.deleteUserPreset(idx);e.deleteArmed=false;e.notice="Preset supprimé";e.refreshRows();}
   }else if(command==5)e.exportUser();
   e.repaint();
  }
 });
}
void BoomAudioProcessorEditor::importUser(){
 chooser=std::make_unique<juce::FileChooser>("Importer un preset BOOM",juce::File{},"*.boom");
 juce::Component::SafePointer<BoomAudioProcessorEditor> safe(this);
 chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
  [safe](const juce::FileChooser& c){
   if(safe==nullptr)return;
   const auto file=c.getResult();if(file==juce::File{})return;
   const bool ok=safe->proc.importUserPreset(file);
   safe->notice=ok?"Preset .boom importé":"Import impossible";
   if(ok){safe->category=11;safe->refreshRows();}
   safe->repaint();
  });
}
void BoomAudioProcessorEditor::exportUser(){
 if(selectedRow<0||selectedRow>=int(rows.size()))return;
 const auto r=rows[static_cast<size_t>(selectedRow)];if(!r.user)return;
 const auto initial=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
  .getChildFile(juce::File::createLegalFileName(proc.userPresetName(r.index))+".boom");
 chooser=std::make_unique<juce::FileChooser>("Exporter un preset BOOM",initial,"*.boom");
 juce::Component::SafePointer<BoomAudioProcessorEditor> safe(this);
 chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|
   juce::FileBrowserComponent::warnAboutOverwriting,
   [safe,index=r.index](const juce::FileChooser& c){
    if(safe==nullptr)return;
    const auto f=c.getResult();if(f==juce::File{})return;
    safe->notice=safe->proc.exportUserPreset(index,f.withFileExtension(".boom"))?
       "Preset exporté":"Export impossible";
    safe->repaint();
   });
}
void BoomAudioProcessorEditor::handleClick(juce::Point<float> p,const juce::MouseEvent& e){
 const bool b=page==Page::browser;
 // Working preset navigation arrows on both supplied reference skins.
 if(hit(p,b?1157.f:1184.f,31.f,36.f,47.f)){cyclePreset(-1);return;}
 if(hit(p,b?1496.f:1554.f,31.f,36.f,47.f)){cyclePreset(1);return;}
 // Header selectors — selection always recalls the DSP parameters.
 if(hit(p,b?31.f:88.f,0,b?370.f:365.f,92)){
  showStyleMenu();return;
 }
 if(hit(p,b?1150.f:1190.f,0,b?405.f:390.f,95)){
  showPresetsMenu();return;
 }
 // Browse mode: interactive category, scrolling rows and detailed preset selection.
 if(b&&hit(p,20,98,1525,338)){
  if(hit(p,1500,99,29,28)){showPage(Page::chain);return;}
  if(hit(p,1041,135,43,35)){gridMode=false;refreshRows();return;}
  if(hit(p,1085,135,43,35)){gridMode=true;refreshRows();return;}
  if(hit(p,29,135,310,278)){
   const int next=juce::jlimit(0,11,int((p.y-136.f)/22.f));
   category=next;styleFilter=-1;firstVisible=0;refreshRows();return;
  }
  if(hit(p,854,138,176,30)){
   juce::PopupMenu styles;styles.addItem(1,"Tous les Styles");
   for(int i=0;i<40;++i)styles.addItem(i+2,juce::String(boom::styles()[static_cast<size_t>(i)].name));
   juce::Component::SafePointer<BoomAudioProcessorEditor> safe(this);
   styles.showMenuAsync(juce::PopupMenu::Options(),[safe](int id){
    if(safe==nullptr||id<=0)return;
    safe->styleFilter=id==1?-1:id-2;safe->firstVisible=0;safe->refreshRows();
   });return;
  }
  if(hit(p,369,211,743,180)){
   const int idx=firstVisible+int((p.y-211.f)/(gridMode?60.f:34.f))*(gridMode?2:1)
                        +(gridMode&&p.x>740?1:0);
   if(idx>=0&&idx<int(rows.size())){
    selectRow(idx);
    const auto r=rows[static_cast<size_t>(idx)];
    const bool clickedStar=gridMode?(p.x>(p.x>740.f?1070.f:697.f)):(p.x>1066.f);
    if(clickedStar&&!r.user){proc.toggleFavorite(r.index);refreshRows();}
   }
   return;
  }
  if(hit(p,1195,353,285,39)){loadRow();return;}
  if(hit(p,373,396,119,26)){selectRow(selectedRow-1);return;}
  if(hit(p,501,396,117,26)){selectRow(selectedRow+1);return;}
  if(hit(p,631,396,123,26)){category=10;firstVisible=0;refreshRows();return;}
  if(hit(p,1113,132,44,34)){category=-1;styleFilter=-1;firstVisible=0;refreshRows();return;}
  if(hit(p,771,396,159,26)){showUserMenu();return;}
  if(hit(p,1466,143,35,31)&&selectedRow>=0&&selectedRow<int(rows.size())){
   const auto r=rows[static_cast<size_t>(selectedRow)];
   if(!r.user){proc.toggleFavorite(r.index);refreshRows();}return;
  }
 }
 // Master console always remains playable while browser, metering or help is open.
 const int bar=b?441:349;
 if(hit(p,b?30.f:48.f,bar,100,36)){showPage(Page::chain);return;}
 if(hit(p,b?132.f:159.f,bar,111,36)){showPage(Page::meters);return;}
 if(hit(p,b?250.f:283.f,bar,113,36)){showPage(Page::settings);return;}
 if(hit(p,b?370.f:411.f,bar,93,36)){showPage(Page::help);return;}
 if(hit(p,b?464.f:515.f,bar,77,36)){toggle("language");english=raw("language")>.5f;repaint();return;}
 if(hit(p,b?1145.f:1195.f,bar,42,36)){proc.recallComparisonState(0);selectedComparison=0;repaint();return;}
 if(hit(p,b?1190.f:1240.f,bar,45,36)){proc.recallComparisonState(1);selectedComparison=1;repaint();return;}
 if(hit(p,b?1223.f:1274.f,bar,54,36)){proc.storeComparisonState(0);notice="État A mémorisé";repaint();return;}
 if(hit(p,b?1278.f:1331.f,bar,54,36)){proc.storeComparisonState(1);notice="État B mémorisé";repaint();return;}
 if(hit(p,b?1333.f:1384.f,bar,54,36)){proc.copyComparisonState(0,1);notice="A copié vers B";repaint();return;}
 if(hit(p,b?1388.f:1442.f,bar,54,36)){proc.copyComparisonState(1,0);notice="B copié vers A";repaint();return;}
 if(hit(p,b?1443.f:1500.f,bar,86,36)){toggle("outputLevelComp");return;}
 // Existing meter / bypass / mono controls on both photographs.
 if(hit(p,b?438.f:458.f,b?507.f:429.f,b?230.f:248.f,38)){toggle("meterInput");return;}
 if(hit(p,b?738.f:776.f,b?506.f:429.f,b?104.f:120.f,40)){toggle("bypass");return;}
 if(hit(p,b?907.f:965.f,b?506.f:429.f,b?200.f:225.f,40)){toggle("mono");return;}
 // INPUT LINK and TRIMS remain operative when the browser is open.
 if(b&&hit(p,124.f,862.f,100.f,31.f)){toggle("inputLink");return;}
 if(b&&hit(p,230.f,862.f,72.f,31.f)){showTrimsMenu();return;}
 if(page==Page::meters||page==Page::settings||page==Page::help){
  if(hit(p,1527,106,52,47)){showPage(Page::chain);return;}
  if(page==Page::meters&&hit(p,1300,165,240,140))proc.requestLoudnessReset();
  if(page==Page::settings){
   const std::array<Field,4> settings{{
    {{92,202,170,33},"inputLTrim",1.f," dB",1},
    {{464,202,170,33},"inputRTrim",1.f," dB",1},
    {{838,202,170,33},"colorOut",1.f," dB",1},
    {{1204,202,170,33},"deAtt",1.f," ms",1}
   }};
   for(const auto& f:settings)if(f.area.contains(p)){openFieldEditor(f);return;}
   if(hit(p,92,277,224,30)){
    juce::PopupMenu modes;
    modes.addItem(1,"TRANSPARENT");modes.addItem(2,"PUNCH");modes.addItem(3,"LOUD");
    juce::Component::SafePointer<BoomAudioProcessorEditor> safe(this);
    modes.showMenuAsync(juce::PopupMenu::Options(),[safe](int index){
     if(safe!=nullptr&&index>=1&&index<=3)safe->setParam("limitMode",float(index-1));
    });return;
   }
   if(hit(p,464,277,205,30)){toggle("language");return;}
   if(hit(p,838,277,205,30)){toggle("inputLink");return;}
   if(hit(p,1204,277,205,30)){toggle("limiterAutoRelease");return;}
  }
  return;
 }
 if(page==Page::chain){
  if(hit(p,1550,102,48,30)){showPage(Page::browser);return;}
  static const std::array<std::pair<juce::Rectangle<float>,const char*>,5> toggles{{
   {{228,154,81,24},"colorBypass"},{{470,154,83,24},"eqBypass"},
   {{719,154,82,24},"deEssBypass"},{{969,154,85,24},"compBypass"},
   {{1227,154,86,24},"limiterBypass"}
  }};
  for(const auto& t:toggles)if(t.first.contains(p)){toggle(t.second);return;}
  if(hit(p,218,180,86,29)){
   juce::PopupMenu mode;mode.addItem(1,"CLEAN");mode.addItem(2,"TUBE");mode.addItem(3,"TAPE");
   juce::Component::SafePointer<BoomAudioProcessorEditor> safe(this);
   mode.showMenuAsync(juce::PopupMenu::Options(),[safe](int i){if(safe!=nullptr&&i>=1&&i<=3)safe->setParam("color",float(i-1));});return;
  }
  if(hit(p,846,290,129,29)){toggle("compStereoLink");return;}
  if(hit(p,1095,250,122,27)){toggle("limiterAutoRelease");return;}
  if(hit(p,1095,272,122,27)){toggle("limiterStereoLink");return;}
  if(hit(p,1351,207,143,23)){toggle("outPeakGuard");return;}
  if(hit(p,1351,233,143,23)){toggle("outputLevelComp");return;}
  if(hit(p,146,832,97,28)){toggle("inputLink");return;}
  if(hit(p,250,832,70,28)){showTrimsMenu();return;}
  if(hit(p,1494,154,78,25)){toggle("outPeakGuard");return;}
  if(hit(p,296,154,32,24)){toggle("colorBypass");return;}
  if(hit(p,537,154,32,24)){toggle("eqBypass");return;}
  if(hit(p,786,154,32,24)){toggle("deEssBypass");return;}
  if(hit(p,1036,154,32,24)){toggle("compBypass");return;}
  if(hit(p,1293,154,32,24)){toggle("limiterBypass");return;}
  if(const auto* field=chainField(p)){
   beginDrag(field->id,e);pendingNumeric=*field;return;
  }
 }
 if(const auto* knob=activeKnob(p)){beginDrag(knob->id,e);return;}
 // Convenience: clicking the browser button in the preset header opens the full browser.
}
void BoomAudioProcessorEditor::mouseDown(const juce::MouseEvent& e){
 if(e.mods.isRightButtonDown()){
  const auto p=artPoint(e);
  if(page==Page::chain){
   if(const auto* field=chainField(p)){
    const Field target=*field;
    juce::PopupMenu popup;
    popup.addItem(1,"Saisir une valeur…");
    popup.addItem(2,"Rétablir la valeur par défaut");
    juce::Component::SafePointer<BoomAudioProcessorEditor> safe(this);
    popup.showMenuAsync(juce::PopupMenu::Options(),[safe,target](int choice){
     if(safe==nullptr)return;
     if(choice==1)safe->openFieldEditor(target);
     if(choice==2)if(auto* param=safe->proc.state().getParameter(target.id))
      safe->setParam(target.id,param->convertFrom0to1(param->getDefaultValue()));
    });
    return;
   }
  }
  if(const auto* k=activeKnob(p)){
   juce::PopupMenu menu;menu.addItem(1,"Réinitialiser le paramètre");
   juce::Component::SafePointer<BoomAudioProcessorEditor> safe(this);
   const juce::String id(k->id);
   menu.showMenuAsync(juce::PopupMenu::Options(),[safe,id](int action){
    if(safe==nullptr||action!=1)return;
    if(auto* param=safe->proc.state().getParameter(id))safe->setParam(id.toRawUTF8(),param->convertFrom0to1(param->getDefaultValue()));
   });return;
  }
 }
 handleClick(artPoint(e),e);
}
void BoomAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e){
 if(!isGesture||dragged.isEmpty())return;
 auto* p=proc.state().getParameter(dragged);if(!p)return;
 const auto range=p->getNormalisableRange();
 const auto now=artPoint(e);
 if(std::abs(now.x-startPosition.x)+std::abs(now.y-startPosition.y)<3.f&&!dragChanged)return;
 dragChanged=true;
 const float span=range.end-range.start;
 const float sensitivity=e.mods.isShiftDown()?.0008f:.0038f;
 const float value=juce::jlimit(range.start,range.end,startValue+(startPosition.y-now.y)*span*sensitivity);
 setParam(dragged.toRawUTF8(),value);
}
void BoomAudioProcessorEditor::mouseUp(const juce::MouseEvent&){
 const auto editorField=pendingNumeric;
 const bool clickToEdit=isGesture&&!dragChanged&&editorField.has_value();
 finishDrag();
 pendingNumeric.reset();
 if(clickToEdit)openFieldEditor(*editorField);
}
void BoomAudioProcessorEditor::mouseDoubleClick(const juce::MouseEvent& e){
 const auto p=artPoint(e);
 if(page==Page::chain){if(const auto* f=chainField(p)){openFieldEditor(*f);return;}}
 if(page==Page::browser && hit(p,368,211,744,181)){loadRow();return;}
 if(const auto* k=activeKnob(p)){
  if(auto* param=proc.state().getParameter(k->id))setParam(k->id,param->convertFrom0to1(param->getDefaultValue()));
 }
}
void BoomAudioProcessorEditor::mouseWheelMove(const juce::MouseEvent& e,const juce::MouseWheelDetails& d){
 if(page==Page::browser){const auto p=artPoint(e);
  if(hit(p,363,206,749,183)){
   firstVisible=juce::jlimit(0,juce::jmax(0,int(rows.size())-(gridMode?6:5)),firstVisible+(d.deltaY<0?1:-1)*juce::jmax(1,int(std::abs(d.deltaY)*3)));
   repaint();return;
  }
 }
 if(page==Page::chain){
  if(const auto* f=chainField(artPoint(e))){
   if(auto* param=proc.state().getParameter(f->id)){
    const auto r=param->getNormalisableRange();
    const float amount=(r.interval>0.f?r.interval:(r.end-r.start)*.01f)*(d.deltaY>0?1.f:-1.f);
    param->beginChangeGesture();setParam(f->id,raw(f->id)+amount);param->endChangeGesture();
   }
   return;
  }
 }
 if(const auto* k=activeKnob(artPoint(e))){
  if(auto* param=proc.state().getParameter(k->id)){
   auto r=param->getNormalisableRange();
   param->beginChangeGesture();
   setParam(k->id,raw(k->id)+(d.deltaY>0?1.f:-1.f)*(r.end-r.start)*.01f);
   param->endChangeGesture();
  }
 }
}
bool BoomAudioProcessorEditor::keyPressed(const juce::KeyPress& key){
 if(key==juce::KeyPress::escapeKey){showPage(Page::chain);return true;}
 if(page==Page::browser && key==juce::KeyPress::returnKey){loadRow();return true;}
 if(page==Page::browser && key==juce::KeyPress::downKey){selectRow(selectedRow+1);return true;}
 if(page==Page::browser && key==juce::KeyPress::upKey){selectRow(selectedRow-1);return true;}
 return false;
}