#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <vector>
namespace gross {
struct Tempo {double bpm,firstBeat,confidence;};
inline Tempo detectTempo(const std::vector<double>& energy){
 if(energy.size()<800)throw std::runtime_error("Use at least 8 seconds of rhythmic audio.");
 std::vector<double> onset(energy.size());double peak=0;
 for(size_t i=1;i<energy.size();i++){onset[i]=std::max(0.0,energy[i]-energy[i-1]);peak=std::max(peak,onset[i]);}
 if(peak<1e-7)throw std::runtime_error("No clear beat detected; set BPM manually.");
 double best=0;int lag=0;
 for(int l=33;l<=100;l++){
  double dot=0,aa=0,bb=0;
  for(size_t i=l;i<onset.size();i++){dot+=onset[i]*onset[i-l];aa+=onset[i]*onset[i];bb+=onset[i-l]*onset[i-l];}
  double score=dot/std::sqrt(std::max(1e-30,aa*bb));
  if(score>best){best=score;lag=l;}
 }
 if(best<0.15)throw std::runtime_error("Beat is ambiguous; use manual BPM.");
 // First strong onset provides an alignment anchor, not a musical downbeat.
 size_t first=0;while(first<onset.size()&&onset[first]<peak*0.4)first++;
 return {6000.0/lag,first/100.0,best};
}
inline Tempo analyzeWav(const char* path){
 std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("Cannot open music WAV.");
 auto u16=[](const unsigned char* b){return unsigned(b[0])|(unsigned(b[1])<<8);};
 auto u32=[](const unsigned char* b){return std::uint32_t(b[0])|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24);};
 unsigned char header[12];if(!f.read((char*)header,12)||std::memcmp(header,"RIFF",4)||std::memcmp(header+8,"WAVE",4))throw std::runtime_error("Choose a PCM16/24 or float32 WAV file.");
 unsigned format=0,channels=0,rate=0,bits=0,align=0;
 std::uint32_t bytes=0;bool found=false;
 for(int chunks=0;chunks<1024;chunks++){
  unsigned char h[8];if(!f.read((char*)h,8))break;auto size=u32(h+4);
  if(!std::memcmp(h,"fmt ",4)){
   if(size<16)throw std::runtime_error("Invalid WAV format.");
   unsigned char fmt[16];if(!f.read((char*)fmt,16))break;
   format=u16(fmt);channels=u16(fmt+2);rate=u32(fmt+4);align=u16(fmt+12);bits=u16(fmt+14);
   f.seekg(std::streamoff(size-16+(size&1)),std::ios::cur);
  }else if(!std::memcmp(h,"data",4)){bytes=size;found=true;break;}
  else f.seekg(std::streamoff(size)+(size&1),std::ios::cur);
 }
 if(!found||channels<1||channels>8||rate<8000||rate>192000||align!=channels*(bits/8)||!((format==1&&(bits==16||bits==24))||(format==3&&bits==32)))throw std::runtime_error("Supported WAV: PCM16/24 or float32, 8–192 kHz, up to 8 channels.");
 const unsigned frames=rate/100;std::vector<unsigned char> block(frames*align);std::vector<double> energy;
 for(unsigned i=0;i<3000&&bytes>=block.size();i++){
  if(!f.read((char*)block.data(),block.size()))throw std::runtime_error("Truncated WAV data.");
  bytes-=block.size();double sum=0;
  for(size_t offset=0;offset<block.size();offset+=bits/8){
   const auto b=block.data()+offset;double sample=0;
   if(format==3){auto word=u32(b);float v;std::memcpy(&v,&word,4);sample=std::isfinite(v)?v:0;}
   else if(bits==16){int v=int(u16(b));if(v>=32768)v-=65536;sample=v/32768.0;}
   else {int v=int(b[0])|(int(b[1])<<8)|(int(b[2])<<16);if(v>=8388608)v-=16777216;sample=v/8388608.0;}
   sum+=sample*sample;
  }
  energy.push_back(sum/(frames*channels));
 }
 auto result=detectTempo(energy);
 // Correct the analysis grid when sample rate is not divisible by 100.
 double secondsPerBin=double(frames)/rate;
 result.bpm*=0.01/secondsPerBin;result.firstBeat*=secondsPerBin/0.01;
 return result;
}
}
