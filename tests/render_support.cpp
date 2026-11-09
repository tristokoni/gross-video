#include "../src/render_support.h"
#include <cassert>
#include <iostream>
#include <limits>
#include <vector>
int main(){
 assert(gross::clampFrame(120,0,99)==99); // final partial cycle
 assert(gross::clampFrame(-10,0,99)==0);
 assert(gross::clampFrame(99,0,99.75)==99);
 assert(gross::clampFrame(10,12,12)==12);
 assert(gross::clampFrame(20,99,0)==20); // invalid host metadata
 assert(gross::clampFrame(20,0,std::numeric_limits<double>::infinity())==20);
 float pixels[16]={};int bounds[]={0,0,4,1};
 assert(gross::validImage(pixels,bounds,64));
 assert(gross::validImage(pixels,bounds,-64));
 assert(!gross::validImage(pixels,bounds,63));
 assert(!gross::validImage(nullptr,bounds,64));
 bool used=false;std::vector<double> requests;
 auto fetch=[&](double t){requests.push_back(t);return t==99?0:1;};
 assert(gross::fetchTemporal(120,99,true,fetch,used)==0&&used);
 assert((requests==std::vector<double>{120,99}));
 requests.clear();assert(gross::fetchTemporal(120,99,false,fetch,used)==1&&!used);
 assert(requests.size()==1);
 requests.clear();assert(gross::fetchTemporal(99,99,true,fetch,used)==0&&!used);
 assert(requests.size()==1);
 requests.clear();assert(gross::fetchTemporal(120,100,true,fetch,used)==1&&used);
 assert(requests.size()==2); // never conceal a failure of the current frame
 std::cout<<"Render support tests passed: end bounds, strides, fallback, failure propagation.\n";
}
