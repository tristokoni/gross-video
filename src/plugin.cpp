#include "ofxImageEffect.h"
#include "ofxParam.h"
#include "envelope.h"
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
static int choiceValue(OfxParamSetHandle ps,const char* name,double t){OfxParamHandle h;int v=0;a->paramGetHandle(ps,name,&h,nullptr);a->paramGetValueAtTime(h,t,&v);return v;}
static void choice(OfxParamSetHandle ps,const char* name,std::initializer_list<const char*> labels){OfxPropertySetHandle q;a->paramDefine(ps,kOfxParamTypeChoice,name,&q);int i=0;for(auto label:labels)str(q,kOfxParamPropChoiceOption,label,i++);}
static void toggle(OfxParamSetHandle ps,const char* name,int initial){OfxPropertySetHandle q;a->paramDefine(ps,kOfxParamTypeBoolean,name,&q);integer(q,kOfxParamPropDefault,initial);}
static double mapped(OfxImageEffectHandle fx,double t){
 OfxParamSetHandle ps;e->getParamSet(fx,&ps);OfxParamHandle h;int mode=0;a->paramGetHandle(ps,"Preset",&h,nullptr);a->paramGetValueAtTime(h,t,&mode);
 OfxImageClipHandle clip;OfxPropertySetHandle cp;e->clipGetHandle(fx,"Source",&clip,&cp);double fps=24;p->propGetDouble(cp,kOfxImageEffectPropFrameRate,0,&fps);
 if(!choiceValue(ps,"Time enabled",t))return t;
 std::array<double,16> steps{};if(mode==5)for(int i=0;i<16;i++){char name[32];std::snprintf(name,sizeof(name),"Step %02d",i+1);steps[i]=value(ps,name,t);}
 const double bpm=value(ps,"BPM",t),beats=value(ps,"Beats",t),origin=value(ps,"Origin frame",t);
 if(mode==5 && choiceValue(ps,"Custom interpolation",t)!=0){
  const double length=std::max(1.0,fps*60/bpm*beats),cycle=std::floor((t-origin)/length);
  const double phase=(t-origin)/length-cycle;
  steps.fill(gross::curve(steps,phase,choiceValue(ps,"Custom interpolation",t)));
 }
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
 OfxParamSetHandle ps;e->getParamSet(fx,&ps);a->paramDefine(ps,kOfxParamTypeChoice,"Preset",&q);for(int i=0;i<int(gross::presetNames.size());i++)str(q,kOfxParamPropChoiceOption,gross::presetNames[i],i);
 toggle(ps,"Time enabled",1);choice(ps,"Custom interpolation",{"Hold","Linear","Smooth"});
 number(ps,"BPM",120,1,400);number(ps,"Beats",4,1,16);number(ps,"Origin frame",0,-1000000,1000000);
 for(int i=0;i<16;i++){char name[32];std::snprintf(name,sizeof(name),"Step %02d",i+1);number(ps,name,i/16.0,0,1);}
 toggle(ps,"Fade enabled",0);
 a->paramDefine(ps,kOfxParamTypeChoice,"Fade preset",&q);for(int i=0;i<int(gross::fadeNames.size());i++)str(q,kOfxParamPropChoiceOption,gross::fadeNames[i],i);
 choice(ps,"Fade target",{"Black","Opacity"});
 number(ps,"Fade beats",1,0.0625,16);number(ps,"Fade phase",0,0,1);
 number(ps,"Fade depth",1,0,1);number(ps,"Fade minimum",0,0,1);number(ps,"Fade duty",0.5,0.01,0.99);
 choice(ps,"Fade interpolation",{"Hold","Linear","Smooth"});
 for(int i=0;i<16;i++){char name[32];std::snprintf(name,sizeof(name),"Fade point %02d",i+1);number(ps,name,1,0,1);}
 // Group controls while retaining all original parameter identifiers.
 for(auto group:{"Timing","Custom time","Video fade","Custom fade"}){a->paramDefine(ps,kOfxParamTypeGroup,group,&q);integer(q,kOfxParamPropGroupOpen,0);}
 for(auto name:{"Preset","Time enabled","BPM","Beats","Origin frame"}){OfxParamHandle h;a->paramGetHandle(ps,name,&h,&q);str(q,kOfxParamPropParent,"Timing");}
 for(auto name:{"Fade enabled","Fade preset","Fade target","Fade beats","Fade phase","Fade depth","Fade minimum","Fade duty"}){OfxParamHandle h;a->paramGetHandle(ps,name,&h,&q);str(q,kOfxParamPropParent,"Video fade");}
 for(int i=0;i<16;i++){char name[32];OfxParamHandle h;std::snprintf(name,sizeof(name),"Step %02d",i+1);a->paramGetHandle(ps,name,&h,&q);str(q,kOfxParamPropParent,"Custom time");std::snprintf(name,sizeof(name),"Fade point %02d",i+1);a->paramGetHandle(ps,name,&h,&q);str(q,kOfxParamPropParent,"Custom fade");}
 for(auto name:{"Custom interpolation","Fade interpolation"}){OfxParamHandle h;a->paramGetHandle(ps,name,&h,&q);str(q,kOfxParamPropParent,!std::strcmp(name,"Custom interpolation")?"Custom time":"Custom fade");}
 return kOfxStatOK;}
 if(!std::strcmp(action,kOfxImageEffectActionIsIdentity)){
  double t;p->propGetDouble(in,kOfxPropTime,0,&t);OfxParamSetHandle ps;e->getParamSet(fx,&ps);
  if(!choiceValue(ps,"Fade enabled",t)||value(ps,"Fade depth",t)==0||value(ps,"Fade minimum",t)==1){
   str(out,kOfxPropName,"Source");p->propSetDouble(out,kOfxPropTime,0,mapped(fx,t));return kOfxStatOK;
  }
  return kOfxStatReplyDefault;
 }
 if(!std::strcmp(action,kOfxImageEffectActionGetFramesNeeded)){double t;p->propGetDouble(in,kOfxPropTime,0,&t);double sourceTime=mapped(fx,t);double r[2]={sourceTime,sourceTime};p->propSetDoubleN(out,"OfxImageClipPropFrameRange_Source",2,r);return kOfxStatOK;}
 if(!std::strcmp(action,kOfxImageEffectActionRender)){
 double t;p->propGetDouble(in,kOfxPropTime,0,&t);OfxImageClipHandle source,dest;e->clipGetHandle(fx,"Source",&source,nullptr);e->clipGetHandle(fx,"Output",&dest,nullptr);Image s,d;
 if(e->clipGetImage(source,mapped(fx,t),nullptr,&s.h)!=kOfxStatOK)return kOfxStatFailed;
 if(e->clipGetImage(dest,t,nullptr,&d.h)!=kOfxStatOK)return kOfxStatFailed;
 void *sp,*dp;int sb[4],db[4],win[4],sr,dr;p->propGetPointer(s.h,kOfxImagePropData,0,&sp);p->propGetPointer(d.h,kOfxImagePropData,0,&dp);p->propGetIntN(s.h,kOfxImagePropBounds,4,sb);p->propGetIntN(d.h,kOfxImagePropBounds,4,db);p->propGetIntN(in,kOfxImageEffectPropRenderWindow,4,win);p->propGetInt(s.h,kOfxImagePropRowBytes,0,&sr);p->propGetInt(d.h,kOfxImagePropRowBytes,0,&dr);
 OfxParamSetHandle ps;e->getParamSet(fx,&ps);float gain=1;bool opacity=false,premultiplied=true;
 if(choiceValue(ps,"Fade enabled",t)){
  OfxPropertySetHandle cp;e->clipGetPropertySet(source,&cp);double fps=24;p->propGetDouble(cp,kOfxImageEffectPropFrameRate,0,&fps);
  const double length=std::max(1.0,fps*60/value(ps,"BPM",t)*value(ps,"Fade beats",t));
  double phase=(t-value(ps,"Origin frame",t))/length+value(ps,"Fade phase",t);phase-=std::floor(phase);
  int preset=choiceValue(ps,"Fade preset",t);std::array<double,16> points{};
  if(preset==9)for(int i=0;i<16;i++){char name[32];std::snprintf(name,sizeof(name),"Fade point %02d",i+1);points[i]=value(ps,name,t);}
  double env=gross::envelope(phase,preset,value(ps,"Fade duty",t),points,choiceValue(ps,"Fade interpolation",t));
  gain=float(1-value(ps,"Fade depth",t)*(1-value(ps,"Fade minimum",t))*(1-env));
  opacity=choiceValue(ps,"Fade target",t)==1;
  char* premult=nullptr;if(p->propGetString(s.h,kOfxImageEffectPropPreMultiplication,0,&premult)==kOfxStatOK && premult)premultiplied=std::strcmp(premult,kOfxImageUnPreMultiplied)!=0;
 }
 const int left=std::max(win[0],db[0]),right=std::min(win[2],db[2]);
 for(int y=std::max(win[1],db[1]);y<std::min(win[3],db[3]);y++){
  if(e->abort(fx))break;
  if(right<=left)continue;
  auto row=(char*)dp+std::ptrdiff_t(y-db[1])*dr;
  const int lo=std::max(left,sb[0]),hi=std::min(right,sb[2]);
  if(y<sb[1]||y>=sb[3]||hi<=lo){std::memset(row+(left-db[0])*16,0,(right-left)*16);continue;}
  if(lo>left)std::memset(row+(left-db[0])*16,0,(lo-left)*16);
  if(hi<right)std::memset(row+(hi-db[0])*16,0,(right-hi)*16);
  auto src=(const float*)((char*)sp+std::ptrdiff_t(y-sb[1])*sr+(lo-sb[0])*16);
  auto dst=(float*)(row+(lo-db[0])*16);
  if(gain==1)std::memcpy(dst,src,(hi-lo)*16);else gross::fadeRow(src,dst,hi-lo,gain,opacity,premultiplied);
 }
 return kOfxStatOK;}
 return kOfxStatReplyDefault;
 }catch(...){return kOfxStatFailed;}
}
static OfxPlugin plugin={kOfxImageEffectPluginApi,1,"com.tristokoni.grossvideo",1,1,setHost,entry};
extern "C" __attribute__((visibility("default"))) int OfxGetNumberOfPlugins(){return 1;}
extern "C" __attribute__((visibility("default"))) OfxPlugin* OfxGetPlugin(int n){return n==0?&plugin:nullptr;}
