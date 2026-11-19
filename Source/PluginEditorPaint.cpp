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
constexpr float browserCY[]={676,797,797,797,797,797,676};
// The new knobs are completely vector-drawn. Artwork radii describe the
// live metal bezel, NOT the radius used to erase old photographic controls.
constexpr float chainRadius[]={86,46,46,46,46,46,86};
constexpr float browserRadius[]={80,41,41,41,41,41,80};
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
 // One native surface replaces the ENTIRE lower photographed console.
 // The reference JPEG is still present outside this panel, but NO old
 // knobs, labels, values, switches, rings or shadows can bleed through.
 const bool b=page==Page::browser;
 const float w=float(artWidth());
 const float top=b?485.f:400.f;
 const float bottom=b?931.f:872.f;
 const juce::Rectangle<float> face(40.f,top,w-80.f,bottom-top);
 juce::ColourGradient panel(juce::Colour::fromRGB(63,25,29),
                            face.getTopLeft(),juce::Colour::fromRGB(28,12,17),
                            face.getBottomLeft(),false);
 g.setGradientFill(panel);g.fillRoundedRectangle(face,5.f);
 g.setColour(copper.withAlpha(.45f));
 g.drawRoundedRectangle(face.reduced(.6f),5.f,1.1f);
 // Faint texture rails are decorative; they never hide captions or controls.
 g.setColour(gold.withAlpha(.09f));
 g.drawHorizontalLine(juce::roundToInt(top+5.f),face.getX()+10.f,face.getRight()-10.f);
 g.drawHorizontalLine(juce::roundToInt(bottom-4.f),face.getX()+10.f,face.getRight()-10.f);
 auto label=[&](const juce::String& value,float x,float y,float width,
                float height,float size,bool bold,juce::Colour ink){
  g.setColour(ink);g.setFont(font(size,bold));
  g.drawFittedText(value,juce::Rectangle<int>(
   juce::roundToInt(x),juce::roundToInt(y),
   juce::roundToInt(width),juce::roundToInt(height)),
   juce::Justification::centred,1);
 };
 label("LIBERTY / STEREO MASTERING ENGINE",58.f,top+11.f,
       b?344.f:390.f,18.f,11.5f,true,gold);
 label("THE POWER BEHIND YOUR MASTER.",w*.34f,
       b?top:top+6.f,w*.32f,19.f,14.f,true,ivory);
 label("MODEL B-01",w-208.f,top+11.f,158.f,18.f,11.5f,true,gold);
 label(english?"ANALOG INPUT STAGE":"ETAGE D'ENTREE ANALOGIQUE",
       110.f,b?543.f:456.f,254.f,18.f,13.f,true,gold);
 label(english?"MASTERING INTENSITY":"INTENSITE DE MASTERING",
       w-364.f,b?543.f:456.f,254.f,18.f,13.f,true,gold);
 const float eqTitle=b?708.f:648.f;
 const float eqCentre=(b?775.f:835.f);
 label(english?"MUSICAL MASTERING EQ - 5 BANDS":
               "EGALISEUR MUSICAL DE MASTERING - 5 BANDES",
       eqCentre-310.f,eqTitle,620.f,18.f,14.f,true,gold);
 g.setColour(copper.withAlpha(.48f));
 g.drawLine(eqCentre-409.f,eqTitle+11.f,eqCentre-319.f,eqTitle+11.f,1.f);
 g.drawLine(eqCentre+319.f,eqTitle+11.f,eqCentre+409.f,eqTitle+11.f,1.f);

 // Repaint the actual controls at their unique mapped click rectangles;
 // no shared opaque button-row stripe can cross the slogan.
 const auto* zones=b?boom::ui::browser:boom::ui::chain;
 const auto count=b?std::size(boom::ui::browser):
                    std::size(boom::ui::chain);
 const char* ids[]={"meterInput","bypass","mono","inputLink","trimMenu"};
 for(const auto* id:ids){
  for(std::size_t n=0;n<count;++n){
   const auto& z=zones[n];
   if(juce::String(z.id)!=id||z.y<(b?500.f:410.f))continue;
   const juce::Rectangle<float> rect(z.x,z.y,z.w,z.h);
   bool selected=false;
   juce::String title;
   if(juce::String(id)=="meterInput"){
    selected=read("meterInput")>.5f;
    title=english?(selected?"VU / INPUT":"VU / OUTPUT"):
                   (selected?"VU / ENTREE":"VU / SORTIE");
   }else if(juce::String(id)=="bypass"){
    selected=read("bypass")>.5f;title="BYPASS";
   }else if(juce::String(id)=="mono"){
    selected=read("mono")>.5f;
    title=english?"MONO / HOLD":"MONO / MAINTENIR";
   }else if(juce::String(id)=="inputLink"){
    selected=read("inputLink")>.5f;
    title=english?"LINK L/R":"LIEN G/D";
   }else title="TRIMS";
   const auto button=rect.reduced(.8f);
   juce::ColourGradient fill(
       selected?juce::Colour::fromRGB(107,34,47):
                juce::Colour::fromRGB(66,27,32),
       button.getTopLeft(),juce::Colour::fromRGB(27,14,20),
       button.getBottomLeft(),false);
   g.setGradientFill(fill);g.fillRoundedRectangle(button,4.f);
   g.setColour(selected?gold:copper.withAlpha(.92f));
   g.drawRoundedRectangle(button,4.f,selected?1.6f:1.1f);
   if(selected){
    g.setColour(gold);
    g.fillEllipse(button.getX()+7.f,button.getCentreY()-2.5f,5.f,5.f);
   }
   label(title,rect.getX()+5.f,rect.getY()+2.f,
         rect.getWidth()-10.f,rect.getHeight()-4.f,
         rect.getHeight()>34.f?12.2f:11.8f,selected,ivory);
   break;
  }
 }
 // Clear, independent captions: fixed gaps before and after value readings.
 const float largeName=b?833.f:784.f;
 const float secondary=b?862.f:815.f;
 label("INPUT DRIVE",102.f,largeName,268.f,25.f,23.f,true,ivory);
 label("PUSH",w-370.f,largeName,268.f,25.f,23.f,true,ivory);
 label(english?"CHAIN INPUT GAIN":"GAIN A L'ENTREE DE LA CHAINE",
       100.f,secondary,272.f,15.f,11.5f,false,gold);
 label(english?"ENERGY - DENSITY - IMPACT":"ENERGIE - DENSITE - IMPACT",
       w-375.f,secondary,278.f,15.f,11.5f,false,gold);
 if(b){
  label("SUBTIL",w-349.f,893.f,80.f,15.f,10.f,false,gold);
  label("AFFIRME",w-180.f,893.f,83.f,15.f,10.f,false,gold);
  g.setColour(gold.withAlpha(.65f));
  g.drawLine(w-262.f,902.f,w-186.f,902.f,1.3f);
 }else{
  label("SUBTIL",w-349.f,844.f,80.f,15.f,10.f,false,gold);
  label("AFFIRME",w-180.f,844.f,83.f,15.f,10.f,false,gold);
  g.setColour(gold.withAlpha(.65f));
  g.drawLine(w-262.f,852.f,w-186.f,852.f,1.3f);
 }
}

