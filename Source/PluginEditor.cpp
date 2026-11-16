#include "PluginEditor.h"
#include <cmath>

namespace {
const juce::Colour brass=juce::Colour::fromRGB(198,161,101);
const juce::Colour ivory=juce::Colour::fromRGB(239,222,179);
const juce::Colour charcoal=juce::Colour::fromRGB(31,29,26);
const juce::Colour walnut=juce::Colour::fromRGB(57,37,29);
const juce::Colour amber=juce::Colour::fromRGB(244,163,69);
const juce::Colour deep=juce::Colour::fromRGB(17,16,15);
const juce::Font font(float size,bool bold=false){return juce::Font(juce::FontOptions(size,bold?juce::Font::bold:juce::Font::plain));}

class OverlayBase:public juce::Component {
public:
 explicit OverlayBase(juce::String title,std::function<void()> close):heading(std::move(title)),onClose(std::move(close)){
  addAndMakeVisible(closeButton);closeButton.setButtonText("FERMER  ×");closeButton.onClick=[this]{
   auto callback=onClose; juce::MessageManager::callAsync([callback]{callback();});
  };
 }
 void paint(juce::Graphics& g)override{
  auto bounds=getLocalBounds().toFloat();
  g.setColour(juce::Colours::black.withAlpha(0.83f));g.fillRoundedRectangle(bounds,10);
  g.setColour(brass);g.drawRoundedRectangle(bounds.reduced(2),10,2);
  g.setColour(ivory);g.setFont(font(22,true));g.drawText(heading,24,12,getWidth()-200,32,juce::Justification::centredLeft);
  g.setColour(brass.withAlpha(.5f));g.drawLine(20.f,56.f,float(getWidth()-20),56.f);
 }
 void resized()override{closeButton.setBounds(getWidth()-132,14,110,32);}
protected:
 juce::String heading;
 juce::TextButton closeButton;
 std::function<void()> onClose;
};

class BrowserPanel final:public OverlayBase,private juce::ListBoxModel{
public:
 explicit BrowserPanel(BoomAudioProcessor& processor,std::function<void()> close):
 OverlayBase("BOOM  |  PRESET BROWSER",std::move(close)),proc(processor){
  addAndMakeVisible(search);
  search.setTextToShowWhenEmpty("Rechercher un preset...",juce::Colours::grey);
  search.onTextChange=[this]{refresh();};
  addAndMakeVisible(category);
  category.addItem("Toutes les catégories",1);
  for(int c=0;c<10;++c)category.addItem(juce::String(boom::categories[c]),c+2);
  category.setSelectedId(1,juce::dontSendNotification);
  category.onChange=[this]{refresh();};
  addAndMakeVisible(styleFilter);
  styleFilter.addItem("Tous les Styles",1);
  for(int i=0;i<40;++i)styleFilter.addItem(boom::styles()[i].name,i+2);
  styleFilter.setSelectedId(1,juce::dontSendNotification);
  styleFilter.onChange=[this]{refresh();};
  addAndMakeVisible(sourceFilter);
  sourceFilter.addItem("USINE + UTILISATEUR",1);
  sourceFilter.addItem("USINE",2);
  sourceFilter.addItem("UTILISATEUR",3);
  sourceFilter.setSelectedId(1,juce::dontSendNotification);
  sourceFilter.onChange=[this]{refresh();};
  addAndMakeVisible(list);list.setModel(this);list.setRowHeight(32);
  addAndMakeVisible(userName);
  userName.setTextToShowWhenEmpty("Nom du preset utilisateur...",juce::Colours::grey);
  auto addButton=[this](juce::TextButton& b,const juce::String& t, std::function<void()> callback){
   addAndMakeVisible(b);b.setButtonText(t);b.onClick=std::move(callback);
  };
  addButton(load,"CHARGER",[this]{loadSelected();});
  addButton(previous,"PRÉCÉDENT",[this]{move(-1);});
  addButton(next,"SUIVANT",[this]{move(1);});
  addButton(favourite,"☆ FAVORI",[this]{const auto row=getSelected();
   if(!row.valid||row.user)return;
   proc.toggleFavorite(row.index);list.repaint();
  });
  addButton(saveUser,"SAUVER",[this]{
   const int index=proc.saveUserPreset(userName.getText());
   message.setText(index>=0?"Preset utilisateur sauvegardé dans le projet DAW.":
       "Indiquez un nom valide (maximum 512 presets).",juce::dontSendNotification);
   if(index>=0){
    category.setSelectedId(1,juce::dontSendNotification);
    styleFilter.setSelectedId(1,juce::dontSendNotification);
    search.clear();
    sourceFilter.setSelectedId(3,juce::dontSendNotification);refresh();selectRow(true,index);
   }
  });
  addButton(renameUser,"RENOMMER",[this]{const auto r=getSelected();
   if(!r.valid||!r.user)return;
   const bool ok=proc.renameUserPreset(r.index,userName.getText());
   message.setText(ok?"Preset renommé.":"Nom invalide.",juce::dontSendNotification);
   refresh();if(ok)selectRow(true,r.index);
  });
  addButton(deleteUser,"SUPPRIMER",[this]{const auto r=getSelected();
   if(!r.valid||!r.user)return;
   if(!deleteArmed||deleteIndex!=r.index){
    deleteArmed=true;deleteIndex=r.index;
    message.setText("Cliquez une deuxième fois sur SUPPRIMER pour confirmer.",juce::dontSendNotification);
    return;
   }
   const bool ok=proc.deleteUserPreset(r.index);
   deleteArmed=false;deleteIndex=-1;
   message.setText(ok?"Preset utilisateur supprimé.":"Suppression impossible.",juce::dontSendNotification);
   refresh();
  });
  addButton(importUser,"IMPORTER .BOOM",[this]{openImport();});
  addButton(exportUser,"EXPORTER .BOOM",[this]{openExport();});
  addAndMakeVisible(message);message.setColour(juce::Label::textColourId,ivory);
  message.setJustificationType(juce::Justification::centredLeft);
  message.setFont(font(12));
  message.setText("Les presets utilisateur sont intégrés à l'état du projet ; exportez-les pour les partager.",juce::dontSendNotification);
  refresh();
 }
 ~BrowserPanel() override {list.setModel(nullptr);}
 int getNumRows() override{return static_cast<int>(shown.size());}
 void selectedRowsChanged(int) override {
  deleteArmed=false;deleteIndex=-1;
  const auto row=getSelected();
  if(row.valid&&row.user)userName.setText(proc.userPresetName(row.index),juce::dontSendNotification);
 }
 void paintListBoxItem(int row,juce::Graphics& g,int width,int height,bool selected)override{
  if(row<0||row>=static_cast<int>(shown.size()))return;
  const auto pr=shown[static_cast<std::size_t>(row)];
  if(selected){g.setColour(brass.withAlpha(.27f));g.fillRect(0,0,width,height);}
  else {g.setColour(row%2?juce::Colour::fromRGB(46,42,36):juce::Colour::fromRGB(39,36,32));g.fillRect(0,0,width,height);}
  juce::String name,style;
  if(pr.user){name="◆  "+proc.userPresetName(pr.index);style="UTILISATEUR";}
  else {const auto p=boom::makePreset(pr.index);
   name=juce::String(proc.isFavorite(pr.index)?"★  ":"☆  ")+p.name;
   style=juce::String(boom::styles()[p.style].name);
  }
  g.setColour(ivory);g.setFont(font(15));
  g.drawText(name,12,0,width-250,height,juce::Justification::centredLeft);
  g.setColour(brass);g.setFont(font(12));
  g.drawText(style,width-232,0,218,height,juce::Justification::centredRight);
 }
 void listBoxItemDoubleClicked(int row,const juce::MouseEvent&)override {list.selectRow(row);loadSelected();}
 void resized()override{
  OverlayBase::resized();
  const auto r=getLocalBounds().reduced(22);
  const int width=r.getWidth(),x=r.getX();
  search.setBounds(x,77,width*37/100,36);
  category.setBounds(x+width*38/100,77,width*29/100,36);
  styleFilter.setBounds(x+width*68/100,77,width*32/100,36);
  sourceFilter.setBounds(x,120,220,34);
  list.setBounds(x,164,width,getHeight()-333);
  userName.setBounds(x,getHeight()-160,width*38/100,34);
  saveUser.setBounds(x+width*39/100,getHeight()-160,120,34);
  renameUser.setBounds(x+width*52/100,getHeight()-160,130,34);
  deleteUser.setBounds(x+width*68/100,getHeight()-160,136,34);
  message.setBounds(x,getHeight()-116,width,25);
  const int y=getHeight()-79;
  previous.setBounds(x,y,106,32);
  next.setBounds(x+114,y,106,32);
  favourite.setBounds(x+229,y,104,32);
  importUser.setBounds(x+346,y,148,32);
  exportUser.setBounds(x+504,y,148,32);
  load.setBounds(x+width-176,y,176,32);
 }
private:
 struct Row {bool user=false;int index=-1;bool valid=false;};
 BoomAudioProcessor& proc;
 juce::TextEditor search,userName;
 juce::ComboBox category,styleFilter,sourceFilter;
 juce::ListBox list;
 juce::TextButton load,previous,next,favourite,saveUser,renameUser,deleteUser,importUser,exportUser;
 juce::Label message;
 std::unique_ptr<juce::FileChooser> fileChooser;
 std::vector<Row> shown;
 bool deleteArmed=false;int deleteIndex=-1;
 Row getSelected()const {
  const auto row=list.getSelectedRow();
  return row>=0&&row<int(shown.size())?shown[static_cast<std::size_t>(row)]:Row{};
 }
 void selectRow(bool isUser,int index){
  for(int i=0;i<int(shown.size());++i)if(shown[static_cast<std::size_t>(i)].user==isUser &&
     shown[static_cast<std::size_t>(i)].index==index){list.selectRow(i);break;}
 }
 void refresh(){
  shown.clear();const auto query=search.getText().trim();
  if(sourceFilter.getSelectedId()!=3){
   for(int i=0;i<100;++i){const auto pr=boom::makePreset(i);
    if(category.getSelectedId()>1&&pr.category!=category.getSelectedId()-2)continue;
    if(styleFilter.getSelectedId()>1&&pr.style!=styleFilter.getSelectedId()-2)continue;
    if(!juce::String(pr.name).containsIgnoreCase(query))continue;
    shown.push_back({false,i,true});
   }
  }
  if(sourceFilter.getSelectedId()!=2 && category.getSelectedId()==1 && styleFilter.getSelectedId()==1){
   for(int i=0;i<proc.userPresetCount();++i){
    if(proc.userPresetName(i).containsIgnoreCase(query))shown.push_back({true,i,true});
   }
  }
  list.updateContent();list.deselectAllRows();if(!shown.empty())list.selectRow(0);
 }
 void move(int delta){if(shown.empty())return;
  list.selectRow(juce::jlimit(0,getNumRows()-1,list.getSelectedRow()+delta));
 }
 void loadSelected(){const auto row=getSelected();if(!row.valid)return;
  const bool ok=row.user?proc.loadUserPreset(row.index):(proc.loadFactoryPreset(row.index),true);
  if(ok)message.setText("Preset chargé : les paramètres audio ont été rappelés.",juce::dontSendNotification);
  if(row.user)userName.setText(proc.userPresetName(row.index),juce::dontSendNotification);
 }
 void openImport(){
  fileChooser=std::make_unique<juce::FileChooser>("Importer un preset BOOM",juce::File{},"*.boom");
  juce::Component::SafePointer<BrowserPanel> safe(this);
  fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
    [safe](const juce::FileChooser& chooser){
     if(safe==nullptr)return;
     const auto file=chooser.getResult();if(file==juce::File{})return;
     const bool ok=safe->proc.importUserPreset(file);
     safe->message.setText(ok?"Preset importé dans la bibliothèque du projet.":
       "Fichier invalide ou bibliothèque pleine.",juce::dontSendNotification);
     if(ok){
      safe->category.setSelectedId(1,juce::dontSendNotification);
      safe->styleFilter.setSelectedId(1,juce::dontSendNotification);
      safe->search.clear();
      safe->sourceFilter.setSelectedId(3,juce::dontSendNotification);
      safe->refresh();safe->selectRow(true,safe->proc.userPresetCount()-1);
     }
    });
 }
 void openExport(){
  const auto row=getSelected();if(!row.valid||!row.user){
   message.setText("Sélectionnez un preset utilisateur pour l'exporter.",juce::dontSendNotification);return;
  }
  auto suggested=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
   .getChildFile(juce::File::createLegalFileName(proc.userPresetName(row.index))+".boom");
  fileChooser=std::make_unique<juce::FileChooser>("Exporter un preset BOOM",suggested,"*.boom");
  juce::Component::SafePointer<BrowserPanel> safe(this);
  const int index=row.index;
  fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles |
    juce::FileBrowserComponent::warnAboutOverwriting,
    [safe,index](const juce::FileChooser& chooser){
     if(safe==nullptr)return;
     auto file=chooser.getResult();if(file==juce::File{})return;
     file=file.withFileExtension(".boom");
     const bool ok=safe->proc.exportUserPreset(index,file);
     safe->message.setText(ok?"Preset exporté vers : "+file.getFileName():
      "Impossible d'exporter le preset.",juce::dontSendNotification);
    });
 }
};

