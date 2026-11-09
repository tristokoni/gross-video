#include "../src/music.h"
#include <cassert>
#include <iostream>
int main(int argc,char** argv){
 std::vector<double> energy(2000);for(int i=20;i<2000;i+=50)energy[i]=1;
 auto result=gross::detectTempo(energy);assert(std::abs(result.bpm-120)<0.01);assert(result.firstBeat==0.2);
 bool failed=false;try{gross::detectTempo(std::vector<double>(1000));}catch(const std::runtime_error&){failed=true;}assert(failed);
 failed=false;try{gross::analyzeWav("/nonexistent/gross.wav");}catch(const std::runtime_error&){failed=true;}assert(failed);
 if(argc>1){result=gross::analyzeWav(argv[1]);assert(std::abs(result.bpm-120)<1);}
 std::cout<<"Music tests passed: synthetic 120 BPM, silence and missing file.\n";
}
