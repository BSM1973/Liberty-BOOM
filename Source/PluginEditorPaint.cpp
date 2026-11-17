#include "PluginEditor.h"
#include <BinaryData.h>
#include <algorithm>
#include <cmath>

namespace {
const auto ivory=juce::Colour::fromRGB(248,228,191);
const auto gold=juce::Colour::fromRGB(228,192,133);
const auto copper=juce::Colour::fromRGB(189,95,74);
const auto crimson=juce::Colour::fromRGB(110,22,35);
const auto dark=juce::Colour::fromRGB(22,10,13);
juce::Font font(float size,bool bold=false){
 return juce::Font(juce::FontOptions(size,bold?juce::Font::bold:juce::Font::plain));
}
constexpr float chainCX[]={236,503,663,822,981,1141,1425};
constexpr float chainCY[]={629,744,744,744,744,744,629};
constexpr float browserCX[]={218,497,633,775,914,1052,1331};
constexpr float browserCY[]={676,778,778,778,778,778,676};
constexpr float chainRadius[]={78,39,39,39,39,39,79};
constexpr float browserRadius[]={71,35,35,35,35,35,73};
const char* keys[]={"inputDb","eq60","eq120","eq3k","eq6k","eq10k","push"};
}

void BoomAudioProcessorEditor::prepareFaces(){
 auto capture=[&](const juce::Image& src,std::array<juce::Image,7>& faces,
                  const float* xs,const float* ys,const float* radii){
  if(!src.isValid())return;
  for(int i=0;i<7;++i){
   const int r=juce::roundToInt(radii[i]);
   faces[static_cast<std::size_t>(i)]=src.getClippedImage(
     {juce::roundToInt(xs[i])-r,juce::roundToInt(ys[i])-r,2*r,2*r});
  }
 };
 capture(chainSkin,chainFaces,chainCX,chainCY,chainRadius);
 capture(browserSkin,browserFaces,browserCX,browserCY,browserRadius);
 // Remove the photographed idle needles once, from the in-memory art only.
 // Needle animations therefore do not draw over the old frozen needles.
 if(chainSkin.isValid()){
  chainSkin=chainSkin.createCopy();
  eraseReferenceNeedle(chainSkin,{620,632},{501,541});
  eraseReferenceNeedle(chainSkin,{1037,632},{918,541});
 }
 if(browserSkin.isValid()){
  browserSkin=browserSkin.createCopy();
  eraseReferenceNeedle(browserSkin,{597,699},{478,613});
  eraseReferenceNeedle(browserSkin,{961,699},{843,613});
 }
}
void BoomAudioProcessorEditor::eraseReferenceNeedle(
 juce::Image& img,juce::Point<float> start,juce::Point<float> end){
 if(!img.isValid())return;
 const auto original=img.createCopy();
 const auto d=end-start;
 const float length=std::sqrt(d.x*d.x+d.y*d.y);
 if(length<1.f)return;
 const auto normal=juce::Point<float>(-d.y/length,d.x/length);
 const int segments=juce::roundToInt(length);
 for(int t=0;t<=segments;++t){
  const auto p=start+d*(float(t)/float(juce::jmax(1,segments)));
  for(int side=-5;side<=5;++side){
   const auto point=p+normal*float(side);
   const int x=juce::roundToInt(point.x),y=juce::roundToInt(point.y);
   if(x<=1||y<=1||x>=img.getWidth()-2||y>=img.getHeight()-2)continue;
   const auto left=p+normal*-9.f,right=p+normal*9.f;
   const auto first=original.getPixelAt(
     juce::jlimit(0,img.getWidth()-1,juce::roundToInt(left.x)),
     juce::jlimit(0,img.getHeight()-1,juce::roundToInt(left.y)));
   const auto second=original.getPixelAt(
     juce::jlimit(0,img.getWidth()-1,juce::roundToInt(right.x)),
     juce::jlimit(0,img.getHeight()-1,juce::roundToInt(right.y)));
   img.setPixelAt(x,y,first.interpolatedWith(second,(side+5.f)/10.f));
  }
 }
}
void BoomAudioProcessorEditor::paint(juce::Graphics& g){
 juce::Graphics::ScopedSaveState saved(g);
 g.addTransform(juce::AffineTransform::scale(
  float(getWidth())/artWidth(),float(getHeight())/artHeight()));
 paintSkin(g);
 // Native JUCE chain tab view paints over the legacy rack with no duplicate numbers.
 // The native BrowserComponent paints and handles its own panel.
 if(page==Page::meters||page==Page::settings||page==Page::help)paintOther(g);
 paintKnobs(g);
 paintMeter(g);
 paintHead(g);
}
void BoomAudioProcessorEditor::paintSkin(juce::Graphics& g){
 const auto& skin=page==Page::browser?browserSkin:chainSkin;
 if(skin.isValid())
  g.drawImage(skin,0,0,artWidth(),artHeight(),
                0,0,skin.getWidth(),skin.getHeight());
 else{
  g.fillAll(juce::Colour::fromRGB(33,10,16));
  g.setColour(ivory);g.setFont(font(26,true));
  g.drawText("BOOM — référence graphique manquante",50,40,artWidth()-100,55,
             juce::Justification::centred);
 }
}
void BoomAudioProcessorEditor::paintReading(
 juce::Graphics& g,juce::Rectangle<float> r,juce::String value,bool emphasis){
 // Replace only the INSIDE of the value window photographed in the source.
 // The original metal frame, controls and engraved labels stay untouched.
 const auto inner=r.reduced(3.f,3.f);
 g.setColour(dark);g.fillRect(inner);
 g.setColour(emphasis?ivory:gold);
 g.setFont(font(r.getHeight()>25.f?19.f:12.f,emphasis));
 g.drawFittedText(value,inner.toNearestInt().reduced(1,0),
                  juce::Justification::centredRight,1);
}
void BoomAudioProcessorEditor::paintHead(juce::Graphics& g){
 const bool browser=page==Page::browser;
 const int style=processor.styleIndex(),preset=processor.presetIndex();
 const auto styleName=style>=0&&style<40?
   juce::String(boom::styles()[static_cast<std::size_t>(style)].name):
   (english?"Choose a Style":"Choisir un Style");
 const auto presetName=preset>=0&&preset<100?
   juce::String(boom::makePreset(preset).name):
   (english?"Custom Master":"Master personnalisé");
 // The actual photographed dropdowns remain. Only variable text is refreshed.
 auto fillText=[&](int x,int y,int w,int h,juce::String text){
  g.setColour(juce::Colour::fromRGB(25,9,12));
  g.fillRect(x,y,w,h);
  g.setColour(ivory);g.setFont(font(browser?21.f:23.f,true));
  g.drawFittedText(text,x+2,y,w-4,h,juce::Justification::centredLeft,1);
 };
 if(browser){
  fillText(72,37,286,28,styleName);
  fillText(1235,36,233,28,presetName);
 }else{
  fillText(132,37,285,29,styleName);
  fillText(1241,38,271,29,presetName);
 }
}
void BoomAudioProcessorEditor::paintKnobs(juce::Graphics& g){
 const bool b=page==Page::browser;
 const auto& faces=b?browserFaces:chainFaces;
 const float* xs=b?browserCX:chainCX;
 const float* ys=b?browserCY:chainCY;
 const float* radii=b?browserRadius:chainRadius;
 for(int i=0;i<7;++i){
  const auto& src=faces[static_cast<std::size_t>(i)];
  if(!src.isValid())continue;
  const auto* param=processor.state().getParameter(keys[i]);
  if(param==nullptr)continue;
  const auto range=param->getNormalisableRange();
  const float v=read(keys[i]);
  const float angle=i==6?4.5f*juce::jlimit(0.f,1.f,v/100.f):
    (v-((range.start+range.end)*.5f))*
     (2.32f/juce::jmax(.01f,(range.end-range.start)*.5f));
  const float cx=xs[i],cy=ys[i],r=radii[i];
  juce::Path face;face.addEllipse(cx-r,cy-r,2*r,2*r);
  juce::Graphics::ScopedSaveState saved(g);
  g.reduceClipRegion(face);
  // The photographed rotary face actually rotates: no false dot, no new knob.
  const auto transform=juce::AffineTransform::rotation(angle,r,r)
    .translated(cx-r,cy-r);
  g.drawImageTransformed(src,transform);
 }
 // Dynamic gain figures replace the original static readings inside the skin.
 const float* y=b?browserCY:chainCY;
 for(int i=0;i<7;++i){
  const bool large=i==0||i==6;
  const float width=large?134.f:86.f;
  const float xx=xs[i]-width*.5f;
  const float yy=large?(b?802.f:757.f):(b?833.f:802.f);
  const float height=large?29.f:23.f;
  const auto text=juce::String(read(keys[i]),1)+(i==6?" %":" dB");
  // Only the original *number*, not any knob or caption, is replaced.
  g.setColour(juce::Colour::fromRGB(28,11,15));
  g.fillRect(xx,yy,width,height);
  g.setColour(ivory);
  g.setFont(font(large?20.f:15.f));
  g.drawText(text,juce::roundToInt(xx),juce::roundToInt(yy),
             juce::roundToInt(width),juce::roundToInt(height),
             juce::Justification::centred);
 }
 (void)y;
}
void BoomAudioProcessorEditor::paintMeter(juce::Graphics& g){
 const bool b=page==Page::browser;
 const float basesX[]={b?597.f:620.f,b?961.f:1037.f};
 const float baseY=b?699.f:632.f,length=b?133.f:140.f;
 const float levels[]={vuL,vuR};
 for(int c=0;c<2;++c){
  const float angle=juce::jmap(juce::jlimit(-20.f,3.f,levels[c]),
                               -20.f,3.f,-1.1f,1.1f);
  const auto p=juce::Point<float>(basesX[c],baseY);
  const auto tip=p+juce::Point<float>(std::sin(angle),-std::cos(angle))*length;
  g.setColour(juce::Colours::black.withAlpha(.40f));
  g.drawLine(p.x+1.f,p.y+2.f,tip.x+1.f,tip.y+2.f,4.f);
  g.setColour(juce::Colour::fromRGB(71,42,25));
  g.drawLine(p.x,p.y,tip.x,tip.y,2.2f);
  g.setColour(gold);g.fillEllipse(p.x-3.f,p.y-3.f,6.f,6.f);
 }
 // The tiny CLIP indicator is latched on audio peaks until the user resets it.
 if(clipHold){
  const int x=b?1156:1460,y=b?930:904;
  g.setColour(juce::Colours::red.withAlpha(.82f));
  g.fillEllipse(float(x),float(y),6.f,6.f);
 }
}
void BoomAudioProcessorEditor::paintSwitch(
 juce::Graphics& g,const boom::ui::Zone& z){
 const juce::String id(z.id);
 const bool isBypass=id.endsWith("Bypass")||id=="outputBypass";
 const bool active=isBypass?read(z.id)<.5f:read(z.id)>.5f;
 if(isBypass){
  // Active is the photograph's untouched ACTIF hardware switch.
  // Only OFF needs to replace its printed text; do not draw a second button.
  if(!active && z.w>40.f && z.y<340.f){
   const auto r=juce::Rectangle<float>(z.x+5,z.y+3,z.w-10,z.h-6);
   g.setColour(dark);g.fillRect(r);
   g.setColour(copper);g.setFont(font(10,true));
   g.drawFittedText("○ OFF",r.toNearestInt(),
                    juce::Justification::centred,1);
  }
  return;
 }
 // For already-printed switches, only the small built-in LED changes colour.
 const float radius=z.w>=100.f?4.f:3.f;
 g.setColour(active?gold:juce::Colour::fromRGB(70,48,40));
 g.fillEllipse(z.x+6.f,z.y+z.h*.5f-radius,radius*2,radius*2);
}
void BoomAudioProcessorEditor::paintChain(juce::Graphics& g){
 for(const auto& z:boom::ui::chain){
  if(z.kind==boom::ui::Kind::field){
   const juce::String id(z.id);
   float scale=1.f;
   int decimals=1;
   if(id=="deFreq"){scale=.001f;decimals=2;}
   else if(id=="colorMix"||id=="deAmt"||id=="compMix")scale=100.f;
   else if(id=="colorPushCharacter")decimals=0;
   else if(id=="compRatio")decimals=2;
   else if(id=="deRel"||id=="compAttack"||id=="compRelease"||
           id=="limitRelease")decimals=0;
   paintReading(g,{z.x,z.y,z.w,z.h},juce::String(read(z.id)*scale,decimals));
  }else if(z.kind==boom::ui::Kind::toggle)paintSwitch(g,z);
 }
 const char* modes[]={"CLEAN","TUBE","TAPE"};
 const int color=juce::jlimit(0,2,int(read("color")+.5f));
 paintReading(g,{217.f,180.f,89.f,25.f},modes[color]);
 const auto m=processor.lastMeters();
 struct Gr {float x,y,w;float value;};
 const std::array<Gr,3> reductions{{{764,309,46,m.compressorGainReductionDb},
                                    {1246,309,46,m.limiterGainReductionDb},
                                    {1499,307,78,m.outputPeakL>=1.f||m.outputPeakR>=1.f?1.f:0.f}}};
 for(int i=0;i<2;++i){
  const auto& gr=reductions[static_cast<size_t>(i)];
  g.setColour(dark);g.fillRect(gr.x,gr.y,gr.w,16.f);
  g.setColour(gold);g.setFont(font(11));
  g.drawText(juce::String(gr.value,1)+" dB",int(gr.x),int(gr.y),
             int(gr.w),16,juce::Justification::centredRight);
 }
}
