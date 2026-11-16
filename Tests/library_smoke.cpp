#include "../Source/FactoryLibrary.h"
#ifdef NDEBUG
#undef NDEBUG // Never compile out validation checks in Release/CI.
#endif
#include <cassert>
#include <set>
#include <iostream>
int main(){
 assert(boom::styles().size()==40);
 std::set<std::string> names;
 for(int i=0;i<100;++i){auto p=boom::makePreset(i);assert(p.category==i/10);assert(p.style>=0 && p.style<40);
 assert(p.params.push>=0 && p.params.push<=100);names.insert(p.name);
 }
 assert(names.size()==100);
 std::cout<<"Preset library smoke test: PASS (40 styles, 100 unique preset names)\n";
}
