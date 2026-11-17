#include "PluginEditor.h"
#include "BoomUiTheme.h"
#include "BoomBrowser.h"
#include "BoomChainPanel.h"
#include "BoomChrome.h"
#include <BinaryData.h>
#include <algorithm>
#include <cmath>

namespace {
using namespace boom;
using namespace boom::ui;
const auto ivory=boom::ui::theme::ivory();
const auto copper=boom::ui::theme::copper();
const auto gold=boom::ui::theme::gold();
const auto charcoal=boom::ui::theme::background();
juce::Font font(float size,bool bold=false){
 return boom::ui::theme::font(size,bold);
}
bool in(juce::Point<float> p,float x,float y,float w,float h){
 return p.x>=x && p.x<x+w && p.y>=y && p.y<y+h;
}
}

// Choose the largest crisp, proportional size that fits the display work area.
// Artwork coordinates are unchanged, so all existing mouse hit regions scale too.
static juce::Point<int> boomFittedWindowSize(int originalWidth,int originalHeight){
 const auto& displays=juce::Desktop::getInstance().getDisplays();
 const auto* display=displays.getDisplayForPoint(juce::Desktop::getMousePosition());
 if(display==nullptr)display=displays.getPrimaryDisplay();
 if(display==nullptr)return {originalWidth,originalHeight};
 const auto available=display->userArea;
 // Reserve space for the DAW's plugin title bar and the OS window controls.
 const int maxWidth=juce::jmax(1,available.getWidth()-44);
 const int maxHeight=juce::jmax(1,available.getHeight()-72);
 const float ratio=std::min(1.0f,std::min(float(maxWidth)/originalWidth,
                                          float(maxHeight)/originalHeight));
 return {juce::jmax(1,juce::roundToInt(originalWidth*ratio)),
         juce::jmax(1,juce::roundToInt(originalHeight*ratio))};
}

