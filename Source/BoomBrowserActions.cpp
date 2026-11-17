#include "BoomBrowser.h"
#include <algorithm>

namespace {
const auto gold=juce::Colour::fromRGB(224,184,119);
const auto ivory=juce::Colour::fromRGB(244,224,187);
const auto bronze=juce::Colour::fromRGB(174,89,70);
const auto dark=juce::Colour::fromRGB(23,10,13);
const auto burgundy=juce::Colour::fromRGB(108,26,38);
juce::Font font(float size,bool strong=false){
 return juce::Font(juce::FontOptions(size,strong?juce::Font::bold:juce::Font::plain));
}
}
void BoomBrowser::loadSelection(){
 const auto* e=current();
 if(e==nullptr){setStatus("Aucun \u00E9l\u00E9ment s\u00E9lectionn\u00E9");return;}
 bool ok=true;
 if(e->user)ok=proc.loadUserPreset(e->index);
 else if(mode==boom::browser::Mode::Styles)proc.loadStyle(e->index);
 else proc.loadFactoryPreset(e->index);
 setStatus(ok?" "+e->name+"  -  r\u00E9glages charg\u00E9s":"Chargement impossible");
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
 setStatus(" Preset utilisateur sauvegard\u00E9 dans le projet DAW");
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
 setStatus(ok?" Tags sauvegard\u00E9s dans le projet et le .boom":"Tags non enregistr\u00E9s");
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
    safe->setStatus(" Preset .boom import\u00E9");
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
    safe->setStatus(ok?" Preset et tags export\u00E9s":"\u00C9chec de l'export");
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
 put(status,16,307,219,29);
 put(favorite,244,307,115,30);
 put(save,244,307,96,30);
 put(rename,347,307,103,30);
 put(remove,457,307,102,30);
 put(import,566,307,102,30);
 put(exportFile,675,307,102,30);
 put(load,1217,307,316,30);
 put(nameEdit,1230,211,294,25);
 put(tagsEdit,1230,242,205,25);
 put(writeTags,1441,242,82,25);
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
 g.drawFittedText("BOOM  /  BIBLIOTH\u00C8QUE DE MASTERING",scaleBox(19,10,290,28),
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
 g.setColour(juce::Colour::fromRGB(23,11,14));g.fillRoundedRectangle(r,4.f);
 g.setColour(bronze.withAlpha(.75f));g.drawRoundedRectangle(r.reduced(.5f),4.f,1.f);
 auto put=[&](juce::String text,float x,float y,float w,float h,float size,
              bool strong=false,bool centered=false){
  g.setColour(strong?ivory:gold);g.setFont(font(size,strong));
  g.drawFittedText(text,scaleBox(x,y,w,h),
                   centered?juce::Justification::centred:
                            juce::Justification::centredLeft,1);
 };
 const auto* item=current();
 put(english?"SOUND PROFILE":"PROFIL SONORE",1230,129,292,18,11,true);
 if(item==nullptr){
  put(english?"No match. Change filters.":"Aucun r\u00E9sultat. Modifie les filtres.",
      1230,164,291,43,12);
  return;
 }
 put(item->name,1230,152,286,26,19,true,true);
 if(mode==boom::browser::Mode::User){
  put("USER  /  "+juce::String(proc.userPresetCount())+" presets",1230,184,287,19,11);
  return;
 }
 put(categoryName(*item)+"   /   "+styleName(*item),1230,184,288,19,10);
 boom::Parameters p=item->index>=0?
  (mode==boom::browser::Mode::Styles?boom::makeStyle(item->index):
                                    boom::makePreset(item->index).params):boom::Parameters{};
 put("PUSH  "+juce::String(p.push,0)+" %    /    DRIVE "+
     juce::String(p.inputDb,1)+" dB",1230,207,288,19,12,true);
 put("GLUE  "+juce::String(p.compRatio,2)+":1     CEILING "+
     juce::String(p.ceilingDb,1)+" dBFS",1230,229,288,20,11);
 const auto all=tagsText(*item).upToFirstOccurrenceOf(", ",false,false);
 put("TAGS  "+all,1230,263,291,25,11);
}
