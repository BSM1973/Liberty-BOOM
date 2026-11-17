#include "../Source/BoomBrowserCatalog.h"
#include <cassert>
#include <iostream>
#include <set>
#include <string>
int main(){
 using namespace boom;
 using namespace boom::browser;
 std::set<std::string> names;
 std::array<int,10> counts{};
 int warm=0,clean=0,loud=0,tagged=0;
 for(int i=0;i<100;++i){
  const auto e=factory(i);
  assert(e.index==i && e.mode==Mode::Presets);
  assert(e.style>=0&&e.style<40 && e.category>=0&&e.category<10);
  ++counts[static_cast<std::size_t>(e.category)];
  assert(names.insert(e.name).second);
  if(hasTag(e.tags,Tag::Warm))++warm;
  if(hasTag(e.tags,Tag::Clean))++clean;
  if(hasTag(e.tags,Tag::Loud))++loud;
  if(e.tags!=0u)++tagged;
  assert(matches(e,Tag::All,-1,""));
  assert(matches(e,Tag::All,e.category,e.name));
  assert(!matches(e,Tag::All,(e.category+1)%10,""));
 }
 for(int cat:counts)assert(cat==10);
 assert(tagged>=90 && warm>0 && clean>0 && loud>0);
 names.clear();
 int styleTagged=0;
 for(int i=0;i<40;++i){
  const auto e=style(i);
  assert(e.mode==Mode::Styles&&e.index==i&&e.style==i);
  assert(e.category>=0&&e.category<10);
  assert(names.insert(e.name).second);
  assert(matches(e,Tag::All,-1,e.name));
  if(e.tags!=0u)++styleTagged;
 }
 assert(styleTagged>=30);
 static_assert(tagFr.size()==tagEn.size());
 std::cout<<"BOOM native browser catalog PASS: 100 presets, 40 Styles, "
          <<"10 categories, dynamic sonic tags and search.\n";
}
