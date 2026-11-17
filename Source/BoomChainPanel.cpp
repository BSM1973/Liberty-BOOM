#include "BoomChainPanel.h"
#include "BoomUiTheme.h"
#include "BoomSafeText.h"
#include <algorithm>
#include <utility>
#include <cmath>

namespace {
const auto wood=boom::ui::theme::background();
const auto charcoal=boom::ui::theme::surface();
const auto burgundy=boom::ui::theme::active();
const auto copper=boom::ui::theme::copper();
const auto gold=boom::ui::theme::gold();
const auto ivory=boom::ui::theme::ivory();
juce::Font font(float pts,bool bold=false){
 return boom::ui::theme::font(pts,bold);
}
constexpr const char* namesFR[]={
 "ENTREE","ANALOGIQUE","EGALISEUR","DE-ESSER",
 "COMPRESSEUR","LIMITEUR","SORTIE"
};
constexpr const char* namesEN[]={
 "INPUT","ANALOG","EQUALIZER","DE-ESSER","COMPRESSOR","LIMITER","OUTPUT"
};
// Titles are presentation-only: parameter IDs and DSP attachments never change.
juce::String englishTitle(const juce::String& id,const juce::String& french){
 static constexpr std::pair<const char*,const char*> translated[]{
  {"inputLTrim","LEFT TRIM"},{"inputRTrim","RIGHT TRIM"},
  {"inputLink","LINK L/R"},{"color","CIRCUIT"},{"colorDrive","SATURATION"},
  {"colorMix","MIX"},{"colorPushCharacter","PUSH CHARACTER"},
  {"colorOut","OUTPUT COMPENSATION"},{"colorBypass","BYPASS"},
  {"eqBypass","BYPASS"},{"deFreq","FREQUENCY"},{"deTh","THRESHOLD"},
  {"deAmt","INTENSITY"},{"deAtt","ATTACK"},{"deRel","RELEASE"},
  {"deEssBypass","BYPASS"},{"compTh","THRESHOLD"},
  {"compAttack","ATTACK"},{"compRelease","RELEASE"},
  {"compMakeup","MAKEUP GAIN"},{"compMix","PARALLEL MIX"},
  {"compStereoLink","STEREO LINK"},{"compBypass","BYPASS"},
  {"limitTh","THRESHOLD"},{"ceiling","CEILING"},
  {"limitRelease","RELEASE"},{"limitMode","CHARACTER"},
  {"limiterStereoLink","STEREO LINK"},{"limiterBypass","BYPASS"},
  {"outDb","OUTPUT LEVEL"},{"outPeakGuard","PEAK PROTECTION"},
  {"outputLevelComp","LEVEL MATCH"},{"mono","MONO MONITORING"},
  {"meterInput","INPUT METER"},{"outputBypass","BYPASS"},
 };
 for(const auto& entry:translated)if(id==entry.first)return entry.second;
 return french;
}
}
void BoomChainPanel::CopperLook::drawButtonBackground(
 juce::Graphics& g,juce::Button& button,const juce::Colour&,bool hover,bool down){
 const auto r=button.getLocalBounds().toFloat().reduced(1.f);
 const bool active=button.isEnabled()&&button.getToggleState();
 juce::ColourGradient fill(active?burgundy:charcoal,r.getTopLeft(),
                            (button.isEnabled()&&(hover||down))?burgundy:wood,
                            r.getBottomLeft(),false);
 g.setGradientFill(fill);g.fillRoundedRectangle(r,5.f);
 g.setColour(!button.isEnabled()?copper.withAlpha(.25f):
             active?gold:((hover||down)?gold.withAlpha(.82f):copper.withAlpha(.7f)));
 g.drawRoundedRectangle(r,5.f,active?1.7f:1.f);
 if(active){
  g.setColour(gold);g.fillRect(r.getX()+6,r.getBottom()-3,
                               juce::jmax(0.f,r.getWidth()-12),1.f);
 }
 boom::ui::theme::focusRing(g,r,button.hasKeyboardFocus(true),5.f);
}
void BoomChainPanel::CopperLook::drawButtonText(
 juce::Graphics& g,juce::TextButton& b,bool,bool){
 g.setColour(b.isEnabled()?ivory:ivory.withAlpha(.5f));
 g.setFont(font(juce::jlimit(11.f,14.f,float(b.getHeight())*.46f),b.getToggleState()));
 g.drawFittedText(boom::ui::text::safeUpperText(b.getButtonText()),b.getLocalBounds().reduced(3,2),
                  juce::Justification::centred,1);
}
void BoomChainPanel::CopperLook::drawRotarySlider(
 juce::Graphics& g,int x,int y,int width,int height,float proportional,
 float fromAngle,float toAngle,juce::Slider& slider){
 const auto bounds=juce::Rectangle<float>(float(x),float(y),float(width),float(height));
 const auto side=std::min(bounds.getWidth(),bounds.getHeight())-5.f;
 const auto centre=bounds.getCentre();
 const float radius=std::max(12.f,side*.43f);
 const auto valueAngle=fromAngle+proportional*(toAngle-fromAngle);
 juce::Path background,active;
 background.addCentredArc(centre.x,centre.y,radius,radius,0.f,fromAngle,toAngle,true);
 active.addCentredArc(centre.x,centre.y,radius,radius,0.f,fromAngle,valueAngle,true);
 g.setColour(juce::Colour::fromRGB(67,44,44));
 g.strokePath(background,juce::PathStrokeType(4.5f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 g.setColour(gold);
 g.strokePath(active,juce::PathStrokeType(4.8f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 auto knob=juce::Rectangle<float>(radius*1.45f,radius*1.45f).withCentre(centre);
 juce::ColourGradient metal(copper,knob.getTopLeft(),juce::Colour::fromRGB(58,30,31),
                             knob.getBottomRight(),false);
 g.setGradientFill(metal);g.fillEllipse(knob);
 g.setColour(gold.withAlpha(.78f));g.drawEllipse(knob,1.f);
 juce::Point<float> tip(centre.x+std::sin(valueAngle)*radius*.56f,
                        centre.y-std::cos(valueAngle)*radius*.56f);
 g.setColour(ivory);g.drawLine(centre.x,centre.y,tip.x,tip.y,2.f);
 if(slider.hasKeyboardFocus(true)){
  g.setColour(ivory.withAlpha(.86f));
  g.drawEllipse(bounds.reduced(1.5f),1.5f);
 }
}
void BoomChainPanel::CopperLook::drawToggleButton(
 juce::Graphics& g,juce::ToggleButton& button,bool hover,bool pressed){
 const auto bounds=button.getLocalBounds().toFloat().reduced(1.f);
 const bool enabled=button.getToggleState();
 g.setColour(enabled?burgundy:charcoal);
 g.fillRoundedRectangle(bounds,5.f);
 g.setColour(enabled?gold:(hover||pressed?gold.withAlpha(.82f):copper.withAlpha(.62f)));
 g.drawRoundedRectangle(bounds,5.f,1.1f);
 boom::ui::theme::focusRing(g,bounds,button.hasKeyboardFocus(true),5.f);
 const float cy=bounds.getCentreY();
 g.setColour(enabled?gold:juce::Colour::fromRGB(87,66,62));
 g.fillEllipse(bounds.getX()+7,cy-4,8.f,8.f);
 g.setColour(ivory);
 g.setFont(font(juce::jlimit(10.f,12.5f,float(button.getHeight())*.45f),enabled));
 g.drawFittedText(boom::ui::text::safeUpperText(button.getButtonText()),
                  juce::Rectangle<int>(int(bounds.getX()+20),int(bounds.getY()),
                                       int(bounds.getWidth()-24),int(bounds.getHeight())),
                  juce::Justification::centredLeft,1);
}
juce::String BoomChainPanel::moduleName(int index,bool en){
 if(index<0||index>=7)return {};
 return boom::ui::text::safeUpperText(en?namesEN[index]:namesFR[index]);
}
BoomChainPanel::BoomChainPanel(BoomAudioProcessor& p):processor(p){
 setOpaque(true);
 auto state=processor.state().state;
 expanded=bool(state.getProperty("boomChainExpanded",true));
 currentModule=juce::jlimit(0,6,int(state.getProperty("boomChainModule",0)));
 english=processor.state().getRawParameterValue("language")!=nullptr&&
         processor.state().getRawParameterValue("language")->load()>.5f;
 for(int i=0;i<7;++i){
  auto& tab=tabButtons[static_cast<std::size_t>(i)];
  tab.setLookAndFeel(&look);
  tab.setWantsKeyboardFocus(true);
  tab.setMouseCursor(juce::MouseCursor::PointingHandCursor);
  tab.onClick=[this,i]{selectModule(i);};
  addAndMakeVisible(tab);
 }
 for(auto* b:{&visibilityButton,&resetButton}){
  b->setLookAndFeel(&look);
  b->setWantsKeyboardFocus(true);
  b->setMouseCursor(juce::MouseCursor::PointingHandCursor);
  addAndMakeVisible(*b);
 }
 visibilityButton.onClick=[this]{toggleVisibility();};
 resetButton.onClick=[this]{restoreModuleDefaults();};
 // Entree
 addRotary(0,"inputDb","INPUT DRIVE",1.f," dB",1);
 addRotary(0,"inputLTrim","TRIM GAUCHE",1.f," dB",1);
 addRotary(0,"inputRTrim","TRIM DROIT",1.f," dB",1);
 addToggle(0,"inputLink","LIEN G/D");
 // Couleur analogique
 addChoice(1,"color","CIRCUIT",{"CLEAN","TUBE","TAPE"});
 addRotary(1,"colorDrive","SATURATION",100.f," %",0);
 addRotary(1,"colorMix","MELANGE",100.f," %",0);
 addRotary(1,"colorPushCharacter","CARACTERE PUSH",1.f," %",0);
 addRotary(1,"colorOut","COMPENSATION",1.f," dB",1);
 addToggle(1,"colorBypass","DESACTIVER",true);
 // EQ
 addRotary(2,"eq60","60 Hz",1.f," dB",1);
 addRotary(2,"eq120","120 Hz",1.f," dB",1);
 addRotary(2,"eq3k","3 kHz",1.f," dB",1);
 addRotary(2,"eq6k","6 kHz",1.f," dB",1);
 addRotary(2,"eq10k","10 kHz",1.f," dB",1);
 addToggle(2,"eqBypass","DESACTIVER",true);
 // De-Esser
 addRotary(3,"deFreq","FREQUENCE",.001f," kHz",2);
 addRotary(3,"deTh","SEUIL",1.f," dB",1);
 addRotary(3,"deAmt","INTENSITE",100.f," %",0);
 addRotary(3,"deAtt","ATTAQUE",1.f," ms",1);
 addRotary(3,"deRel","RELACHEMENT",1.f," ms",0);
 addToggle(3,"deEssBypass","DESACTIVER",true);
 // Compression
 addRotary(4,"compTh","SEUIL",1.f," dB",1);
 addRotary(4,"compRatio","RATIO",1.f,":1",2);
 addRotary(4,"compAttack","ATTAQUE",1.f," ms",0);
 addRotary(4,"compRelease","RELACHEMENT",1.f," ms",0);
 addRotary(4,"compMakeup","GAIN",1.f," dB",1);
 addRotary(4,"compMix","MELANGE PARALLELE",100.f," %",0);
 addToggle(4,"compStereoLink","LIAISON STEREO");
 addToggle(4,"compBypass","DESACTIVER",true);
 // Limiter
 addRotary(5,"limitTh","SEUIL",1.f," dB",1);
 addRotary(5,"ceiling","PLAFOND",1.f," dBFS",1);
 addRotary(5,"limitRelease","RELACHEMENT",1.f," ms",0);
 addChoice(5,"limitMode","CARACTERE",{"TRANSPARENT","PUNCH","LOUD"});
 addToggle(5,"limiterAutoRelease","AUTO RELEASE");
 addToggle(5,"limiterStereoLink","LIAISON STEREO");
 addToggle(5,"limiterBypass","DESACTIVER",true);
 // Sortie / monitoring
 addRotary(6,"outDb","NIVEAU SORTIE",1.f," dB",1);
 addToggle(6,"outPeakGuard","PROTECTION CRETES");
 addToggle(6,"outputLevelComp","COMPARAISON NIVEAU");
 addToggle(6,"mono","ECOUTE MONO");
 addToggle(6,"meterInput","MESURE A L'ENTREE");
 addToggle(6,"outputBypass","DESACTIVER",true);
 setEnglish(english);
 updateVisibility();
}
BoomChainPanel::~BoomChainPanel(){
 // Attachments are destroyed before controls; then detach the custom look.
 for(auto& c:rotaries)c->slider.setLookAndFeel(nullptr);
 for(auto& c:choices)c->box.setLookAndFeel(nullptr);
 for(auto& c:toggles)c->button.setLookAndFeel(nullptr);
 rotaries.clear();choices.clear();toggles.clear();
 for(auto& b:tabButtons)b.setLookAndFeel(nullptr);
 visibilityButton.setLookAndFeel(nullptr);
 resetButton.setLookAndFeel(nullptr);
}
void BoomChainPanel::addRotary(
 int module,const char* parameter,const char* title,float multiplier,const char* unit,int digits){
 auto c=std::make_unique<RotaryControl>();
 c->module=module;
 c->parameterId=parameter;
 c->frenchLabel=boom::ui::text::safeUpperText(title);
 c->label.setText(english?englishTitle(c->parameterId,c->frenchLabel):
                          c->frenchLabel,juce::dontSendNotification);
 c->label.setJustificationType(juce::Justification::centred);
 c->label.setColour(juce::Label::textColourId,ivory);
 c->label.setFont(font(13.5f,true));
 c->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
 c->slider.setRotaryParameters(juce::MathConstants<float>::pi*1.20f,
                               juce::MathConstants<float>::pi*2.80f,true);
 c->slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,96,24);
 c->slider.setColour(juce::Slider::textBoxTextColourId,ivory);
 c->slider.setColour(juce::Slider::textBoxBackgroundColourId,wood);
 c->slider.setColour(juce::Slider::textBoxOutlineColourId,copper.withAlpha(.55f));
 c->slider.setColour(juce::Slider::textBoxHighlightColourId,burgundy);
 c->slider.setLookAndFeel(&look);
 c->slider.setWantsKeyboardFocus(true);
 c->slider.textFromValueFunction=[multiplier,unit=juce::String(unit),digits](double value){
  return juce::String(value*multiplier,digits)+unit;
 };
 c->slider.valueFromTextFunction=[multiplier](const juce::String& text){
  return text.replaceCharacter(',','.').getDoubleValue()/multiplier;
 };
 c->slider.setDoubleClickReturnValue(true,processor.state().getParameter(parameter)
   ?processor.state().getParameter(parameter)->convertFrom0to1(
     processor.state().getParameter(parameter)->getDefaultValue()):0.f);
 c->attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
   processor.state(),parameter,c->slider);
 c->slider.setTooltip(boom::ui::text::safeUpperText(title)+"  -  "+juce::String(parameter));
 addAndMakeVisible(c->label);addAndMakeVisible(c->slider);
 rotaries.push_back(std::move(c));
}
void BoomChainPanel::addChoice(
 int module,const char* parameter,const char* title,std::initializer_list<const char*> values){
 auto c=std::make_unique<ChoiceControl>();
 c->module=module;
 c->parameterId=parameter;
 c->frenchLabel=boom::ui::text::safeUpperText(title);
 c->label.setText(english?englishTitle(c->parameterId,c->frenchLabel):
                          c->frenchLabel,juce::dontSendNotification);
 c->label.setJustificationType(juce::Justification::centred);
 c->label.setFont(font(13.5f,true));
 c->label.setColour(juce::Label::textColourId,ivory);
 int index=1;
 for(auto v:values)c->box.addItem(juce::String(v),index++);
 c->box.setLookAndFeel(&menuLook);
 c->box.setColour(juce::ComboBox::backgroundColourId,wood);
 c->box.setColour(juce::ComboBox::textColourId,ivory);
 c->box.setColour(juce::ComboBox::outlineColourId,copper);
 c->box.setColour(juce::ComboBox::arrowColourId,gold);
 c->box.setJustificationType(juce::Justification::centred);
 c->attachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
   processor.state(),parameter,c->box);
 addAndMakeVisible(c->label);addAndMakeVisible(c->box);
 choices.push_back(std::move(c));
}
void BoomChainPanel::addToggle(
 int module,const char* parameter,const char* title,bool primary){
 auto t=std::make_unique<ToggleControl>();
 t->module=module;
 t->primary=primary;
 t->parameterId=parameter;
 t->frenchLabel=boom::ui::text::safeUpperText(title);
 t->button.setButtonText(english?englishTitle(t->parameterId,t->frenchLabel):
                                 t->frenchLabel);
 t->button.setLookAndFeel(&look);
 t->button.setWantsKeyboardFocus(true);
 t->button.setColour(juce::ToggleButton::textColourId,ivory);
 t->button.setMouseCursor(juce::MouseCursor::PointingHandCursor);
 t->button.setTooltip(boom::ui::text::safeUpperText(title)+"  -  "+juce::String(parameter));
 t->attachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
   processor.state(),parameter,t->button);
 addAndMakeVisible(t->button);
 toggles.push_back(std::move(t));
}
void BoomChainPanel::selectModule(int module){
 currentModule=juce::jlimit(0,6,module);
 processor.state().state.setProperty("boomChainModule",currentModule,nullptr);
 updateVisibility();
}
void BoomChainPanel::toggleVisibility(){
 expanded=!expanded;
 processor.state().state.setProperty("boomChainExpanded",expanded,nullptr);
 updateVisibility();
}
void BoomChainPanel::restoreModuleDefaults(){
 // Only this tab's actual DSP parameters are reset; all other modules keep
 // their values. APVTS host gestures make the operation automatable/undoable.
 static const std::array<std::vector<const char*>,7> ids{{
  {"inputDb","inputLTrim","inputRTrim","inputLink"},
  {"color","colorDrive","colorMix","colorPushCharacter","colorOut","colorBypass"},
  {"eq60","eq120","eq3k","eq6k","eq10k","eqBypass"},
  {"deFreq","deTh","deAmt","deAtt","deRel","deEssBypass"},
  {"compTh","compRatio","compAttack","compRelease","compMakeup","compMix","compStereoLink","compBypass"},
  {"limitTh","ceiling","limitRelease","limitMode","limiterAutoRelease","limiterStereoLink","limiterBypass"},
  {"outDb","outPeakGuard","outputLevelComp","mono","meterInput","outputBypass"}
 }};
 for(const auto* id:ids[static_cast<std::size_t>(currentModule)]){
  if(auto* p=processor.state().getParameter(id)){
   p->beginChangeGesture();
   p->setValueNotifyingHost(p->getDefaultValue());
   p->endChangeGesture();
  }
 }
 repaint();
}
void BoomChainPanel::setEnglish(bool en){
 english=en;
 for(int i=0;i<7;++i)
  tabButtons[static_cast<std::size_t>(i)].setButtonText(moduleName(i,english));
 for(auto& c:rotaries)
  c->label.setText(english?englishTitle(c->parameterId,c->frenchLabel):
                           c->frenchLabel,juce::dontSendNotification);
 for(auto& c:choices)
  c->label.setText(english?englishTitle(c->parameterId,c->frenchLabel):
                           c->frenchLabel,juce::dontSendNotification);
 for(auto& t:toggles)
  t->button.setButtonText(english?englishTitle(t->parameterId,t->frenchLabel):
                                  t->frenchLabel);
 resetButton.setButtonText(english?"RESET MODULE":"REINITIALISER");
 updateVisibility();
}
void BoomChainPanel::syncFromHost(){
 // All parameters are refreshed by the APVTS attachments.
 // This redraw only refreshes the live ACTIVE / BYPASS status label.
 repaint();
}
void BoomChainPanel::updateVisibility(){
 for(int i=0;i<7;++i){
  auto& tab=tabButtons[static_cast<std::size_t>(i)];
  tab.setVisible(expanded);
  tab.setToggleState(i==currentModule,juce::dontSendNotification);
 }
 resetButton.setVisible(expanded);
 visibilityButton.setButtonText(expanded?(english?"HIDE CHAIN":"MASQUER"):
                                   (english?"SHOW CHAIN":"AFFICHER"));
 for(auto& c:rotaries){
  const bool active=expanded&&c->module==currentModule;
  c->label.setVisible(active);c->slider.setVisible(active);
 }
 for(auto& c:choices){
  const bool active=expanded&&c->module==currentModule;
  c->label.setVisible(active);c->box.setVisible(active);
 }
 for(auto& c:toggles)c->button.setVisible(expanded&&c->module==currentModule);
 resized();repaint();
}
void BoomChainPanel::resized(){
 const int w=getWidth(),h=getHeight();
 const int header=juce::jlimit(31,46,h/5);
 const int pad=juce::jmax(8,w/130);
 const int title=juce::jmax(96,w*11/100);
 const int hideW=juce::jmax(87,w*9/100);
 const int resetW=juce::jmax(90,w*10/100);
 const int bypassW=juce::jmax(90,w*10/100);
 const int tabsX=pad+title;
 const int resetX=w-hideW-resetW-bypassW-3*pad;
 const int tabsW=juce::jmax(7*27,resetX-tabsX-pad);
 const int tabW=tabsW/7;
 for(int i=0;i<7;++i)
  tabButtons[static_cast<std::size_t>(i)].setBounds(
   tabsX+i*tabW,pad/2,juce::jmax(26,tabW-4),header-7);
 visibilityButton.setBounds(w-hideW-pad,pad/2,hideW,header-7);
 resetButton.setBounds(resetX,pad/2,resetW,header-7);
 for(auto& c:toggles)if(c->primary)
  c->button.setBounds(w-hideW-bypassW-2*pad,pad/2,bypassW,header-7);
 if(!expanded)return;
 int regularToggles=0,items=0;
 for(const auto& c:rotaries)if(c->module==currentModule)++items;
 for(const auto& c:choices)if(c->module==currentModule)++items;
 for(const auto& c:toggles)if(c->module==currentModule&&!c->primary)++regularToggles;
 const int toggleH=regularToggles?juce::jmax(22,h/7):0;
 const int top=header+juce::jmax(7,h/28);
 const int space=juce::jmax(1,w-2*pad);
 const int tile=space/std::max(1,items);
 int col=0;
 const int labelHeight=juce::jlimit(17,24,h/9);
 const int sliderH=std::max(52,h-top-labelHeight-toggleH-8);
 for(auto& c:rotaries)if(c->module==currentModule){
  const int x=pad+col*tile;
  c->label.setBounds(x,top,tile,labelHeight);
  c->slider.setBounds(x+4,top+labelHeight,std::max(30,tile-8),sliderH);
  ++col;
 }
 for(auto& c:choices)if(c->module==currentModule){
  const int x=pad+col*tile;
  c->label.setBounds(x,top,tile,labelHeight);
  c->box.setBounds(x+juce::jmax(5,tile/10),top+labelHeight+sliderH/3,
                   std::max(40,tile-juce::jmax(10,tile/5)),juce::jmax(26,labelHeight+5));
  ++col;
 }
 int t=0;
 for(auto& c:toggles)if(c->module==currentModule&&!c->primary){
  const int itemW=juce::jmin(190,space/std::max(1,regularToggles));
  c->button.setBounds(pad+t*(itemW+10),h-toggleH-2,itemW,toggleH);
  ++t;
 }
}
void BoomChainPanel::paint(juce::Graphics& g){
 const auto area=getLocalBounds().toFloat().reduced(.5f);
 juce::ColourGradient fill(juce::Colour::fromRGB(77,24,32),area.getTopLeft(),wood,area.getBottomLeft(),false);
 g.setGradientFill(fill);g.fillRoundedRectangle(area,6.f);
 g.setColour(copper.withAlpha(.85f));g.drawRoundedRectangle(area,6.f,1.2f);
 const int header=juce::jlimit(31,46,getHeight()/5);
 g.setColour(juce::Colour::fromRGB(31,13,17));
 g.fillRoundedRectangle(juce::Rectangle<float>(1,1,float(getWidth()-2),float(header)),5.f);
 g.setColour(gold);g.setFont(font(juce::jlimit(11.f,15.f,getWidth()/92.f),true));
 g.drawFittedText("BOOM  /  CHAIN",11,3,juce::jmax(75,getWidth()*11/100),header-7,
                  juce::Justification::centredLeft,1);
 if(!expanded){
  g.setColour(gold.withAlpha(.88f));
  g.setFont(font(juce::jlimit(16.f,23.f,getWidth()/70.f),true));
  g.drawFittedText(english?"MASTERING CHAIN HIDDEN":"CHAINE DE MASTERING MASQUEE",
                   20,header+15,getWidth()-40,std::max(27,(getHeight()-header)/2),
                   juce::Justification::centred,1);
  g.setColour(ivory.withAlpha(.65f));g.setFont(font(12));
  g.drawFittedText(english?"Audio processing remains active":
                   "Les traitements audio restent actifs",20,getHeight()-42,
                   getWidth()-40,22,juce::Justification::centred,1);
  return;
 }
 g.setColour(gold.withAlpha(.22f));
 g.drawHorizontalLine(header+2,9.f,float(getWidth()-9));
 // Active/bypass information belongs to the module's native toggle button.
 // No status text is overpainted on the header actions.
}