BoomAudioProcessorEditor::BoomAudioProcessorEditor(BoomAudioProcessor& p)
 :AudioProcessorEditor(&p),processor(p){
 // Resources are discovered by original filename, not an assumed JUCE symbol.
 // This is valid on MSVC and Apple Clang with JUCE 8 BinaryData generation.
 for(int i=0;i<BinaryData::namedResourceListSize;++i){
  const auto* resourceName=BinaryData::namedResourceList[i];
  int count=0;
  const auto* data=BinaryData::getNamedResource(resourceName,count);
  if(data==nullptr||count<=0)continue;
  const juce::String path(BinaryData::getNamedResourceOriginalFilename(resourceName));
  if(path.containsIgnoreCase("console-reference"))
   chainSkin=juce::ImageFileFormat::loadFrom(data,static_cast<std::size_t>(count));
  else if(path.containsIgnoreCase("browser-reference"))
   browserSkin=juce::ImageFileFormat::loadFrom(data,static_cast<std::size_t>(count));
 }
 // Capture real knob faces from the approved source art before preparing meters.
 prepareFaces();
 english=read("language")>.5f;
 auto styleText=[&](juce::TextEditor& e,juce::String placeholder,float size){
  addAndMakeVisible(e);
  e.setColour(juce::TextEditor::backgroundColourId,charcoal.withAlpha(.98f));
  e.setColour(juce::TextEditor::textColourId,ivory);
  e.setColour(juce::TextEditor::highlightColourId,copper);
  e.setColour(juce::TextEditor::outlineColourId,juce::Colours::transparentBlack);
  e.setColour(juce::TextEditor::focusedOutlineColourId,gold);
  e.setFont(font(size));
  e.setTextToShowWhenEmpty(placeholder,gold.withAlpha(.75f));
 };
 styleText(search,english?"Search presets":"Rechercher parmi les 100 presets...",15.f);
 search.onTextChange=[this]{refreshResults();};
 styleText(userPresetName,english?"New preset name":"Nom du preset utilisateur...",13.f);
 userPresetName.onReturnKey=[this]{saveUser();};
 styleText(numericEdit,"",14.f);
 numericEdit.setInputRestrictions(18,"0123456789.,-+");
 numericEdit.setJustification(juce::Justification::centredRight);
 numericEdit.onReturnKey=[this]{commitNumeric();};
 numericEdit.onEscapeKey=[this]{cancelNumeric();};
 numericEdit.onFocusLost=[this]{if(editing.parameter.isNotEmpty())commitNumeric();};
 numericEdit.setVisible(false);
 // A true JUCE browser replaces the static photo's upper panel.
 nativeBrowser=std::make_unique<BoomBrowser>(processor,[this]{showPage(Page::chain);});
 addAndMakeVisible(*nativeBrowser);
 nativeBrowser->setVisible(false);
 nativeChain=std::make_unique<BoomChainPanel>(processor);
 addAndMakeVisible(*nativeChain);
 nativeChain->setVisible(true);
 auto invoke=[this](BoomChrome::Command action){
  using Cmd=BoomChrome::Command;
  switch(action){
   case Cmd::Style:openStyleMenu();break;
   case Cmd::Preset:openPresetMenu();break;
   case Cmd::Previous:cyclePreset(-1);break;
   case Cmd::Next:cyclePreset(1);break;
   case Cmd::Chain:showPage(Page::chain);break;
   case Cmd::Meters:showPage(Page::meters);break;
   case Cmd::Settings:showPage(Page::settings);break;
   case Cmd::Help:showPage(Page::help);break;
   case Cmd::Language:openLanguageMenu();break;
   case Cmd::RecallA:processor.recallComparisonState(0);selectedRecall=0;break;
   case Cmd::RecallB:processor.recallComparisonState(1);selectedRecall=1;break;
   case Cmd::StoreA:processor.storeComparisonState(0);break;
   case Cmd::StoreB:processor.storeComparisonState(1);break;
   case Cmd::CopyAtoB:processor.copyComparisonState(0,1);break;
   case Cmd::CopyBtoA:processor.copyComparisonState(1,0);break;
   case Cmd::Level:toggle("outputLevelComp");break;
  }
  refreshChrome();
 };
 nativeHeader=std::make_unique<BoomChrome>(BoomChrome::Area::Header,invoke);
 nativeToolbar=std::make_unique<BoomChrome>(BoomChrome::Area::Toolbar,invoke);
 addAndMakeVisible(*nativeHeader);
 addAndMakeVisible(*nativeToolbar);
 setWantsKeyboardFocus(true);
 setResizable(false,false);  // Keep the full console visible, without scrollbars.
 refreshResults();
 const auto initialSize=boomFittedWindowSize(CW,CH);
 setSize(initialSize.x,initialSize.y);
 resized();refreshChrome();
 startTimerHz(30);
}
void BoomAudioProcessorEditor::refreshChrome(){
 if(nativeHeader!=nullptr){
  const int style=processor.styleIndex();
  const int preset=processor.presetIndex();
  const auto styleText=style>=0&&style<40?
   juce::String(boom::styles()[static_cast<std::size_t>(style)].name):
   juce::String(english?"Choose a Style":"Choisir un Style");
  const auto presetText=preset>=0&&preset<100?
   juce::String(boom::makePreset(preset).name):
   juce::String(english?"Custom Master":"Master personnalis\u00E9");
  nativeHeader->setNames(styleText,presetText);
  nativeHeader->setState(english,int(page),read("outputLevelComp")>.5f,selectedRecall);
 }
 if(nativeToolbar!=nullptr)
  nativeToolbar->setState(english,int(page),read("outputLevelComp")>.5f,selectedRecall);
}
BoomAudioProcessorEditor::~BoomAudioProcessorEditor(){
 stopTimer();
 endGesture();
}
int BoomAudioProcessorEditor::artWidth()const noexcept {
 return page==Page::browser?BW:CW;
}
int BoomAudioProcessorEditor::artHeight()const noexcept {
 return page==Page::browser?BH:CH;
}
juce::Point<float> BoomAudioProcessorEditor::pointOf(const juce::MouseEvent& e)const{
 return {e.position.x*artWidth()/juce::jmax(1,getWidth()),
         e.position.y*artHeight()/juce::jmax(1,getHeight())};
}
juce::Rectangle<int> BoomAudioProcessorEditor::canvasRect(float x,float y,float w,float h)const{
 return {juce::roundToInt(x*getWidth()/artWidth()),
         juce::roundToInt(y*getHeight()/artHeight()),
         juce::roundToInt(w*getWidth()/artWidth()),
         juce::roundToInt(h*getHeight()/artHeight())};
}
void BoomAudioProcessorEditor::resized(){
 // Legacy photograph text fields are hidden: the native browser owns them now.
 search.setVisible(false);
 userPresetName.setVisible(false);
 if(nativeBrowser!=nullptr){
  nativeBrowser->setBounds(canvasRect(18.f,96.f,1557.f,337.f));
  nativeBrowser->setVisible(page==Page::browser);
  if(page==Page::browser)nativeBrowser->toFront(false);
 }
 if(nativeChain!=nullptr){
  nativeChain->setBounds(canvasRect(47.f,98.f,1577.f,237.f));
  nativeChain->setVisible(page==Page::chain);
  if(page==Page::chain)nativeChain->toFront(false);
 }
 // The original JPG does not paint the header or the comparison toolbar.
 // These components own the entire area, including their native backgrounds.
 if(nativeHeader!=nullptr){
  nativeHeader->setBounds(canvasRect(0.f,0.f,float(artWidth()),
                                     page==Page::browser?95.f:98.f));
  nativeHeader->toFront(false);
 }
 if(nativeToolbar!=nullptr){
  const float top=page==Page::browser?435.f:343.f;
  const float bottom=page==Page::browser?481.f:396.f;
  nativeToolbar->setBounds(canvasRect(0.f,top,float(artWidth()),bottom-top));
  nativeToolbar->toFront(false);
 }
 if(editing.parameter.isNotEmpty())numericEdit.setVisible(false);
}
void BoomAudioProcessorEditor::showPage(Page target){
 endGesture();cancelNumeric();
 if(target==page)return;
 page=target;
 if(page==Page::chain&&nativeChain!=nullptr)nativeChain->setEnglish(english);
 if(page==Page::browser && nativeBrowser!=nullptr){
  nativeBrowser->updateLanguage(english);
  nativeBrowser->refresh();
 }
 const auto fittedSize=boomFittedWindowSize(artWidth(),artHeight());
 setSize(fittedSize.x,fittedSize.y);
 resized();refreshChrome();repaint();
}
float BoomAudioProcessorEditor::read(const char* id)const noexcept{
 if(const auto* v=processor.state().getRawParameterValue(id))return v->load();
 return 0.f;
}
void BoomAudioProcessorEditor::write(const char* id,float value,bool gesture){
 if(auto* parameter=processor.state().getParameter(id)){
  auto range=parameter->getNormalisableRange();
  const float corrected=juce::jlimit(range.start,range.end,value);
  if(gesture)parameter->beginChangeGesture();
  parameter->setValueNotifyingHost(parameter->convertTo0to1(corrected));
  if(gesture)parameter->endChangeGesture();
  // G/D linkage updates both DAW parameters as one user operation.
  if(read("inputLink")>.5f &&
     (juce::String(id)=="inputLTrim"||juce::String(id)=="inputRTrim")){
   const char* other=juce::String(id)=="inputLTrim"?"inputRTrim":"inputLTrim";
   if(auto* linked=processor.state().getParameter(other)){
    if(gesture)linked->beginChangeGesture();
    linked->setValueNotifyingHost(linked->convertTo0to1(corrected));
    if(gesture)linked->endChangeGesture();
   }
  }
 }
 repaint();
}
void BoomAudioProcessorEditor::toggle(const char* id){
 write(id,read(id)>.5f?0.f:1.f);
}
void BoomAudioProcessorEditor::endGesture(){
 if(dragging)
  if(auto* parameter=processor.state().getParameter(dragParameter))
   parameter->endChangeGesture();
 dragging=false;dragMoved=false;dragParameter.clear();dragZone=nullptr;
}
void BoomAudioProcessorEditor::beginGesture(const Zone& zone,const juce::MouseEvent& e){
 endGesture();
 if(auto* parameter=processor.state().getParameter(zone.id)){
  dragZone=&zone;dragParameter=zone.id;
  dragStartValue=read(zone.id);dragOrigin=pointOf(e);
  parameter->beginChangeGesture();dragging=true;
 }
}
const boom::ui::Zone* BoomAudioProcessorEditor::zoneAt(juce::Point<float> p)const noexcept{
 // The top header, mastering rack and comparison rail are now native.
 // Never dispatch old photographed hit regions, including spaces between
 // the new buttons. Native JUCE children own the interactions.
 if(page==Page::browser ? p.y<481.f : p.y<396.f)return nullptr;
 if(page==Page::browser){
  for(const auto& z:boom::ui::browser)if(z.contains(p.x,p.y))return &z;
 }else{
  for(const auto& z:boom::ui::chain)if(z.contains(p.x,p.y))return &z;
 }
 return nullptr;
}
void BoomAudioProcessorEditor::mouseDown(const juce::MouseEvent& e){
 cancelNumeric();
 const auto p=pointOf(e);
 if(page==Page::settings||page==Page::meters||page==Page::help){
  if(p.y>=96.f && p.y<340.f && handleAuxiliary(p,e))return;
 }
 if(e.mods.isRightButtonDown()){
  if(const auto* hit=zoneAt(p)){
   const auto& item=*hit;
   if(item.kind==Kind::field||item.kind==Kind::knob){
    juce::PopupMenu m;
    m.addItem(1,english?"Enter a value":"Saisir une valeur");
    m.addItem(2,english?"Reset to default":"Valeur par d\u00E9faut");
    const auto target=item;
    juce::Component::SafePointer<BoomAudioProcessorEditor> self(this);
    m.showMenuAsync(juce::PopupMenu::Options(),[self,target](int command){
     if(self==nullptr)return;
     if(command==1)self->numericFor(target);
     if(command==2)
      if(auto* param=self->processor.state().getParameter(target.id))
       self->write(target.id,param->convertFrom0to1(param->getDefaultValue()));
    });
   }
   return;
  }
  return;
 }
 if(const auto* hit=zoneAt(p)){
  const auto& item=*hit;
  // Other pages inherit only console strip controls, not buried CHAIN rack.
  if(page!=Page::chain&&page!=Page::browser&&p.y>=96.f&&p.y<340.f)return;
  if(item.kind==Kind::knob||item.kind==Kind::field){
   focusedKnob=-1;
   if(item.kind==Kind::knob){
    constexpr const char* keys[]={"inputDb","eq60","eq120","eq3k","eq6k","eq10k","push"};
    for(int i=0;i<7;++i)
     if(juce::String(item.id)==keys[i]){focusedKnob=i;break;}
    if(focusedKnob>=0)grabKeyboardFocus();
   }
   beginGesture(item,e);
  }else{focusedKnob=-1;doAction(item,e);}
  repaint();
  return;
 }
}
void BoomAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e){
 if(!dragging||dragZone==nullptr)return;
 const auto now=pointOf(e);
 const float dy=dragOrigin.y-now.y,dx=now.x-dragOrigin.x;
 if(std::abs(dx)+std::abs(dy)<3.f&&!dragMoved)return;
 dragMoved=true;
 if(auto* parameter=processor.state().getParameter(dragParameter)){
  const auto range=parameter->getNormalisableRange();
  const float fine=e.mods.isShiftDown()?.13f:1.f;
  float delta=(dy+dx*.45f)*fine*(range.end-range.start)/230.f;
  if(e.mods.isAltDown())delta*=.20f;
  write(dragParameter.toRawUTF8(),dragStartValue+delta,false);
 }
}
void BoomAudioProcessorEditor::mouseMove(const juce::MouseEvent& e){
 const auto* hit=zoneAt(pointOf(e));
 int next=-1;
 if(hit!=nullptr&&hit->kind==Kind::knob){
  constexpr const char* keys[]={"inputDb","eq60","eq120","eq3k","eq6k","eq10k","push"};
  for(int i=0;i<7;++i)
   if(juce::String(hit->id)==keys[i]){next=i;break;}
 }
 if(next!=hoveredKnob){
  hoveredKnob=next;
  setMouseCursor(next>=0?juce::MouseCursor::PointingHandCursor:
                          juce::MouseCursor::NormalCursor);
  repaint();
 }
}
void BoomAudioProcessorEditor::mouseExit(const juce::MouseEvent&){
 if(hoveredKnob>=0){hoveredKnob=-1;repaint();}
 setMouseCursor(juce::MouseCursor::NormalCursor);
}
void BoomAudioProcessorEditor::mouseUp(const juce::MouseEvent&){
 const bool numericClick=dragging&&!dragMoved&&dragZone!=nullptr&&dragZone->kind==Kind::field;
 const auto target=numericClick?*dragZone:Zone{};
 endGesture();
 if(numericClick)numericFor(target);
}
void BoomAudioProcessorEditor::mouseDoubleClick(const juce::MouseEvent& e){
 const auto p=pointOf(e);
 if(const auto* hit=zoneAt(p)){
  const auto& item=*hit;
  if(item.kind==Kind::knob||item.kind==Kind::field){
   if(auto* param=processor.state().getParameter(item.id))
    write(item.id,param->convertFrom0to1(param->getDefaultValue()));
  }else if(juce::String(item.id)=="presetRows")
   loadSelected();
  return;
 }
}
void BoomAudioProcessorEditor::mouseWheelMove(
 const juce::MouseEvent& e,const juce::MouseWheelDetails& wheel){
 const auto p=pointOf(e);
 if(page==Page::browser && in(p,372,209,742,180)){
  firstVisible=juce::jlimit(0,juce::jmax(0,int(results.size())-(grid?6:5)),
                           firstVisible+(wheel.deltaY<0?1:-1)*(grid?2:1));
  repaint();return;
 }
 if(const auto* hit=zoneAt(p)){
  const auto& item=*hit;
  if(item.kind==Kind::knob||item.kind==Kind::field){
   if(auto* param=processor.state().getParameter(item.id)){
    const auto range=param->getNormalisableRange();
    const float step=range.interval>0?range.interval:(range.end-range.start)*.01f;
    write(item.id,read(item.id)+(wheel.deltaY>0?step:-step)*
         (e.mods.isShiftDown()?.2f:1.f));
   }return;
  }
 }
 AudioProcessorEditor::mouseWheelMove(e,wheel);
}
bool BoomAudioProcessorEditor::keyPressed(const juce::KeyPress& k){
 const auto code=k.getKeyCode();
 if(focusedKnob>=0&&hasKeyboardFocus(false)){
  const bool increase=code==juce::KeyPress::upKey||code==juce::KeyPress::rightKey;
  const bool decrease=code==juce::KeyPress::downKey||code==juce::KeyPress::leftKey;
  if(increase||decrease){
   constexpr const char* keys[]={"inputDb","eq60","eq120","eq3k","eq6k","eq10k","push"};
   const char* id=keys[focusedKnob];
   if(const auto* param=processor.state().getParameter(id)){
    const auto range=param->getNormalisableRange();
    float step=range.interval>0.f?range.interval:(range.end-range.start)*.01f;
    if(k.getModifiers().isShiftDown())step*=.2f;
    write(id,read(id)+(increase?step:-step));
   }
   return true;
  }
 }
 if(k==juce::KeyPress::escapeKey){
  focusedKnob=-1;repaint();
  cancelNumeric();
  if(page==Page::browser)showPage(Page::chain);
  return true;
 }
 if(page==Page::browser){
  if(k==juce::KeyPress::returnKey){loadSelected();return true;}
  if(k==juce::KeyPress::upKey){changeSelected(selectedRow-1);return true;}
  if(k==juce::KeyPress::downKey){changeSelected(selectedRow+1);return true;}
 }
 return false;
}
void BoomAudioProcessorEditor::numericFor(const Zone& item){
 float scale=1.f;
 const juce::String id(item.id);
 if(id=="deFreq")scale=.001f;
 else if(id=="colorMix"||id=="deAmt"||id=="compMix")scale=100.f;
 numericFor(item.id,scale,{item.x,item.y,item.w,item.h});
}
void BoomAudioProcessorEditor::numericFor(
 const char* id,float scale,juce::Rectangle<float> area){
 endGesture();cancelNumeric();
 editing={juce::String(id),scale};
 numericEdit.setBounds(canvasRect(area.getX(),area.getY(),area.getWidth(),area.getHeight()));
 const auto digits=editing.parameter=="deFreq"?2:1;
 numericEdit.setText(juce::String(read(id)*scale,digits),false);
 numericEdit.setVisible(true);numericEdit.toFront(false);
 numericEdit.grabKeyboardFocus();numericEdit.selectAll();
}
void BoomAudioProcessorEditor::commitNumeric(){
 const auto id=editing.parameter;
 if(id.isEmpty())return;
 const float scale=editing.scale;
 const auto entered=numericEdit.getText().trim().replaceCharacter(',','.');
 editing.parameter.clear();numericEdit.setVisible(false);
 if(entered.isEmpty())return;
 const float value=entered.getFloatValue()/juce::jmax(.00001f,scale);
 if(std::isfinite(value))write(id.toRawUTF8(),value);
}
void BoomAudioProcessorEditor::cancelNumeric(){
 editing.parameter.clear();numericEdit.setVisible(false);
}
void BoomAudioProcessorEditor::doAction(const Zone& zone,const juce::MouseEvent& e){
 const juce::String id(zone.id);
 if(zone.kind==Kind::toggle){toggle(zone.id);return;}
 if(zone.kind==Kind::menu){
  if(id=="styleMenu")openStyleMenu();
  else if(id=="presetMenu")openPresetMenu();
  else if(id=="styleFilter")openStyleFilter();
  else if(id=="color")openColorMenu();
  else if(id=="language")openLanguageMenu();
  else if(id=="userMenu")openUserMenu();
  return;
 }
 if(id=="presetPrevious"){cyclePreset(-1);return;}
 if(id=="presetNext"){cyclePreset(1);return;}
 if(id=="pageChain"){showPage(Page::chain);return;}
 if(id=="pageMeters"){showPage(Page::meters);return;}
 if(id=="pageSettings"){showPage(Page::settings);return;}
 if(id=="pageHelp"){showPage(Page::help);return;}
 if(id=="closeBrowser"){showPage(Page::chain);return;}
 if(id=="recallA"){processor.recallComparisonState(0);notice="A";return;}
 if(id=="recallB"){processor.recallComparisonState(1);notice="B";return;}
 if(id=="storeA"){processor.storeComparisonState(0);notice="M\u00E9moire A enregistr\u00E9e";return;}
 if(id=="storeB"){processor.storeComparisonState(1);notice="M\u00E9moire B enregistr\u00E9e";return;}
 if(id=="copyAtoB"){processor.copyComparisonState(0,1);notice="A vers B";return;}
 if(id=="copyBtoA"){processor.copyComparisonState(1,0);notice="B vers A";return;}
 if(id=="trimMenu"){openTrimsMenu();return;}
 if(id=="categoryFavorites"){selectCategory(10);return;}
 if(id=="categoryUser"){selectCategory(11);return;}
 if(id.startsWith("category")){
  selectCategory(id.substring(8).getIntValue());return;
 }
 if(id=="viewList"){grid=false;refreshResults();return;}
 if(id=="viewGrid"){grid=true;refreshResults();return;}
 if(id=="presetRows"){
  const auto p=pointOf(e);
  int line=grid?int((p.y-211.f)/60.f):int((p.y-211.f)/34.f);
  int selected=firstVisible+line*(grid?2:1)+(grid&&p.x>741.f?1:0);
  if(selected>=int(results.size()))return;
  changeSelected(selected);
  bool hitStar=grid?(p.x>(p.x>741.f?1071.f:702.f)):p.x>1067.f;
  if(hitStar&& !selectedIsUser()){
   processor.toggleFavorite(results[static_cast<std::size_t>(selectedRow)].index);
   refreshResults();
  }else if(e.getNumberOfClicks()>1)loadSelected();
  return;
 }
 if(id=="favoriteSelected"){
  if(selectedRow>=0&&selectedRow<int(results.size())&&!selectedIsUser()){
   auto preset=results[static_cast<std::size_t>(selectedRow)].index;
   processor.toggleFavorite(preset);refreshResults();
  }return;
 }
 if(id=="loadSelected"){loadSelected();return;}
 if(id=="saveUser"){saveUser();return;}
 if(id=="rowPrevious"){changeSelected(selectedRow-1);return;}
 if(id=="rowNext"){changeSelected(selectedRow+1);return;}
}
void BoomAudioProcessorEditor::timerCallback(){
 const bool newLanguage=read("language")>.5f;
 if(newLanguage!=english){
  english=newLanguage;
  if(nativeChain!=nullptr)nativeChain->setEnglish(english);
  if(nativeBrowser!=nullptr)nativeBrowser->updateLanguage(english);
 }
 if(nativeChain!=nullptr&&page==Page::chain)nativeChain->syncFromHost();
 refreshChrome();
 const auto meter=processor.lastMeters();
 auto needleDb=[](float linear){
  return juce::jlimit(-20.f,3.f,
    boom::gainToDb(std::max(0.00000001f,linear)/boom::dbToGain(-18.f)));
 };
 vuL+=.18f*(needleDb(meter.vuL)-vuL);
 vuR+=.18f*(needleDb(meter.vuR)-vuR);
 if(meter.outputPeakL>=1.f||meter.outputPeakR>=1.f)clipHold=true;
 repaint();
}
