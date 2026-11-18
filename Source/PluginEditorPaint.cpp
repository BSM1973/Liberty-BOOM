#include "PluginEditor.h"
#include "BoomUiTheme.h"
#include <BinaryData.h>
#include <algorithm>
#include <cmath>

namespace {
const auto ivory=boom::ui::theme::ivory();
const auto gold=boom::ui::theme::gold();
const auto copper=boom::ui::theme::copper();
const auto crimson=boom::ui::theme::active();
const auto dark=boom::ui::theme::background();
juce::Font font(float size,bool bold=false){
 return boom::ui::theme::font(size,bold);
}
// Coordinates measured against the original 1672-pixel console artwork.
// Old EQ centres drifted progressively left, by up to 26 px at 10 kHz.
constexpr float chainCX[]={236,505,669,834,1000,1167,1425};
constexpr float chainCY[]={629,744,744,744,744,744,629};
constexpr float browserCX[]={218,497,633,775,914,1052,1331};
constexpr float browserCY[]={676,778,778,778,778,778,676};
// Native EQ bezel covers the ENTIRE photographic knob, including the
// specular ring, while leaving the exterior engraved tick labels in place.
constexpr float chainRadius[]={78,59,59,59,59,59,79};
constexpr float browserRadius[]={71,53,53,53,53,53,73};
const char* keys[]={"inputDb","eq60","eq120","eq3k","eq6k","eq10k","push"};
}

void BoomAudioProcessorEditor::prepareFaces(){
 // EQ control faces are NEVER sampled from reference artwork. Each is
 // constructed entirely from vector graphics in paintKnobs().
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
 // Opaque, aligned controls completely replace the photographic lower buttons.
 paintLowerButtons(g);
 // Native JUCE chain tab view paints over the legacy rack with no duplicate numbers.
 // The native BrowserComponent paints and handles its own panel.
 if(page==Page::meters||page==Page::settings||page==Page::help)paintOther(g);
 paintKnobs(g);
 paintCleanMeterFaces(g);
 paintMeter(g);
 // Header and toolbar text are native JUCE components, never JPG overlays.
}
void BoomAudioProcessorEditor::paintSkin(juce::Graphics& g){
 const auto& skin=page==Page::browser?browserSkin:chainSkin;
 if(skin.isValid()){
  const int headerBottom=page==Page::browser?95:98;
  const int toolbarTop=page==Page::browser?435:343;
  const int toolbarBottom=page==Page::browser?481:396;
  // The JPEG is used ONLY for non-interactive artwork, never for the header
  // or comparison strip: no double printing is possible in those regions.
  for(const auto part:{
       juce::Rectangle<int>(0,headerBottom,artWidth(),toolbarTop-headerBottom),
       juce::Rectangle<int>(0,toolbarBottom,artWidth(),artHeight()-toolbarBottom)}){
   juce::Graphics::ScopedSaveState saved(g);
   g.reduceClipRegion(part);
   g.drawImage(skin,0,0,artWidth(),artHeight(),
               0,0,skin.getWidth(),skin.getHeight());
  }
 }else{
  g.fillAll(juce::Colour::fromRGB(33,10,16));
  g.setColour(ivory);g.setFont(font(26,true));
  g.drawText("BOOM - reference graphique manquante",50,40,artWidth()-100,55,
             juce::Justification::centred);
 }
}
void BoomAudioProcessorEditor::paintReading(
 juce::Graphics& g,juce::Rectangle<float> r,juce::String value,
 bool emphasis,bool centered){
 // Replace only the INSIDE of the value window photographed in the source.
 // The original metal frame, controls and engraved labels stay untouched.
 const auto inner=r.reduced(3.f,3.f);
 g.setColour(dark);g.fillRect(inner);
 g.setColour(emphasis?ivory:gold);
 g.setFont(font(r.getHeight()>25.f?19.f:12.f,emphasis));
 g.drawFittedText(value,inner.toNearestInt().reduced(1,0),
                  centered?juce::Justification::centred:juce::Justification::centredRight,1);
}

