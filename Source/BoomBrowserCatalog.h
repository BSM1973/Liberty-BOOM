#pragma once
// BOOM browser metadata derived from real DSP presets and Styles.
// Header is dependency-free (C++17) so the same taxonomy is tested in CI.
#include "FactoryLibrary.h"
#include <array>
#include <cstdint>
#include <string>
#include <algorithm>
#include <cctype>
#include <vector>
namespace boom::browser {
enum class Mode:int { Presets=0, Styles=1, User=2 };
enum class Tag:int { All=0, Punch, Warm, Air, Clean, Loud, Gentle, Vocal, Streaming };
inline constexpr std::array<const char*,9> tagFr{{
 "Tous","Punch","Chaud","Air","Clean","Puissant","Doux","Vocal","Streaming"
}};
inline constexpr std::array<const char*,9> tagEn{{
 "All","Punch","Warm","Air","Clean","Loud","Gentle","Vocal","Streaming"
}};
inline constexpr std::array<const char*,10> categoryShort{{
 "Pop & Commercial","Rock & Alternatif","Metal & Heavy","Hip-Hop & Trap",
 "Electro & Dance","Jazz & Acoustique","Cinema & Ambient","Vintage Analog",
 "Streaming","Outils Mastering"
}};
inline constexpr std::uint32_t bit(Tag t){return std::uint32_t(1)<<static_cast<int>(t);}
inline std::uint32_t tagsFor(const Parameters& p,int category){
 std::uint32_t tags=0;
 if(p.push>=40.f)tags|=bit(Tag::Punch);
 if(p.color!=0&&p.colorMix>=.2f)tags|=bit(Tag::Warm);
 if(p.eqDb[4]>=1.0f||p.eqDb[3]>=1.4f)tags|=bit(Tag::Air);
 if(p.color==0||p.colorMix<.12f)tags|=bit(Tag::Clean);
 if(p.push>=60.f||p.limiterThreshold<=-7.f)tags|=bit(Tag::Loud);
 if(p.compRatio<=1.6f&&p.push<35.f)tags|=bit(Tag::Gentle);
 if(category==0||category==3||category==5)tags|=bit(Tag::Vocal);
 if(category==8||category==9)tags|=bit(Tag::Streaming);
 return tags;
}
struct Entry {
 Mode mode=Mode::Presets;
 int index=0,category=-1,style=-1;
 std::string name;
 std::uint32_t tags=0;
};
inline Entry factory(int index){
 const auto p=makePreset(index);
 return {Mode::Presets,index,p.category,p.style,p.name,tagsFor(p.params,p.category)};
}
inline Entry style(int index){
 const auto p=makeStyle(index);
 const auto name=std::string(styles()[static_cast<std::size_t>(index)].name);
 // Style classification maps to its actual DSP processing and musical family.
 const int category=(index<10?1:index<16?3:index<24?4:
                     index<34?5:index<38?7:8);
 return {Mode::Styles,index,category,index,name,tagsFor(p,category)};
}
inline bool hasTag(std::uint32_t mask,Tag t){
 return t==Tag::All || (mask&bit(t))!=0;
}
inline bool matches(const Entry& item,Tag tag,int category,const std::string& query){
 if(category>=0&&item.category!=category)return false;
 if(!hasTag(item.tags,tag))return false;
 if(query.empty())return true;
 std::string name=item.name;
 std::transform(name.begin(),name.end(),name.begin(),
                [](unsigned char c){return static_cast<char>(std::tolower(c));});
 std::string needle=query;
 std::transform(needle.begin(),needle.end(),needle.begin(),
                [](unsigned char c){return static_cast<char>(std::tolower(c));});
 return name.find(needle)!=std::string::npos;
}
}