class ModulePanel final:public juce::Component{
public:
 ModulePanel(juce::AudioProcessorValueTreeState& state,
  std::initializer_list<std::pair<const char*,const char*>> configs,
  const char* bypassId=nullptr,const char* modeId=nullptr,bool stereoLink=false){
  for(const auto& c:configs){
   auto slider=std::make_unique<juce::Slider>();
   slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
   slider->setTextBoxStyle(juce::Slider::TextBoxBelow,false,105,23);
   slider->setColour(juce::Slider::textBoxTextColourId,ivory);
   slider->setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
   addAndMakeVisible(*slider);
   auto label=std::make_unique<juce::Label>();label->setText(c.second,juce::dontSendNotification);
   label->setJustificationType(juce::Justification::centred);label->setColour(juce::Label::textColourId,ivory);
   addAndMakeVisible(*label);
   attachments.emplace_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state,c.first,*slider));
   sliders.push_back(std::move(slider));labels.push_back(std::move(label));
  }
  if(bypassId!=nullptr){
   bypass=std::make_unique<juce::ToggleButton>("BYPASS MODULE");
   addAndMakeVisible(*bypass);
   bypassAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(state,bypassId,*bypass);
  }
  if(modeId!=nullptr){
   mode=std::make_unique<juce::ComboBox>();
   const bool analog=juce::String(modeId)=="color";
   const char* const colors[]={"CLEAN","TUBE","TAPE"};
   const char* const limiter[]={"TRANSPARENT","PUNCH","LOUD"};
   for(int i=0;i<3;++i)mode->addItem(analog?colors[i]:limiter[i],i+1);
   addAndMakeVisible(*mode);
   modeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(state,modeId,*mode);
  }
  if(stereoLink){
   link=std::make_unique<juce::ToggleButton>("STEREO LINK");
   addAndMakeVisible(*link);
   linkAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(state,"compStereoLink",*link);
  }
 }
 void resized()override{
  auto b=getLocalBounds().reduced(12);
  auto head=b.removeFromTop(43);
  if(mode)mode->setBounds(head.removeFromLeft(220).reduced(0,5));
  if(link)link->setBounds(head.removeFromLeft(154));
  if(bypass)bypass->setBounds(head.removeFromRight(170));
  const auto n=static_cast<int>(sliders.size());if(n==0)return;
  const int cellW=b.getWidth()/n;
  for(int i=0;i<n;++i){
   auto col=b.removeFromLeft(i==n-1?b.getWidth():cellW);
   labels[static_cast<std::size_t>(i)]->setBounds(col.removeFromTop(32));
   sliders[static_cast<std::size_t>(i)]->setBounds(col.reduced(5,3));
  }
 }
