#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "BOOMUIMap.h"
#include <array>
#include <memory>
#include <vector>

// One authoritative, interactive hardware surface. The reference artwork is
// the skin; transparent hit regions invoke real APVTS parameters or actions.
// No opaque JUCE buttons are placed on top of the pictured hardware.
class BoomAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                       private juce::Timer {
public:
 explicit BoomAudioProcessorEditor(BoomAudioProcessor&);
 ~BoomAudioProcessorEditor() override;
 void paint(juce::Graphics&) override;
 void resized() override;
 void mouseDown(const juce::MouseEvent&) override;
 void mouseDrag(const juce::MouseEvent&) override;
 void mouseUp(const juce::MouseEvent&) override;
 void mouseDoubleClick(const juce::MouseEvent&) override;
 void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
 bool keyPressed(const juce::KeyPress&) override;
private:
 enum class Page {chain,browser,meters,settings,help};
 struct Row {bool isUser=false;int index=-1;};
 struct Numeric {juce::String parameter;float scale=1.f;};
 BoomAudioProcessor& processor;
 juce::Image chainSkin,browserSkin;
 std::array<juce::Image,7> chainFaces,browserFaces;
 Page page=Page::chain;
 juce::TextEditor search,userPresetName,numericEdit;
 std::unique_ptr<juce::FileChooser> fileChooser;
 std::vector<Row> results;
 int selectedRow=-1,firstVisible=0,category=-1,styleFilter=-1;
 bool grid=false,english=false,dragging=false,dragMoved=false;
 bool deleteConfirmation=false,clipHold=false;
 float vuL=-20.f,vuR=-20.f,dragStartValue=0.f;
 juce::Point<float> dragOrigin;
 juce::String dragParameter,notice;
 Numeric editing;
 const boom::ui::Zone* editZone=nullptr;
 const boom::ui::Zone* dragZone=nullptr;
 static constexpr int CW=boom::ui::chainWidth,CH=boom::ui::chainHeight;
 static constexpr int BW=boom::ui::browserWidth,BH=boom::ui::browserHeight;
 int artWidth()const noexcept;
 int artHeight()const noexcept;
 juce::Point<float> pointOf(const juce::MouseEvent&)const;
 juce::Rectangle<int> canvasRect(float,float,float,float)const;
 void showPage(Page);
 void paintSkin(juce::Graphics&);
 void paintKnobs(juce::Graphics&);
 void paintMeter(juce::Graphics&);
 void paintHead(juce::Graphics&);
 void paintChain(juce::Graphics&);
 void paintBrowser(juce::Graphics&);
 void paintOther(juce::Graphics&);
 void paintReading(juce::Graphics&,juce::Rectangle<float>,juce::String,bool emphasis=false);
 void paintSwitch(juce::Graphics&,const boom::ui::Zone&);
 void refreshResults();
 void changeSelected(int);
 void loadSelected();
 void cyclePreset(int);
 void loadPreset(int);
 void loadStyle(int);
 void selectCategory(int);
 void openStyleMenu();
 void openPresetMenu();
 void openStyleFilter();
 void openColorMenu();
 void openLanguageMenu();
 void openUserMenu();
 void openTrimsMenu();
 void saveUser();
 void renameUser();
 void deleteUser();
 void importUser();
 void exportUser();
 bool selectedIsUser()const;
 float read(const char*)const noexcept;
 void write(const char*,float,bool gesture=true);
 void toggle(const char*);
 const boom::ui::Zone* zoneAt(juce::Point<float>)const noexcept;
 void doAction(const boom::ui::Zone&,const juce::MouseEvent&);
 void endGesture();
 void beginGesture(const boom::ui::Zone&,const juce::MouseEvent&);
 void numericFor(const boom::ui::Zone&);
 void numericFor(const char*,float,juce::Rectangle<float>);
 void commitNumeric();
 void cancelNumeric();
 bool handleAuxiliary(juce::Point<float>,const juce::MouseEvent&);
 void timerCallback() override;
 void prepareFaces();
 static void eraseReferenceNeedle(juce::Image&,juce::Point<float>,juce::Point<float>);
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BoomAudioProcessorEditor)
};
