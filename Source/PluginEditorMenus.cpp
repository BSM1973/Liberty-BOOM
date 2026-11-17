#include "PluginEditor.h"
#include "BoomBrowser.h"
#include <algorithm>

namespace {
juce::PopupMenu::Options popupOptions(juce::Component* target){
 return juce::PopupMenu::Options().withTargetComponent(target);
}
}

bool BoomAudioProcessorEditor::selectedIsUser()const{
 return selectedRow>=0&&selectedRow<int(results.size())
   &&results[static_cast<std::size_t>(selectedRow)].isUser;
}
void BoomAudioProcessorEditor::refreshResults(){
 Row prior;
 bool hasPrior=selectedRow>=0&&selectedRow<int(results.size());
 if(hasPrior)prior=results[static_cast<std::size_t>(selectedRow)];
 results.clear();
 const auto term=search.getText().trim();
 if(category!=11){
  for(int index=0;index<100;++index){
   const auto p=boom::makePreset(index);
   if(category>=0&&category<=9 && p.category!=category)continue;
   if(category==10&&!processor.isFavorite(index))continue;
   if(styleFilter>=0&&p.style!=styleFilter)continue;
   const auto name=juce::String(p.name);
   const auto family=juce::String(boom::categories[static_cast<std::size_t>(p.category)]);
   const auto style=juce::String(boom::styles()[static_cast<std::size_t>(p.style)].name);
   if(term.isNotEmpty()&&!name.containsIgnoreCase(term)
      &&!family.containsIgnoreCase(term)&&!style.containsIgnoreCase(term))continue;
   results.push_back({false,index});
  }
 }
 if((category==-1||category==11)&&styleFilter<0)
  for(int i=0;i<processor.userPresetCount();++i)
   if(processor.userPresetName(i).containsIgnoreCase(term))results.push_back({true,i});
 selectedRow=results.empty()?-1:0;
 if(hasPrior){
  for(int i=0;i<int(results.size());++i)
   if(results[static_cast<std::size_t>(i)].isUser==prior.isUser &&
      results[static_cast<std::size_t>(i)].index==prior.index){
    selectedRow=i;break;
   }
 }
 const int count=grid?6:5;
 firstVisible=juce::jlimit(0,juce::jmax(0,int(results.size())-count),firstVisible);
 if(selectedRow<firstVisible)firstVisible=selectedRow>=0?selectedRow:0;
 if(selectedRow>=firstVisible+count)firstVisible=selectedRow-count+1;
 repaint();
}
void BoomAudioProcessorEditor::changeSelected(int row){
 if(results.empty())return;
 selectedRow=juce::jlimit(0,int(results.size())-1,row);
 const int count=grid?6:5;
 if(selectedRow<firstVisible)firstVisible=selectedRow;
 if(selectedRow>=firstVisible+count)firstVisible=selectedRow-count+1;
 deleteConfirmation=false;
 repaint();
}
void BoomAudioProcessorEditor::loadSelected(){
 if(selectedRow<0||selectedRow>=int(results.size()))return;
 const auto r=results[static_cast<std::size_t>(selectedRow)];
 if(r.isUser){
  if(!processor.loadUserPreset(r.index)){notice="Impossible de charger le preset";return;}
 }else processor.loadFactoryPreset(r.index);
 notice=english?"Preset loaded":"Preset chargé";
 repaint();
}
void BoomAudioProcessorEditor::loadPreset(int index){
 if(index<0||index>=100)return;
 processor.loadFactoryPreset(index);
 notice="Preset : "+juce::String(boom::makePreset(index).name);
 repaint();
}
void BoomAudioProcessorEditor::cyclePreset(int offset){
 const int current=processor.presetIndex();
 loadPreset(current<0?(offset<0?99:0):(current+offset+100)%100);
}
void BoomAudioProcessorEditor::loadStyle(int index){
 if(index<0||index>=40)return;
 processor.loadStyle(index);
 notice="Style : "+juce::String(boom::styles()[static_cast<std::size_t>(index)].name);
 repaint();
}
void BoomAudioProcessorEditor::selectCategory(int index){
 if(index<0||index>11)return;
 category=index;firstVisible=0;refreshResults();
}
void BoomAudioProcessorEditor::openStyleMenu(){
 if(nativeBrowser!=nullptr){
  nativeBrowser->setMode(boom::browser::Mode::Styles);
  showPage(Page::browser);
 }
}