private:
 std::vector<std::unique_ptr<juce::Slider>> sliders;
 std::vector<std::unique_ptr<juce::Label>> labels;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
 std::unique_ptr<juce::ToggleButton> bypass,link;
 std::unique_ptr<juce::ComboBox> mode;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment,linkAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
};

class ChainPanel final:public OverlayBase{
public:
 explicit ChainPanel(juce::AudioProcessorValueTreeState& state,std::function<void()> close):
 OverlayBase("BOOM  |  MASTERING CHAIN",std::move(close)),tabs(juce::TabbedButtonBar::TabsAtTop){
  addAndMakeVisible(tabs);tabs.setTabBarDepth(40);
  tabs.addTab("ANALOG COLOR",charcoal,new ModulePanel(state,{{"colorDrive","DRIVE"},{"colorMix","MIX"},{"colorOut","OUTPUT COMP"}},"colorBypass","color"),true);
  tabs.addTab("MASTER EQ",charcoal,new ModulePanel(state,{{"eq60","60 Hz"},{"eq120","120 Hz"},{"eq3k","3 kHz"},{"eq6k","6 kHz"},{"eq10k","10 kHz"}},"eqBypass"),true);
  tabs.addTab("DE-ESSER",charcoal,new ModulePanel(state,{{"deFreq","FREQUENCY"},{"deTh","THRESHOLD"},{"deAmt","AMOUNT"},{"deAtt","ATTACK"},{"deRel","RELEASE"}},"deEssBypass"),true);
  tabs.addTab("GLUE COMP",charcoal,new ModulePanel(state,{{"compTh","THRESHOLD"},{"compRatio","RATIO"},{"compAttack","ATTACK"},{"compRelease","RELEASE"},{"compMakeup","MAKEUP"},{"compMix","MIX"}},"compBypass",nullptr,true),true);
  tabs.addTab("PEAK LIMIT",charcoal,new ModulePanel(state,{{"limitTh","THRESHOLD"},{"ceiling","CEILING"},{"limitRelease","RELEASE"}},"limiterBypass","limitMode"),true);
  tabs.addTab("OUTPUT",charcoal,new ModulePanel(state,{{"inputLTrim","INPUT L"},{"inputRTrim","INPUT R"},{"outDb","OUTPUT TRIM"}}),true);
 }
 void resized() override{OverlayBase::resized();tabs.setBounds(15,78,getWidth()-30,getHeight()-94);}
private: juce::TabbedComponent tabs;
};

