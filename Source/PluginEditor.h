#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <memory>
#include <vector>

class HardwareLook final : public juce::LookAndFeel_V4 {
public:
 HardwareLook();
 void drawRotarySlider(juce::Graphics&,int x,int y,int width,int height,float sliderPos,
  float rotaryStartAngle,float rotaryEndAngle,juce::Slider&) override;
 void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
 void drawComboBox(juce::Graphics&,int width,int height,bool,int,int,int,int,juce::ComboBox&) override;
 juce::Font getComboBoxFont(juce::ComboBox&) override;
};

class AnalogVu : public juce::Component {
public:
 explicit AnalogVu(juce::String channel):name(std::move(channel)){}
 void setLevel(float linear);
 void paint(juce::Graphics&) override;
private:
 juce::String name;float current=-20.0f,target=-20.0f;
};

class BoomAudioProcessorEditor final:public juce::AudioProcessorEditor,private juce::Timer {
public:
 explicit BoomAudioProcessorEditor(BoomAudioProcessor&);
 ~BoomAudioProcessorEditor() override;
 void paint(juce::Graphics&) override;
 void resized() override;
private:
 BoomAudioProcessor& processor;
 HardwareLook look;
 std::array<juce::Slider,7> knobs;
 std::array<juce::Label,7> knobLabels;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> knobAttachments;
 juce::ComboBox styleBox,presetBox;
 juce::TextButton browserButton{"PRESETS"},chainButton{"CHAIN"},metersButton{"METERS"},languageButton{"FR / EN"},helpButton{"?"};
 juce::TextButton bypassButton{"BYPASS"},monoButton{"MONO"},meterSourceButton{"INPUT / PROCESS"};
 juce::TextButton storeA{"SAVE A"},storeB{"SAVE B"},recallA{"A"},recallB{"B"};
 juce::TextButton abMenuButton{"A/B"};
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment,monoAttachment,meterAttachment;
 AnalogVu vuL{"LEFT"},vuR{"RIGHT"};
 std::unique_ptr<juce::Component> overlay;
 std::unique_ptr<juce::Component> browserPanel;
 std::unique_ptr<juce::Component> chainPanel;
 std::unique_ptr<juce::Component> meteringPanel;
 std::unique_ptr<juce::Component> helpPanel;
 void timerCallback() override;
 void showBrowser();void showChain();void showMeters();void showHelp();void closeOverlays();
 void setLanguage(bool english);
 void storeState(int slot);void recallState(int slot);
 void paintStudio(juce::Graphics&,float,float);
 juce::Rectangle<int> rect(int x,int y,int w,int h) const;
 void attach(juce::Slider&,const char* id);
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BoomAudioProcessorEditor)
};
