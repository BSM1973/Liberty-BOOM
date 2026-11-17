#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "BoomBrowserCatalog.h"
#include "BoomPremiumMenuLook.h"
#include <array>
#include <functional>
#include <memory>
#include <vector>

// Native JUCE browser. It owns the search boxes, mode tabs, filter chips,
// scrollable rows and action buttons. It REPLACES the photographed browser
// panel rather than stacking controls over the still image.
class BoomBrowser final : public juce::Component,
                          private juce::ListBoxModel {
public:
 explicit BoomBrowser(BoomAudioProcessor& processor,std::function<void()> close);
 ~BoomBrowser()override;
 void setMode(boom::browser::Mode mode);
 void updateLanguage(bool english);
 void refresh(bool preserveSelection=true);
 void paint(juce::Graphics&)override;
 void resized()override;
 bool keyPressed(const juce::KeyPress&)override;
 void mouseMove(const juce::MouseEvent&)override;
 void mouseExit(const juce::MouseEvent&)override;
private:
 struct Item{bool user=false;int index=0;int category=-1;int style=-1;
             juce::String name;std::uint32_t tags=0;};
 class CategoryModel final:public juce::ListBoxModel{
 public:
  explicit CategoryModel(BoomBrowser& b):owner(b){}
  int getNumRows()override;
  void paintListBoxItem(int row,juce::Graphics&,int w,int h,bool selected)override;
  void selectedRowsChanged(int row)override;
 private:
  BoomBrowser& owner;
 };
 class BrassButtons final:public juce::LookAndFeel_V4{
 public:
  void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,
                            bool highlight,bool down)override;
  void drawButtonText(juce::Graphics&,juce::TextButton&,bool highlight,bool down)override;
 };
 BoomAudioProcessor& proc;
 std::function<void()> onClose;
 BrassButtons buttonsLook;
 BoomPremiumMenuLook menuLook;
 CategoryModel categoryModel;
 juce::ListBox categories,items;
 juce::TextEditor search,nameEdit,tagsEdit;
 juce::ComboBox sortCombo;
 std::array<juce::TextButton,9> tagButtons;
 juce::TextButton presets,styles,user,onlyFavorites,viewList,viewGrid,close;
 juce::TextButton load,favorite,save,rename,remove,import,exportFile,writeTags;
 juce::TextButton clearFilters;
 juce::Label status;
 std::vector<Item> rows;
 boom::browser::Mode mode=boom::browser::Mode::Presets;
 boom::browser::Tag tag=boom::browser::Tag::All;
 int category=-1,chosen=-1,hoveredIndex=-1,hoveredCategory=-1;
 bool english=false,grid=false,favoritesFilter=false;
 bool confirmDelete=false;
 std::unique_ptr<juce::FileChooser> chooser;
 juce::Rectangle<int> previewRect;
 juce::Rectangle<int> listRect;
 int getNumRows()override;
 void paintListBoxItem(int,juce::Graphics&,int,int,bool)override;
 void selectedRowsChanged(int)override;
 void listBoxItemDoubleClicked(int,const juce::MouseEvent&)override;
 void listBoxItemClicked(int,const juce::MouseEvent&)override;
 void setCategory(int);
 void selectCurrent(bool);
 void loadSelection();
 void toggleFavorite();
 void saveCurrent();
 void renameCurrent();
 void deleteCurrent();
 void importCurrent();
 void exportCurrent();
 void writeUserTags();
 void selectDelta(int);
 bool currentUser()const;
 const Item* current()const;
 juce::String tagsText(const Item&)const;
 juce::String categoryName(const Item&)const;
 juce::String styleName(const Item&)const;
 void resetDelete(){confirmDelete=false;}
 void setStatus(juce::String text);
 void styleButton(juce::TextButton&,juce::String,std::function<void()>);
 void syncButtons();
 juce::Rectangle<int> scaleBox(float x,float y,float w,float h)const;
 void paintPreview(juce::Graphics&);
 void paintCapsule(juce::Graphics&,juce::Rectangle<int>,const juce::String&,bool)const;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BoomBrowser)
};
