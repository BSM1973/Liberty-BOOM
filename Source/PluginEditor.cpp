#include "PluginEditor.h"
#include "BoomBrowser.h"
#include <BinaryData.h>
#include <algorithm>
#include <cmath>

namespace {
using namespace boom;
using namespace boom::ui;
const auto ivory=juce::Colour::fromRGB(247,228,187);
const auto copper=juce::Colour::fromRGB(190,103,78);
const auto gold=juce::Colour::fromRGB(221,183,118);
const auto charcoal=juce::Colour::fromRGB(22,9,12);
juce::Font font(float size,bool bold=false){
 return juce::Font(juce::FontOptions(size,bold?juce::Font::bold:juce::Font::plain));
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
 setWantsKeyboardFocus(true);
 setResizable(false,false);  // Keep the full console visible, without scrollbars.
 refreshResults();
 const auto initialSize=boomFittedWindowSize(CW,CH);
 setSize(initialSize.x,initialSize.y);
 resized();
 startTimerHz(30);
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
  nativeBrowser->setBounds(canvasRect(18.f,95.f,1557.f,344.f));
  nativeBrowser->setVisible(page==Page::browser);
  if(page==Page::browser)nativeBrowser->toFront(false);
 }
 if(editing.parameter.isNotEmpty())numericEdit.setVisible(false);
}
void BoomAudioProcessorEditor::showPage(Page target){
 endGesture();cancelNumeric();
 if(target==page)return;
 page=target;
 if(page==Page::browser && nativeBrowser!=nullptr){
  nativeBrowser->updateLanguage(english);
  nativeBrowser->refresh();
 }
 const auto fittedSize=boomFittedWindowSize(artWidth(),artHeight());
 setSize(fittedSize.x,fittedSize.y);
 resized();repaint();
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
    m.addItem(2,english?"Reset to default":"Valeur par défaut");
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
   beginGesture(item,e);
  }else doAction(item,e);
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
 if(k==juce::KeyPress::escapeKey){
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
 if(id=="storeA"){processor.storeComparisonState(0);notice="Mémoire A enregistrée";return;}
 if(id=="storeB"){processor.storeComparisonState(1);notice="Mémoire B enregistrée";return;}
 if(id=="copyAtoB"){processor.copyComparisonState(0,1);notice="A → B";return;}
 if(id=="copyBtoA"){processor.copyComparisonState(1,0);notice="B → A";return;}
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
 english=read("language")>.5f;
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