void BoomAudioProcessorEditor::openPresetMenu(){
 if(nativeBrowser!=nullptr){
  nativeBrowser->setMode(boom::browser::Mode::Presets);
  showPage(Page::browser);
 }
}

void BoomAudioProcessorEditor::openStyleFilter(){
 juce::PopupMenu menu;
 menu.addItem(1,english?"All Styles":"Tous les Styles",true,styleFilter<0);
 for(int group=0;group<4;++group){
  juce::PopupMenu sub;
  for(int k=0;k<10;++k){
   const int index=group*10+k;
   sub.addItem(index+2,juce::String(boom::styles()[static_cast<std::size_t>(index)].name),
               true,styleFilter==index);
  }
  menu.addSubMenu(juce::String(group*10+1)+"-"+juce::String(group*10+10),sub);
 }
 juce::Component::SafePointer<BoomAudioProcessorEditor> self(this);
 menu.showMenuAsync(popupOptions(this),[self](int choice){
  if(self==nullptr||choice==0)return;
  self->styleFilter=choice==1?-1:choice-2;
  self->firstVisible=0;self->refreshResults();
 });
}
void BoomAudioProcessorEditor::openColorMenu(){
 juce::PopupMenu menu;
 constexpr const char* labels[]={"CLEAN","TUBE","TAPE"};
 for(int i=0;i<3;++i)menu.addItem(i+1,labels[i],true,int(read("color"))==i);
 juce::Component::SafePointer<BoomAudioProcessorEditor> self(this);
 menu.showMenuAsync(popupOptions(this),[self](int value){
  if(self!=nullptr&&value>0)self->write("color",float(value-1));
 });
}
void BoomAudioProcessorEditor::openLanguageMenu(){
 juce::PopupMenu menu;
 menu.addItem(1,"Français",true,read("language")<.5f);
 menu.addItem(2,"English",true,read("language")>.5f);
 juce::Component::SafePointer<BoomAudioProcessorEditor> self(this);
 menu.showMenuAsync(popupOptions(this),[self](int value){
  if(self!=nullptr&&value>0){
   self->write("language",float(value-1));
   self->english=value==2;
  }
 });
}
void BoomAudioProcessorEditor::openTrimsMenu(){
 juce::PopupMenu menu;
 menu.addItem(1,english?"Left trim...":"Trim gauche...");
 menu.addItem(2,english?"Right trim...":"Trim droit...");
 menu.addItem(3,english?"Reset trims":"Réinitialiser les trims");
 menu.addItem(4,read("inputLink")>.5f?"Délier G/D":"Lier G/D");
 juce::Component::SafePointer<BoomAudioProcessorEditor> self(this);
 menu.showMenuAsync(popupOptions(this),[self](int action){
  if(self==nullptr)return;
  const bool b=self->page==Page::browser;
  const juce::Rectangle<float> place=b?
    juce::Rectangle<float>(230,861,72,30):juce::Rectangle<float>(257,830,75,30);
  if(action==1)self->numericFor("inputLTrim",1.f,place);
  if(action==2)self->numericFor("inputRTrim",1.f,place);
  if(action==3){
   self->write("inputLTrim",0.f);self->write("inputRTrim",0.f);
  }
  if(action==4)self->toggle("inputLink");
 });
}
void BoomAudioProcessorEditor::openUserMenu(){
 juce::PopupMenu menu;
 menu.addItem(1,english?"Save current preset":"Enregistrer le réglage actuel");
 menu.addItem(2,english?"Rename selected user preset":"Renommer le preset utilisateur",
              selectedIsUser());
 menu.addItem(3,english?"Delete selected user preset":"Supprimer le preset utilisateur",
              selectedIsUser());
 menu.addSeparator();
 menu.addItem(4,english?"Import .boom":"Importer un preset .boom");
 menu.addItem(5,english?"Export selected .boom":"Exporter le preset .boom",
              selectedIsUser());
 juce::Component::SafePointer<BoomAudioProcessorEditor> self(this);
 menu.showMenuAsync(popupOptions(this),[self](int action){
  if(self==nullptr)return;
  switch(action){
   case 1:self->saveUser();break;
   case 2:self->renameUser();break;
   case 3:self->deleteUser();break;
   case 4:self->importUser();break;
   case 5:self->exportUser();break;
   default:break;
  }
 });
}
void BoomAudioProcessorEditor::saveUser(){
 const auto name=userPresetName.getText().trim();
 if(name.isEmpty()){
  notice=english?"Enter a name to save the current settings.":"Saisis un nom pour ce preset.";
  return;
 }
 const int index=processor.saveUserPreset(name);
 if(index<0){notice="Enregistrement impossible (512 presets maximum)";return;}
 category=11;styleFilter=-1;firstVisible=0;search.clear();
 refreshResults();
 for(int i=0;i<int(results.size());++i)
  if(results[static_cast<std::size_t>(i)].isUser&&
     results[static_cast<std::size_t>(i)].index==index){
   changeSelected(i);break;
  }
 notice="Preset enregistré dans le projet DAW  -  export .boom possible.";
 repaint();
}
void BoomAudioProcessorEditor::renameUser(){
 if(!selectedIsUser())return;
 auto newName=userPresetName.getText().trim();
 if(newName.isEmpty()){
  notice="Saisis le nouveau nom dans le champ de sauvegarde.";return;
 }
 const auto index=results[static_cast<std::size_t>(selectedRow)].index;
 if(processor.renameUserPreset(index,newName)){
  notice="Preset renommé";refreshResults();
 }
}
void BoomAudioProcessorEditor::deleteUser(){
 if(!selectedIsUser())return;
 if(!deleteConfirmation){
  deleteConfirmation=true;
  notice="Choisis Supprimer une seconde fois pour confirmer.";repaint();return;
 }
 const int index=results[static_cast<std::size_t>(selectedRow)].index;
 deleteConfirmation=false;
 if(processor.deleteUserPreset(index)){refreshResults();notice="Preset supprimé";}
}
void BoomAudioProcessorEditor::importUser(){
 fileChooser=std::make_unique<juce::FileChooser>(
  "Importer un preset BOOM",juce::File{},"*.boom");
 juce::Component::SafePointer<BoomAudioProcessorEditor> self(this);
 fileChooser->launchAsync(
  juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
  [self](const juce::FileChooser& chooser){
   if(self==nullptr)return;
   const auto file=chooser.getResult();
   if(file==juce::File{})return;
   if(self->processor.importUserPreset(file)){
    self->category=11;self->styleFilter=-1;self->search.clear();
    self->refreshResults();self->notice="Preset importé";
   }else self->notice="Fichier .boom invalide ou bibliothèque pleine";
   self->repaint();
  });
}
void BoomAudioProcessorEditor::exportUser(){
 if(!selectedIsUser())return;
 const auto index=results[static_cast<std::size_t>(selectedRow)].index;
 const auto name=juce::File::createLegalFileName(processor.userPresetName(index));
 auto proposed=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
  .getChildFile(name+".boom");
 fileChooser=std::make_unique<juce::FileChooser>(
  "Exporter un preset BOOM",proposed,"*.boom");
 juce::Component::SafePointer<BoomAudioProcessorEditor> self(this);
 fileChooser->launchAsync(
  juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|
  juce::FileBrowserComponent::warnAboutOverwriting,
  [self,index](const juce::FileChooser& chooser){
   if(self==nullptr)return;
   auto file=chooser.getResult();
   if(file==juce::File{})return;
   const bool ok=self->processor.exportUserPreset(index,file.withFileExtension(".boom"));
   self->notice=ok?"Preset exporté":"Échec export";self->repaint();
  });
}
