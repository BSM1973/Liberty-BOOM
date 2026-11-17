#include "BoomBrowser.h"
#include <algorithm>
#include <array>

namespace {
const auto bg=juce::Colour::fromRGB(23,10,13);
const auto panel=juce::Colour::fromRGB(33,16,20);
const auto burgundy=juce::Colour::fromRGB(108,26,38);
const auto bronze=juce::Colour::fromRGB(174,89,70);
const auto gold=juce::Colour::fromRGB(224,184,119);
const auto ivory=juce::Colour::fromRGB(244,224,187);
juce::Font f(float size,bool strong=false){
 return juce::Font(juce::FontOptions(size,strong?juce::Font::bold:juce::Font::plain));
}
constexpr float designWidth=1550.f,designHeight=345.f;
}

void BoomBrowser::BrassButtons::drawButtonBackground(
 juce::Graphics& g,juce::Button& b,const juce::Colour&,bool hovered,bool pressed){
 auto r=b.getLocalBounds().toFloat().reduced(.5f);
 const bool active=b.getToggleState();
 juce::ColourGradient fill(active?burgundy:juce::Colour::fromRGB(55,24,28),
                           r.getTopLeft(),pressed?bg:(hovered?burgundy:bg),r.getBottomLeft(),false);
 g.setGradientFill(fill);g.fillRoundedRectangle(r,4.f);
 g.setColour(active?gold:(hovered?bronze:bronze.withAlpha(.65f)));
 g.drawRoundedRectangle(r,4.f,active?1.6f:1.f);
 if(active){g.setColour(gold.withAlpha(.85f));g.fillRect(r.getX()+5.f,r.getBottom()-3.f,r.getWidth()-10.f,1.5f);}
}
void BoomBrowser::BrassButtons::drawButtonText(
 juce::Graphics& g,juce::TextButton& b,bool,bool){
 g.setColour(b.isEnabled()?ivory:ivory.withAlpha(.4f));
 const auto height=static_cast<float>(b.getHeight());
 g.setFont(f(juce::jlimit(10.f,15.f,height*.45f),b.getToggleState()));
 g.drawFittedText(b.getButtonText(),b.getLocalBounds().reduced(5,2),
                  juce::Justification::centred,1);
}
void BoomBrowser::styleButton(juce::TextButton& b,juce::String text,
                              std::function<void()> callback){
 b.setLookAndFeel(&buttonsLook);
 b.setButtonText(std::move(text));
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
 styleButton(close,"✕",[this]{if(onClose)onClose();});
 styleButton(onlyFavorites,"★ FAVORIS",[this]{
  favoritesFilter=!favoritesFilter;category=-1;refresh();});
 styleButton(viewList,"☷",[this]{grid=false;refresh();});
 styleButton(viewGrid,"▦",[this]{grid=true;refresh();});
 styleButton(clearFilters,"EFFACER",[this]{
  category=-1;tag=boom::browser::Tag::All;favoritesFilter=false;
  search.clear();refresh();});
 for(int i=0;i<static_cast<int>(tagButtons.size());++i){
  styleButton(tagButtons[static_cast<std::size_t>(i)],
   boom::browser::tagFr[static_cast<std::size_t>(i)],
   [this,i]{tag=static_cast<boom::browser::Tag>(i);refresh();});
 }
 styleButton(load,"▶ CHARGER",[this]{loadSelection();});
 styleButton(favorite,"☆ FAVORI",[this]{toggleFavorite();});
 styleButton(save,"SAUVER",[this]{saveCurrent();});
 styleButton(rename,"RENOMMER",[this]{renameCurrent();});
 styleButton(remove,"SUPPRIMER",[this]{deleteCurrent();});
 styleButton(import,"IMPORTER",[this]{importCurrent();});
 styleButton(exportFile,"EXPORTER",[this]{exportCurrent();});
 styleButton(writeTags,"TAGS ✓",[this]{writeUserTags();});
 for(auto* e:{&search,&nameEdit,&tagsEdit}){
  e->setColour(juce::TextEditor::backgroundColourId,bg);
  e->setColour(juce::TextEditor::textColourId,ivory);
  e->setColour(juce::TextEditor::outlineColourId,bronze);
  e->setColour(juce::TextEditor::focusedOutlineColourId,gold);
  e->setFont(f(13.f));
  addAndMakeVisible(*e);
 }
 search.setTextToShowWhenEmpty("Rechercher un son, un Style, un tag...",gold.withAlpha(.63f));
 search.onTextChange=[this]{refresh();};
 nameEdit.setTextToShowWhenEmpty("Nom du preset utilisateur...",gold.withAlpha(.6f));
 tagsEdit.setTextToShowWhenEmpty("Tags : chaud, punch, voix...",gold.withAlpha(.6f));
 nameEdit.onReturnKey=[this]{if(mode==boom::browser::Mode::User)renameCurrent();else saveCurrent();};
 tagsEdit.onReturnKey=[this]{writeUserTags();};
 categories.setModel(&categoryModel);
 categories.setColour(juce::ListBox::backgroundColourId,bg);
 categories.setOutlineThickness(0);
 categories.setRowHeight(24);
 categories.setMouseCursor(juce::MouseCursor::PointingHandCursor);
 addAndMakeVisible(categories);
 items.setModel(this);
 items.setColour(juce::ListBox::backgroundColourId,bg);
 items.setOutlineThickness(0);
 items.setRowHeight(30);
 addAndMakeVisible(items);
 addAndMakeVisible(sortCombo);
 sortCombo.addItem("Ordre d'usine",1);
 sortCombo.addItem("Nom A–Z",2);
 sortCombo.addItem("Nom Z–A",3);
 sortCombo.setSelectedId(1,juce::dontSendNotification);
 sortCombo.onChange=[this]{refresh();};
 sortCombo.setColour(juce::ComboBox::backgroundColourId,bg);
 sortCombo.setColour(juce::ComboBox::outlineColourId,bronze);
 sortCombo.setColour(juce::ComboBox::textColourId,ivory);
 sortCombo.setColour(juce::ComboBox::arrowColourId,gold);
 addAndMakeVisible(status);
 status.setJustificationType(juce::Justification::centredLeft);
 status.setColour(juce::Label::textColourId,gold);
 status.setFont(f(11.f));
 setStatus("100 presets d'usine • 40 Styles • bibliothèque utilisateur");
 updateLanguage(false);
 refresh(false);
}
BoomBrowser::~BoomBrowser(){
 items.setModel(nullptr);
 categories.setModel(nullptr);
 auto clear=[this](juce::TextButton& b){b.setLookAndFeel(nullptr);};
 for(auto* b:{&presets,&styles,&user,&close,&onlyFavorites,&viewList,&viewGrid,
              &clearFilters,&load,&favorite,&save,&rename,&remove,&import,&exportFile,&writeTags})
  clear(*b);
 for(auto& b:tagButtons)clear(b);
}
void BoomBrowser::setStatus(juce::String msg){
 status.setText(std::move(msg),juce::dontSendNotification);
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
 presets.setButtonText(en?"PRESETS":"PRESETS");
 styles.setButtonText(en?"STYLES":"STYLES");
 user.setButtonText(en?"USER":"UTILISATEUR");
 onlyFavorites.setButtonText(en?"★ FAVORITES":"★ FAVORIS");
 clearFilters.setButtonText(en?"RESET":"EFFACER");
 load.setButtonText(en?"▶ LOAD":"▶ CHARGER");
 favorite.setButtonText(en?"☆ FAVORITE":"☆ FAVORI");
 save.setButtonText(en?"SAVE":"SAUVER");
 rename.setButtonText(en?"RENAME":"RENOMMER");
 remove.setButtonText(en?"DELETE":"SUPPRIMER");
 import.setButtonText(en?"IMPORT":"IMPORTER");
 exportFile.setButtonText(en?"EXPORT":"EXPORTER");
 writeTags.setButtonText(en?"SET TAGS":"TAGS ✓");
 for(int i=0;i<9;++i)
  tagButtons[static_cast<std::size_t>(i)].setButtonText(
   en?boom::browser::tagEn[static_cast<std::size_t>(i)]
     :boom::browser::tagFr[static_cast<std::size_t>(i)]);
 syncButtons();
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
 return e.category>=0&&e.category<10?
  juce::String(boom::categories[static_cast<std::size_t>(e.category)]):juce::String{};
}
juce::String BoomBrowser::styleName(const Item& e)const{
 if(e.user)return "USER";
 return e.style>=0&&e.style<40?
  juce::String(boom::styles()[static_cast<std::size_t>(e.style)].name):juce::String{};
}
juce::String BoomBrowser::tagsText(const Item& e)const{
 if(e.user)return proc.userPresetTags(e.index);
 juce::String result;
 for(int i=1;i<9;++i)
  if((e.tags & boom::browser::bit(static_cast<boom::browser::Tag>(i)))!=0){
   if(result.isNotEmpty())result+=", ";
   result+=english?boom::browser::tagEn[static_cast<std::size_t>(i)]
                  :boom::browser::tagFr[static_cast<std::size_t>(i)];
  }
 return result;
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
 favorite.setButtonText(hasChoice&&!userChoice&&
  ((mode==boom::browser::Mode::Styles)?
   proc.isStyleFavorite(current()->index):proc.isFavorite(current()->index))?
   "★ FAVORI":"☆ FAVORI");
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
   const juce::String text(e.name),family=juce::String(boom::categories[static_cast<std::size_t>(e.category)]);
   const auto matching=needle.isEmpty()||text.containsIgnoreCase(needle)||
     family.containsIgnoreCase(needle)||
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
  (owner.english?"★ Favorites":"★ Favoris"):
  juce::String(boom::browser::categoryShort[static_cast<std::size_t>(row-1)]);
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
 g.setColour(active?burgundy:((row%2)?panel:bg));g.fillRect(0,0,w,h);
 if(active){g.setColour(gold);g.fillRect(0,0,3,h);}
 g.setColour(selected||active?ivory:gold);g.setFont(f(juce::jmin(13.f,h*.56f),active));
 g.drawFittedText(name,8,0,w-40,h,juce::Justification::centredLeft,1);
 g.setColour(gold.withAlpha(.8f));
 g.drawText(juce::String(count),w-42,0,34,h,juce::Justification::centredRight);
}
int BoomBrowser::getNumRows(){
 return grid?(static_cast<int>(rows.size())+1)/2:static_cast<int>(rows.size());
}
void BoomBrowser::selectedRowsChanged(int row){
 if(row<0||rows.empty())return;
 chosen=juce::jlimit(0,static_cast<int>(rows.size())-1,grid?row*2:row);
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
  auto cell=juce::Rectangle<int>(grid?c*(w/2)+3:3,2,grid?w/2-7:w-7,h-4).toFloat();
  g.setColour(active?burgundy:((row+c)%2?juce::Colour::fromRGB(37,20,23):panel));
  g.fillRoundedRectangle(cell,3.f);
  if(active){g.setColour(gold);g.drawRoundedRectangle(cell.reduced(.5f),3.f,1.2f);}
  g.setColour(ivory);g.setFont(f(grid?13.f:14.f,active));
  const int x=int(cell.getX())+10;
  g.drawFittedText(item.name,x,int(cell.getY())+1,
                   int(cell.getWidth())-(grid?38:175),grid?27:24,
                   juce::Justification::centredLeft,1);
  if(grid){
   g.setColour(gold.withAlpha(.73f));g.setFont(f(10.f));
   g.drawFittedText(categoryName(item),x,int(cell.getY())+29,
                    int(cell.getWidth())-50,21,juce::Justification::centredLeft,1);
  }else{
   g.setColour(gold.withAlpha(.8f));g.setFont(f(11.f));
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
   g.setColour(starred?gold:gold.withAlpha(.38f));g.setFont(f(17.f));
   g.drawText(starred?"★":"☆",int(cell.getRight())-32,
              int(cell.getY())+2,28,24,juce::Justification::centred);
  }
 }
}
