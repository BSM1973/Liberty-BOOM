#pragma once
#include <JuceHeader.h>
#include <array>
#include <functional>

// The banner and transport rail are completely native JUCE surfaces.
// Their photographic counterparts are NEVER rendered beneath them.
class BoomChrome final : public juce::Component {
public:
 enum class Area {Header,Toolbar};
 enum class Command {
  Style, Preset, Previous, Next,
  Chain, Meters, Settings, Help, Language,
  RecallA, RecallB, StoreA, StoreB, CopyAtoB, CopyBtoA, Level
 };
 using OnCommand=std::function<void(Command)>;
 explicit BoomChrome(Area,OnCommand);
 ~BoomChrome() override;
 void paint(juce::Graphics&) override;
 void resized() override;
 void setNames(const juce::String& style,const juce::String& preset);
 void setState(bool english,int activePage,bool levelComp,int selectedRecall);
private:
 class BrassLook final:public juce::LookAndFeel_V4 {
 public:
  void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,
                            bool hover,bool pressed)override;
  void drawButtonText(juce::Graphics&,juce::TextButton&,bool hover,bool pressed)override;
 };
 Area area;
 OnCommand command;
 BrassLook look;
 std::array<juce::TextButton,4> headerButtons;
 std::array<juce::TextButton,12> toolbarButtons;
 juce::String styleName,presetName;
 bool english=false,levelEnabled=false;
 int activePage=0,selectedRecall=-1;
 void wireButton(juce::TextButton&,Command,juce::String);
 void updateButtons();
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BoomChrome)
};
