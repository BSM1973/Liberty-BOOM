#include "BoomBrowser.h"
#include "BoomUiTheme.h"
#include "BoomSafeText.h"
#include <algorithm>
#include <array>

namespace {
const auto bg=boom::ui::theme::background();
const auto panel=boom::ui::theme::surface();
const auto burgundy=boom::ui::theme::active();
const auto bronze=boom::ui::theme::copper();
const auto gold=boom::ui::theme::gold();
const auto ivory=boom::ui::theme::ivory();
juce::Font f(float size,bool strong=false){
 return boom::ui::theme::font(size,strong);
}
constexpr const char* categoriesFR[10]={
 "Pop / Commercial","Rock / Alternatif","M\u00E9tal / Heavy","Hip-Hop / Trap",
 "\u00C9lectro / Dance","Jazz / Acoustique","Cin\u00E9ma / Ambient",
 "Vintage analogique","Streaming","Outils mastering"
};
constexpr const char* categoriesEN[10]={
 "Pop / Commercial","Rock / Alternative","Metal / Heavy","Hip-Hop / Trap",
 "Electronic / Dance","Jazz / Acoustic","Cinema / Ambient","Vintage Analog",
 "Streaming","Mastering Tools"
};
juce::String categoryDisplay(int index,bool english){
 if(index<0||index>=10)return {};
 return boom::ui::text::safeUpperText(english?categoriesEN[index]:categoriesFR[index]);
}
}

