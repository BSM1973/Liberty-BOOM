#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <memory>
#include <vector>

// BOOM v0.7: the user-approved console artwork is the actual plugin skin.
// Interactive hit regions are mapped to its native image pixel coordinates;
// every audio control uses APVTS and the current DSP engine.
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
 void mouseWheelMove(const juce::MouseEvent&,const juce::MouseWheelDetails&) override;
 bool keyPressed(const juce::KeyPress&) override;
private:
 enum class Page { chain,browser,meters,settings,help };
 struct PresetRow { bool user=false; int index=-1; };
 struct Knob {float x,y,r; const char* id; float tx,ty;};
 struct Field {juce::Rectangle<float> area;const char* id;};
 BoomAudioProcessor& proc;
 juce::Image chainSkin,browserSkin;
 Page page=Page::chain;
 juce::TextEditor search,saveName;
 std::unique_ptr<juce::FileChooser> chooser;
 std::vector<PresetRow> rows;
 int category=0,styleFilter=-1,selectedRow=0,firstVisible=0;
 int selectedComparison=0;
 bool english=false,deleteArmed=false;
 juce::String notice;
 juce::String dragged;
 juce::Point<float> startPosition;
 float startValue=0;
 bool isGesture=false;
 float smoothVuL=0.0f,smoothVuR=0.0f;
 bool showMeterNeedles=true;
 static constexpr int chainW=1672,chainH=941,browserW=1592,browserH=988;
 int artW()const noexcept;
 int artH()const noexcept;
 juce::Point<float> artPoint(const juce::MouseEvent&)const;
 juce::Rectangle<int> artRect(int x,int y,int w,int h)const;
 void showPage(Page);
 void timerCallback() override;
 float raw(const char* id)const;
 void setParam(const char* id,float rawValue);
 void toggle(const char* id);
 void beginDrag(const char* id,const juce::MouseEvent&);
 void finishDrag();
 const Knob* activeKnob(juce::Point<float>)const;
 const Field* chainField(juce::Point<float>)const;
 void renderKnobs(juce::Graphics&);
 void renderDynamicText(juce::Graphics&);
 void renderMeterNeedles(juce::Graphics&);
 void renderBrowser(juce::Graphics&);
 void renderExtra(juce::Graphics&);
 void refreshRows();
 void selectRow(int row);
 void loadRow();
 void choosePreset(int index);
 void chooseStyle(int index);
 void showPresetsMenu();
 void showStyleMenu();
 void showUserMenu();
 void saveCurrent();
 void importUser();
 void exportUser();
 void handleClick(juce::Point<float>,const juce::MouseEvent&);
 void drawValue(juce::Graphics&,const juce::String&,float x,float y,float w,float h,int decimals=1)const;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BoomAudioProcessorEditor)
};