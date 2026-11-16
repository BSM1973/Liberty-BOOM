#include "PluginEditor.h"
#include <cmath>

namespace {
const juce::Colour brass=juce::Colour::fromRGB(198,161,101);
const juce::Colour ivory=juce::Colour::fromRGB(239,222,179);
const juce::Colour charcoal=juce::Colour::fromRGB(30,17,19);
const juce::Colour walnut=juce::Colour::fromRGB(62,19,24);
const juce::Colour amber=juce::Colour::fromRGB(198,100,72);
const juce::Colour deep=juce::Colour::fromRGB(17,9,12);
const juce::Colour copper=juce::Colour::fromRGB(170,79,62);
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

class BrowserPanel final : public OverlayBase, private juce::ListBoxModel {
private:
 struct Row { bool user=false; int index=-1; bool valid=false; };
 class CategoryModel final : public juce::ListBoxModel {
 public:
  explicit CategoryModel(BrowserPanel& p):owner(p){}
  int getNumRows() override { return 13; }
  void paintListBoxItem(int row,juce::Graphics& g,int w,int h,bool selected) override {
   owner.paintCategoryItem(row,g,w,h,selected);
  }
  void selectedRowsChanged(int) override { owner.refresh(); }
 private:
  BrowserPanel& owner;
 };
public:
 explicit BrowserPanel(BoomAudioProcessor& p,std::function<void()> close)
  :OverlayBase("PRESETS D'USINE & UTILISATEUR",std::move(close)),proc(p),categoryModel(*this) {
  english=proc.state().getRawParameterValue("language")->load()>0.5f;
  closeButton.setButtonText("×");
  closeButton.setTooltip(english?"Close browser":"Fermer le navigateur");
  addAndMakeVisible(categoriesList);
  categoriesList.setModel(&categoryModel);
  categoriesList.setRowHeight(28);
  categoriesList.setColour(juce::ListBox::backgroundColourId,deep);
  categoriesList.setOutlineThickness(0);
  addAndMakeVisible(search);
  search.setTextToShowWhenEmpty(english?"Search across 100 factory presets...":"Rechercher parmi les 100 presets...",brass.withAlpha(.65f));
  search.setColour(juce::TextEditor::backgroundColourId,deep);
  search.setColour(juce::TextEditor::textColourId,ivory);
  search.setColour(juce::TextEditor::outlineColourId,copper);
  search.onTextChange=[this]{refresh();};
  addAndMakeVisible(styleFilter);
  styleFilter.addItem(english?"All Styles":"Tous les Styles",1);
  for(int i=0;i<40;++i)styleFilter.addItem(juce::String(boom::styles()[static_cast<size_t>(i)].name),i+2);
  styleFilter.setSelectedId(1,juce::dontSendNotification);
  styleFilter.onChange=[this]{refresh();};
  addAndMakeVisible(list);
  list.setModel(this);
  list.setRowHeight(31);
  list.setOutlineThickness(0);
  list.setColour(juce::ListBox::backgroundColourId,deep);
  list.setColour(juce::ListBox::outlineColourId,copper);
  addAndMakeVisible(userName);
  userName.setTextToShowWhenEmpty(english?"Name for your preset...":"Nom de votre preset utilisateur...",brass.withAlpha(.65f));
  userName.setColour(juce::TextEditor::backgroundColourId,deep);
  userName.setColour(juce::TextEditor::textColourId,ivory);
  userName.setColour(juce::TextEditor::outlineColourId,copper);
  const auto addButton=[this](juce::TextButton& b,const juce::String& text,std::function<void()> fn){
   addAndMakeVisible(b);b.setButtonText(text);b.onClick=std::move(fn);
  };
  addButton(previous,english?"◀ PREV":"◀ PRÉC.",[this]{move(-1);});
  addButton(next,english?"NEXT ▶":"SUIV. ▶",[this]{move(1);});
  addButton(favourite,english?"☆ FAVORITE":"☆ FAVORI",[this]{
   auto r=getSelected();
   if(!r.valid||r.user){setStatus(english?"Select a factory preset.":"Sélectionnez un preset d'usine.");return;}
   proc.toggleFavorite(r.index);
   refresh();
   selectRow(false,r.index);
  });
  addButton(saveUser,english?"SAVE AS...":"ENREGISTRER...",[this]{save();});
  addButton(userActions,english?"USER LIBRARY ▾":"UTILISATEUR ▾",[this]{openActions();});
  addButton(load,english?"▶ LOAD PRESET":"▶ CHARGER LE PRESET",[this]{loadSelected();});
  addAndMakeVisible(status);
  status.setColour(juce::Label::textColourId,brass);
  status.setFont(font(11));
  status.setText(english?"Double-click a preset to load it. Factory sounds remain unchanged."
                        :"Double-cliquez sur un preset pour le charger. Les 100 presets d'usine restent intacts.",
                 juce::dontSendNotification);
  categoriesList.selectRow(0);
  refresh();
 }
 ~BrowserPanel()override {
  list.setModel(nullptr);
  categoriesList.setModel(nullptr);
 }
 int getNumRows() override { return static_cast<int>(shown.size()); }
 void selectedRowsChanged(int) override {
  deleteArmed=false;
  const auto r=getSelected();
  if(r.valid && r.user)userName.setText(proc.userPresetName(r.index),juce::dontSendNotification);
  repaint();
 }
 void paintListBoxItem(int row,juce::Graphics& g,int w,int h,bool selected)override {
  if(row<0||row>=static_cast<int>(shown.size()))return;
  const auto entry=shown[static_cast<size_t>(row)];
  const bool loaded=!entry.user&&proc.presetIndex()==entry.index;
  g.setColour(selected?juce::Colour::fromRGB(94,27,35):
    (row%2==0?juce::Colour::fromRGB(27,16,18):juce::Colour::fromRGB(35,20,22)));
  g.fillRect(0,0,w,h);
  if(selected){g.setColour(copper);g.fillRect(0,0,3,h);}
  g.setColour(copper.withAlpha(selected?.90f:.32f));g.drawHorizontalLine(h-1,0.f,float(w));
  juce::String name,cat,style;
  bool star=false;
  if(entry.user){
   name=proc.userPresetName(entry.index);
   cat=english?"USER":"UTILISATEUR";style="—";
  }else{
   const auto p=boom::makePreset(entry.index);
   name=juce::String(p.name);
   cat=juce::String(boom::categories[static_cast<size_t>(p.category)]);
   style=juce::String(boom::styles()[static_cast<size_t>(p.style)].name);
   star=proc.isFavorite(entry.index);
  }
  const int starW=40;
  const int nameW=juce::roundToInt((w-starW)*.46f);
  const int catW=juce::roundToInt((w-starW)*.28f);
  g.setColour(selected?ivory:ivory.withAlpha(.92f));g.setFont(font(13,selected));
  g.drawFittedText((loaded?"●  ":"   ")+name,8,1,nameW-10,h-2,juce::Justification::centredLeft,1);
  g.setColour(brass.withAlpha(.95f));g.setFont(font(11));
  g.drawFittedText(cat,nameW,1,catW-8,h-2,juce::Justification::centredLeft,1);
  g.drawFittedText(style,nameW+catW,1,w-starW-nameW-catW-6,h-2,juce::Justification::centredLeft,1);
  g.setColour(star?ivory:brass.withAlpha(.6f));g.setFont(font(18));
  g.drawText(entry.user?"◆":star?"★":"☆",w-starW,0,starW,h,juce::Justification::centred);
 }
 void listBoxItemClicked(int row,const juce::MouseEvent& event)override {
  if(row<0||row>=getNumRows())return;
  // The rightmost cell is an interactive favourite control for factory presets.
  const auto r=shown[static_cast<size_t>(row)];
  if(event.x>list.getWidth()-47 && !r.user){
   proc.toggleFavorite(r.index);
   refresh();selectRow(false,r.index);
  }
 }
 void listBoxItemDoubleClicked(int row,const juce::MouseEvent&)override {
  if(row>=0&&row<getNumRows()){list.selectRow(row);loadSelected();}
 }
 void paint(juce::Graphics& g)override {
  auto b=getLocalBounds().toFloat().reduced(1);
  g.setColour(juce::Colours::black.withAlpha(.8f));g.fillRoundedRectangle(b.translated(2.f,3.f),8.f);
  juce::ColourGradient panel(juce::Colour::fromRGB(53,17,22),b.getTopLeft(),
                             juce::Colour::fromRGB(19,11,13),b.getBottomRight(),false);
  g.setGradientFill(panel);g.fillRoundedRectangle(b,8.f);
  g.setColour(copper);g.drawRoundedRectangle(b.reduced(1.f),8.f,1.5f);
  juce::ColourGradient header(juce::Colour::fromRGB(104,36,41),0.f,0.f,
                              juce::Colour::fromRGB(30,15,18),0.f,47.f,false);
  g.setGradientFill(header);g.fillRoundedRectangle(b.removeFromTop(47.f),7.f);
  g.setColour(copper.withAlpha(.75f));g.drawHorizontalLine(47,8.f,float(getWidth()-8));
  g.setColour(ivory);g.setFont(font(16,true));
  g.drawText(english?"FACTORY & USER PRESETS":"PRESETS D'USINE & UTILISATEUR",
             20,9,getWidth()-300,30,juce::Justification::centredLeft);
  g.setColour(brass);g.setFont(font(11,true));
  g.drawText("100 PRESETS  ·  40 STYLES",getWidth()-310,12,240,23,juce::Justification::centredRight);
  const auto a=areas();
  auto frame=[&](juce::Rectangle<int> r){
   g.setColour(juce::Colour::fromRGB(19,12,14));g.fillRoundedRectangle(r.toFloat(),4.f);
   g.setColour(copper.withAlpha(.52f));g.drawRoundedRectangle(r.toFloat().reduced(.5f),4.f,1.f);
  };
  frame({a.sidebarX,53,a.sidebarW,getHeight()-110});
  frame({a.mainX,89,a.mainW,getHeight()-151});
  frame({a.detailX,53,a.detailW,getHeight()-110});
  g.setColour(brass);g.setFont(font(10,true));
  g.drawText(english?"CATEGORIES":"CATÉGORIES",a.sidebarX+12,61,a.sidebarW-24,22,juce::Justification::centredLeft);
  g.drawText(english?"PRESET NAME":"NOM DU PRESET",a.mainX+11,93,a.mainW*46/100-10,20,juce::Justification::centredLeft);
  g.drawText(english?"CATEGORY":"CATÉGORIE",a.mainX+a.mainW*46/100,93,a.mainW*28/100,20,juce::Justification::centredLeft);
  g.drawText("STYLE",a.mainX+a.mainW*74/100,93,a.mainW*20/100,20,juce::Justification::centredLeft);
  g.drawText("☆",a.mainX+a.mainW-35,93,25,20,juce::Justification::centred);
  paintDetails(g,a);
 }
 void resized()override {
  auto a=areas();
  closeButton.setBounds(getWidth()-45,8,30,30);
  categoriesList.setBounds(a.sidebarX+7,87,a.sidebarW-14,getHeight()-154);
  search.setBounds(a.mainX,55,a.mainW-189,30);
  styleFilter.setBounds(a.mainX+a.mainW-183,55,183,30);
  list.setBounds(a.mainX+2,114,a.mainW-4,getHeight()-175);
  userName.setBounds(a.detailX+12,getHeight()-102,a.detailW-24,27);
  const int y=getHeight()-53;
  previous.setBounds(a.mainX,y,78,29);
  next.setBounds(a.mainX+84,y,78,29);
  favourite.setBounds(a.mainX+169,y,119,29);
  saveUser.setBounds(a.mainX+295,y,155,29);
  userActions.setBounds(a.mainX+457,y,150,29);
  load.setBounds(a.detailX+12,y,a.detailW-24,30);
  status.setBounds(18,getHeight()-22,getWidth()-36,19);
 }
private:
 struct Areas {
  int sidebarX,sidebarW,mainX,mainW,detailX,detailW;
 };
 Areas areas()const{
  const int pad=14,sidebar=205,detail=260,gap=12;
  const int mainX=pad+sidebar+gap,mainW=getWidth()-2*pad-sidebar-detail-2*gap;
  return {pad,sidebar,mainX,mainW,mainX+mainW+gap,detail};
 }
 BoomAudioProcessor& proc;
 CategoryModel categoryModel;
 juce::ListBox categoriesList,list;
 juce::TextEditor search,userName;
 juce::ComboBox styleFilter;
 juce::TextButton previous,next,favourite,saveUser,userActions,load;
 juce::Label status;
 std::unique_ptr<juce::FileChooser> fileChooser;
 std::vector<Row> shown;
 bool english=false,deleteArmed=false;
 int deleteIndex=-1;
 Row getSelected()const {
  const int r=list.getSelectedRow();
  return r>=0&&r<static_cast<int>(shown.size())?shown[static_cast<size_t>(r)]:Row{};
 }
 void selectRow(bool user,int index){
  for(int i=0;i<static_cast<int>(shown.size());++i){
   auto row=shown[static_cast<size_t>(i)];
   if(row.user==user&&row.index==index){list.selectRow(i);return;}
  }
 }
 void setStatus(const juce::String& text){
  status.setText(text,juce::dontSendNotification);
 }
 void refresh(){
  const auto old=getSelected();
  shown.clear();
  const int cat=categoriesList.getSelectedRow();
  const int style=styleFilter.getSelectedId();
  const auto query=search.getText().trim();
  if(cat!=12){
   for(int i=0;i<100;++i){
    const auto p=boom::makePreset(i);
    if(cat>=1&&cat<=10&&p.category!=cat-1)continue;
    if(cat==11&&!proc.isFavorite(i))continue;
    if(style>1&&p.style!=style-2)continue;
    if(!juce::String(p.name).containsIgnoreCase(query)
        && !juce::String(boom::styles()[static_cast<size_t>(p.style)].name).containsIgnoreCase(query)
        && !juce::String(boom::categories[static_cast<size_t>(p.category)]).containsIgnoreCase(query))continue;
    shown.push_back({false,i,true});
   }
  }
  if((cat==0||cat==12)&&style<=1){
   for(int i=0;i<proc.userPresetCount();++i){
    if(proc.userPresetName(i).containsIgnoreCase(query))shown.push_back({true,i,true});
   }
  }
  list.updateContent();
  list.deselectAllRows();
  if(old.valid)selectRow(old.user,old.index);
  if(list.getSelectedRow()<0&&!shown.empty())list.selectRow(0);
  categoriesList.repaint();
  repaint();
 }
 void paintCategoryItem(int row,juce::Graphics& g,int w,int h,bool selected){
  if(row<0||row>=13)return;
  juce::String name;
  int count=0;
  if(row==0){name=english?"ALL PRESETS":"TOUS LES PRESETS";count=100+proc.userPresetCount();}
  else if(row<=10){name=juce::String(boom::categories[static_cast<size_t>(row-1)]);count=10;}
  else if(row==11){
   name=english?"★ Favorites":"★ Favoris";
   for(int i=0;i<100;++i)if(proc.isFavorite(i))++count;
  }else{name=english?"◆ User presets":"◆ Mes presets";count=proc.userPresetCount();}
  g.setColour(selected?juce::Colour::fromRGB(99,26,37):deep.withAlpha(row%2?0.78f:.95f));
  g.fillRect(0,0,w,h);
  if(selected){g.setColour(copper);g.fillRect(0,0,3,h);}
  g.setColour(selected?ivory:brass);g.setFont(font(11,selected));
  g.drawFittedText(name,8,0,w-42,h,juce::Justification::centredLeft,1);
  g.setColour(selected?ivory:brass.withAlpha(.65f));g.setFont(font(10));
  g.drawText(juce::String(count),w-38,0,31,h,juce::Justification::centredRight);
 }
 void paintDetails(juce::Graphics& g,const Areas& a){
  const auto r=getSelected();
  const int x=a.detailX+14,y=62,w=a.detailW-28;
  g.setColour(brass);g.setFont(font(11,true));
  g.drawText(english?"MASTERING PROFILE":"PROFIL DE MASTERING",x,y,w,17,juce::Justification::centredLeft);
  if(!r.valid){
   g.setColour(ivory.withAlpha(.8f));g.setFont(font(13));
   g.drawFittedText(english?"Choose a preset to preview its settings."
                :"Sélectionnez un preset pour découvrir ses réglages.",
                x,y+29,w,62,juce::Justification::topLeft,3);
   return;
  }
  juce::String title,sub;
  if(r.user){
   title=proc.userPresetName(r.index);
   sub=english?"User preset · saved in DAW project":"Preset utilisateur · enregistré dans le projet DAW";
   g.setColour(ivory);g.setFont(font(17,true));
   g.drawFittedText(title,x,y+22,w,26,juce::Justification::centredLeft,1);
   g.setColour(brass);g.setFont(font(11));g.drawFittedText(sub,x,y+51,w,37,juce::Justification::topLeft,2);
   g.drawFittedText(english?"Load, rename, export or delete via USER LIBRARY."
                :"Charger, renommer, exporter ou supprimer via UTILISATEUR.",
                x,y+94,w,48,juce::Justification::topLeft,3);
   return;
  }
  const auto p=boom::makePreset(r.index);
  title=juce::String(p.name);
  sub=juce::String(boom::categories[static_cast<size_t>(p.category)])
      +"  |  "+juce::String(boom::styles()[static_cast<size_t>(p.style)].name);
  g.setColour(ivory);g.setFont(font(17,true));
  g.drawFittedText(title,x,y+22,w,27,juce::Justification::centredLeft,1);
  g.setColour(brass);g.setFont(font(10));
  g.drawFittedText(sub,x,y+51,w,22,juce::Justification::centredLeft,1);
  g.setColour(copper.withAlpha(.55f));g.drawLine(float(x),float(y+78),float(x+w),float(y+78),1.f);
  const auto colorName=p.params.color==0?"CLEAN":p.params.color==1?"TUBE":"TAPE";
  const juce::String names[]={
   english?"INPUT":"ENTRÉE",english?"PUSH":"PUSH",
   english?"COLOR":"COULEUR",english?"GLUE":"GLUE",
   english?"CEILING":"PLAFOND",english?"STYLE":"STYLE"
  };
  const juce::String values[]={
   juce::String(p.params.inputDb,1)+" dB",juce::String(p.params.push,0)+" %",
   juce::String(colorName),juce::String(p.params.compRatio,2)+" : 1",
   juce::String(p.params.ceilingDb,1)+" dBFS",
   juce::String(boom::styles()[static_cast<size_t>(p.style)].name)
  };
  for(int i=0;i<6;++i){
   const int yy=y+84+i*17;
   g.setColour(brass);g.setFont(font(10));g.drawText(names[i],x,yy,78,17,juce::Justification::centredLeft);
   g.setColour(ivory);g.setFont(font(10,true));
   g.drawFittedText(values[i],x+78,yy,w-78,17,juce::Justification::centredRight,1);
  }
  g.setColour(brass.withAlpha(.85f));g.setFont(font(10));
  g.drawFittedText(english?"Full audio-chain recall · double-click to load."
               :"Rappel de toute la chaîne DSP · double-clic pour charger.",
               x,getHeight()-121,w,20,juce::Justification::centredLeft,1);
 }
 void move(int offset){
  if(shown.empty())return;
  list.selectRow(juce::jlimit(0,getNumRows()-1,juce::jmax(0,list.getSelectedRow())+offset));
 }
 void loadSelected(){
  const auto r=getSelected();
  if(!r.valid){setStatus(english?"No preset selected.":"Aucun preset sélectionné.");return;}
  const bool ok=r.user?proc.loadUserPreset(r.index):(proc.loadFactoryPreset(r.index),true);
  setStatus(ok?(english?"Preset loaded — audio parameters updated.":"Preset chargé — tous les paramètres audio sont actualisés.")
              :(english?"Unable to load preset.":"Chargement impossible."));
  repaint();
 }
 void save(){
  const int idx=proc.saveUserPreset(userName.getText());
  setStatus(idx>=0?(english?"Saved to DAW project. Export .boom to share.":"Enregistré dans le projet DAW. Exportez le .boom pour le partager.")
     :(english?"Enter a name (up to 512 user presets).":"Saisissez un nom (512 presets utilisateur maximum)."));
  if(idx>=0){search.clear();styleFilter.setSelectedId(1,juce::dontSendNotification);
   categoriesList.selectRow(12);refresh();selectRow(true,idx);}
 }
 void openActions(){
  juce::PopupMenu menu;
  menu.addItem(1,english?"Save current settings":"Enregistrer le réglage actuel");
  menu.addItem(2,english?"Rename selected user preset":"Renommer le preset utilisateur");
  menu.addItem(3,english?"Delete selected user preset":"Supprimer le preset utilisateur");
  menu.addSeparator();
  menu.addItem(4,english?"Import .boom preset":"Importer un preset .boom");
  menu.addItem(5,english?"Export selected .boom preset":"Exporter le preset .boom");
  juce::Component::SafePointer<BrowserPanel> safe(this);
  menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(userActions),
      [safe](int action){if(safe!=nullptr)safe->handleAction(action);});
 }
 void handleAction(int action){
  if(action==1){save();return;}
  const auto r=getSelected();
  if(action==4){openImport();return;}
  if(!r.valid||!r.user){
   setStatus(english?"Select a user preset first.":"Sélectionnez d'abord un preset utilisateur.");return;
  }
  if(action==2){
   const bool ok=proc.renameUserPreset(r.index,userName.getText());
   setStatus(ok?(english?"Preset renamed.":"Preset renommé."):(english?"Name required.":"Nom requis."));
   refresh();return;
  }
  if(action==3){
   if(!deleteArmed||deleteIndex!=r.index){
    deleteArmed=true;deleteIndex=r.index;
    setStatus(english?"Select Delete a second time to confirm.":"Sélectionnez Supprimer une deuxième fois pour confirmer.");return;
   }
   const bool ok=proc.deleteUserPreset(r.index);
   deleteArmed=false;deleteIndex=-1;
   setStatus(ok?(english?"Preset deleted.":"Preset supprimé."):(english?"Unable to delete.":"Suppression impossible."));
   refresh();return;
  }
  if(action==5)openExport();
 }
 void openImport(){
  fileChooser=std::make_unique<juce::FileChooser>(english?"Import BOOM preset":"Importer un preset BOOM",juce::File{},"*.boom");
  juce::Component::SafePointer<BrowserPanel> safe(this);
  fileChooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
   [safe](const juce::FileChooser& chooser){
    if(safe==nullptr)return;
    auto file=chooser.getResult();if(file==juce::File{})return;
    const bool ok=safe->proc.importUserPreset(file);
    safe->setStatus(ok?(safe->english?"Preset imported.":"Preset importé."):(safe->english?"Invalid file or full library.":"Fichier invalide ou bibliothèque pleine."));
    if(ok){safe->search.clear();safe->styleFilter.setSelectedId(1,juce::dontSendNotification);
      safe->categoriesList.selectRow(12);safe->refresh();safe->selectRow(true,safe->proc.userPresetCount()-1);}
   });
 }
 void openExport(){
  const auto r=getSelected();if(!r.valid||!r.user)return;
  const auto suggested=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
    .getChildFile(juce::File::createLegalFileName(proc.userPresetName(r.index))+".boom");
  fileChooser=std::make_unique<juce::FileChooser>(english?"Export BOOM preset":"Exporter un preset BOOM",suggested,"*.boom");
  juce::Component::SafePointer<BrowserPanel> safe(this);
  const int index=r.index;
  fileChooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|
    juce::FileBrowserComponent::warnAboutOverwriting,
    [safe,index](const juce::FileChooser& chooser){
     if(safe==nullptr)return;
     auto file=chooser.getResult();if(file==juce::File{})return;
     file=file.withFileExtension(".boom");
     const bool ok=safe->proc.exportUserPreset(index,file);
     safe->setStatus(ok?(safe->english?"Preset exported.":"Preset exporté."):(safe->english?"Export failed.":"Échec de l'export."));
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
 const auto center=juce::Point<float>(face.getCentreX(),face.getBottom()-10);
 const float rad=face.getWidth()*.42f;
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
  overlay->setBounds(isPresetBrowser?rect(24,124,1392,292):rect(190,435,1060,432));
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