void BoomBrowser::BrassButtons::drawButtonBackground(
 juce::Graphics& g,juce::Button& b,const juce::Colour&,bool hovered,bool pressed){
 auto r=b.getLocalBounds().toFloat().reduced(1.f);
 const bool enabled=b.isEnabled(),active=enabled&&b.getToggleState();
 const auto top=!enabled?bg:active?burgundy:
                 (hovered?juce::Colour::fromRGB(79,29,37):juce::Colour::fromRGB(55,24,28));
 const auto bottom=pressed?juce::Colour::fromRGB(85,24,33):bg;
 juce::ColourGradient fill(top,r.getTopLeft(),bottom,r.getBottomLeft(),false);
 g.setGradientFill(fill);g.fillRoundedRectangle(r,boom::ui::theme::cornerRadius);
 g.setColour(!enabled?bronze.withAlpha(.25f):
             active?gold:(hovered?gold.withAlpha(.84f):bronze.withAlpha(.70f)));
 g.drawRoundedRectangle(r,boom::ui::theme::cornerRadius,active?1.65f:1.f);
 if(active){
  g.setColour(gold.withAlpha(.86f));
  g.fillRect(r.getX()+boom::ui::theme::inset,r.getBottom()-3.f,
             juce::jmax(0.f,r.getWidth()-2.f*boom::ui::theme::inset),1.5f);
 }
 boom::ui::theme::focusRing(g,r,b.hasKeyboardFocus(true));
}
void BoomBrowser::BrassButtons::drawButtonText(
 juce::Graphics& g,juce::TextButton& b,bool,bool){
 g.setColour(b.isEnabled()?ivory:ivory.withAlpha(.4f));
 const auto height=static_cast<float>(b.getHeight());
 g.setFont(f(juce::jlimit(11.f,15.f,height*.47f),b.getToggleState()));
 g.drawFittedText(boom::ui::text::safeUpperText(b.getButtonText()),b.getLocalBounds().reduced(5,2),
                  juce::Justification::centred,1);
}
void BoomBrowser::styleButton(juce::TextButton& b,juce::String text,
                              std::function<void()> callback){
 b.setLookAndFeel(&buttonsLook);
 b.setWantsKeyboardFocus(true);
 b.setButtonText(boom::ui::text::safeUpperText(text));
 b.setTooltip(b.getButtonText());
 b.setMouseCursor(juce::MouseCursor::PointingHandCursor);
 b.onClick=std::move(callback);
 addAndMakeVisible(b);
}
BoomBrowser::BoomBrowser(BoomAudioProcessor& processor,std::function<void()> closeFunction)
 :proc(processor),onClose(std::move(closeFunction)),categoryModel(*this){
 setOpaque(true);
 setWantsKeyboardFocus(true);
 styleButton(presets,"PRESETS",[this]{setMode(boom::browser::Mode::Presets);});
 styleButton(styles,"STYLES",[this]{setMode(boom::browser::Mode::Styles);});
 styleButton(user,"USER",[this]{setMode(boom::browser::Mode::User);});
 styleButton(close,"X",[this]{if(onClose)onClose();});
 styleButton(onlyFavorites,"FAVORIS",[this]{
  favoritesFilter=!favoritesFilter;category=-1;refresh();});
 styleButton(viewList,"LISTE",[this]{grid=false;refresh();});
 styleButton(viewGrid,"GRILLE",[this]{grid=true;refresh();});
 styleButton(clearFilters,"EFFACER",[this]{
  category=-1;tag=boom::browser::Tag::All;favoritesFilter=false;
  search.clear();refresh();});
 for(int i=0;i<static_cast<int>(tagButtons.size());++i){
  styleButton(tagButtons[static_cast<std::size_t>(i)],
   boom::browser::tagFr[static_cast<std::size_t>(i)],
   [this,i]{tag=static_cast<boom::browser::Tag>(i);refresh();});
 }
 styleButton(load,"CHARGER",[this]{loadSelection();});
 styleButton(favorite,"FAVORI",[this]{toggleFavorite();});
 styleButton(save,"SAUVER",[this]{saveCurrent();});
 styleButton(rename,"RENOMMER",[this]{renameCurrent();});
 styleButton(remove,"SUPPRIMER",[this]{deleteCurrent();});
 styleButton(import,"IMPORTER",[this]{importCurrent();});
 styleButton(exportFile,"EXPORTER",[this]{exportCurrent();});
 styleButton(writeTags,"TAGS",[this]{writeUserTags();});
 for(auto* e:{&search,&nameEdit,&tagsEdit}){
  e->setColour(juce::TextEditor::backgroundColourId,bg);
  e->setColour(juce::TextEditor::textColourId,ivory);
  e->setColour(juce::TextEditor::outlineColourId,bronze);
  e->setColour(juce::TextEditor::focusedOutlineColourId,gold);
  e->setFont(f(boom::ui::theme::body));
  addAndMakeVisible(*e);
 }
 search.setTextToShowWhenEmpty("Rechercher un son, un Style, un tag...",gold.withAlpha(.63f));
 search.onTextChange=[this]{refresh();};
 nameEdit.setTextToShowWhenEmpty("Nom du preset utilisateur...",gold.withAlpha(.6f));
 tagsEdit.setTextToShowWhenEmpty("Tags : chaud, punch, voix...",gold.withAlpha(.6f));
 nameEdit.onReturnKey=[this]{if(mode==boom::browser::Mode::User)renameCurrent();else saveCurrent();};
 tagsEdit.onReturnKey=[this]{writeUserTags();};
 categories.setModel(&categoryModel);
 categories.setWantsKeyboardFocus(true);
 categories.addMouseListener(this,true);
 categories.setColour(juce::ListBox::backgroundColourId,bg);
 categories.setOutlineThickness(0);
 categories.setRowHeight(24);
 categories.setMouseCursor(juce::MouseCursor::PointingHandCursor);
 addAndMakeVisible(categories);
 items.setModel(this);
 items.setWantsKeyboardFocus(true);
 items.addMouseListener(this,true);
 items.setMouseCursor(juce::MouseCursor::PointingHandCursor);
 items.setColour(juce::ListBox::backgroundColourId,bg);
 items.setOutlineThickness(0);
 items.setRowHeight(30);
 addAndMakeVisible(items);
 addAndMakeVisible(sortCombo);
 sortCombo.addItem("Ordre d'usine",1);
 sortCombo.addItem("Nom A-Z",2);
 sortCombo.addItem("Nom Z-A",3);
 sortCombo.setSelectedId(1,juce::dontSendNotification);
 sortCombo.onChange=[this]{refresh();};
 sortCombo.setColour(juce::ComboBox::backgroundColourId,bg);
 sortCombo.setColour(juce::ComboBox::outlineColourId,bronze);
 sortCombo.setColour(juce::ComboBox::textColourId,ivory);
 sortCombo.setColour(juce::ComboBox::arrowColourId,gold);
 addAndMakeVisible(status);
 status.setJustificationType(juce::Justification::centredLeft);
 status.setColour(juce::Label::textColourId,gold);
 status.setFont(f(12.f));
 setStatus("100 presets d'usine  /  40 Styles  /  biblioth\u00E8que utilisateur");
 updateLanguage(false);
 refresh(false);
}
BoomBrowser::~BoomBrowser(){
 items.removeMouseListener(this);
 categories.removeMouseListener(this);
 items.setModel(nullptr);
 categories.setModel(nullptr);
 auto clear=[this](juce::TextButton& b){b.setLookAndFeel(nullptr);};
 for(auto* b:{&presets,&styles,&user,&close,&onlyFavorites,&viewList,&viewGrid,
              &clearFilters,&load,&favorite,&save,&rename,&remove,&import,&exportFile,&writeTags})
  clear(*b);
 for(auto& b:tagButtons)clear(b);
}
void BoomBrowser::setStatus(juce::String msg){
 const auto visible=boom::ui::text::safeUpperText(msg);
 status.setTooltip(visible);
 status.setText(visible,juce::dontSendNotification);
}
void BoomBrowser::setMode(boom::browser::Mode requested){
 mode=requested;category=-1;tag=boom::browser::Tag::All;
 favoritesFilter=false;confirmDelete=false;
 search.clear();
 updateLanguage(english);
 refresh(false);
}
void BoomBrowser::updateLanguage(bool en){
 english=en;
 search.setTextToShowWhenEmpty(
  en?"Search presets, Styles or tags...":"Rechercher par nom, Style ou tag...",
  gold.withAlpha(.7f));
 nameEdit.setTextToShowWhenEmpty(
  en?"User preset name...":"Nom du preset utilisateur...",gold.withAlpha(.7f));
 tagsEdit.setTextToShowWhenEmpty(
  en?"Tags: warm, punch, voice...":"Tags : chaud, punch, voix...",gold.withAlpha(.7f));
 const auto previousOrder=sortCombo.getSelectedId();
 sortCombo.clear(juce::dontSendNotification);
 sortCombo.addItem(en?"Factory order":"Ordre d'usine",1);
 sortCombo.addItem(en?"Name A-Z":"Nom A-Z",2);
 sortCombo.addItem(en?"Name Z-A":"Nom Z-A",3);
 sortCombo.setSelectedId(previousOrder>0?previousOrder:1,juce::dontSendNotification);
 presets.setButtonText(en?"PRESETS":"PRESETS");
 styles.setButtonText(en?"STYLES":"STYLES");
 user.setButtonText(en?"USER":"UTILISATEUR");
 onlyFavorites.setButtonText(en?"FAVORITES":"FAVORIS");
 clearFilters.setButtonText(en?"RESET":"EFFACER");
 load.setButtonText(en?"LOAD":"CHARGER");
 favorite.setButtonText(en?"FAVORITE":"FAVORI");
 save.setButtonText(en?"SAVE":"SAUVER");
 rename.setButtonText(en?"RENAME":"RENOMMER");
 remove.setButtonText(en?"DELETE":"SUPPRIMER");
 import.setButtonText(en?"IMPORT":"IMPORTER");
 exportFile.setButtonText(en?"EXPORT":"EXPORTER");
 writeTags.setButtonText(en?"SET TAGS":"TAGS");
 load.setTooltip(en?"Apply the selected master to the DSP":
                    "Charger le master s\u00E9lectionn\u00E9 dans le moteur audio");
 onlyFavorites.setTooltip(en?"Show starred factory presets or Styles":
                             "Afficher uniquement les favoris");
 viewList.setTooltip(en?"List view":"Affichage en liste");
 viewGrid.setTooltip(en?"Grid view":"Affichage en grille");
 save.setTooltip(en?"Save the current master as a user preset":
                    "Enregistrer le master courant dans la biblioth\u00E8que utilisateur");
 rename.setTooltip(en?"Rename the selected user preset":"Renommer le preset utilisateur");
 remove.setTooltip(en?"Delete the selected user preset":"Supprimer le preset utilisateur");
 import.setTooltip(en?"Import a .boom file":"Importer un fichier .boom");
 exportFile.setTooltip(en?"Export the current user preset":"Exporter le preset utilisateur");
 writeTags.setTooltip(en?"Save personal search tags":"Enregistrer les tags de recherche");
 for(int i=0;i<9;++i)
  tagButtons[static_cast<std::size_t>(i)].setButtonText(
   en?boom::browser::tagEn[static_cast<std::size_t>(i)]
     :boom::browser::tagFr[static_cast<std::size_t>(i)]);
 setStatus(en?"100 factory presets / 40 Styles / user library":
              "100 presets d'usine / 40 Styles / bibliotheque utilisateur");
 categories.repaint();items.repaint();
 syncButtons();repaint();
}
const BoomBrowser::Item* BoomBrowser::current()const{
 return chosen>=0&&chosen<static_cast<int>(rows.size())?
  &rows[static_cast<std::size_t>(chosen)]:nullptr;
}
bool BoomBrowser::currentUser()const{
 const auto* c=current();return c!=nullptr&&c->user;
}
juce::String BoomBrowser::categoryName(const Item& e)const{
 if(e.user)return english?"User":"Utilisateur";
 return categoryDisplay(e.category,english);
}
juce::String BoomBrowser::styleName(const Item& e)const{
 if(e.user)return "USER";
 return e.style>=0&&e.style<40?
  boom::ui::text::safeUpperText(juce::String(boom::styles()[static_cast<std::size_t>(e.style)].name)):juce::String{};
}
juce::String BoomBrowser::tagsText(const Item& e)const{
 if(e.user)return boom::ui::text::safeUpperText(proc.userPresetTags(e.index));
 juce::String result;
 for(int i=1;i<9;++i)
  if((e.tags & boom::browser::bit(static_cast<boom::browser::Tag>(i)))!=0){
   if(result.isNotEmpty())result+=", ";
   result+=english?boom::browser::tagEn[static_cast<std::size_t>(i)]
                  :boom::browser::tagFr[static_cast<std::size_t>(i)];
  }
 return boom::ui::text::safeUpperText(result);
}
void BoomBrowser::syncButtons(){
 presets.setToggleState(mode==boom::browser::Mode::Presets,juce::dontSendNotification);
 styles.setToggleState(mode==boom::browser::Mode::Styles,juce::dontSendNotification);
 user.setToggleState(mode==boom::browser::Mode::User,juce::dontSendNotification);
 onlyFavorites.setToggleState(favoritesFilter,juce::dontSendNotification);
 viewList.setToggleState(!grid,juce::dontSendNotification);
 viewGrid.setToggleState(grid,juce::dontSendNotification);
 for(int i=0;i<9;++i)tagButtons[static_cast<std::size_t>(i)]
  .setToggleState(static_cast<int>(tag)==i,juce::dontSendNotification);
 const bool hasChoice=current()!=nullptr;
 const bool userChoice=currentUser();
 favorite.setEnabled(hasChoice&&!userChoice);
 const bool starred=hasChoice&&!userChoice&&
  (mode==boom::browser::Mode::Styles?
   proc.isStyleFavorite(current()->index):proc.isFavorite(current()->index));
 favorite.setToggleState(starred,juce::dontSendNotification);
 favorite.setButtonText(english?"FAVORITE":"FAVORI");
 load.setEnabled(hasChoice);
 rename.setEnabled(userChoice);
 remove.setEnabled(userChoice);
 exportFile.setEnabled(userChoice);
 writeTags.setEnabled(userChoice);
 onlyFavorites.setEnabled(mode!=boom::browser::Mode::User);
 nameEdit.setVisible(mode==boom::browser::Mode::User);
 tagsEdit.setVisible(mode==boom::browser::Mode::User);
 writeTags.setVisible(mode==boom::browser::Mode::User);
 save.setVisible(mode==boom::browser::Mode::User);
 rename.setVisible(mode==boom::browser::Mode::User);
 remove.setVisible(mode==boom::browser::Mode::User);
 import.setVisible(mode==boom::browser::Mode::User);
 exportFile.setVisible(mode==boom::browser::Mode::User);
 favorite.setVisible(mode!=boom::browser::Mode::User);
 categories.repaint();
}
void BoomBrowser::refresh(bool preserve){
 Item old;
 bool hasOld=preserve&&current()!=nullptr;
 if(hasOld)old=*current();
 rows.clear();
 const auto needle=search.getText().trim();
 const juce::String tagName=english?boom::browser::tagEn[static_cast<std::size_t>(tag)]:
                                   boom::browser::tagFr[static_cast<std::size_t>(tag)];
 if(mode==boom::browser::Mode::User){
  for(int i=0;i<proc.userPresetCount();++i){
   const auto name=proc.userPresetName(i),tags=proc.userPresetTags(i);
   if(needle.isNotEmpty()&&!name.containsIgnoreCase(needle)&&!tags.containsIgnoreCase(needle))continue;
   if(tag!=boom::browser::Tag::All&&!tags.containsIgnoreCase(tagName))continue;
   rows.push_back({true,i,-1,-1,name,0u});
  }
 }else{
  const int count=mode==boom::browser::Mode::Styles?40:100;
  for(int i=0;i<count;++i){
   const auto e=mode==boom::browser::Mode::Styles?boom::browser::style(i):boom::browser::factory(i);
   if(category>=0&&category<10&&e.category!=category)continue;
   if(!boom::browser::hasTag(e.tags,tag))continue;
   const bool starred=mode==boom::browser::Mode::Styles?proc.isStyleFavorite(i):proc.isFavorite(i);
   if((favoritesFilter||category==10)&&!starred)continue;
   const juce::String text(e.name),family=categoryDisplay(e.category,english);
   const juce::String englishFamily(boom::categories[static_cast<std::size_t>(e.category)]);
   const auto matching=needle.isEmpty()||text.containsIgnoreCase(needle)||
     family.containsIgnoreCase(needle)||englishFamily.containsIgnoreCase(needle)||
     juce::String(boom::styles()[static_cast<std::size_t>(e.style)].name).containsIgnoreCase(needle);
   if(!matching){
    bool tagMatch=false;
    for(int t=1;t<9;++t)if((e.tags&boom::browser::bit(static_cast<boom::browser::Tag>(t)))!=0)
     if(juce::String(boom::browser::tagFr[static_cast<std::size_t>(t)]).containsIgnoreCase(needle)||
        juce::String(boom::browser::tagEn[static_cast<std::size_t>(t)]).containsIgnoreCase(needle))tagMatch=true;
    if(!tagMatch)continue;
   }
   rows.push_back({false,i,e.category,e.style,text,e.tags});
  }
 }
 const auto sort=sortCombo.getSelectedId();
 if(sort==2||sort==3)
  std::stable_sort(rows.begin(),rows.end(),[sort](const Item&a,const Item&b){
   const auto cmp=a.name.compareIgnoreCase(b.name);
   return sort==2?cmp<0:cmp>0;
  });
 chosen=rows.empty()?-1:0;
 if(hasOld)
  for(int i=0;i<static_cast<int>(rows.size());++i)
   if(rows[static_cast<std::size_t>(i)].user==old.user&&
      rows[static_cast<std::size_t>(i)].index==old.index){chosen=i;break;}
 const int row=chosen<0?-1:(grid?chosen/2:chosen);
 items.setRowHeight(grid?65:31);
 items.updateContent();
 if(row>=0)items.selectRow(row);
 categories.updateContent();
 categories.repaint();
 if(currentUser()){
  nameEdit.setText(current()->name,juce::dontSendNotification);
  tagsEdit.setText(proc.userPresetTags(current()->index),juce::dontSendNotification);
 }
 syncButtons();
 repaint();
}
int BoomBrowser::CategoryModel::getNumRows(){
 return owner.mode==boom::browser::Mode::User?1:12;
}
void BoomBrowser::CategoryModel::selectedRowsChanged(int row){
 if(row<0)return;
 if(owner.mode==boom::browser::Mode::User)return;
 const int filter=row==0?-1:row==11?10:row-1;
 if(owner.category==filter)return;
 owner.setCategory(filter);
}
void BoomBrowser::setCategory(int value){
 category=value;favoritesFilter=value==10;
 refresh(false);
}
void BoomBrowser::CategoryModel::paintListBoxItem(
 int row,juce::Graphics& g,int w,int h,bool selected){
 if(row<0||row>=getNumRows())return;
 const juce::String name=row==0?(owner.english?"ALL":"TOUS"):row==11?
  (owner.english?"Favorites":"Favoris"):
  categoryDisplay(row-1,owner.english);
 int count=0;
 if(row==0)count=owner.mode==boom::browser::Mode::Styles?40:
                    owner.mode==boom::browser::Mode::User?owner.proc.userPresetCount():100;
 else if(row==11){
  const int total=owner.mode==boom::browser::Mode::Styles?40:100;
  for(int i=0;i<total;++i)if(owner.mode==boom::browser::Mode::Styles?
   owner.proc.isStyleFavorite(i):owner.proc.isFavorite(i))++count;
 }else{
  const int total=owner.mode==boom::browser::Mode::Styles?40:100;
  for(int i=0;i<total;++i)
   if((owner.mode==boom::browser::Mode::Styles?
       boom::browser::style(i).category:boom::browser::factory(i).category)==row-1)++count;
 }
 const bool active=row==(owner.category==-1?0:owner.category==10?11:owner.category+1);
 const bool hovered=owner.hoveredCategory==row;
 g.setColour(active?burgundy:(hovered?juce::Colour::fromRGB(66,28,34):
                             ((row%2)?panel:bg)));
 g.fillRect(0,0,w,h);
 if(active){g.setColour(gold);g.fillRect(0,0,3,h);}
 else if(hovered){g.setColour(bronze);g.fillRect(0,0,2,h);}
 if(selected&&owner.categories.hasKeyboardFocus(true)){
  g.setColour(ivory.withAlpha(.9f));
  g.drawRect(1,1,w-2,h-2,1);
 }
 g.setColour(selected||active?ivory:gold);g.setFont(f(juce::jmin(13.5f,h*.60f),active));
 g.drawFittedText(boom::ui::text::safeUpperText(name),8,0,w-40,h,juce::Justification::centredLeft,1);
 g.setColour(gold.withAlpha(.8f));
 g.drawText(juce::String(count),w-42,0,34,h,juce::Justification::centredRight);
}
int BoomBrowser::getNumRows(){
 return grid?(static_cast<int>(rows.size())+1)/2:static_cast<int>(rows.size());
}
void BoomBrowser::selectedRowsChanged(int row){
 if(row<0||rows.empty())return;
 const int next=grid?row*2:row;
 if(grid&&chosen>=0&&chosen/2==row)return;
 chosen=juce::jlimit(0,static_cast<int>(rows.size())-1,next);
 resetDelete();
 if(currentUser()){
  nameEdit.setText(current()->name,juce::dontSendNotification);
  tagsEdit.setText(proc.userPresetTags(current()->index),juce::dontSendNotification);
 }
 syncButtons();repaint();
}
void BoomBrowser::listBoxItemDoubleClicked(int row,const juce::MouseEvent& e){
 listBoxItemClicked(row,e);
 loadSelection();
}
void BoomBrowser::listBoxItemClicked(int row,const juce::MouseEvent& e){
 if(row<0||rows.empty())return;
 const int column=grid&&e.getEventRelativeTo(&items).position.x>items.getWidth()/2?1:0;
 const int index=grid?row*2+column:row;
 if(index<0||index>=static_cast<int>(rows.size()))return;
 chosen=index;
 if(currentUser()){
  nameEdit.setText(current()->name,juce::dontSendNotification);
  tagsEdit.setText(proc.userPresetTags(current()->index),juce::dontSendNotification);
 }
 resetDelete();syncButtons();repaint();
}
void BoomBrowser::paintListBoxItem(
 int row,juce::Graphics& g,int w,int h,bool){
 if(row<0)return;
 const int cells=grid?2:1;
 for(int c=0;c<cells;++c){
  const int index=grid?row*2+c:row;
  if(index>=static_cast<int>(rows.size()))continue;
  const auto& item=rows[static_cast<std::size_t>(index)];
  const bool active=index==chosen;
  const bool hovered=index==hoveredIndex;
  auto cell=juce::Rectangle<int>(grid?c*(w/2)+3:3,2,grid?w/2-7:w-7,h-4).toFloat();
  g.setColour(active?burgundy:(hovered?juce::Colour::fromRGB(61,28,34):
                                          ((row+c)%2?juce::Colour::fromRGB(37,20,23):panel)));
  g.fillRoundedRectangle(cell,3.f);
  if(active||hovered){
   g.setColour(active?gold:bronze.withAlpha(.9f));
   g.drawRoundedRectangle(cell.reduced(.5f),3.f,active?1.5f:1.f);
  }
  if(active&&items.hasKeyboardFocus(true)){
   g.setColour(ivory.withAlpha(.9f));
   g.drawRoundedRectangle(cell.reduced(2.5f),3.f,1.3f);
  }
  g.setColour(ivory);g.setFont(f(grid?13.5f:14.5f,active));
  const int x=int(cell.getX())+10;
  g.drawFittedText(boom::ui::text::safeUpperText(item.name),x,int(cell.getY())+1,
                   int(cell.getWidth())-(grid?38:175),grid?27:24,
                   juce::Justification::centredLeft,1);
  if(grid){
   g.setColour(gold.withAlpha(.73f));g.setFont(f(11.5f));
   g.drawFittedText(categoryName(item),x,int(cell.getY())+29,
                    int(cell.getWidth())-50,21,juce::Justification::centredLeft,1);
  }else{
   g.setColour(gold.withAlpha(.8f));g.setFont(f(11.5f));
   g.drawFittedText(categoryName(item),x+int(cell.getWidth())*.48f,
                    int(cell.getY())+2,int(cell.getWidth())*.25f,24,
                    juce::Justification::centredLeft,1);
   g.drawFittedText(styleName(item),x+int(cell.getWidth())*.73f,
                    int(cell.getY())+2,int(cell.getWidth())*.21f,24,
                    juce::Justification::centredLeft,1);
  }
  if(!item.user){
   const bool starred=mode==boom::browser::Mode::Styles?
    proc.isStyleFavorite(item.index):proc.isFavorite(item.index);
   if(starred){
    g.setColour(gold);g.setFont(f(10.f,true));
    g.drawText("FAV",int(cell.getRight())-35,
               int(cell.getY())+2,31,24,juce::Justification::centred);
   }
  }
 }
}