void BoomAudioProcessorEditor::paintKnobs(juce::Graphics& g){
 // No legacy bitmap layers, circular erasure decals or duplicated scales.
 // All seven knobs, their legends and readouts use one native coordinate grid.
 const bool b=page==Page::browser;
 const float* centres=b?browserCX:chainCX;
 const float* ys=b?browserCY:chainCY;
 const float* radii=b?browserRadius:chainRadius;
 const char* bands[]={"60 Hz","120 Hz","3 kHz","6 kHz","10 kHz"};
 const char* captionsFR[]={"FONDATION","IMPACT","PRESENCE","DETAIL","AIR"};
 const char* captionsEN[]={"FOUNDATION","IMPACT","PRESENCE","DETAIL","AIR"};
 for(int i=0;i<7;++i){
  const auto* parameter=processor.state().getParameter(keys[i]);
  if(parameter==nullptr)continue;
  const float normalized=juce::jlimit(0.f,1.f,
                            parameter->convertTo0to1(read(keys[i])));
  const float angle=-2.35f+normalized*4.70f;
  const bool eq=i>=1&&i<=5;
  const float cx=centres[i],cy=ys[i],radius=radii[i];
  // Ticks are completely separate from the knob: each scale is drawn ONCE.
  const float tickInner=radius+(eq?4.f:6.f);
  const float tickOuter=radius+(eq?10.f:17.f);
  for(int k=0;k<=10;++k){
   const float a=-2.35f+4.70f*float(k)/10.f;
   const float sx=std::sin(a),sy=-std::cos(a);
   const float outer=tickOuter+(k%5==0?3.f:0.f);
   g.setColour(k==5?ivory:gold.withAlpha(k%5==0?.87f:.49f));
   g.drawLine(cx+sx*tickInner,cy+sy*tickInner,
              cx+sx*outer,cy+sy*outer,k==5?2.1f:1.1f);
  }
  // Three numeric references with generous gutters around the value field.
  g.setFont(font(eq?10.5f:13.f,true));
  for(int k:{0,5,10}){
   const float a=-2.35f+4.70f*float(k)/10.f;
   const float textR=radius+(eq?19.f:32.f);
   const float px=cx+std::sin(a)*textR;
   const float py=cy-std::cos(a)*textR;
   juce::String caption;
   if(eq){
    const int span=(i==1||i==5)?8:10;
    caption=k==5?"0":(k==0?"-"+juce::String(span):
                                 "+"+juce::String(span));
   }else if(i==0)caption=k==5?"0":(k==0?"-18":"+18");
   else caption=k==5?"50":(k==0?"0":"100");
   g.setColour(ivory);
   g.drawFittedText(caption,
    juce::roundToInt(px-21.f),juce::roundToInt(py-8.f),42,16,
    juce::Justification::centred,1);
  }
  const juce::Rectangle<float> disk(cx-radius,cy-radius,2.f*radius,2.f*radius);
  juce::ColourGradient brass(juce::Colour::fromRGB(108,55,45),
                             disk.getTopLeft(),
                             juce::Colour::fromRGB(25,15,19),
                             disk.getBottomRight(),false);
  g.setGradientFill(brass);g.fillEllipse(disk);
  g.setColour(juce::Colour::fromRGB(199,131,90));
  g.drawEllipse(disk.reduced(1.f),eq?2.2f:3.f);
  g.setColour(juce::Colour::fromRGB(29,19,22));
  g.drawEllipse(disk.reduced(eq?5.f:8.f),eq?2.6f:3.8f);
  g.setColour(gold.withAlpha(.63f));
  g.drawEllipse(disk.reduced(eq?8.f:12.f),1.f);
  const auto inner=disk.reduced(radius*.25f);
  juce::ColourGradient core(juce::Colour::fromRGB(70,33,37),
                            inner.getTopLeft(),
                            juce::Colour::fromRGB(16,13,18),
                            inner.getBottomRight(),false);
  g.setGradientFill(core);g.fillEllipse(inner);
  g.setColour(juce::Colour::fromRGB(137,82,68).withAlpha(.7f));
  g.drawEllipse(inner.reduced(.5f),1.f);
  // Exactly one position pointer (and its subtle dark shadow).
  const auto position=[&](float len){
   return juce::Point<float>(cx+std::sin(angle)*len,
                              cy-std::cos(angle)*len);
  };
  const auto start=position(radius*.27f);
  const auto tip=position(radius*.73f);
  g.setColour(juce::Colour::fromRGB(21,12,14).withAlpha(.8f));
  g.drawLine(start.x+1.f,start.y+1.f,tip.x+1.f,tip.y+1.f,4.f);
  g.setColour(ivory);
  g.drawLine(start.x,start.y,tip.x,tip.y,eq?3.f:4.5f);
  const bool focus=focusedKnob==i&&hasKeyboardFocus(false);
  if(focus||hoveredKnob==i){
   g.setColour(focus?ivory.withAlpha(.94f):gold.withAlpha(.75f));
   g.drawEllipse(disk.expanded(2.f),focus?2.f:1.3f);
  }

  const float valY=eq?(b?853.f:804.f):(b?799.f:745.f);
  const float valueWidth=eq?122.f:200.f;
  g.setColour(ivory);g.setFont(font(eq?15.f:21.f,true));
  g.drawFittedText(juce::String(read(keys[i]),1)+(i==6?" %":" dB"),
    juce::roundToInt(cx-valueWidth*.5f),juce::roundToInt(valY),
    juce::roundToInt(valueWidth),eq?22:29,juce::Justification::centred,1);
  if(eq){
   const int band=i-1;
   g.setColour(gold);g.setFont(font(16.f,true));
   g.drawFittedText(bands[band],juce::roundToInt(cx-70.f),
       b?878:830,140,21,juce::Justification::centred,1);
   g.setColour(ivory.withAlpha(.78f));g.setFont(font(11.5f));
   g.drawFittedText(english?captionsEN[band]:captionsFR[band],
       juce::roundToInt(cx-75.f),b?902:852,150,18,
       juce::Justification::centred,1);
  }
 }
}

