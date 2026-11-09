#include "ofxImageEffect.h"
#include "ofxParam.h"
#include "timing.h"
#include <cstring>
#include <cstdio>
#include <string>
static OfxHost* host;
static OfxPropertySuiteV1* p;
static OfxImageEffectSuiteV1* e;
static OfxParameterSuiteV1* a;
static void setHost(OfxHost* h){host=h;}
static void str(OfxPropertySetHandle h,const char* k,const char* v,int i=0){p->propSetString(h,k,i,v);}
static void integer(OfxPropertySetHandle h,const char* k,int v){p->propSetInt(h,k,0,v);}
static void number(OfxParamSetHandle ps,const char* name,double value,double low,double high){
 OfxPropertySetHandle q; a->paramDefine(ps,kOfxParamTypeDouble,name,&q);
 p->propSetDouble(q,kOfxParamPropDefault,0,value);p->propSetDouble(q,kOfxParamPropMin,0,low);p->propSetDouble(q,kOfxParamPropMax,0,high);
 p->propSetDouble(q,kOfxParamPropDisplayMin,0,low);p->propSetDouble(q,kOfxParamPropDisplayMax,0,high);
}
static double value(OfxParamSetHandle ps,const char* name,double t){OfxParamHandle h;double v=0;a->paramGetHandle(ps,name,&h,nullptr);a->paramGetValueAtTime(h,t,&v);return v;}
static double mapped(OfxImageEffectHandle fx,double t){
 OfxParamSetHandle ps;e->getParamSet(fx,&ps);OfxParamHandle h;int mode=0;a->paramGetHandle(ps,"Preset",&h,nullptr);a->paramGetValueAtTime(h,t,&mode);
 OfxImageClipHandle clip;OfxPropertySetHandle cp;e->clipGetHandle(fx,"Source",&clip,&cp);double fps=24;p->propGetDouble(cp,kOfxImageEffectPropFrameRate,0,&fps);
 std::array<double,16> steps{};for(int i=0;i<16;i++){char name[32];std::snprintf(name,sizeof(name),"Step %02d",i+1);steps[i]=value(ps,name,t);}
 double result=gross::frame(t,fps,value(ps,"BPM",t),value(ps,"Beats",t),value(ps,"Origin frame",t),mode,steps);
 double range[2];if(p->propGetDoubleN(cp,kOfxImageEffectPropFrameRange,2,range)==kOfxStatOK)result=std::clamp(result,range[0],range[1]);return result;
}
struct Image {OfxPropertySetHandle h=nullptr;~Image(){if(h)e->clipReleaseImage(h);}};
static OfxStatus entry(const char* action,const void* handle,OfxPropertySetHandle in,OfxPropertySetHandle out){
 try {
 auto fx=(OfxImageEffectHandle)handle;
 if(!std::strcmp(action,kOfxActionLoad)){
 p=(OfxPropertySuiteV1*)host->fetchSuite(host->host,kOfxPropertySuite,1);e=(OfxImageEffectSuiteV1*)host->fetchSuite(host->host,kOfxImageEffectSuite,1);a=(OfxParameterSuiteV1*)host->fetchSuite(host->host,kOfxParameterSuite,1);return p&&e&&a?kOfxStatOK:kOfxStatErrMissingHostFeature;}
 if(!std::strcmp(action,kOfxActionDescribe)){
 OfxPropertySetHandle q;e->getPropertySet(fx,&q);str(q,kOfxPropLabel,"Gross Video");str(q,kOfxImageEffectPluginPropGrouping,"Gross Video");str(q,kOfxImageEffectPropSupportedContexts,kOfxImageEffectContextFilter);str(q,kOfxImageEffectPropSupportedPixelDepths,kOfxBitDepthFloat);integer(q,kOfxImageEffectPropTemporalClipAccess,1);integer(q,kOfxImageEffectPropSupportsTiles,0);integer(q,kOfxImageEffectPropSupportsMultiResolution,0);str(q,kOfxImageEffectPluginRenderThreadSafety,kOfxImageEffectRenderFullySafe);return kOfxStatOK;}
 if(!std::strcmp(action,kOfxImageEffectActionDescribeInContext)){
 OfxPropertySetHandle q;for(auto name:{"Source","Output"}){e->clipDefine(fx,name,&q);str(q,kOfxImageEffectPropSupportedComponents,kOfxImageComponentRGBA);integer(q,kOfxImageEffectPropTemporalClipAccess,1);}
 OfxParamSetHandle ps;e->getParamSet(fx,&ps);a->paramDefine(ps,kOfxParamTypeChoice,"Preset",&q);const char* labels[]={"Normal","Stutter x4","Reverse","Freeze","Ping pong","Custom steps"};for(int i=0;i<6;i++)str(q,kOfxParamPropChoiceOption,labels[i],i);
 number(ps,"BPM",120,1,400);number(ps,"Beats",4,1,16);number(ps,"Origin frame",0,-1000000,1000000);
 for(int i=0;i<16;i++){char name[32];std::snprintf(name,sizeof(name),"Step %02d",i+1);number(ps,name,i/16.0,0,1);}return kOfxStatOK;}
 if(!std::strcmp(action,kOfxImageEffectActionGetFramesNeeded)){double t;p->propGetDouble(in,kOfxPropTime,0,&t);double r[2]={mapped(fx,t),mapped(fx,t)};p->propSetDoubleN(out,"OfxImageClipPropFrameRange_Source",2,r);return kOfxStatOK;}
 if(!std::strcmp(action,kOfxImageEffectActionRender)){
 double t;p->propGetDouble(in,kOfxPropTime,0,&t);OfxImageClipHandle source,dest;e->clipGetHandle(fx,"Source",&source,nullptr);e->clipGetHandle(fx,"Output",&dest,nullptr);Image s,d;
 if(e->clipGetImage(source,mapped(fx,t),nullptr,&s.h)!=kOfxStatOK)return kOfxStatFailed;
 if(e->clipGetImage(dest,t,nullptr,&d.h)!=kOfxStatOK)return kOfxStatFailed;
 void *sp,*dp;int sb[4],db[4],win[4],sr,dr;p->propGetPointer(s.h,kOfxImagePropData,0,&sp);p->propGetPointer(d.h,kOfxImagePropData,0,&dp);p->propGetIntN(s.h,kOfxImagePropBounds,4,sb);p->propGetIntN(d.h,kOfxImagePropBounds,4,db);p->propGetIntN(in,kOfxImageEffectPropRenderWindow,4,win);p->propGetInt(s.h,kOfxImagePropRowBytes,0,&sr);p->propGetInt(d.h,kOfxImagePropRowBytes,0,&dr);
 for(int y=std::max(win[1],db[1]);y<std::min(win[3],db[3]);y++){if(e->abort(fx))break;for(int x=std::max(win[0],db[0]);x<std::min(win[2],db[2]);x++){auto dst=(char*)dp+(y-db[1])*dr+(x-db[0])*16;if(x>=sb[0]&&x<sb[2]&&y>=sb[1]&&y<sb[3])std::memcpy(dst,(char*)sp+(y-sb[1])*sr+(x-sb[0])*16,16);else std::memset(dst,0,16);}}return kOfxStatOK;}
 return kOfxStatReplyDefault;
 }catch(...){return kOfxStatFailed;}
}
static OfxPlugin plugin={kOfxImageEffectPluginApi,1,"com.tristokoni.grossvideo",1,0,setHost,entry};
extern "C" __attribute__((visibility("default"))) int OfxGetNumberOfPlugins(){return 1;}
extern "C" __attribute__((visibility("default"))) OfxPlugin* OfxGetPlugin(int n){return n==0?&plugin:nullptr;}