void BoomAudioProcessorEditor::paintLowerButtons(juce::Graphics& g){
 // The former photographic controls remain on the JPEG. First mask the
 // COMPLETE rows, including bevels and shadows, then paint exactly one live
 // button per existing BOOMUIMap interaction zone. No image duplication.
 const bool browser=page==Page::browser;
 const auto surround=browser?juce::Colour::fromRGB(25,19,22):
                             juce::Colour::fromRGB(31,12,17);
 const auto row=browser?juce::Rectangle<float>(432.f,498.f,712.f,51.f):
                       juce::Rectangle<float>(448.f,420.f,751.f,53.f);
 const auto lowerRow=browser?juce::Rectangle<float>(115.f,853.f,199.f,47.f):
                            juce::Rectangle<float>(140.f,824.f,204.f,44.f);
 // No extraneous framed background: replace the old photographed controls
 // and their bevels before drawing ONE row of interactive controls.
 for(const auto area:{row,lowerRow}){
  const auto top=surround.brighter(.12f);
  juce::ColourGradient opaque(top,area.getTopLeft(),
                             surround.darker(.10f),area.getBottomLeft(),false);
  g.setGradientFill(opaque);
  g.fillRoundedRectangle(area,4.f);
 }
 const auto* zones=browser?boom::ui::browser:boom::ui::chain;
 const auto count=browser?std::size(boom::ui::browser):
                          std::size(boom::ui::chain);
 const char* ids[]={"meterInput","bypass","mono","inputLink","trimMenu"};
 for(const auto* id:ids){
  for(std::size_t i=0;i<count;++i){
   const auto& z=zones[i];
   if(juce::String(z.id)!=id || z.y<(browser?500.f:410.f))continue;
   const juce::Rectangle<float> r(z.x,z.y,z.w,z.h);
   bool selected=false;
   juce::String label;
   if(juce::String(id)=="meterInput"){
    selected=read("meterInput")>.5f;
    label=english?(selected?"VU / INPUT":"VU / OUTPUT"):
                  (selected?"VU / ENTREE":"VU / SORTIE");
   }else if(juce::String(id)=="bypass"){
    selected=read("bypass")>.5f;label="BYPASS";
   }else if(juce::String(id)=="mono"){
    selected=read("mono")>.5f;
    label=english?"MONO / HOLD":"MONO / MAINTENIR";
   }else if(juce::String(id)=="inputLink"){
    selected=read("inputLink")>.5f;
    label=english?"LINK L/R":"LIEN G/D";
   }else{label="TRIMS";}
   const auto shape=r.reduced(.5f);
   juce::ColourGradient fill(
    selected?juce::Colour::fromRGB(97,29,38):juce::Colour::fromRGB(62,26,31),
    shape.getTopLeft(),dark,shape.getBottomLeft(),false);
   g.setGradientFill(fill);
   g.fillRoundedRectangle(shape,5.f);
   g.setColour(selected?gold:copper.withAlpha(.82f));
   g.drawRoundedRectangle(shape,5.f,selected?1.5f:1.1f);
   if(selected){
    g.setColour(gold.withAlpha(.9f));
    g.fillEllipse(r.getX()+8.f,r.getCentreY()-2.5f,5.f,5.f);
   }
   g.setColour(ivory);
   g.setFont(font(r.getHeight()>=34.f?12.5f:11.5f,selected));
   g.drawFittedText(label,r.toNearestInt().reduced(10,2),
                    juce::Justification::centred,1);
   break;
  }
 }
}

