#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace gross {
inline double frame(double t,double fps,double bpm,double beats,double origin,int preset,const std::array<double,16>& steps) {
 const double length=std::max(1.0,fps*60.0/std::max(1.0,bpm)*beats);
 const double cycle=std::floor((t-origin)/length), phase=(t-origin)/length-cycle;
 double p=phase;
 switch(preset){
 case 1:p=std::fmod(phase*4.0,1.0)/4.0;break;
 case 2:p=1.0-phase-1.0/length;break;
 case 3:p=0;break;
 case 4:p=phase<0.5?phase*2:(1-phase)*2-1.0/length;break;
 case 5:p=steps[std::min(15,int(phase*16))];break;
 }
 return std::floor(origin+cycle*length+std::clamp(p,0.0,1.0-1.0/length)*length+1e-7);
}
}
