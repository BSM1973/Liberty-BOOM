#include "BoomBrowser.h"
#include "BoomUiTheme.h"
#include <algorithm>

namespace {
const auto gold=boom::ui::theme::gold();
const auto ivory=boom::ui::theme::ivory();
const auto bronze=boom::ui::theme::copper();
const auto dark=boom::ui::theme::background();
const auto burgundy=boom::ui::theme::active();
juce::Font font(float size,bool strong=false){
 return boom::ui::theme::font(size,strong);
}
}
void BoomBrowser::loadSelection(){
 const auto* e=current();
 if(e==nullptr){setStatus("Aucun \u00E9l\u00E9ment s\u00E9lectionn\u00E9");return;}
 bool ok=true;
 if(e->user)ok=proc.loadUserPreset(e->index);
 else if(mode==boom::browser::Mode::Styles)proc.loadStyle(e->index);
 else proc.loadFactoryPreset(e->index);
 setStatus(ok?e->name+" : r\u00E9glages charg\u00E9s":"Chargement impossible");
 repaint();
}
void BoomBrowser::toggleFavorite(){
 const auto* e=current();
 if(e==nullptr||e->user)return;
 if(mode==boom::browser::Mode::Styles)proc.toggleStyleFavorite(e->index);
 else proc.toggleFavorite(e->index);
 setStatus("Favoris mis \u00E0 jour");
 refresh();
}
void BoomBrowser::saveCurrent(){
 const auto name=nameEdit.getText().trim();
 if(name.isEmpty()){setStatus("Saisis un nom pour sauvegarder le master.");return;}
 const int saved=proc.saveUserPreset(name);
 if(saved<0){setStatus("Biblioth\u00E8que pleine ou nom invalide.");return;}
 proc.setUserPresetTags(saved,tagsEdit.getText());
 mode=boom::browser::Mode::User;category=-1;tag=boom::browser::Tag::All;
 favoritesFilter=false;
 search.clear();
 refresh(false);
 for(int i=0;i<static_cast<int>(rows.size());++i)
  if(rows[static_cast<std::size_t>(i)].user&&rows[static_cast<std::size_t>(i)].index==saved){
   chosen=i;items.selectRow(grid?i/2:i);break;
  }
 setStatus("Preset utilisateur sauvegard\u00E9 dans le projet DAW");
 syncButtons();repaint();
}
void BoomBrowser::renameCurrent(){
 if(!currentUser()){setStatus("Choisis un preset UTILISATEUR.");return;}
 const auto newName=nameEdit.getText().trim();
 if(newName.isEmpty()){setStatus("Le nom ne peut pas \u00EAtre vide.");return;}
 if(proc.renameUserPreset(current()->index,newName)){
  setStatus("Preset renomm\u00E9");refresh();
 }else setStatus("Renommage impossible");
}
void BoomBrowser::writeUserTags(){
 if(!currentUser()){setStatus("S\u00E9lectionne un preset utilisateur.");return;}
 const bool ok=proc.setUserPresetTags(current()->index,tagsEdit.getText());
 setStatus(ok?"Tags sauvegard\u00E9s dans le projet et le .boom":"Tags non enregistr\u00E9s");
 if(ok)refresh();
}
void BoomBrowser::deleteCurrent(){
 if(!currentUser())return;
 if(!confirmDelete){
  confirmDelete=true;
  setStatus("SUPPRIMER de nouveau pour confirmer la suppression");
  return;
 }
 const int index=current()->index;
 confirmDelete=false;
 if(proc.deleteUserPreset(index)){
  chosen=-1;refresh(false);
  setStatus("Preset utilisateur supprim\u00E9");
 }else setStatus("Suppression impossible");
}
void BoomBrowser::importCurrent(){
 chooser=std::make_unique<juce::FileChooser>("Importer un preset BOOM",
                                              juce::File{},"*.boom");
 juce::Component::SafePointer<BoomBrowser> safe(this);
 chooser->launchAsync(juce::FileBrowserComponent::openMode|
                      juce::FileBrowserComponent::canSelectFiles,
  [safe](const juce::FileChooser& picker){
   if(safe==nullptr)return;
   const auto file=picker.getResult();
   if(file==juce::File{})return;
   if(safe->proc.importUserPreset(file)){
    safe->mode=boom::browser::Mode::User;
    safe->category=-1;safe->tag=boom::browser::Tag::All;
    safe->search.clear();safe->refresh(false);
    safe->chosen=safe->rows.empty()?-1:static_cast<int>(safe->rows.size())-1;
    if(safe->chosen>=0)safe->items.selectRow(safe->grid?safe->chosen/2:safe->chosen);
    safe->setStatus("Preset .boom import\u00E9");
   }else safe->setStatus("Import refus\u00E9 : fichier incorrect ou biblioth\u00E8que pleine");
  });
}
void BoomBrowser::exportCurrent(){
 if(!currentUser())return;
 const int index=current()->index;
 const auto file=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
  .getChildFile(juce::File::createLegalFileName(current()->name)+".boom");
 chooser=std::make_unique<juce::FileChooser>("Exporter un preset BOOM",file,"*.boom");
 juce::Component::SafePointer<BoomBrowser> safe(this);
 chooser->launchAsync(juce::FileBrowserComponent::saveMode|
                      juce::FileBrowserComponent::canSelectFiles|
                      juce::FileBrowserComponent::warnAboutOverwriting,
   [safe,index](const juce::FileChooser& picker){
    if(safe==nullptr)return;
    const auto file=picker.getResult();
    if(file==juce::File{})return;
    const bool ok=safe->proc.exportUserPreset(index,file.withFileExtension(".boom"));
    safe->setStatus(ok?"Preset et tags export\u00E9s":"\u00C9chec de l'export");
   });
}
void BoomBrowser::selectDelta(int delta){
 if(rows.empty())return;
 chosen=juce::jlimit(0,static_cast<int>(rows.size())-1,juce::jmax(0,chosen)+delta);
 items.selectRow(grid?chosen/2:chosen);
 syncButtons();repaint();
}
bool BoomBrowser::keyPressed(const juce::KeyPress& key){
 if(key==juce::KeyPress::returnKey){loadSelection();return true;}
 if(key==juce::KeyPress::escapeKey){if(onClose)onClose();return true;}
 if(key==juce::KeyPress::upKey){selectDelta(-1);return true;}
 if(key==juce::KeyPress::downKey){selectDelta(1);return true;}
 return false;
}
juce::Rectangle<int> BoomBrowser::scaleBox(float x,float y,float w,float h)const{
 const float sx=float(getWidth())/1550.f,sy=float(getHeight())/345.f;
 return {juce::roundToInt(x*sx),juce::roundToInt(y*sy),
         juce::jmax(1,juce::roundToInt(w*sx)),
         juce::jmax(1,juce::roundToInt(h*sy))};
}
void BoomBrowser::resized(){
 auto put=[this](juce::Component& c,float x,float y,float w,float h){
  c.setBounds(scaleBox(x,y,w,h));
 };
 put(presets,316,8,106,32);
 put(styles,429,8,106,32);
 put(user,542,8,125,32);
 put(viewList,1307,8,43,32);
 put(viewGrid,1358,8,43,32);
 put(clearFilters,1410,8,80,32);
 put(close,1499,8,38,32);
 put(search,245,47,637,30);
 put(sortCombo,893,47,160,30);
 put(onlyFavorites,1063,47,150,30);
 for(int i=0;i<9;++i)
  put(tagButtons[static_cast<std::size_t>(i)],245.f+i*105.f,86,99,28);
 put(categories,15,125,214,175);
 put(items,240,125,968,175);
 previewRect=scaleBox(1221,125,311,175);
 listRect=items.getBounds();
 put(status,791,306,414,31);
 put(favorite,244,307,115,30);
 put(save,244,307,96,30);
 put(rename,347,307,103,30);
 put(remove,457,307,102,30);
 put(import,566,307,102,30);
 put(exportFile,675,307,102,30);
 put(load,1217,307,316,30);
 put(nameEdit,1230,212,294,25);
 put(tagsEdit,1230,261,205,25);
 put(writeTags,1441,261,82,25);
 items.setRowHeight(juce::jmax(21,juce::roundToInt((grid?65.f:31.f)*getHeight()/345.f)));
 categories.setRowHeight(juce::jmax(19,juce::roundToInt(24.f*getHeight()/345.f)));
 syncButtons();
}
void BoomBrowser::paintCapsule(
 juce::Graphics& g,juce::Rectangle<int> rectangle,const juce::String& label,bool active)const{
 const auto r=rectangle.toFloat().reduced(.5f);
 g.setColour(active?burgundy:dark);g.fillRoundedRectangle(r,6.f);
 g.setColour(active?gold:bronze.withAlpha(.6f));g.drawRoundedRectangle(r,6.f,1.f);
 g.setColour(active?ivory:gold);g.setFont(font(11.f,active));
 g.drawFittedText(label,rectangle.reduced(5,1),juce::Justification::centred,1);
}
void BoomBrowser::paint(juce::Graphics& g){
 auto area=getLocalBounds().toFloat();
 juce::ColourGradient surface(juce::Colour::fromRGB(73,22,29),area.getTopLeft(),
                               juce::Colour::fromRGB(14,8,10),area.getBottomRight(),false);
 g.setGradientFill(surface);g.fillRoundedRectangle(area.reduced(.8f),7.f);
 g.setColour(bronze);g.drawRoundedRectangle(area.reduced(1.f),7.f,1.3f);
 auto side=scaleBox(12,121,220,182);
 g.setColour(dark);g.fillRoundedRectangle(side.toFloat(),4.f);
 g.setColour(bronze.withAlpha(.42f));g.drawRoundedRectangle(side.toFloat().reduced(.5f),4.f,1.f);
 auto center=scaleBox(237,121,973,182);
 g.setColour(dark);g.fillRoundedRectangle(center.toFloat(),4.f);
 g.setColour(bronze.withAlpha(.42f));g.drawRoundedRectangle(center.toFloat().reduced(.5f),4.f,1.f);
 g.setColour(gold);g.setFont(font(15.f,true));
 g.drawFittedText(english?"BOOM / LIBRARY":"BOOM / BIBLIOTH\u00C8QUE",scaleBox(19,10,290,28),
                  juce::Justification::centredLeft,1);
 g.setColour(gold.withAlpha(.9f));g.setFont(font(11.f));
 const auto total=mode==boom::browser::Mode::Styles?40:
                  mode==boom::browser::Mode::User?proc.userPresetCount():100;
 g.drawFittedText(juce::String(rows.size())+" / "+juce::String(total)+
    (mode==boom::browser::Mode::Styles?" Styles":mode==boom::browser::Mode::User?
     " USER":" presets"),
     scaleBox(1216,49,312,26),juce::Justification::centredRight,1);
 paintPreview(g);
}
void BoomBrowser::paintPreview(juce::Graphics& g){
 const auto r=previewRect.toFloat();
 g.setColour(dark);g.fillRoundedRectangle(r,4.f);
 g.setColour(bronze.withAlpha(.76f));g.drawRoundedRectangle(r.reduced(.5f),4.f,1.f);
 auto write=[&](const juce::String& content,float x,float y,float w,float h,
                float size,bool bold=false,bool centered=false){
  g.setColour(bold?ivory:gold);
  g.setFont(font(size,bold));
  g.drawFittedText(content,scaleBox(x,y,w,h),
   centered?juce::Justification::centred:juce::Justification::centredLeft,1);
 };
 auto rule=[&](float y){
  const auto q=scaleBox(1232,y,290,1);
  g.setColour(bronze.withAlpha(.45f));g.fillRect(q);
 };
 write(english?"SOUND PROFILE":"PROFIL SONORE",1232,129,290,17,11.f,true);
 const auto* item=current();
 if(item==nullptr){
  write(english?"No results. Change the search or filters.":
                "Aucun r\u00E9sultat. Modifiez les filtres.",
        1233,167,285,40,12.5f,false,true);
  return;
 }
 write(item->name,1231,148,292,29,19.f,true,true);
 if(mode==boom::browser::Mode::User){
  write(english?"USER PRESET":"PRESET UTILISATEUR",1232,180,290,15,11.f);
  rule(201);
  write(english?"PRESET NAME":"NOM DU MASTER",1232,199,290,12,10.5f,true);
  write(english?"CUSTOM TAGS":"TAGS PERSONNALIS\u00C9S",1232,246,290,12,10.5f,true);
  return; // Native edit fields occupy y=212..238 and y=261..287.
 }
 write(categoryName(*item)+" / "+styleName(*item),
       1232,179,290,19,11.f,false,true);
 rule(203);
 const auto params=mode==boom::browser::Mode::Styles?
                  boom::makeStyle(item->index):
                  boom::makePreset(item->index).params;
 auto metric=[&](float x,float y,const juce::String& title,const juce::String& value){
  write(title,x,y,66,19,10.5f);
  write(value,x+68,y,76,19,12.f,true);
 };
 metric(1232,208,"DRIVE",juce::String(params.inputDb,1)+" dB");
 metric(1380,208,"PUSH",juce::String(params.push,0)+" %");
 metric(1232,232,"GLUE",juce::String(params.compRatio,2)+":1");
 metric(1380,232,english?"CEILING":"PLAFOND",
                   juce::String(params.ceilingDb,1)+" dBFS");
 rule(257);
 const auto tags=tagsText(*item);
 write((english?"TAGS : ":"TAGS : ")+(tags.isEmpty()?"-":tags),
       1232,267,290,24,11.5f);
}
