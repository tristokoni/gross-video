#include "../src/envelope.h"
#include <cassert>
#include <iostream>
int main(){
 std::array<double,16> points{};points[1]=1;
 assert(gross::curve(points,1.0/32,0)==0);
 assert(gross::curve(points,1.0/32,1)==0.5);
 assert(gross::curve(points,1.0/32,2)==0.5);
 assert(gross::envelope(0.25,0,0.5,points,0)==1);
 assert(gross::envelope(0.75,0,0.5,points,0)==0);
 assert(gross::envelope(0.25,2,0.5,points,0)==0.25);
 assert(gross::envelope(0.25,3,0.5,points,0)==0.75);
 assert(gross::envelope(0.5,5,0.5,points,0)==1);
 for(int mode=0;mode<10;mode++)for(int i=0;i<1000;i++)for(int interp=0;interp<3;interp++){
  double v=gross::envelope(i/1000.0,mode,0.3,points,interp);assert(v>=0&&v<=1);
 }
 float src[]={4,2,-1,0.5f},out[4];
 gross::fadeRow(src,out,1,0.5f,false,true);
 assert(out[0]==2&&out[2]==-0.5f&&out[3]==0.5f);
 gross::fadeRow(src,out,1,0.5f,true,true);
 assert(out[0]==2&&out[3]==0.25f);
 gross::fadeRow(src,out,1,0.5f,true,false);
 assert(out[0]==4&&out[3]==0.25f);
 gross::fadeRow(src,out,1,0,false,true);
 assert(out[0]==0&&out[3]==0.5f);
 std::cout<<"Envelope tests passed: all presets, interpolation, HDR, opacity and premultiplication.\n";
}
