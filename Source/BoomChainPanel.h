#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>
#include <memory>
#include <vector>

// Dedicated, native JUCE mastering chain. Each widget is attached directly to
// its real APVTS parameter. One module is displayed at a time: the legacy
// photographed rack is covered, never stacked with additional fake buttons.
class BoomChainPanel final : public juce::Component {
public:
 explicit BoomChainPanel(BoomAudioProcessor&);
 ~BoomChainPanel() override;
 void paint(juce::Graphics&) override;
 void resized() override;
 void setEnglish(bool);
 void syncFromHost();
 bool isExpanded() const noexcept {return expanded;}
private:
 struct RotaryControl {
  int module=0;
  juce::String parameterId,frenchLabel;
  juce::Label label;
  juce::Slider slider;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
 };
 struct ChoiceControl {
  int module=0;
  juce::String parameterId,frenchLabel;
  juce::Label label;
  juce::ComboBox box;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
 };
 struct ToggleControl {
  int module=0;
  bool primary=false;
  juce::String parameterId,frenchLabel;
  juce::ToggleButton button;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
 };
 class CopperLook final : public juce::LookAndFeel_V4 {
 public:
  void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,
                            bool,bool) override;
  void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override;
  void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,
                        juce::Slider&) override;
  void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool) override;
 };
 BoomAudioProcessor& processor;
 CopperLook look;
 std::array<juce::TextButton,7> tabButtons;
 juce::TextButton visibilityButton,resetButton;
 std::vector<std::unique_ptr<RotaryControl>> rotaries;
 std::vector<std::unique_ptr<ChoiceControl>> choices;
 std::vector<std::unique_ptr<ToggleControl>> toggles;
 int currentModule=0;
 bool expanded=true,english=false;
 void addRotary(int,const char*,const char*,float,const char*,int);
 void addChoice(int,const char*,const char*,std::initializer_list<const char*>);
 void addToggle(int,const char*,const char*,bool primary=false);
 void selectModule(int);
 void toggleVisibility();
 void restoreModuleDefaults();
 void updateVisibility();
 static juce::String moduleName(int,bool);
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BoomChainPanel)
};