class MeterPanel final:public OverlayBase,private juce::Timer{
public:
 explicit MeterPanel(BoomAudioProcessor& p,std::function<void()> close):
 OverlayBase("BOOM  |  AUDIO METERS",std::move(close)),proc(p){
  addAndMakeVisible(reset);reset.setButtonText("RESET LUFS / LRA");
  reset.onClick=[this]{proc.requestLoudnessReset();};
  startTimerHz(20);
 }
 ~MeterPanel()override{stopTimer();}
 void timerCallback()override{repaint();}
 void resized() override{OverlayBase::resized();reset.setBounds(25,getHeight()-47,172,30);}
 void paint(juce::Graphics& g)override{
  OverlayBase::paint(g);const auto m=proc.lastMeters();
  struct Reading {const char* label;float value;const char* unit;bool useDb;};
  const std::array<Reading,16> data{{
   {"INPUT L",m.inputPeakL,"dBFS",true},{"INPUT R",m.inputPeakR,"dBFS",true},
   {"OUTPUT L",m.outputPeakL,"dBFS",true},{"OUTPUT R",m.outputPeakR,"dBFS",true},
   {"RMS L",m.outputRmsL,"dBFS",true},{"RMS R",m.outputRmsR,"dBFS",true},
   {"VU L",m.vuL,"dBFS",true},{"VU R",m.vuR,"dBFS",true},
   {"COMP GR",m.compressorGainReductionDb,"dB",false},{"LIMIT GR",m.limiterGainReductionDb,"dB",false},
   {"MOMENTARY",m.momentaryLufs,"LUFS",false},{"SHORT-TERM",m.shortTermLufs,"LUFS",false},
   {"INTEGRATED",m.integratedLufs,"LUFS",false},{"LOUDNESS RANGE",m.lra,"LU",false},
   {"TRUE-PEAK EST.",m.truePeak,"dBTP*",true},{"CORRELATION",m.correlation,"",false}
  }};
  const int gap=9,columns=4,w=(getWidth()-50-(columns-1)*gap)/columns;
  for(std::size_t i=0;i<data.size();++i){
   const int x=25+int(i%4)*(w+gap),y=80+int(i/4)*72;
   g.setColour(juce::Colour::fromRGB(46,41,35));g.fillRoundedRectangle(float(x),float(y),float(w),64.f,5.f);
   g.setColour(brass);g.setFont(font(12,true));g.drawText(data[i].label,x+10,y+5,w-20,18,juce::Justification::centredLeft);
   const float v=data[i].useDb?boom::gainToDb(data[i].value):data[i].value;
   juce::String display=(v<=-99.9f&&juce::String(data[i].unit)=="LUFS")?"—":juce::String(v,1);
   g.setColour(ivory);g.setFont(font(20,true));g.drawText(display+" "+data[i].unit,x+8,y+28,w-16,26,juce::Justification::centredRight);
  }
  g.setColour(brass);g.setFont(font(11));
  g.drawText("* dBTP estimé : interpolation 4×, conformité non certifiée.",215,getHeight()-45,getWidth()-242,29,juce::Justification::centredRight);
 }
private:
 BoomAudioProcessor& proc;juce::TextButton reset;
};

class HelpPanel final:public OverlayBase{
public:
 explicit HelpPanel(std::function<void()> close):OverlayBase("BOOM  |  GUIDE RAPIDE",std::move(close)){}
 void paint(juce::Graphics& g)override{
  OverlayBase::paint(g);g.setColour(ivory);g.setFont(font(18));
  g.drawFittedText("1. Sélectionnez un STYLE.\n2. Choisissez un PRESET.\n3. Ajustez INPUT DRIVE et les 5 bandes EQ.\n4. Montez PUSH pour un mastering plus dense.\n5. Comparez A / B à niveau similaire.\n\nATTENTION : le mode MONO modifie l'écoute du plugin tant qu'il est actif.\nCe prototype doit être validé avant un usage commercial.",30,94,getWidth()-60,getHeight()-130,juce::Justification::topLeft,16);
 }
};
}

