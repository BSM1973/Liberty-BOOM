#pragma once
#include <JuceHeader.h>
#include "BoomUiTheme.h"
#include "BoomSafeText.h"

// A single premium palette for all BOOM popup menus and native combo boxes.
// UI only: no parameter IDs, preset data, attachments or audio are changed.
class BoomPremiumMenuLook final : public juce::LookAndFeel_V4 {
public:
 BoomPremiumMenuLook(){
  setColour(juce::PopupMenu::backgroundColourId,boom::ui::theme::background());
  setColour(juce::PopupMenu::textColourId,boom::ui::theme::ivory());
  setColour(juce::PopupMenu::highlightedBackgroundColourId,boom::ui::theme::active());
  setColour(juce::PopupMenu::highlightedTextColourId,boom::ui::theme::ivory());
 }
 juce::Font getPopupMenuFont() override {return boom::ui::theme::font(14.f);}
 void getIdealPopupMenuItemSize(const juce::String& text,bool separator,int,
                                int& width,int& height) override {
  if(separator){width=205;height=12;return;}
  width=juce::jlimit(215,410,95+int(text.length())*8);height=36;
 }
 void drawPopupMenuBackground(juce::Graphics& g,int width,int height) override {
  auto r=juce::Rectangle<float>(0.f,0.f,float(width),float(height)).reduced(1.f);
  g.fillAll(boom::ui::theme::background());
  juce::ColourGradient fill(boom::ui::theme::surface(),r.getTopLeft(),
                           boom::ui::theme::background(),r.getBottomLeft(),false);
  g.setGradientFill(fill);g.fillRoundedRectangle(r,7.f);
  g.setColour(boom::ui::theme::copper().withAlpha(.86f));
  g.drawRoundedRectangle(r,7.f,1.2f);
 }
 void drawPopupMenuItem(juce::Graphics& g,const juce::Rectangle<int>& area,
                        bool separator,bool active,bool highlighted,bool ticked,
                        bool submenu,const juce::String& text,
                        const juce::String& shortcut,const juce::Drawable*,
                        const juce::Colour* colour) override {
  if(separator){
   g.setColour(boom::ui::theme::copper().withAlpha(.3f));
   g.drawHorizontalLine(area.getCentreY(),float(area.getX()+14),float(area.getRight()-14));
   return;
  }
  auto row=area.toFloat().reduced(5.f,2.f);
  if(active&&highlighted){
   juce::ColourGradient fill(boom::ui::theme::active(),row.getTopLeft(),
                            boom::ui::theme::surface(),row.getBottomLeft(),false);
   g.setGradientFill(fill);g.fillRoundedRectangle(row,5.f);
   g.setColour(boom::ui::theme::gold());
   g.fillRect(row.getX()+2.f,row.getY()+5.f,2.5f,juce::jmax(0.f,row.getHeight()-10.f));
  }
  g.setColour(!active?boom::ui::theme::muted().withAlpha(.42f):
              (colour?*colour:boom::ui::theme::ivory()));
  g.setFont(boom::ui::theme::font(13.5f,ticked));
  auto label=area.reduced(16,2);
  if(ticked)label.removeFromLeft(16);
  if(submenu)label.removeFromRight(13);
  g.drawFittedText(boom::ui::text::safeUpperText(text),label,
                   juce::Justification::centredLeft,1);
  if(ticked){
   g.setColour(boom::ui::theme::gold());
   g.fillEllipse(float(area.getX()+13),float(area.getCentreY()-3),6.f,6.f);
  }
  if(shortcut.isNotEmpty()){
   g.setColour(boom::ui::theme::muted());g.setFont(boom::ui::theme::font(11.f));
   g.drawFittedText(shortcut,area.withX(area.getRight()-92).withWidth(80),
                    juce::Justification::centredRight,1);
  }
  if(submenu){
   juce::Path arrow;
   const float x=float(area.getRight()-16),y=float(area.getCentreY());
   arrow.startNewSubPath(x-3.f,y-5.f);arrow.lineTo(x+2.f,y);arrow.lineTo(x-3.f,y+5.f);
   g.setColour(boom::ui::theme::gold());
   g.strokePath(arrow,juce::PathStrokeType(1.8f));
  }
 }
 void drawComboBox(juce::Graphics& g,int width,int height,bool down,
                   int,int,int,int,juce::ComboBox&) override {
  auto r=juce::Rectangle<float>(0.f,0.f,float(width),float(height)).reduced(1.f);
  juce::ColourGradient fill(down?boom::ui::theme::active():boom::ui::theme::surface(),
                           r.getTopLeft(),boom::ui::theme::background(),
                           r.getBottomLeft(),false);
  g.setGradientFill(fill);g.fillRoundedRectangle(r,5.f);
  g.setColour(boom::ui::theme::copper());g.drawRoundedRectangle(r,5.f,1.2f);
  juce::Path arrow;
  const float x=float(width-17),y=float(height)*.52f;
  arrow.startNewSubPath(x-5.f,y-2.f);arrow.lineTo(x,y+3.f);arrow.lineTo(x+5.f,y-2.f);
  g.setColour(boom::ui::theme::gold());g.strokePath(arrow,juce::PathStrokeType(1.8f));
 }
 juce::Font getComboBoxFont(juce::ComboBox& box) override {
  return boom::ui::theme::font(juce::jlimit(11.f,14.f,float(box.getHeight())*.45f),true);
 }
};