void BoomAudioProcessorEditor::paintCleanMeterFaces(juce::Graphics& g){
 // Entire stereo VU frames and faces are native, not old photographic layers.
 // The two meter needles are drawn afterwards in paintMeter().
 const bool b=page==Page::browser;
 const float centres[]={b?597.f:620.f,b?961.f:1037.f};
 const float cy=b?699.f:632.f;
 const float frameTop=b?549.f:477.f;
 const float frameWidth=b?337.f:354.f;
 const float frameHeight=b?157.f:165.f;
 const int values[]={-20,-10,-7,-5,-3,-2,-1,0,1,2,3};
 const float angles[]={-1.06f,-.83f,-.57f,-.35f,-.15f,
                       .02f,.19f,.37f,.61f,.82f,1.04f};
 for(int channel=0;channel<2;++channel){
  const float cx=centres[channel];
  const juce::Rectangle<float> frame(cx-frameWidth*.5f,frameTop,
                                     frameWidth,frameHeight);
  juce::ColourGradient surround(juce::Colour::fromRGB(123,81,48),
                                frame.getTopLeft(),
                                juce::Colour::fromRGB(35,21,19),
                                frame.getBottomLeft(),false);
  g.setGradientFill(surround);g.fillRoundedRectangle(frame,8.f);
  g.setColour(gold.withAlpha(.61f));
  g.drawRoundedRectangle(frame.reduced(1.f),7.f,1.4f);
  const auto inset=frame.reduced(8.f,8.f);
  g.setColour(juce::Colour::fromRGB(20,14,15));
  g.fillRoundedRectangle(inset,6.f);
  const auto face=inset.reduced(6.f,5.f);
  juce::ColourGradient paper(juce::Colour::fromRGB(255,237,192),
                             face.getTopLeft(),
                             juce::Colour::fromRGB(209,171,113),
                             face.getBottomLeft(),false);
  g.setGradientFill(paper);g.fillRoundedRectangle(face,5.f);
  g.setColour(juce::Colour::fromRGB(107,75,48));
  g.drawRoundedRectangle(face.reduced(.5f),5.f,1.1f);
  const float tickR=b?108.f:117.f;
  const float labelR=b?119.f:128.f;
  for(int k=0;k<11;++k){
   const float a=angles[k];
   const auto at=[&](float radius){
    return juce::Point<float>(cx+std::sin(a)*radius,
                              cy-std::cos(a)*radius);
   };
   const auto from=at(tickR-(k==7?14.f:9.f));
   const auto to=at(tickR);
   const bool hot=values[k]>0;
   g.setColour(hot?juce::Colour::fromRGB(158,42,41):
                    juce::Colour::fromRGB(65,44,36));
   g.drawLine(from.x,from.y,to.x,to.y,k==7?1.9f:1.2f);
   const auto pos=at(labelR);
   const auto label=juce::String(values[k]>0?"+":"")+juce::String(values[k]);
   g.setFont(font(b?9.f:10.f,true));
   g.drawFittedText(label,juce::roundToInt(pos.x-18.f),
         juce::roundToInt(pos.y-8.f),36,16,juce::Justification::centred,1);
  }
  g.setColour(juce::Colour::fromRGB(57,43,34));
  g.setFont(font(31.f,true));
  g.drawText("VU",juce::roundToInt(cx-49.f),juce::roundToInt(cy-83.f),
             98,40,juce::Justification::centred);
  g.setColour(juce::Colour::fromRGB(109,75,51));
  g.setFont(font(8.f,true));
  g.drawFittedText("LIBERTY MUSIC SOFTWARE",
       juce::roundToInt(cx-108.f),juce::roundToInt(cy-42.f),
       216,14,juce::Justification::centred,1);
  g.setFont(font(9.f,true));
  g.drawText(channel==0?(english?"LEFT":"GAUCHE"):
                       (english?"RIGHT":"DROITE"),
       juce::roundToInt(face.getX()+11.f),
       juce::roundToInt(face.getBottom()-19.f),75,13,
       juce::Justification::centredLeft);
  g.drawText("300 ms",juce::roundToInt(face.getRight()-73.f),
       juce::roundToInt(face.getBottom()-19.f),61,13,
       juce::Justification::centredRight);
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
