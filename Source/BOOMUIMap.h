#pragma once
// BOOM v0.9 — single-source interaction geometry, in the pixel coordinates
// of the two supplied reference images. No overlay buttons are instantiated.
// Rectangles are deliberately disjoint; all are painted by one JUCE editor.
#include <array>
#include <cstddef>
namespace boom::ui {
enum class Kind { knob, field, toggle, menu, action };
struct Zone {
 float x,y,w,h; Kind kind; const char* id;
 constexpr bool contains(float px,float py)const noexcept {
  return px>=x&&px<x+w&&py>=y&&py<y+h;
 }
 constexpr bool intersects(const Zone& o)const noexcept {
  return x<o.x+o.w && x+w>o.x && y<o.y+o.h && y+h>o.y;
 }
};
inline constexpr int chainWidth=1672,chainHeight=941,browserWidth=1592,browserHeight=988;
inline constexpr Zone chain[]={
 {90,31,360,45,Kind::menu,"styleMenu"},
 {1163,34,31,40,Kind::action,"presetPrevious"},
 {1198,33,329,42,Kind::menu,"presetMenu"},
 {1551,34,31,40,Kind::action,"presetNext"},
 // Six hardware rack modules — exactly the hit zones of the supplied console.
 {230,154,68,24,Kind::toggle,"colorBypass"},
 {300,154,25,24,Kind::toggle,"colorBypass"},
 {217,180,89,25,Kind::menu,"color"},
 {246,205,60,22,Kind::field,"colorMix"},
 {246,229,60,22,Kind::field,"colorPushCharacter"},
 {470,153,67,24,Kind::toggle,"eqBypass"},
 {539,153,23,24,Kind::toggle,"eqBypass"},
 {482,183,63,21,Kind::field,"eq60"},
 {482,205,63,21,Kind::field,"eq120"},
 {482,227,63,21,Kind::field,"eq3k"},
 {482,249,63,21,Kind::field,"eq6k"},
 {482,271,63,21,Kind::field,"eq10k"},
 {719,153,67,24,Kind::toggle,"deEssBypass"},
 {788,153,23,24,Kind::toggle,"deEssBypass"},
 {729,181,64,22,Kind::field,"deFreq"},
 {729,204,64,22,Kind::field,"deTh"},
 {729,227,64,22,Kind::field,"deAmt"},
 {729,250,64,22,Kind::field,"deRel"},
 {972,153,66,24,Kind::toggle,"compBypass"},
 {1040,153,23,24,Kind::toggle,"compBypass"},
 {990,180,61,21,Kind::field,"compTh"},
 {990,201,61,21,Kind::field,"compRatio"},
 {990,222,61,21,Kind::field,"compAttack"},
 {990,243,61,21,Kind::field,"compRelease"},
 {990,264,61,21,Kind::field,"compMakeup"},
 {990,285,61,21,Kind::field,"compMix"},
 {845,291,128,24,Kind::toggle,"compStereoLink"},
 {1231,153,66,24,Kind::toggle,"limiterBypass"},
 {1299,153,23,24,Kind::toggle,"limiterBypass"},
 {1237,180,62,21,Kind::field,"limitTh"},
 {1237,202,62,21,Kind::field,"ceiling"},
 {1237,224,62,21,Kind::field,"limitRelease"},
 {1096,244,121,24,Kind::toggle,"limiterAutoRelease"},
 {1096,271,121,24,Kind::toggle,"limiterStereoLink"},
 {1490,154,69,24,Kind::toggle,"outputBypass"},
 {1563,154,20,24,Kind::toggle,"outputBypass"},
 {1510,180,66,22,Kind::field,"outDb"},
 {1351,205,140,24,Kind::toggle,"outPeakGuard"},
 {1351,232,140,24,Kind::toggle,"outputLevelComp"},
 // Toolbar buttons all activate the displayed page or comparison action.
 {49,350,105,33,Kind::action,"pageChain"},
 {160,350,115,33,Kind::action,"pageMeters"},
 {284,350,125,33,Kind::action,"pageSettings"},
 {418,350,80,33,Kind::action,"pageHelp"},
 {517,350,69,33,Kind::menu,"language"},
 {1198,350,35,33,Kind::action,"recallA"},
 {1237,350,36,33,Kind::action,"recallB"},
 {1281,350,45,33,Kind::action,"storeA"},
 {1333,350,44,33,Kind::action,"storeB"},
 {1383,350,61,33,Kind::action,"copyAtoB"},
 {1450,350,59,33,Kind::action,"copyBtoA"},
 {1516,350,84,33,Kind::toggle,"outputLevelComp"},
 // Console navigation and actual motor controls.
 {458,428,242,41,Kind::toggle,"meterInput"},
 {773,429,116,40,Kind::toggle,"bypass"},
 {958,429,230,40,Kind::toggle,"mono"},
 {129,522,219,219,Kind::knob,"inputDb"},
 {439,681,124,124,Kind::knob,"eq60"},
 {602,681,122,124,Kind::knob,"eq120"},
 {761,681,122,124,Kind::knob,"eq3k"},
 {920,681,122,124,Kind::knob,"eq6k"},
 {1080,681,122,124,Kind::knob,"eq10k"},
 {1323,522,217,219,Kind::knob,"push"},
 {147,831,97,29,Kind::toggle,"inputLink"},
 {256,831,78,29,Kind::action,"trimMenu"}
};
inline constexpr Zone browser[]={
 {32,28,354,42,Kind::menu,"styleMenu"},
 {1159,29,31,38,Kind::action,"presetPrevious"},
 {1196,28,287,42,Kind::menu,"presetMenu"},
 {1491,29,31,38,Kind::action,"presetNext"},
 // Navigation in the top two-thirds of the browser reference.
 {42,137,288,23,Kind::action,"category0"},
 {42,160,288,22,Kind::action,"category1"},
 {42,182,288,22,Kind::action,"category2"},
 {42,204,288,22,Kind::action,"category3"},
 {42,226,288,22,Kind::action,"category4"},
 {42,248,288,22,Kind::action,"category5"},
 {42,270,288,22,Kind::action,"category6"},
 {42,292,288,22,Kind::action,"category7"},
 {42,314,288,22,Kind::action,"category8"},
 {42,336,288,22,Kind::action,"category9"},
 {42,359,288,22,Kind::action,"categoryFavorites"},
 {42,382,288,22,Kind::action,"categoryUser"},
 {854,137,178,31,Kind::menu,"styleFilter"},
 {1041,136,43,32,Kind::action,"viewList"},
 {1088,136,42,32,Kind::action,"viewGrid"},
 {372,209,741,180,Kind::action,"presetRows"},
 {1473,138,41,32,Kind::action,"favoriteSelected"},
 {1198,350,275,40,Kind::action,"loadSelected"},
 {370,395,122,30,Kind::action,"rowPrevious"},
 {502,395,120,30,Kind::action,"rowNext"},
 {630,395,121,30,Kind::action,"favoriteSelected"},
 {772,395,144,30,Kind::menu,"userMenu"},
 {1135,395,146,30,Kind::action,"saveUser"},
 {1515,97,28,27,Kind::action,"closeBrowser"},
 // Footer rail and transport.
 {32,442,91,31,Kind::action,"pageChain"},
 {131,442,106,31,Kind::action,"pageMeters"},
 {247,442,124,31,Kind::action,"pageSettings"},
 {378,442,80,31,Kind::action,"pageHelp"},
 {465,442,60,31,Kind::menu,"language"},
 {1147,442,33,31,Kind::action,"recallA"},
 {1186,442,31,31,Kind::action,"recallB"},
 {1223,442,45,31,Kind::action,"storeA"},
 {1276,442,44,31,Kind::action,"storeB"},
 {1326,442,60,31,Kind::action,"copyAtoB"},
 {1392,442,60,31,Kind::action,"copyBtoA"},
 {1459,442,76,31,Kind::toggle,"outputLevelComp"},
 // Equalizer and active analog controls on the photographed console.
 {441,507,244,40,Kind::toggle,"meterInput"},
 {741,507,112,40,Kind::toggle,"bypass"},
 {906,507,220,40,Kind::toggle,"mono"},
 {120,575,196,198,Kind::knob,"inputDb"},
 {444,731,104,104,Kind::knob,"eq60"},
 {583,731,104,104,Kind::knob,"eq120"},
 {722,731,104,104,Kind::knob,"eq3k"},
 {860,731,104,104,Kind::knob,"eq6k"},
 {999,731,104,104,Kind::knob,"eq10k"},
 {1230,575,203,198,Kind::knob,"push"},
 {123,861,102,32,Kind::toggle,"inputLink"},
 {228,861,77,32,Kind::action,"trimMenu"}
};
template<std::size_t N> constexpr bool uniqueRects(const Zone (&zones)[N]) {
 for(std::size_t i=0;i<N;++i)
  for(std::size_t j=i+1;j<N;++j)
   if(zones[i].intersects(zones[j]))return false;
 return true;
}
} // namespace boom::ui