HardwareLook::HardwareLook(){
 setColour(juce::ComboBox::textColourId,ivory);
 setColour(juce::ComboBox::backgroundColourId,charcoal);
 setColour(juce::ComboBox::outlineColourId,brass);
 setColour(juce::PopupMenu::backgroundColourId,charcoal);
 setColour(juce::PopupMenu::textColourId,ivory);
 setColour(juce::TextButton::textColourOffId,ivory);
 setColour(juce::TextButton::buttonColourId,charcoal);
 setColour(juce::Label::textColourId,ivory);
 setColour(juce::Slider::textBoxTextColourId,ivory);
 setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
}
void HardwareLook::drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,float position,
 float start,float end,juce::Slider&){
 auto b=juce::Rectangle<float>(float(x),float(y),float(width),float(height));
 float radius=std::min(b.getWidth(),b.getHeight())*0.37f;
 auto centre=b.getCentre();const float angle=start+position*(end-start);
 g.setColour(juce::Colours::black.withAlpha(.6f));g.fillEllipse(centre.x-radius+3,centre.y-radius+6,radius*2,radius*2);
 juce::ColourGradient trim(brass.brighter(.35f),centre.x-radius,centre.y-radius,
    juce::Colour::fromRGB(57,43,31),centre.x+radius,centre.y+radius,false);
 g.setGradientFill(trim);g.fillEllipse(centre.x-radius,centre.y-radius,radius*2,radius*2);
 g.setColour(juce::Colours::black.withAlpha(.7f));g.drawEllipse(centre.x-radius,centre.y-radius,radius*2,radius*2,3.5f);
 radius*=0.84f;
 juce::ColourGradient metal(juce::Colour::fromRGB(185,172,145),centre.x-radius,centre.y-radius,
  juce::Colour::fromRGB(65,57,47),centre.x+radius,centre.y+radius,true);
 metal.addColour(.48,juce::Colour::fromRGB(98,87,72));metal.addColour(.68,juce::Colour::fromRGB(49,43,37));
 g.setGradientFill(metal);g.fillEllipse(centre.x-radius,centre.y-radius,radius*2,radius*2);
 g.setColour(juce::Colours::white.withAlpha(.14f));g.drawEllipse(centre.x-radius+2,centre.y-radius+2,radius*2-4,radius*2-4,2.f);
 const float tip=radius*.83f,tail=radius*.22f;
 auto direction=juce::Point<float>(std::sin(angle),-std::cos(angle));
 auto perpendicular=juce::Point<float>(direction.y,-direction.x);
 juce::Path pointer;pointer.startNewSubPath(centre+direction*tip);
 pointer.lineTo(centre+direction*tail+perpendicular*2.7f);
 pointer.lineTo(centre+direction*tail-perpendicular*2.7f);pointer.closeSubPath();
 g.setColour(amber);g.fillPath(pointer);
 const float tickRadius=radius*1.24f;
 for(int i=0;i<=10;++i){float a=start+(end-start)*i/10.0f;
  auto from=centre+juce::Point<float>(std::sin(a),-std::cos(a))*tickRadius;
  auto to=centre+juce::Point<float>(std::sin(a),-std::cos(a))*(tickRadius+(i%5==0?10.f:5.f));
  g.setColour((i<=int(position*10+.2f))?ivory.withAlpha(.82f):brass.withAlpha(.4f));
  g.drawLine(from.x,from.y,to.x,to.y,i%5==0?2.f:1.f);
 }
}
void HardwareLook::drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool highlighted,bool down){
 auto r=b.getLocalBounds().toFloat().reduced(1);
 juce::ColourGradient fill(down?brass.darker():juce::Colour::fromRGB(70,60,48),r.getX(),r.getY(),
  down?charcoal:deep,r.getRight(),r.getBottom(),false);
 g.setGradientFill(fill);g.fillRoundedRectangle(r,5);
 g.setColour(b.getToggleState()?amber:(highlighted?ivory:brass.withAlpha(.6f)));
 g.drawRoundedRectangle(r,5,b.getToggleState()?2.f:1.f);
}
void HardwareLook::drawComboBox(juce::Graphics& g,int w,int h,bool,int,int,int,int,juce::ComboBox&){
 juce::ColourGradient fill(juce::Colour::fromRGB(58,49,40),0,0,deep,0,float(h),false);
 g.setGradientFill(fill);g.fillRoundedRectangle(0,0,float(w),float(h),5);
 g.setColour(brass);g.drawRoundedRectangle(0,0,float(w),float(h),5,1);
 juce::Path p;p.startNewSubPath(float(w-24),float(h*.42));p.lineTo(float(w-16),float(h*.62));p.lineTo(float(w-8),float(h*.42));
 g.strokePath(p,juce::PathStrokeType(1.7f));
}
juce::Font HardwareLook::getComboBoxFont(juce::ComboBox&){return font(15,true);}

void AnalogVu::setLevel(float linear){
 const float zeroRef=boom::dbToGain(-18.f);
 target=juce::jlimit(-20.f,3.f,boom::gainToDb(linear/zeroRef));
 current+=0.24f*(target-current);repaint();
}
void AnalogVu::paint(juce::Graphics& g){
 auto r=getLocalBounds().toFloat().reduced(2);
 g.setColour(juce::Colours::black.withAlpha(.6f));g.fillRoundedRectangle(r.translated(2,4),8);
 juce::ColourGradient metal(brass.brighter(.25f),r.getTopLeft(),brass.darker(.75f),r.getBottomRight(),false);
 g.setGradientFill(metal);g.fillRoundedRectangle(r,8);
 auto face=r.reduced(8,9);
 g.setColour(ivory);g.fillRoundedRectangle(face,4);
 juce::ColourGradient shade(juce::Colours::white.withAlpha(.15f),face.getTopLeft(),juce::Colour::fromRGB(139,112,79).withAlpha(.29f),face.getBottomRight(),false);
 g.setGradientFill(shade);g.fillRoundedRectangle(face,4);
 const auto center=juce::Point<float>(face.getCentreX(),face.getBottom()-10),rad=face.getWidth()*.42f;
 for(int i=0;i<=10;++i){const float v=-20.f+i*2.3f;
  const float a=juce::jmap(v,-20.f,3.f,-1.11f,1.11f);
  const auto origin=center+juce::Point<float>(std::sin(a),-std::cos(a))*rad*.70f;
  const auto end=center+juce::Point<float>(std::sin(a),-std::cos(a))*rad*.83f;
  g.setColour(i>=9?juce::Colours::darkred:charcoal);g.drawLine(origin.x,origin.y,end.x,end.y,i%2==0?1.6f:.9f);
 }
 g.setColour(charcoal);g.setFont(font(11,true));
 g.drawFittedText("-20       -10          0   +3",face.toNearestInt().reduced(8).withTrimmedTop(int(face.getHeight()*.40f)).withHeight(20),juce::Justification::centred,1);
 g.setFont(font(18,true));g.drawText("VU",face.toNearestInt().withTrimmedTop(int(face.getHeight()*.42f)).withHeight(24),juce::Justification::centred);
 g.setFont(font(11,true));g.drawText(name,face.toNearestInt().removeFromTop(23),juce::Justification::centred);
 const float a=juce::jmap(current,-20.f,3.f,-1.11f,1.11f);
 const auto tip=center+juce::Point<float>(std::sin(a),-std::cos(a))*rad*.88f;
 g.setColour(juce::Colours::darkred);g.drawLine(center.x,center.y,tip.x,tip.y,2.1f);
 g.setColour(charcoal);g.fillEllipse(center.x-5,center.y-5,10,10);
}

