#include "../src/timing.h"
#include <cassert>
#include <iostream>
int main(){
 std::array<double,16> s{};for(int i=0;i<16;i++)s[i]=i/16.0;
 auto f=[&](double t,int mode){return gross::frame(t,24,120,4,0,mode,s);};
 for(int i=-96;i<144;i++)assert(f(i,0)==i);
 assert(f(12,1)==0);assert(f(13,1)==1);assert(f(48,1)==48);
 assert(f(0,2)==47);assert(f(47,2)==0);assert(f(48,2)==95);
 assert(f(25,3)==0);assert(f(49,3)==48);
 assert(f(0,4)==0);assert(f(24,4)==47);assert(f(47,4)==1);
 assert(f(5,5)==3);s.fill(0.5);assert(f(5,5)==24);
 assert(gross::frame(110,24,120,4,100,3,s)==100);
 for(int mode=0;mode<int(gross::presetNames.size());mode++)for(int t=-500;t<500;t++){
 auto v=gross::frame(t,23.976,137,4,7,mode,s);assert(std::isfinite(v));assert(v==std::floor(v));}
 // Explicit expectations for new effects at a 64-frame cycle.
 auto g=[&](double t,int mode){return gross::frame(t,32,120,4,0,mode,s);};
 assert(g(4,6)==44);assert(g(0,6)==3);
 assert(g(4,7)==0);assert(g(36,7)==32);assert(g(48,7)==63);
 assert(g(8,8)==0);assert(g(4,9)==0);
 assert(g(0,10)==3);assert(g(3,10)==0);
 assert(g(20,11)==10);assert(g(20,12)==40);assert(g(32,12)==0);
 assert(g(32,13)==24);assert(g(32,14)==16);
 assert(g(0,15)==0);assert(g(8,15)==15);
 assert(g(4,16)==16);assert(g(16,17)==28);
 for(int mode=6;mode<int(gross::presetNames.size());mode++){
   std::array<double,64> rendered{};
   for(int t=0;t<64;t++){rendered[t]=g(t,mode);assert(rendered[t]>=0&&rendered[t]<64);}
   // Scrub backward and revisit other cycles: no mutable random state.
   for(int t=63;t>=0;t--){assert(g(t,mode)==rendered[t]);assert(g(t+64,mode)==rendered[t]+64);assert(g(t-64,mode)==rendered[t]-64);}
 }
 std::cout<<"Timing tests passed: presets, boundaries, custom steps, origin, fractional fps.\n";
}
