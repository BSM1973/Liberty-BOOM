#include "BoomChrome.h"
#include <algorithm>
#include <cmath>

namespace {
const auto body=juce::Colour::fromRGB(25,12,13);
const auto burgundy=juce::Colour::fromRGB(76,20,28);
const auto copper=juce::Colour::fromRGB(161,80,61);
const auto gold=juce::Colour::fromRGB(224,188,130);
const auto cream=juce::Colour::fromRGB(245,228,195);
juce::Font makeFont(float size,bool strong=false){
 return juce::Font(juce::FontOptions(size,strong?juce::Font::bold:juce::Font::plain));
}
void textIn(juce::Graphics& g,const juce::String& text,juce::Rectangle<int> r,
            float size,bool heavy,juce::Colour colour,juce::Justification why=juce::Justification::centred){
 g.setColour(colour);g.setFont(makeFont(size,heavy));
 g.drawFittedText(text,r,why,1);
}
}
void BoomChrome::BrassLook::drawButtonBackground(
 juce::Graphics& g,juce::Button& b,const juce::Colour&,bool hover,bool pressed){
 const auto r=b.getLocalBounds().toFloat().reduced(.7f);
 const auto active=b.getToggleState();
 juce::ColourGradient fill(active?juce::Colour::fromRGB(110,32,44):
                                  juce::Colour::fromRGB(47,22,23),
                          r.getTopLeft(),hover||pressed?burgundy:body,
                          r.getBottomLeft(),false);
 g.setGradientFill(fill);g.fillRoundedRectangle(r,4.f);
 g.setColour(active?gold:hover?copper:gold.withAlpha(.48f));
 g.drawRoundedRectangle(r,4.f,active?1.5f:1.f);
 if(active){g.setColour(gold.withAlpha(.75f));g.fillRect(r.getX()+6.f,r.getBottom()-3.f,
                                                       std::max(0.f,r.getWidth()-12.f),1.5f);}
}
void BoomChrome::BrassLook::drawButtonText(
 juce::Graphics& g,juce::TextButton& b,bool,bool){
 g.setColour(b.isEnabled()?cream:cream.withAlpha(.45f));
 const auto big=b.getName()=="headerStyle"||b.getName()=="headerPreset";
 const float size=big?juce::jlimit(12.f,22.f,float(b.getHeight())*.57f):
   juce::jlimit(9.5f,13.f,float(b.getHeight())*.47f);
 g.setFont(makeFont(size,b.getToggleState()||big));
 g.drawFittedText(b.getButtonText(),b.getLocalBounds().reduced(5,1),
                  juce::Justification::centred,1);
}
BoomChrome::BoomChrome(Area which,OnCommand action)
 :area(which),command(std::move(action)){
 setOpaque(true);
 if(area==Area::Header){
  wireButton(headerButtons[0],Command::Style,"");
  wireButton(headerButtons[1],Command::Previous,"<");
  wireButton(headerButtons[2],Command::Preset,"");
  wireButton(headerButtons[3],Command::Next,">");
  headerButtons[0].setName("headerStyle");
  headerButtons[2].setName("headerPreset");
 }else{
  constexpr Command commands[]{
   Command::Chain,Command::Meters,Command::Settings,Command::Help,Command::Language,
   Command::RecallA,Command::RecallB,Command::StoreA,Command::StoreB,
   Command::CopyAtoB,Command::CopyBtoA,Command::Level
  };
  for(size_t i=0;i<toolbarButtons.size();++i)
   wireButton(toolbarButtons[i],commands[i],"");
 }
 updateButtons();
}
BoomChrome::~BoomChrome(){
 for(auto& b:headerButtons)b.setLookAndFeel(nullptr);
 for(auto& b:toolbarButtons)b.setLookAndFeel(nullptr);
}
void BoomChrome::wireButton(juce::TextButton& button,Command id,juce::String name){
 button.setLookAndFeel(&look);
 button.setButtonText(std::move(name));
 button.setMouseCursor(juce::MouseCursor::PointingHandCursor);
 button.onClick=[this,id]{if(command)command(id);};
 addAndMakeVisible(button);
}
void BoomChrome::setNames(const juce::String& style,const juce::String& preset){
 if(styleName==style&&presetName==preset)return;
 styleName=style;presetName=preset;
 if(area==Area::Header){
  headerButtons[0].setButtonText(styleName);
  headerButtons[2].setButtonText(presetName);
 }
 repaint();
}
void BoomChrome::setState(bool en,int page,bool level,int selected){
 if(english==en&&activePage==page&&levelEnabled==level&&selectedRecall==selected)return;
 english=en;activePage=page;levelEnabled=level;selectedRecall=selected;
 updateButtons();repaint();
}
void BoomChrome::updateButtons(){
 if(area==Area::Header)return;
 const auto local=[this](const char* fr,const char* en){return juce::String(english?en:fr);};
 const juce::String labels[]{
  local("CHA\u00CENE","CHAIN"),local("MESURES","METERS"),
  local("R\u00C9GLAGES","SETTINGS"),local("AIDE","HELP"),
  english?"EN":"FR",
  "A","B",local("SAUV. A","SAVE A"),local("SAUV. B","SAVE B"),
  "A > B","B > A",local("NIVEAU","LEVEL")
 };
 for(size_t i=0;i<toolbarButtons.size();++i)
  if(toolbarButtons[i].getButtonText()!=labels[i])
   toolbarButtons[i].setButtonText(labels[i]);
 constexpr int pagesForButtons[]={0,2,3,4}; // Chain, Meters, Settings, Help
 for(int i=0;i<4;++i)
  toolbarButtons[static_cast<size_t>(i)].setToggleState(activePage==pagesForButtons[i],
                                                        juce::dontSendNotification);
 toolbarButtons[5].setToggleState(selectedRecall==0,juce::dontSendNotification);
 toolbarButtons[6].setToggleState(selectedRecall==1,juce::dontSendNotification);
 toolbarButtons[11].setToggleState(levelEnabled,juce::dontSendNotification);
}
void BoomChrome::resized(){
 const int w=getWidth(),h=getHeight();
 if(area==Area::Header){
  const int y=juce::roundToInt(.34f*h),bh=std::max(23,juce::roundToInt(.36f*h));
  const int leftX=juce::roundToInt(.027f*w),leftW=juce::roundToInt(.235f*w);
  const int arrowW=juce::jlimit(23,34,juce::roundToInt(.025f*w));
  const int rightX=juce::roundToInt(.752f*w);
  const int rightW=juce::roundToInt(.216f*w);
  const int gap=4;
  headerButtons[0].setBounds(leftX,y,leftW,bh);
  headerButtons[1].setBounds(rightX,y,arrowW,bh);
  headerButtons[2].setBounds(rightX+arrowW+gap,y,
                             std::max(80,rightW-arrowW*2-gap*2),bh);
  headerButtons[3].setBounds(rightX+rightW-arrowW,y,arrowW,bh);
  return;
 }
 // Fixed design proportions are scaled uniformly for 13-inch screens.
 const float r=std::min(1.f,float(w)/1230.f);
 const int margin=juce::roundToInt(13.f*r),gap=juce::jmax(3,juce::roundToInt(6.f*r));
 const int bw[]={91,97,109,73,56,38,38,73,73,58,58,88};
 int x=margin;
 const int buttonY=juce::jmax(2,juce::roundToInt(h*.16f));
 const int buttonH=juce::jmax(23,juce::roundToInt(h*.67f));
 for(int i=0;i<5;++i){
  const int width=juce::roundToInt(bw[i]*r);
  toolbarButtons[static_cast<size_t>(i)].setBounds(x,buttonY,width,buttonH);
  x+=width+gap;
 }
 int compareWidth=0;
 for(int i=5;i<12;++i)compareWidth+=juce::roundToInt(bw[i]*r)+gap;
 const int rightX=w-margin-compareWidth;
 x=rightX;
 for(int i=5;i<12;++i){
  const int width=juce::roundToInt(bw[i]*r);
  toolbarButtons[static_cast<size_t>(i)].setBounds(x,buttonY,width,buttonH);
  x+=width+gap;
 }
}
void BoomChrome::paint(juce::Graphics& g){
 const int w=getWidth(),h=getHeight();
 juce::ColourGradient finish(juce::Colour::fromRGB(44,14,18),0.f,0.f,
                              body,0.f,float(h),false);
 g.setGradientFill(finish);g.fillRect(getLocalBounds());
 g.setColour(copper.withAlpha(.65f));
 g.fillRect(0,h-2,w,1);
 if(area==Area::Header){
  const float scale=std::min(float(w)/1450.f,float(h)/100.f);
  textIn(g,english?"MUSICAL STYLE":"STYLE MUSICAL",
         {juce::roundToInt(.03f*w),juce::roundToInt(.10f*h),
          juce::roundToInt(.23f*w),juce::roundToInt(.18f*h)},
         juce::jlimit(9.f,13.f,13.f*scale),true,gold,
         juce::Justification::centredLeft);
  textIn(g,english?"MASTERING PRESET":"PRESET DE MASTERING",
         {juce::roundToInt(.75f*w),juce::roundToInt(.10f*h),
          juce::roundToInt(.218f*w),juce::roundToInt(.18f*h)},
         juce::jlimit(9.f,13.f,13.f*scale),true,gold,
         juce::Justification::centredRight);
  // BOOM is a native inscription; the old JPG header is never drawn.
  textIn(g,"BOOM",
         {juce::roundToInt(.345f*w),juce::roundToInt(-.08f*h),
          juce::roundToInt(.31f*w),juce::roundToInt(.68f*h)},
         juce::jlimit(31.f,71.f,64.f*scale),true,cream);
  textIn(g,"PROFESSIONAL ANALOG MASTERING CONSOLE",
         {juce::roundToInt(.31f*w),juce::roundToInt(.59f*h),
          juce::roundToInt(.38f*w),juce::roundToInt(.18f*h)},
         juce::jlimit(7.5f,11.f,10.f*scale),true,gold);
  textIn(g,"LIBERTY MUSIC SOFTWARE",
         {juce::roundToInt(.36f*w),juce::roundToInt(.77f*h),
          juce::roundToInt(.28f*w),juce::roundToInt(.14f*h)},
         juce::jlimit(8.f,12.f,10.f*scale),false,copper);
  textIn(g,english?"40 styles  /  100 factory masters":
            "40 Styles  /  100 masters d'usine",
         {juce::roundToInt(.75f*w),juce::roundToInt(.76f*h),
          juce::roundToInt(.218f*w),juce::roundToInt(.15f*h)},
         juce::jlimit(8.f,11.f,10.f*scale),false,gold,
         juce::Justification::centredRight);
 }else{
  // All comparison controls are genuine JUCE buttons placed on clean chrome.
  const float r=std::min(1.f,float(w)/1230.f);
  const int labelWidth=juce::roundToInt(78.f*r);
  const int startRight=toolbarButtons[5].getX();
  textIn(g,english?"COMPARE":"COMPARER",
         {std::max(0,startRight-labelWidth-6),0,labelWidth,h},
         juce::jlimit(9.f,12.f,11.f*r),true,gold);
 }
}
