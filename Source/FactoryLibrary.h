#pragma once
#include "BoomDSP.h"
#include <array>
#include <string>
namespace boom {
struct StyleDefinition {
 const char* name;
 std::array<float,5> eq;
 float drive, push, compressionThreshold,compressionRatio,limiterThreshold;
 int color;
};
inline const std::array<StyleDefinition,40>& styles(){
 static const std::array<StyleDefinition,40> s{{
 {"Pop Rock",{1.5,0.5,1.1,0.6,1.3},0.25f,34,-15,1.7f,-3.4f,1},
 {"Modern Pop",{1,0,1.6,1.4,1.8},0.18f,46,-17,2,-5,0},
 {"Indie Pop",{0.8,0.7,0.3,-0.2,1.4},0.33f,25,-13,1.5f,-2.5f,2},
 {"Classic Rock",{1.2,1.1,0.7,-0.8,0.8},0.43f,38,-17,1.8f,-3.7f,1},
 {"Hard Rock",{1.1,-0.8,2.0,1,1.3},0.50f,54,-20,2.3f,-6,1},
 {"Heavy Metal",{1.4,-1.5,2.3,0.7,1.4},0.47f,62,-22,2.6f,-7,1},
 {"Modern Metal",{0.8,-2.1,2.4,1.4,2.0},0.36f,69,-23,2.7f,-7.8f,0},
 {"Punk Rock",{0.3,0.8,2.1,1.3,-0.2},0.56f,53,-20,2.2f,-5.4f,2},
 {"Indie Rock",{1.1,1.0,0.4,-0.2,0.4},0.38f,30,-16,1.6f,-3,2},
 {"Alternative Rock",{1.0,-0.7,1.5,0.2,1.1},0.34f,43,-18,2.0f,-4.3f,1},
 {"Hip-Hop",{2.2,1.0,0.2,-0.2,1.2},0.28f,51,-20,2.1f,-6.5f,1},
 {"Trap",{2.7,-1.4,1.5,1.5,1.6},0.20f,65,-23,2.4f,-8,0},
 {"Contemporary R&B",{1.2,1.1,0.5,-0.3,2},0.29f,41,-17,1.8f,-4.5f,1},
 {"Neo Soul",{0.8,1.3,-0.4,-1,1.1},0.37f,25,-14,1.5f,-2.6f,2},
 {"Funk",{1.1,0.8,1.7,0.8,0.7},0.29f,39,-16,1.9f,-4.2f,1},
 {"Disco",{1.5,0.5,0.3,1.0,1.9},0.21f,47,-18,1.9f,-5.3f,0},
 {"EDM",{2.1,-1.1,0.8,1.5,2},0.23f,75,-25,2.8f,-9.5f,0},
 {"House",{1.9,0.1,0.3,0.8,1.7},0.29f,56,-21,2.2f,-6.5f,1},
 {"Deep House",{1.3,1.1,-0.6,-0.5,1.6},0.38f,35,-15,1.6f,-3.5f,2},
 {"Techno",{1.7,-1.8,0.7,1.5,0.6},0.34f,65,-22,2.4f,-7.6f,2},
 {"Trance",{1.0,-0.2,0.8,1.5,2.5},0.15f,68,-23,2.3f,-7.8f,0},
 {"Drum & Bass",{1.9,-1.3,1.5,2.0,1.3},0.28f,72,-25,2.9f,-9,1},
 {"Dubstep",{2.1,-2.1,1.3,2,1.0},0.36f,80,-25,3.1f,-10,2},
 {"Synthwave",{1.4,0.9,-0.2,0.4,0.9},0.45f,36,-16,1.8f,-3.8f,2},
 {"Ambient",{0.5,0.1,-1,-0.9,2.2},0.10f,12,-10,1.2f,-1.5f,0},
 {"Cinematic",{1.4,0.1,0.4,0.5,1.8},0.15f,24,-13,1.4f,-2.6f,0},
 {"Acoustic",{0.1,0.4,-0.2,0.4,1.3},0.13f,12,-11,1.3f,-1.5f,1},
 {"Folk",{0.4,0.6,0.2,-0.1,0.8},0.19f,17,-12,1.4f,-1.9f,2},
 {"Country",{0.9,0.1,0.5,1.0,1.5},0.20f,30,-15,1.6f,-3.1f,1},
 {"Blues",{1.3,1.2,-0.4,-0.3,0.1},0.55f,21,-13,1.5f,-2.5f,1},
 {"Jazz",{0.2,0.4,-0.6,-0.1,0.6},0.12f,8,-10,1.2f,-1.0f,0},
 {"Vocal Jazz",{0.3,0.1,0.3,-0.3,0.8},0.08f,6,-9,1.2f,-0.8f,0},
 {"Classical",{0.2,0.0,0.0,0.1,0.5},0.0f,0,-5,1.0f,0,0},
 {"Orchestral",{0.5,0.1,-0.2,0.0,0.7},0.04f,7,-9,1.1f,-0.5f,0},
 {"Latin",{0.9,0.3,1.2,0.9,1.5},0.26f,38,-17,1.8f,-4.2f,1},
 {"Reggae",{2.1,1.3,-0.5,-0.6,0.1},0.43f,24,-15,1.5f,-2.8f,2},
 {"Afrobeats",{1.5,0.3,1.0,1.3,1.8},0.23f,49,-19,2,-5.6f,0},
 {"Lo-Fi",{-0.2,1.5,-1.2,-2.1,-2.4},0.65f,23,-14,1.6f,-2.1f,2},
 {"Podcast / Voice",{-1.3,-0.4,2.2,0.7,-0.7},0.10f,30,-20,2,-3.7f,0},
 {"Streaming Balanced",{0.5,-0.3,0.5,0.5,1.0},0.11f,32,-17,1.7f,-4,0},
 }};
 return s;
}
inline Parameters makeStyle(int styleIndex){
 const auto& st=styles()[static_cast<std::size_t>(std::clamp(styleIndex,0,39))];
 Parameters p;
 p.eqDb=st.eq;p.color=st.color;p.colorDrive=st.drive;p.push=st.push;
 p.colorMix=(st.color==0?0.0f:0.35f);
 p.compThreshold=st.compressionThreshold;p.compRatio=st.compressionRatio;
 p.compAttackMs=(styleIndex==32||styleIndex==33)?70.0f:28.0f;
 p.compReleaseMs=(styleIndex>=16&&styleIndex<=23)?135.0f:230.0f;
 p.limiterThreshold=st.limiterThreshold;p.ceilingDb=-1.0f;
 p.deEssAmount= (styleIndex==38||styleIndex==1)?0.20f:0.08f;
 return p;
}
static constexpr std::array<const char*,10> categories{{
 "Pop & Commercial", "Rock & Alternative", "Metal & Heavy", "Hip-Hop & Trap",
 "Electronic & Dance", "Jazz & Acoustic", "Cinematic & Ambient", "Vintage Analog",
 "Streaming & Modern Distribution", "Utility & Mastering Tools"
}};
static constexpr const char* names[10][10]={
 {"Radio Polish","Wide Modern","Vocal Forward","Golden Pop","Silky Shine","Glossy Chorus","Air & Definition","Punchy Single","Smooth Release","Commercial Finish"},
 {"Rock Foundation","Guitar Forward","Live Energy","Vintage Crunch","Indie Texture","Bass Control","Arena Presence","Power Chorus","Warm Midrange","Organic Rock"},
 {"Steel Impact","Low-End Tight","Metal Clarity","Fast Attack","Dense Guitars","Heavy Control","Drum Focus","Modern Razor","Power Thrash","Extreme Precision"},
 {"808 Authority","Vocal Pocket","Trap Impact","Warm Rap","Night Flow","Deep Sub","Rhythmic Punch","Smooth R&B","Bold Street","Low-End Monarch"},
 {"Club Pressure","House Groove","Festival Shine","Trance Lift","Warehouse Drive","Sub Discipline","Bright Drop","DNB Force","Deep Movement","Electronic Peak"},
 {"Acoustic Detail","Jazz Warmth","Piano Natural","Vocal Intimacy","Folk Wood","Blue Note","Soft Dynamics","Country Air","Strings Open","Quiet Master"},
 {"Film Panorama","Atmospheric Glow","Orchestral Lift","Ambient Bloom","Epic Contrast","Delicate Score","Hybrid Cinema","Wide Emotions","Soundtrack Air","Natural Dynamics"},
 {"Bronze 1973","Warm Valve","Soft Tape","Analog Glue","Walnut Room","Classic Mix Bus","Late Night Vinyl","Vintage Soul","Rounded Peaks","Soft Saturation"},
 {"Stream Balanced","Podcast Clear","Transparent -1","Loudness Control","Mobile Definition","Online Dynamic","Neutral Master","Headroom Safe","Stereo Translation","Clean Release"},
 {"Gentle Glue","Sub Cleanup","Mid Focus","Top Polish","Mono Check","Soft Ceiling","Warmth Only","Presence Trim","Transient Save","Reference Match"}
};
struct PresetDefinition { std::string name;int category=0,style=0;Parameters params; };
inline PresetDefinition makePreset(int index){
 index=std::clamp(index,0,99);const int cat=index/10,n=index%10;
 static constexpr int styleMap[10][10]={
  {1,1,0,2,1,1,12,1,3,1},
  {3,4,9,3,8,0,4,3,8,9},
  {5,6,6,7,5,6,5,6,5,6},
  {10,10,11,10,12,11,10,12,10,11},
  {16,17,16,20,19,18,16,21,18,22},
  {26,30,30,31,27,29,26,28,30,32},
  {25,24,33,24,25,33,25,24,25,33},
  {3,29,23,3,13,3,37,13,3,2},
  {39,38,39,39,39,39,39,39,39,39},
  {39,10,30,1,39,39,13,38,26,39}
 };
 const int style=styleMap[cat][n];
 Parameters p=makeStyle(style);
 // Each of the 100 configurations varies tone, drive, dynamics and limiting.
 // Category-specific presets preserve the distinct source Style's profile.
 const float tone=static_cast<float>(n-4.5f);
 p.eqDb[0]=clamp(p.eqDb[0]+tone*0.15f,-8,8);
 p.eqDb[1]=clamp(p.eqDb[1]-tone*0.12f,-10,10);
 p.eqDb[2]=clamp(p.eqDb[2]+tone*0.17f,-10,10);
 p.eqDb[3]=clamp(p.eqDb[3]+(float(n%3)-1)*0.27f,-10,10);
 p.eqDb[4]=clamp(p.eqDb[4]-(float(n%4)-1.5f)*0.20f,-8,8);
 p.inputDb=(float(n%4)-1.5f)*0.7f;
 p.push=clamp(p.push+(n%5-2)*4.1f,0,100);
 p.colorDrive=clamp(p.colorDrive+tone*0.018f,0,1);
 p.compThreshold=clamp(p.compThreshold+(n%3-1)*1.7f,-40,0);
 p.compRatio=clamp(p.compRatio+tone*0.045f,1,4);
 p.compAttackMs=clamp(p.compAttackMs+float((n%4)-2)*2.5f,1,100);
 p.compReleaseMs=clamp(p.compReleaseMs+(float(n)-4.5f)*10,50,1000);
 p.limiterThreshold=clamp(p.limiterThreshold+tone*0.22f,-18,0);
 p.deEssFreq=clamp(5500.0f+float((n*677+cat*337)%5300),3000,12000);
 p.deEssAmount=clamp(p.deEssAmount+float(n%3)*0.04f,0,1);
 p.ceilingDb=cat==8?-1.0f:-1.0f;
 if(cat==5||cat==6){p.push=std::min(p.push,26.0f);p.limiterThreshold=std::max(p.limiterThreshold,-3.0f);}
 if(cat==9){p.push=std::min(p.push,35.0f);}
 return {names[cat][n],cat,style,p};
}
}
