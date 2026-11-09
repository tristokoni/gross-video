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
 for(int mode=0;mode<6;mode++)for(int t=-500;t<500;t++){
 auto v=gross::frame(t,23.976,137,4,7,mode,s);assert(std::isfinite(v));assert(v==std::floor(v));}
 std::cout<<"Timing tests passed: presets, boundaries, custom steps, origin, fractional fps.\n";
}