BoomAudioProcessorEditor::BoomAudioProcessorEditor(BoomAudioProcessor& p):AudioProcessorEditor(&p),processor(p){
 setLookAndFeel(&look);
 const char* ids[]={"inputDb","eq60","eq120","eq3k","eq6k","eq10k","push"};
 const char* titles[]={"INPUT DRIVE","60 Hz","120 Hz","3 kHz","6 kHz","10 kHz","PUSH"};
 for(int i=0;i<7;++i){
  auto& k=knobs[static_cast<std::size_t>(i)];addAndMakeVisible(k);
  k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  k.setRotaryParameters(juce::degreesToRadians(225.f),juce::degreesToRadians(495.f),true);
  k.setTextBoxStyle(juce::Slider::TextBoxBelow,false,i==0||i==6?110:94,25);
  k.setColour(juce::Slider::textBoxTextColourId,ivory);
  k.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
  k.setDoubleClickReturnValue(true,i==6?0:0);
  attach(k,ids[i]);
  auto& label=knobLabels[static_cast<std::size_t>(i)];addAndMakeVisible(label);
  label.setText(titles[i],juce::dontSendNotification);
  label.setJustificationType(juce::Justification::centred);
  label.setColour(juce::Label::textColourId,ivory);
  label.setFont(font(i==0||i==6?20.f:16.f,true));
 }
 addAndMakeVisible(vuL);addAndMakeVisible(vuR);
 addAndMakeVisible(styleBox);addAndMakeVisible(presetBox);
 for(int i=0;i<40;++i)styleBox.addItem(boom::styles()[i].name,i+1);
 for(int i=0;i<100;++i)presetBox.addItem(boom::makePreset(i).name,i+1);
 styleBox.setSelectedId(processor.styleIndex()+1,juce::dontSendNotification);
 if(processor.presetIndex()>=0)presetBox.setSelectedId(processor.presetIndex()+1,juce::dontSendNotification);
 styleBox.onChange=[this]{if(styleBox.getSelectedId()>0){processor.loadStyle(styleBox.getSelectedId()-1);presetBox.setSelectedId(0,juce::dontSendNotification);}};
 presetBox.onChange=[this]{if(presetBox.getSelectedId()>0){processor.loadFactoryPreset(presetBox.getSelectedId()-1);
  styleBox.setSelectedId(processor.styleIndex()+1,juce::dontSendNotification);}};
 for(auto* b:{&browserButton,&chainButton,&metersButton,&languageButton,&helpButton,
  &bypassButton,&monoButton,&meterSourceButton,&storeA,&storeB,&recallA,&recallB,&abMenuButton})addAndMakeVisible(b);
 bypassButton.setClickingTogglesState(true);monoButton.setClickingTogglesState(true);
 meterSourceButton.setClickingTogglesState(true);
 bypassAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state(),"bypass",bypassButton);
 monoAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state(),"mono",monoButton);
 meterAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state(),"meterInput",meterSourceButton);
 browserButton.onClick=[this]{showBrowser();};chainButton.onClick=[this]{showChain();};
 metersButton.onClick=[this]{showMeters();};helpButton.onClick=[this]{showHelp();};
 languageButton.onClick=[this]{auto* v=processor.state().getRawParameterValue("language");
  auto* param=processor.state().getParameter("language");if(param&&v)param->setValueNotifyingHost(param->convertTo0to1(v->load()<0.5f?1.f:0.f));
  setLanguage(v&&v->load()>0.5f);};
 abMenuButton.onClick=[this]{
  juce::PopupMenu menu;
  const bool en=processor.state().getRawParameterValue("language")->load()>0.5f;
  menu.addItem(1,en?"Copy A to B":"Copier A vers B");
  menu.addItem(2,en?"Copy B to A":"Copier B vers A");
  auto safe=juce::Component::SafePointer<BoomAudioProcessorEditor>(this);
  menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(abMenuButton),[safe](int choice){
   if(safe==nullptr)return;
   if(choice==1)safe->processor.copyComparisonState(0,1);
   if(choice==2)safe->processor.copyComparisonState(1,0);
  });
 };
 storeA.onClick=[this]{storeState(0);};storeB.onClick=[this]{storeState(1);};
 recallA.onClick=[this]{recallState(0);};recallB.onClick=[this]{recallState(1);};
 setLanguage(processor.state().getRawParameterValue("language")->load()>0.5f);
 setResizable(true,true);setResizeLimits(1120,700,1920,1200);
 if(auto* c=getConstrainer())c->setFixedAspectRatio(1.6);
 setSize(1440,900);startTimerHz(30);
}
BoomAudioProcessorEditor::~BoomAudioProcessorEditor(){stopTimer();closeOverlays();setLookAndFeel(nullptr);}
void BoomAudioProcessorEditor::attach(juce::Slider& k,const char* id){
 knobAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state(),id,k));
}
juce::Rectangle<int> BoomAudioProcessorEditor::rect(int x,int y,int w,int h)const{
 return {juce::roundToInt(x*getWidth()/1440.0f),juce::roundToInt(y*getHeight()/900.0f),
  juce::roundToInt(w*getWidth()/1440.0f),juce::roundToInt(h*getHeight()/900.0f)};
}
void BoomAudioProcessorEditor::paintStudio(juce::Graphics& g,float w,float){
 juce::ColourGradient wall(juce::Colour::fromRGB(62,46,35),0,0,
  juce::Colour::fromRGB(13,12,12),0,420,false);
 g.setGradientFill(wall);g.fillRect(0,0,1440,415);
 // Acoustic wood slats, textured treatment and understated indirect warm lighting.
 for(int i=0;i<58;++i){
  int x=i*27;g.setColour(juce::Colour::fromRGB(97,64,39).withAlpha(i%3==0?.40f:.20f));g.fillRect(x,84,9,232);
  g.setColour(juce::Colours::black.withAlpha(.35f));g.fillRect(x+10,84,4,232);
 }
 juce::ColourGradient light(amber.withAlpha(.24f),720,132,juce::Colours::transparentBlack,720,368,true);
 g.setGradientFill(light);g.fillRect(0,95,1440,315);
 for(int channel=0;channel<2;++channel){
  const int x=channel==0?215:1135;
  g.setColour(juce::Colours::black.withAlpha(.6f));g.fillRoundedRectangle(float(x-74),135,148,234,12);
  juce::ColourGradient speaker(juce::Colour::fromRGB(39,39,39),x-70.f,140.f,juce::Colour::fromRGB(7,8,8),x+70.f,370.f,false);
  g.setGradientFill(speaker);g.fillRoundedRectangle(x-68.f,141.f,136.f,222.f,9);
  for(int j=0;j<2;++j){float cy=j==0?195.f:297.f,rad=j==0?28.f:49.f;
   g.setColour(juce::Colour::fromRGB(72,70,63));g.fillEllipse(x-rad,cy-rad,rad*2,rad*2);
   g.setColour(juce::Colour::fromRGB(16,16,16));g.fillEllipse(x-rad*.75f,cy-rad*.75f,rad*1.5f,rad*1.5f);
   g.setColour(juce::Colour::fromRGB(85,80,70));g.fillEllipse(x-rad*.36f,cy-rad*.36f,rad*.72f,rad*.72f);
  }
 }
 // Centered mastering rack and mastering desk
 g.setColour(juce::Colour::fromRGB(16,16,16));g.fillRoundedRectangle(488,183,464,183,8);
 for(int i=0;i<4;++i){
  int y=195+i*40;g.setColour(juce::Colour::fromRGB(71,65,57));g.fillRoundedRectangle(503.f,float(y),434.f,35.f,4);
  g.setColour(brass.withAlpha(.8f));g.drawRoundedRectangle(503.f,float(y),434.f,35.f,4,1);
  for(int j=0;j<8;++j){g.setColour(juce::Colour::fromRGB(32,32,32));g.fillEllipse(523.f+j*48.f,float(y+10),15,15);
   g.setColour(amber.withAlpha((i+j)%3==0?.95f:.40f));g.fillEllipse(529.f+j*48.f,float(y+16),3,3);}
 }
 g.setColour(juce::Colour::fromRGB(94,58,37));g.fillRect(0,375,1440,40);
 g.setColour(juce::Colour::fromRGB(149,92,44).withAlpha(.7f));g.drawLine(0,376,1440,376,3);
 (void)w;
}
void BoomAudioProcessorEditor::paint(juce::Graphics& g){
 juce::Graphics::ScopedSaveState saved(g);
 g.addTransform(juce::AffineTransform::scale(getWidth()/1440.f,getHeight()/900.f));
 paintStudio(g,1440,405);
 // Control panel — walnut border, engraved champagne metal, bronzed fittings.
 g.setColour(walnut);g.fillRect(0,404,1440,496);
 for(int i=0;i<26;++i){g.setColour(juce::Colours::black.withAlpha(i%3==0?.11f:.04f));g.drawHorizontalLine(410+i*18,0,1440);}
 juce::ColourGradient panel(juce::Colour::fromRGB(82,75,63),20,432,
  juce::Colour::fromRGB(29,27,25),1420,900,false);
 g.setGradientFill(panel);g.fillRoundedRectangle(20,426,1400,452,17);
 g.setColour(brass);g.drawRoundedRectangle(21,426,1398,450,15,3);
 g.setColour(charcoal.withAlpha(.65f));g.fillRoundedRectangle(33,443,1374,423,10);
 g.setColour(brass.withAlpha(.45f));g.drawLine(338,462,338,850,1.5f);
 g.drawLine(1091,462,1091,850,1.5f);
 // Top logo with subtle golden emboss
 g.setFont(font(67,true));g.setColour(juce::Colours::black.withAlpha(.5f));
 g.drawText("BOOM",521,30,400,80,juce::Justification::centred);
 g.setColour(ivory);g.drawText("BOOM",520,27,400,80,juce::Justification::centred);
 g.setColour(brass);g.setFont(font(17,true));
 g.drawText("PROFESSIONAL ANALOG MASTERING CONSOLE",475,107,490,26,juce::Justification::centred);
 g.setFont(font(14));g.drawText("LIBERTY MUSIC SOFTWARE",525,137,390,24,juce::Justification::centred);
 g.setFont(font(13,true));g.drawText("STYLE",63,28,280,22,juce::Justification::centredLeft);
 g.drawText("PRESET",1054,28,300,22,juce::Justification::centredLeft);
 g.setFont(font(18,true));g.setColour(ivory);
 g.drawText("THE POWER BEHIND YOUR MASTER",432,389,576,27,juce::Justification::centred);
 g.setFont(font(18,true));g.drawText("MASTERING EQUALIZER",455,633,532,30,juce::Justification::centred);
 g.setColour(brass);g.drawLine(470,662,974,662,1.f);
 // Brass screws engraved into hardware panel
 for(auto x:{46,1392})for(auto y:{452,850}){g.setColour(brass);g.fillEllipse(float(x-5),float(y-5),10,10);
  g.setColour(charcoal);g.drawLine(float(x-2),float(y-2),float(x+2),float(y+2),1.4f);}
}
void BoomAudioProcessorEditor::resized(){
 const int xEq[]={412,535,658,781,904}; // 3px minimum separation between EQ controls
 knobs[0].setBounds(rect(67,545,255,247));knobLabels[0].setBounds(rect(78,793,232,32));
 for(int i=1;i<=5;++i){knobs[static_cast<std::size_t>(i)].setBounds(rect(xEq[i-1],679,120,137));
  knobLabels[static_cast<std::size_t>(i)].setBounds(rect(xEq[i-1],818,120,29));}
 knobs[6].setBounds(rect(1145,545,225,248));knobLabels[6].setBounds(rect(1148,793,218,33));
 vuL.setBounds(rect(478,468,224,157));vuR.setBounds(rect(733,468,224,157));
 styleBox.setBounds(rect(56,52,332,39));presetBox.setBounds(rect(1054,52,326,39));
 browserButton.setBounds(rect(1253,99,126,35));
 abMenuButton.setBounds(rect(529,337,106,34));chainButton.setBounds(rect(646,337,106,34));metersButton.setBounds(rect(763,337,106,34));
 languageButton.setBounds(rect(880,337,106,34));helpButton.setBounds(rect(997,337,42,34));
 bypassButton.setBounds(rect(757,850,108,30));monoButton.setBounds(rect(1168,481,154,35));
 meterSourceButton.setBounds(rect(546,850,193,30));
 storeA.setBounds(rect(357,850,83,30));storeB.setBounds(rect(446,850,83,30));
 recallA.setBounds(rect(878,850,70,30));recallB.setBounds(rect(953,850,70,30));
 if(overlay){
  const bool isPresetBrowser=dynamic_cast<BrowserPanel*>(overlay.get())!=nullptr;
  overlay->setBounds(isPresetBrowser?rect(170,229,1100,640):rect(190,435,1060,432));
 }
}
void BoomAudioProcessorEditor::timerCallback(){
 const auto m=processor.lastMeters();vuL.setLevel(m.vuL);vuR.setLevel(m.vuR);
 const int shownStyle=processor.styleIndex()+1,shownPreset=processor.presetIndex()+1;
 if(styleBox.getSelectedId()!=shownStyle)styleBox.setSelectedId(shownStyle,juce::dontSendNotification);
 if(presetBox.getSelectedId()!=shownPreset)presetBox.setSelectedId(shownPreset,juce::dontSendNotification);
 if(auto* raw=processor.state().getRawParameterValue("language")){
  const bool english=raw->load()>=0.5f;
  if(languageButton.getButtonText()!=(english?"EN / FR":"FR / EN"))setLanguage(english);
 }
}
void BoomAudioProcessorEditor::setLanguage(bool en){
 languageButton.setButtonText(en?"EN / FR":"FR / EN");
 browserButton.setButtonText(en?"PRESETS":"PRESETS");
 metersButton.setButtonText(en?"METERS":"MESURES");
 bypassButton.setButtonText(en?"BYPASS":"BYPASS");
 monoButton.setButtonText(en?"MONO MONITOR":"ÉCOUTE MONO");
 storeA.setButtonText(en?"SAVE A":"SAUVER A");storeB.setButtonText(en?"SAVE B":"SAUVER B");
 meterSourceButton.setButtonText(en?"INPUT / PROCESS":"ENTRÉE / TRAITÉ");
 knobLabels[0].setText(en?"INPUT DRIVE":"GAIN D'ENTRÉE",juce::dontSendNotification);
 // EQ frequency labels are internationally understood.
}
void BoomAudioProcessorEditor::storeState(int slot){processor.storeComparisonState(slot);}
void BoomAudioProcessorEditor::recallState(int slot){
 processor.recallComparisonState(slot);
 styleBox.setSelectedId(processor.styleIndex()+1,juce::dontSendNotification);
 presetBox.setSelectedId(processor.presetIndex()+1,juce::dontSendNotification);
}
void BoomAudioProcessorEditor::closeOverlays(){
 overlay.reset();browserPanel.reset();chainPanel.reset();meteringPanel.reset();helpPanel.reset();
}
void BoomAudioProcessorEditor::showBrowser(){closeOverlays();browserPanel=std::make_unique<BrowserPanel>(processor,[this]{closeOverlays();});
 overlay=std::move(browserPanel);addAndMakeVisible(*overlay);resized();overlay->toFront(true);}
void BoomAudioProcessorEditor::showChain(){closeOverlays();chainPanel=std::make_unique<ChainPanel>(processor.state(),[this]{closeOverlays();});
 overlay=std::move(chainPanel);addAndMakeVisible(*overlay);resized();overlay->toFront(true);}
void BoomAudioProcessorEditor::showMeters(){closeOverlays();meteringPanel=std::make_unique<MeterPanel>(processor,[this]{closeOverlays();});
 overlay=std::move(meteringPanel);addAndMakeVisible(*overlay);resized();overlay->toFront(true);}
void BoomAudioProcessorEditor::showHelp(){closeOverlays();helpPanel=std::make_unique<HelpPanel>([this]{closeOverlays();});
 overlay=std::move(helpPanel);addAndMakeVisible(*overlay);resized();overlay->toFront(true);}
