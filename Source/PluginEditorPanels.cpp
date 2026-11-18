#include "PluginEditor.h"
#include "BoomUiTheme.h"
#include "BoomSafeText.h"
#include <algorithm>
#include <cmath>

namespace {
const auto ivory=boom::ui::theme::ivory();
const auto gold=boom::ui::theme::gold();
const auto copper=boom::ui::theme::copper();
const auto crimson=boom::ui::theme::active();
const auto dark=boom::ui::theme::background();
juce::Font font(float size,bool heavy=false){
 return boom::ui::theme::font(size,heavy);
}
juce::String formatDb(float v){return v<=-90.f?" - ":juce::String(v,1);}
}

void BoomAudioProcessorEditor::paintBrowser(juce::Graphics& g){
 // A single dynamic list inside the panel in the reference photograph.
 // No second search/filter/button widgets are painted above the photographed ones.
 g.setColour(dark);g.fillRect(376,210,731,176);
 const int visible=grid?6:5;
 for(int i=0;i<visible;++i){
  const int index=firstVisible+i;
  if(index>=int(results.size()))break;
  const auto row=results[static_cast<std::size_t>(index)];
  juce::String name,group,style;
  if(row.isUser){
   name=processor.userPresetName(row.index);
   group=english?"User":"Utilisateur";style=" - ";
  }else{
   const auto preset=boom::makePreset(row.index);
   name=juce::String(preset.name);
   group=juce::String(boom::categories[static_cast<std::size_t>(preset.category)]);
   style=juce::String(boom::styles()[static_cast<std::size_t>(preset.style)].name);
  }
  const bool active=index==selectedRow;
  if(grid){
   const int x=376+(i%2)*366,y=210+(i/2)*59;
   g.setColour(active?crimson:juce::Colour::fromRGB(28,16,19));
   g.fillRoundedRectangle(float(x),float(y+1),360.f,54.f,4.f);
   g.setColour(active?gold:copper.withAlpha(.45f));
   g.drawRoundedRectangle(float(x),float(y+1),360.f,54.f,4.f,1.f);
   g.setColour(ivory);g.setFont(font(14,active));
   g.drawFittedText(boom::ui::text::safeUpperText(name),x+13,y+4,308,25,juce::Justification::centredLeft,1);
   g.setColour(gold);g.setFont(font(11));
   g.drawFittedText(boom::ui::text::safeUpperText(group),x+13,y+30,305,17,juce::Justification::centredLeft,1);
   g.setColour(row.isUser?copper:processor.isFavorite(row.index)?gold:gold.withAlpha(.4f));
   g.setFont(font(10,true));
   g.drawText(row.isUser?"USER":processor.isFavorite(row.index)?"FAV":"",
              x+320,y+6,35,42,juce::Justification::centred);
  }else{
   const int y=210+i*35;
   g.setColour(active?crimson:(i%2?juce::Colour::fromRGB(32,17,20):dark));
   g.fillRect(376,y,731,34);
   if(active){g.setColour(copper);g.fillRect(376,y,3,34);}
   g.setColour(ivory);g.setFont(font(15,active));
   g.drawFittedText(boom::ui::text::safeUpperText(name),405,y+1,262,31,juce::Justification::centredLeft,1);
   g.setColour(gold);g.setFont(font(13));
   g.drawFittedText(boom::ui::text::safeUpperText(group),680,y+1,209,31,juce::Justification::centredLeft,1);
   g.drawFittedText(boom::ui::text::safeUpperText(style),901,y+1,164,31,juce::Justification::centredLeft,1);
   g.setColour(row.isUser?copper:processor.isFavorite(row.index)?gold:gold.withAlpha(.5f));
   g.setFont(font(10,true));
   g.drawText(row.isUser?"USER":processor.isFavorite(row.index)?"FAV":"",
              1068,y+1,37,31,juce::Justification::centred);
   g.setColour(copper.withAlpha(.20f));g.drawHorizontalLine(y+33,376.f,1108.f);
  }
 }
 // Highlight categories without drawing another button over the photo.
 if(category>=0&&category<=11){
  const int top=138+category*22;
  g.setColour(gold);g.fillRoundedRectangle(30.f,float(top+2),3.f,17.f,1.f);
 }
 // Live counts replace only the original numerical badges.
 int favorites=0;for(int i=0;i<100;++i)if(processor.isFavorite(i))++favorites;
 for(const auto& item:std::array<std::pair<int,int>,2>{{{364,favorites},
                                                        {387,processor.userPresetCount()}}}){
  g.setColour(juce::Colour::fromRGB(51,17,23));
  g.fillRoundedRectangle(300.f,float(item.first),28.f,16.f,3.f);
  g.setColour(gold);g.setFont(font(11,true));
  g.drawText(juce::String(item.second),302,item.first,23,16,juce::Justification::centredRight);
 }
 // The combobox is already engraved. Redraw its current choice INSIDE the field.
 g.setColour(juce::Colour::fromRGB(25,11,15));g.fillRect(865,141,140,23);
 g.setColour(ivory);g.setFont(font(13));
 g.drawFittedText(styleFilter<0?(english?"All Styles":"Tous les Styles"):
                  juce::String(boom::styles()[static_cast<std::size_t>(styleFilter)].name),
                  865,141,140,23,juce::Justification::centredLeft,1);
 // The original search text is printed in the artwork; replace just its
 // interior so a real, accessible JUCE TextEditor can show actual user text.
 g.setColour(juce::Colour::fromRGB(20,10,14));g.fillRect(380,141,460,24);
 // Original PREVIEW compartment: replace its internal static Radio Polish
 // information with real data from whichever factory/user preset is selected.
 g.setColour(juce::Colour::fromRGB(28,12,16));
 g.fillRect(1156,138,345,204);
 const bool valid=selectedRow>=0&&selectedRow<int(results.size());
 if(valid){
  const auto r=results[static_cast<std::size_t>(selectedRow)];
  juce::String name,group,style,description;
  boom::Parameters params;
  if(r.isUser){
   name=processor.userPresetName(r.index);
   group=english?"User preset":"Preset utilisateur";
   description="Reglages enregistres dans le projet DAW. Import / export .boom.";
  }else{
   const auto p=boom::makePreset(r.index);
   params=p.params;
   name=p.name;
   group=boom::categories[static_cast<std::size_t>(p.category)];
   style=boom::styles()[static_cast<std::size_t>(p.style)].name;
   const char* descriptions[]={
    "Master commercial equilibre, adapte aux plateformes modernes.",
    "Dynamique musicale, guitares et presence naturelle.",
    "Puissance, graves precis et transitoires controles.",
    "Basses maitrisees, voix distinctes et punch rythmique.",
    "Energie electronique et spectre large.",
    "Traitement subtil et naturel pour les instruments.",
    "Profondeur et ampleur pour l'image.",
    "Harmoniques et chaleur analogique maitrisees.",
    "Coherence pour les plateformes de diffusion.",
    "Ajustement technique et finition du master."};
   description=descriptions[juce::jlimit(0,9,p.category)];
  }
  g.setColour(ivory);g.setFont(font(24,true));
  g.drawFittedText(boom::ui::text::safeUpperText(name),1168,143,299,34,juce::Justification::centredLeft,1);
  g.setColour(gold);g.setFont(font(12));
  g.drawFittedText(boom::ui::text::safeUpperText(group+"  |  "+style),1168,181,318,21,juce::Justification::centredLeft,1);
  g.setColour(ivory);g.setFont(font(12));
  g.drawFittedText(boom::ui::text::safeUpperText(description),1168,207,324,42,juce::Justification::topLeft,3);
  g.setColour(copper.withAlpha(.5f));g.drawHorizontalLine(253,1168.f,1495.f);
  if(!r.isUser){
   const int mode=juce::jlimit(0,2,params.color);
   const char* names[]={"CLEAN","TUBE","TAPE"};
   const juce::String line1="DRIVE : "+juce::String(params.inputDb,1)+" dB";
   const juce::String line2="PUSH : "+juce::String(params.push,0)+" %";
   const juce::String line3=juce::String(names[mode])+"    |    GLUE : "+
     juce::String(params.compRatio,2)+":1";
   const juce::String line4="PLAFOND : "+juce::String(params.ceilingDb,1)+" dBFS";
   g.setColour(gold);g.setFont(font(12,true));
   g.drawText(line1,1168,264,160,19,juce::Justification::centredLeft);
   g.drawText(line2,1350,264,150,19,juce::Justification::centredLeft);
   g.drawFittedText(line3,1168,284,320,19,juce::Justification::centredLeft,1);
   g.drawFittedText(line4,1168,306,321,19,juce::Justification::centredLeft,1);
  }
  const bool starred=!r.isUser&&processor.isFavorite(r.index);
  g.setColour(starred?gold:gold.withAlpha(.45f));g.setFont(font(10,true));
  g.drawText(starred?"FAV":"",1470,141,40,31,juce::Justification::centred);
 }
 // A tiny selection outline on the existing list/grid glyph; no fake button.
 g.setColour(gold);const int x=grid?1089:1042;
 g.drawRoundedRectangle(float(x),137.f,40.f,30.f,3.f,1.f);
 if(notice.isNotEmpty()){
  g.setColour(juce::Colour::fromRGB(20,11,13));g.fillRect(938,424,400,16);
  g.setColour(gold);g.setFont(font(11));
  g.drawFittedText(notice,940,424,394,16,juce::Justification::centredLeft,1);
 }
}
void BoomAudioProcessorEditor::paintOther(juce::Graphics& g){
 // Measurements/settings/help REPLACE the rack interior, rather than being
 // slapped over photographed hardware buttons with overlapping controls.
 g.setColour(juce::Colour::fromRGB(29,13,16));
 g.fillRoundedRectangle(48.f,107.f,1550.f,230.f,5.f);
 g.setColour(copper);
 g.drawRoundedRectangle(48.f,107.f,1550.f,230.f,5.f,1.2f);
 g.setColour(gold);g.setFont(font(19.f,true));
 const bool meters=page==Page::meters,settings=page==Page::settings;
 g.drawText(meters?(english?"PRECISION METERS":"MESURES DE PRECISION"):
  settings?(english?"MASTERING SETTINGS":"REGLAGES DE MASTERING"):
           (english?"BOOM CONSOLE GUIDE":"GUIDE DE LA CONSOLE BOOM"),
  72,116,1500,32,juce::Justification::centred);
 g.setColour(copper.withAlpha(.55f));g.drawHorizontalLine(151,72.f,1566.f);
 if(meters){
  const auto m=processor.lastMeters();
  struct Reading{const char* title;float value;const char* unit;bool linear;};
  const std::array<Reading,12> vals{{
   {english?"INPUT L":"ENTREE G",m.inputPeakL,"dBFS",true},
   {english?"INPUT R":"ENTREE D",m.inputPeakR,"dBFS",true},
   {english?"OUTPUT L":"SORTIE G",m.outputPeakL,"dBFS",true},
   {english?"OUTPUT R":"SORTIE D",m.outputPeakR,"dBFS",true},
   {english?"VU LEFT":"VU GAUCHE",m.vuL,"dBFS",true},
   {english?"VU RIGHT":"VU DROITE",m.vuR,"dBFS",true},
   {"LUFS M",m.momentaryLufs,"LUFS",false},
   {"LUFS S",m.shortTermLufs,"LUFS",false},
   {"LUFS I",m.integratedLufs,"LUFS",false},
   {"LRA",m.lra,"LU",false},
   {"TRUE PEAK EST.",m.truePeak,"dBTP*",true},
   {english?"CORRELATION":"CORRELATION",m.correlation,"",false}
  }};
  const int x0=75,y0=163,cellW=242,cellH=53,gapX=6;
  for(std::size_t i=0;i<vals.size();++i){
   const int col=int(i)%6,row=int(i)/6;
   const int x=x0+col*(cellW+gapX),y=y0+row*70;
   g.setColour(juce::Colour::fromRGB(43,21,22));
   g.fillRoundedRectangle(float(x),float(y),float(cellW),float(cellH),3.f);
   g.setColour(gold);g.setFont(font(11.5f,true));
   g.drawText(vals[i].title,x+7,y+5,cellW-14,17,juce::Justification::centredLeft);
   float value=vals[i].linear?boom::gainToDb(vals[i].value):vals[i].value;
   g.setColour(ivory);g.setFont(font(19.f,true));
   g.drawFittedText(formatDb(value)+" "+vals[i].unit,x+7,y+24,cellW-14,24,
                    juce::Justification::centredRight,1);
  }
  g.setColour(gold);g.setFont(font(11,true));
  g.drawText("RESET LUFS / LRA",1344,312,185,20,juce::Justification::centredRight);
  g.drawText("RESET CLIP",1166,312,145,20,juce::Justification::centredRight);
 }else if(settings){
  struct Setting{const char* title;const char* parameter;int x,y;float scale;const char* unit;};
  const Setting settingsFields[]={
   {english?"LEFT TRIM":"TRIM GAUCHE","inputLTrim",93,175,1.f," dB"},
   {english?"RIGHT TRIM":"TRIM DROIT","inputRTrim",443,175,1.f," dB"},
   {english?"COLOR COMPENSATION":"COMPENSATION COULEUR","colorOut",793,175,1.f," dB"},
   {english?"DE-ESSER ATTACK":"ATTAQUE DE-ESSER","deAtt",1143,175,1.f," ms"},
   {english?"SATURATION":"SATURATION","colorDrive",93,260,100.f," %"}
  };
  for(const auto& s:settingsFields){
   g.setColour(gold);g.setFont(font(12.5f,true));
   g.drawText(s.title,s.x,s.y,215,19,juce::Justification::centred);
   paintReading(g,{float(s.x),float(s.y+22),215.f,29.f},
                juce::String(read(s.parameter)*s.scale,1)+s.unit,false,true);
  }
  const char* modes[]={"TRANSPARENT","PUNCH","LOUD"};
  const int chosen=juce::jlimit(0,2,int(read("limitMode")+.5f));
  g.setColour(gold);g.setFont(font(12.5f,true));
  g.drawText(english?"LIMITER MODE":"MODE LIMITEUR",443,260,215,19,
             juce::Justification::centred);
  paintReading(g,{443,282,215,29},modes[chosen],false,true);
  g.setColour(gold);g.drawText(english?"LINK TRIMS L/R":"LIEN TRIMS G/D",793,260,215,19,
                              juce::Justification::centred);
  paintReading(g,{793,282,215,29},read("inputLink")>.5f?
        (english?"LINKED":"LIES"):(english?"UNLINKED":"DELIES"),false,true);
 }else{
  g.setColour(ivory);g.setFont(font(16.f));
  const juce::String help=english?
   juce::String(
    "1. Choose a Style, then one of the 100 presets.\n"
    "2. Adjust INPUT DRIVE, the five EQ bands and PUSH.\n"
    "3. In CHAIN, drag, click or scroll to change a value.\n"
    "4. Right-click a value to enter a number or reset it.\n"
    "5. A/B compares two states. LEVEL enables level matching.\n"
    "6. VU, levels and gain reduction use real DSP measurements.")
   :juce::String(
    "1. Choisir un Style puis un des 100 presets.\n"
    "2. Ajuster INPUT DRIVE, les 5 bandes EQ et PUSH.\n"
    "3. Dans CHAINE, chaque valeur se regle au glisser, clic ou molette.\n"
    "4. Clic droit sur une valeur : saisie numerique ou remise a zero.\n"
    "5. A/B compare deux etats. NIVEAU active la compensation d'ecoute.\n"
    "6. Les VU, les niveaux et la reduction de gain proviennent du DSP.");
  g.drawFittedText(help,85,165,1420,156,juce::Justification::topLeft,7);
 }
}
bool BoomAudioProcessorEditor::handleAuxiliary(
 juce::Point<float> p,const juce::MouseEvent&){
 auto inside=[p](float x,float y,float w,float h){
  return p.x>=x&&p.x<x+w&&p.y>=y&&p.y<y+h;
 };
 if(page==Page::meters){
  if(inside(1338,307,206,29)){processor.requestLoudnessReset();return true;}
  if(inside(1157,307,167,29)){clipHold=false;return true;}
  return true;
 }
 if(page==Page::settings){
  struct Field{float x,y;const char* id;float scale;};
  const std::array<Field,5> fields{{
   {93,197,"inputLTrim",1.f},{443,197,"inputRTrim",1.f},
   {793,197,"colorOut",1.f},{1143,197,"deAtt",1.f},
   {93,282,"colorDrive",100.f}
  }};
  for(const auto& f:fields)
   if(inside(f.x,f.y,215,29)){
    numericFor(f.id,f.scale,{f.x,f.y,215.f,29.f});return true;
   }
  if(inside(443,282,215,29)){
   juce::PopupMenu m;
   m.setLookAndFeel(&menuLook);
   const int selected=juce::jlimit(0,2,int(read("limitMode")+.5f));
   m.addItem(1,"TRANSPARENT",true,selected==0);
   m.addItem(2,"PUNCH",true,selected==1);
   m.addItem(3,"LOUD",true,selected==2);
   juce::Component::SafePointer<BoomAudioProcessorEditor> self(this);
   m.showMenuAsync(popupOptions({443.f,282.f,215.f,29.f}),
     [self](int i){if(self!=nullptr&&i>=1&&i<=3)
       self->write("limitMode",float(i-1));});
   return true;
  }
  if(inside(793,282,215,29)){toggle("inputLink");return true;}
  return true;
 }
 return true;
}
