#include "../Source/BOOMUIMap.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <set>
using namespace boom::ui;
static_assert(uniqueRects(chain),"BOOM chain UI contains overlapping controls");
static_assert(uniqueRects(browser),"BOOM browser UI contains overlapping controls");
int main(){
 const std::set<const char*> none;
 auto validate=[](const auto& zones){
  int knobs=0,fields=0,toggles=0;
  std::set<std::string> names;
  for(const auto& z:zones){
   assert(z.w>=15.f&&z.h>=16.f);
   assert(z.id!=nullptr&&std::strlen(z.id)>0);
   if(z.kind==Kind::knob)++knobs;
   if(z.kind==Kind::field)++fields;
   if(z.kind==Kind::toggle)++toggles;
   names.insert(z.id);
  }
  assert(knobs==7);
  return std::array<int,3>{{knobs,fields,toggles}};
 };
 const auto a=validate(chain),b=validate(browser);
 assert(a[1]>=19);
 assert(a[2]>=13);
 assert(b[1]==0);
 assert(b[2]>=4);
 std::cout<<"BOOM UI map geometry PASS; chain "<<std::size(chain)
          <<" zones, browser "<<std::size(browser)
          <<" zones, no overlaps.\n";
 return 0;
}
