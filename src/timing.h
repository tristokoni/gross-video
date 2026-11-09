#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace gross {
inline constexpr std::array<const char*,18> presetNames={
 "Normal","Stutter x4","Reverse","Freeze","Ping pong","Custom steps",
 "Chaos","Complex","Stutter x8","Stutter x16","Reverse chops",
 "Half speed","Double speed","Tape stop","Speed up","Bounce chops",
 "Shuffle","Glitch hold"};

inline double frame(double t,double fps,double bpm,double beats,double origin,int preset,const std::array<double,16>& steps) {
 const double length=std::max(1.0,fps*60.0/std::max(1.0,bpm)*beats);
 const double cycle=std::floor((t-origin)/length), phase=(t-origin)/length-cycle;
 const int step=std::min(15,int(phase*16));
 const double local=phase*16-step;
 double p=phase;
 switch(preset){
 case 1:p=std::fmod(phase*4.0,1.0)/4.0;break;
 case 2:p=1.0-phase-1.0/length;break;
 case 3:p=0;break;
 case 4:p=phase<0.5?phase*2:(1-phase)*2-1.0/length;break;
 case 5:p=steps[step];break;
 // Fixed permutations make rendering independent of frame request order.
 case 6:{constexpr int order[]={0,11,3,14,6,1,12,7,15,4,9,2,13,5,10,8};
 p=(order[step]+(step%3==0?1-local:local))/16.0;
 if(step%3==0)p-=1.0/length;
 break;}
 case 7:{constexpr int order[]={0,0,2,1,4,7,6,5,8,8,8,11,15,14,13,12};
 double motion=step>=12?1-local:local;
 if(step==9||step==10)motion=0;
 p=(order[step]+motion)/16.0;
 if(step>=12)p-=1.0/length;
 break;}
 case 8:p=std::fmod(phase*8,1.0)/8;break;
 case 9:p=std::fmod(phase*16,1.0)/16;break;
 case 10:p=(step+1-local)/16.0-1.0/length;break;
 case 11:p=phase*0.5;break;
 case 12:p=std::fmod(phase*2,1.0);break;
 case 13:p=phase-phase*phase*0.5;break;
 case 14:p=phase*phase;break;
 case 15:{const double segment=std::floor(phase*4),u=phase*4-segment;
 p=(segment+(u<0.5?2*u:2*(1-u)))/4;
 if(u>=0.5)p-=1.0/length;
 break;}
 case 16:{constexpr int order[]={0,4,8,12,2,6,10,14,1,5,9,13,3,7,11,15};
 p=(order[step]+local)/16.0;
 break;}
 case 17:{constexpr int order[]={0,0,0,3,7,7,6,6,8,12,12,12,4,14,14,15};
 p=order[step]/16.0;
 break;}
 }
 return std::floor(origin+cycle*length+std::clamp(p,0.0,1.0-1.0/length)*length+1e-7);
}
}
