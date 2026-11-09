#pragma once
#include "timing.h"
namespace gross {
inline constexpr std::array<const char*,10> fadeNames={"Gate","Pulse","Fade in","Fade out","Triangle","Sidechain","Strobe","Double pulse","Tremolo","Custom envelope"};
inline double curve(const std::array<double,16>& points,double phase,int mode){
 double pos=phase*16;int i=std::min(15,int(pos));double u=pos-i;
 if(mode==0)return points[i];
 if(mode==2)u=u*u*(3-2*u);
 return points[i]+(points[(i+1)%16]-points[i])*u;
}
inline double envelope(double phase,int preset,double duty,const std::array<double,16>& points,int interpolation){
 duty=std::clamp(duty,0.01,0.99);
 switch(preset){
 case 0:return phase<duty?1:0;
 case 1:return phase<duty?std::sin(phase/duty*3.141592653589793):0;
 case 2:return phase;
 case 3:return 1-phase;
 case 4:return 1-std::abs(2*phase-1);
 case 5:{double u=std::min(1.0,phase/duty);return u*u*(3-2*u);}
 case 6:return std::fmod(phase*8,1.0)<duty?1:0;
 case 7:return std::fmod(phase*2,1.0)<duty?1:0;
 case 8:return 0.5-0.5*std::cos(phase*6.283185307179586);
 case 9:return curve(points,phase,interpolation);
 default:return 1;
 }
}
// Float RGBA, preserving HDR values. Black fade preserves alpha;
// opacity fade scales RGB only for premultiplied input.
inline void fadeRow(const float* src,float* dst,int pixels,float gain,bool opacity,bool premultiplied){
 for(int x=0;x<pixels;x++){
  const float rgb=(!opacity||premultiplied)?gain:1.0f;
  dst[x*4]=src[x*4]*rgb;dst[x*4+1]=src[x*4+1]*rgb;dst[x*4+2]=src[x*4+2]*rgb;
  dst[x*4+3]=src[x*4+3]*(opacity?gain:1.0f);
 }
}
}
