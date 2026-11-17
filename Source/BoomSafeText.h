#pragma once
#include <JuceHeader.h>

// BOOM upper-area compatibility renderer.
//
// A few hosts or historical presets may carry double-decoded Latin-1 text,
// and some fonts cannot draw the expected glyphs.  Only the text DISPLAYED in
// the native header and browser is transliterated to ordinary ASCII.
// Stored preset/style names, tag data, and APVTS parameter values are NEVER
// modified.  This is an explicit safeguard against mojibake and replacement glyph
// substitution on both macOS and Windows.
namespace boom::ui::text {
inline juce::String safeUpperText(const juce::String& input) {
 juce::String out;
 auto emit=[&out](const char* ascii){out+=ascii;};
 for(int i=0;i<input.length();++i){
  const auto cp=static_cast<unsigned int>(input[i]);
  const auto next=i+1<input.length()?static_cast<unsigned int>(input[i+1]):0u;
  const auto third=i+2<input.length()?static_cast<unsigned int>(input[i+2]):0u;
  // Recover the common two-character "UTF-8 interpreted as Latin-1"
  // forms before transliterating individual Latin characters.
  if(cp==0x00C3u && next!=0u){
   const char* replacement=nullptr;
   switch(next){
    case 0x00A0:case 0x00A1:case 0x00A2:case 0x00A3:case 0x00A4:
     replacement="a";break;
    case 0x00A7:replacement="c";break;
    case 0x00A8:case 0x00A9:case 0x00AA:case 0x00AB:
     replacement="e";break;
    case 0x00AC:case 0x00AD:case 0x00AE:case 0x00AF:
     replacement="i";break;
    case 0x00B1:replacement="n";break;
    case 0x00B2:case 0x00B3:case 0x00B4:case 0x00B5:case 0x00B6:
     replacement="o";break;
    case 0x00B9:case 0x00BA:case 0x00BB:case 0x00BC:
     replacement="u";break;
    case 0x00BF:replacement="y";break;
    case 0x20AC:case 0x201A:case 0x201E:replacement="A";break;
    case 0x2030:case 0x02C6:case 0x2039:case 0x0160:
     replacement="E";break;
    case 0x017D:replacement="I";break;
    case 0x2018:case 0x2019:case 0x201C:case 0x201D:
     replacement="O";break;
    case 0x0153:replacement="U";break;
    case 0x00A6:replacement="AE";break;
   }
   if(replacement!=nullptr){emit(replacement);++i;continue;}
  }
  if(cp==0x00C2u && next==0x00A0u){emit(" ");++i;continue;}
  if(cp==0x00C2u && (next==0x00ABu||next==0x00BBu||next==0x00B0u)){
   ++i;continue;
  }
  // Smart punctuation accidentally read as Windows-1252.
  if(cp==0x00E2u && next==0x20ACu && third!=0u){
   const char* replacement=nullptr;
   switch(third){
    case 0x2122:case 0x2018:case 0x2019:replacement="'";break;
    case 0x201C:case 0x201D:replacement="-";break;
    case 0x00A6:replacement="...";break;
    case 0x00A2:replacement=" ";break;
   }
   if(replacement){emit(replacement);i+=2;continue;}
  }
  if(cp>=32u&&cp<=126u){
   out+=juce::String::charToString(static_cast<wchar_t>(cp));
   continue;
  }
  if(cp==9u||cp==10u||cp==13u||cp==0x00A0u){emit(" ");continue;}
  switch(cp){
   case 0x00C0:case 0x00C1:case 0x00C2:case 0x00C3:
   case 0x00C4:case 0x00C5:emit("A");break;
   case 0x00E0:case 0x00E1:case 0x00E2:case 0x00E3:
   case 0x00E4:case 0x00E5:emit("a");break;
   case 0x00C6:emit("AE");break;case 0x00E6:emit("ae");break;
   case 0x00C7:emit("C");break;case 0x00E7:emit("c");break;
   case 0x00C8:case 0x00C9:case 0x00CA:case 0x00CB:emit("E");break;
   case 0x00E8:case 0x00E9:case 0x00EA:case 0x00EB:emit("e");break;
   case 0x00CC:case 0x00CD:case 0x00CE:case 0x00CF:emit("I");break;
   case 0x00EC:case 0x00ED:case 0x00EE:case 0x00EF:emit("i");break;
   case 0x00D0:emit("D");break;case 0x00F0:emit("d");break;
   case 0x00D1:emit("N");break;case 0x00F1:emit("n");break;
   case 0x00D2:case 0x00D3:case 0x00D4:case 0x00D5:
   case 0x00D6:case 0x00D8:emit("O");break;
   case 0x00F2:case 0x00F3:case 0x00F4:case 0x00F5:
   case 0x00F6:case 0x00F8:emit("o");break;
   case 0x00D9:case 0x00DA:case 0x00DB:case 0x00DC:emit("U");break;
   case 0x00F9:case 0x00FA:case 0x00FB:case 0x00FC:emit("u");break;
   case 0x00DD:case 0x0178:emit("Y");break;
   case 0x00FD:case 0x00FF:emit("y");break;
   case 0x0152:emit("OE");break;case 0x0153:emit("oe");break;
   case 0x00DF:emit("ss");break;
   case 0x2018:case 0x2019:case 0x201A:case 0x201B:emit("'");break;
   case 0x2013:case 0x2014:case 0x2212:emit("-");break;
   case 0x2026:emit("...");break;
   case 0x2022:emit(" ");break;
   case 0x00A9:emit("(C)");break;
   // Combining accents, replacement characters and decorative icons are
   // omitted.  This guarantees the header/browser draw only ASCII glyphs.
   default:break;
  }
 }
 return out;
}
}
