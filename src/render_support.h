#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
namespace gross {
inline double clampFrame(double requested,double first,double last){
 if(!std::isfinite(first)||!std::isfinite(last)||first>last)return requested;
 const double lo=std::ceil(first),hi=std::floor(last);
 return lo<=hi?std::clamp(requested,lo,hi):requested;
}
inline bool validImage(const void* data,const int* bounds,int stride){
 const auto width=std::int64_t(bounds[2])-bounds[0],height=std::int64_t(bounds[3])-bounds[1];
 const auto row=stride<0?-std::int64_t(stride):std::int64_t(stride);
 return data&&width>0&&height>0&&width<=std::numeric_limits<int>::max()/16&&height<=std::numeric_limits<int>::max()&&row>=width*4*std::int64_t(sizeof(float));
}
// Request/release ownership stays in the caller; no image handles survive render.
template<class Fetch> int fetchTemporal(double requested,double current,bool fallback,Fetch fetch,bool& usedFallback){
 usedFallback=false;int status=fetch(requested);
 if(status!=0&&fallback&&requested!=current){usedFallback=true;status=fetch(current);}
 return status;
}
}