void BoomAudioProcessorEditor::paintKnobs(juce::Graphics& g){
 const bool b=page==Page::browser;
 const float* xs=b?browserCX:chainCX;
 const float* radii=b?browserRadius:chainRadius;
 for(int i=0;i<7;++i){
  const auto* parameter=processor.state().getParameter(keys[i]);
  if(parameter==nullptr)continue;
  const float normalized=juce::jlimit(0.f,1.f,
                           parameter->convertTo0to1(read(keys[i])));
  const float angle=-2.35f+normalized*4.70f;
  const float cx=xs[i],cy=(b?browserCY:chainCY)[i];
  const bool eq=i>=1&&i<=5;
  const float radius=radii[i]-.5f;
  const juce::Rectangle<float> disk(cx-radius,cy-radius,2*radius,2*radius);
  if(eq){
   // Wipe the FULL photographed EQ control, including the old outer copper
   // bezel and white reflection. The old 39 px overlay left a second knob
   // visible; this 5 px opaque margin outside the NEW bezel prevents it.
   // Leave only the engraved exterior scale in the source photograph.
   const float maskRadius=radius+5.f;
   const juce::Rectangle<float> mask(cx-maskRadius,cy-maskRadius,
                                    2.f*maskRadius,2.f*maskRadius);
   const auto bgMid=b?juce::Colour::fromRGB(42,19,23):
                       juce::Colour::fromRGB(51,20,23);
   const auto bgEdge=b?juce::Colour::fromRGB(30,15,18):
                        juce::Colour::fromRGB(33,13,16);
   g.setGradientFill(juce::ColourGradient(
    bgMid,cx,cy,bgEdge,cx+maskRadius,cy,true));
   g.fillEllipse(mask);
  }
  // One complete metal bezel, inner face and moving pointer, all vector.
  // The original photograph is not reused for any part of an EQ knob.
  juce::ColourGradient brass(juce::Colour::fromRGB(107,57,44),
                             disk.getTopLeft(),
                             juce::Colour::fromRGB(26,18,20),
                             disk.getBottomLeft(),false);
  g.setGradientFill(brass);g.fillEllipse(disk);
  g.setColour(juce::Colour::fromRGB(182,109,76).withAlpha(.91f));
  g.drawEllipse(disk.reduced(.8f),eq?2.2f:1.5f);
  if(eq){
   g.setColour(juce::Colour::fromRGB(32,21,23));
   g.drawEllipse(disk.reduced(5.f),3.f);
   g.setColour(gold.withAlpha(.56f));
   g.drawEllipse(disk.reduced(8.f),1.3f);
  }
  const auto inner=disk.reduced(radius*(eq?.24f:.13f));
  juce::ColourGradient core(juce::Colour::fromRGB(65,28,33),
                            inner.getTopLeft(),
                            juce::Colour::fromRGB(12,17,15),
                            inner.getBottomRight(),false);
  g.setGradientFill(core);g.fillEllipse(inner);
  g.setColour(juce::Colour::fromRGB(138,93,70).withAlpha(.70f));
  g.drawEllipse(inner.reduced(.5f),1.f);
  const auto aPoint=[&](float length){
   return juce::Point<float>(cx+std::sin(angle)*length,
                              cy-std::cos(angle)*length);
  };
  const auto start=aPoint(radius*(eq?.31f:.39f));
  const auto tip=aPoint(radius*(eq?.68f:.82f));
  g.setColour(juce::Colour::fromRGB(19,11,11).withAlpha(.68f));
  g.drawLine(start.x+1.3f,start.y+1.3f,tip.x+1.3f,tip.y+1.3f,4.f);
  g.setColour(ivory);
  g.drawLine(start.x,start.y,tip.x,tip.y,juce::jmax(2.f,radius*.065f));
  const bool focus=focusedKnob==i&&hasKeyboardFocus(false);
  if(focus||hoveredKnob==i){
   g.setColour(focus?ivory.withAlpha(.94f):gold.withAlpha(.75f));
   g.drawEllipse(disk.expanded(2.f),focus?2.f:1.3f);
  }
 }

 // Completely replace the original still-image number and its shadows,
 // including trailing dB fragments. Reconstruct the local photographic
 // background from clean pixels at both sides of each strip, rather than
 // putting a conspicuous rectangular black sticker over the console.
 const auto& skin=b?browserSkin:chainSkin;
 for(int i=0;i<7;++i){
  const bool large=i==0||i==6;
  const float width=large?178.f:144.f;
  const int x=juce::roundToInt(xs[i]-width*.5f);
  const int y=juce::roundToInt(large?(b?795.f:750.f):(b?826.f:795.f));
  const int height=large?39:35;
  if(skin.isValid()){
   for(int line=0;line<height;++line){
    const int py=juce::jlimit(0,skin.getHeight()-1,y+line);
    const auto left=skin.getPixelAt(juce::jlimit(0,skin.getWidth()-1,x-8),py);
    const auto right=skin.getPixelAt(
      juce::jlimit(0,skin.getWidth()-1,x+int(width)+8),py);
    g.setGradientFill(juce::ColourGradient(left,float(x),float(py),
                                          right,float(x+width),float(py),false));
    g.fillRect(float(x),float(py),width,1.f);
   }
  }else{
   g.setColour(juce::Colour::fromRGB(35,14,19));
   g.fillRect(x,y,int(width),height);
  }
  const auto text=juce::String(read(keys[i]),1)+(i==6?" %":" dB");
  g.setColour(ivory);
  g.setFont(font(large?20.f:15.f));
  g.drawText(text,x,y+4,int(width),height-7,juce::Justification::centred);
 }
}
void BoomAudioProcessorEditor::paintCleanMeterFaces(juce::Graphics& g){
 // The original photograph has fixed needles, numbers and old erasure
 // smears baked into the glass. Draw a clean, matching analog scale INSIDE
 // the original brass frames. Only the interior is replaced, never the bezel.
 const bool b=page==Page::browser;
 const float radius=b?112.f:123.f;
 const float labelRadius=radius+13.f;
 const float basesX[]={b?597.f:620.f,b?961.f:1037.f};
 const float baseY=b?699.f:632.f;
 const int top=b?570:498;
 const int panelHeight=b?116:125;
 const int panelWidth=b?286:312;
 const auto faceTop=juce::Colour::fromRGB(251,233,189);
 const auto faceBottom=juce::Colour::fromRGB(210,170,115);
 const int dbMarks[]={-20,-10,-7,-5,-3,-2,-1,0,1,2,3};
 const float angles[]={-1.06f,-.83f,-.57f,-.35f,-.15f,
                        .02f,.19f,.37f,.61f,.82f,1.04f};
 for(int channel=0;channel<2;++channel){
  const float cx=basesX[channel],cy=baseY;
  const auto face=juce::Rectangle<float>(cx-panelWidth*.5f,float(top),
                                         float(panelWidth),float(panelHeight));
  juce::ColourGradient glass(faceTop,face.getTopLeft(),
                             faceBottom,face.getBottomLeft(),false);
  g.setGradientFill(glass);
  g.fillRoundedRectangle(face,9.f);
  g.setColour(juce::Colour::fromRGB(91,50,29).withAlpha(.80f));
  g.drawRoundedRectangle(face.reduced(1.f),8.f,1.5f);
  g.setColour(juce::Colour::fromRGB(92,51,39).withAlpha(.18f));
  g.drawRoundedRectangle(face.reduced(4.f),7.f,1.f);
  for(int j=0;j<11;++j){
   const auto angle=angles[j];
   const auto endpoint=[&](float r){
    return juce::Point<float>(cx+std::sin(angle)*r,
                              cy-std::cos(angle)*r);
   };
   const auto a=endpoint(radius-((j==0||j==7||j==10)?14.f:9.f));
   const auto z=endpoint(radius);
   const bool positive=dbMarks[j]>0;
   g.setColour(positive?juce::Colour::fromRGB(150,34,36):
                        juce::Colour::fromRGB(59,42,34));
   g.drawLine(a.x,a.y,z.x,z.y,(j==0||j==7)?2.f:1.3f);
   const auto text=juce::String(dbMarks[j]>0?"+":"")+juce::String(dbMarks[j]);
   const auto t=endpoint(labelRadius);
   g.setFont(font(b?9.f:10.f,true));
   g.drawText(text,juce::roundToInt(t.x-19.f),juce::roundToInt(t.y-8.f),
              38,16,juce::Justification::centred);
  }
  g.setColour(juce::Colour::fromRGB(57,41,33));
  g.setFont(font(b?28.f:31.f,true));
  g.drawText("VU",juce::roundToInt(cx-51.f),juce::roundToInt(cy-85.f),
             102,43,juce::Justification::centred);
  g.setColour(juce::Colour::fromRGB(108,72,48));
  g.setFont(font(b?7.2f:8.f,true));
  g.drawFittedText("LIBERTY MUSIC SOFTWARE",
    juce::roundToInt(cx-105.f),juce::roundToInt(cy-45.f),210,14,
    juce::Justification::centred,1);
  g.setColour(juce::Colour::fromRGB(89,66,49));
  g.setFont(font(9.f,true));
  g.drawText(channel==0?(english?"LEFT":"GAUCHE"):(english?"RIGHT":"DROITE"),
    juce::roundToInt(face.getX()+13.f),juce::roundToInt(face.getBottom()-24.f),
    80,16,juce::Justification::centredLeft);
  g.drawText("300 ms",juce::roundToInt(face.getRight()-73.f),
    juce::roundToInt(face.getBottom()-24.f),61,16,juce::Justification::centredRight);
 }
}

void BoomAudioProcessorEditor::paintMeter(juce::Graphics& g){
 const bool b=page==Page::browser;
 const float basesX[]={b?597.f:620.f,b?961.f:1037.f};
 const float baseY=b?699.f:632.f,length=b?109.f:119.f;
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
   g.drawFittedText(" OFF",r.toNearestInt(),
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
