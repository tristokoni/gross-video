#include "ofxImageEffect.h"
#include "ofxParam.h"
#include "envelope.h"
#include "render_support.h"
#include "music.h"
#include "ofxMessage.h"
#include <cstring>
#include <cstdio>
#include <string>
static OfxHost* host;
static OfxPropertySuiteV1* p;
static OfxImageEffectSuiteV1* e;
static OfxParameterSuiteV1* a;
static OfxMessageSuiteV1* messages;
static void setHost(OfxHost* h){host=h;}
static void str(OfxPropertySetHandle h,const char* k,const char* v,int i=0){p->propSetString(h,k,i,v);}
static void integer(OfxPropertySetHandle h,const char* k,int v){p->propSetInt(h,k,0,v);}
static void defineParam(OfxParamSetHandle ps,const char* type,const char* name,OfxPropertySetHandle* q){
 auto status=a->paramDefine(ps,type,name,q);if(status!=kOfxStatOK)throw status;
 if(!std::strcmp(type,kOfxParamTypeGroup))return;
 const char* parent="Timing";
 if(!std::strncmp(name,"Step ",5)||!std::strcmp(name,"Custom interpolation"))parent="Custom time";
 else if(!std::strncmp(name,"Fade point ",11)||!std::strcmp(name,"Fade interpolation"))parent="Custom fade";
 else if(!std::strncmp(name,"Fade ",5))parent="Video fade";
 else if(!std::strncmp(name,"Music ",6)||!std::strcmp(name,"Analyze music"))parent="Music sync";
 str(*q,kOfxParamPropParent,parent);
}
static void number(OfxParamSetHandle ps,const char* name,double value,double low,double high){
 OfxPropertySetHandle q; defineParam(ps,kOfxParamTypeDouble,name,&q);
 p->propSetDouble(q,kOfxParamPropDefault,0,value);p->propSetDouble(q,kOfxParamPropMin,0,low);p->propSetDouble(q,kOfxParamPropMax,0,high);
 p->propSetDouble(q,kOfxParamPropDisplayMin,0,low);p->propSetDouble(q,kOfxParamPropDisplayMax,0,high);
}
static double value(OfxParamSetHandle ps,const char* name,double t){OfxParamHandle h=nullptr;double v=0;auto status=a->paramGetHandle(ps,name,&h,nullptr);if(status!=kOfxStatOK)throw status;status=a->paramGetValueAtTime(h,t,&v);if(status!=kOfxStatOK)throw status;if(!std::isfinite(v))throw OfxStatus(kOfxStatErrValue);return v;}
static int choiceValue(OfxParamSetHandle ps,const char* name,double t){OfxParamHandle h=nullptr;int v=0;auto status=a->paramGetHandle(ps,name,&h,nullptr);if(status!=kOfxStatOK)throw status;status=a->paramGetValueAtTime(h,t,&v);if(status!=kOfxStatOK)throw status;return v;}
static void choice(OfxParamSetHandle ps,const char* name,std::initializer_list<const char*> labels){OfxPropertySetHandle q;defineParam(ps,kOfxParamTypeChoice,name,&q);int i=0;for(auto label:labels)str(q,kOfxParamPropChoiceOption,label,i++);}
static void toggle(OfxParamSetHandle ps,const char* name,int initial){OfxPropertySetHandle q;defineParam(ps,kOfxParamTypeBoolean,name,&q);integer(q,kOfxParamPropDefault,initial);}
static double mapped(OfxImageEffectHandle fx,double t){
 OfxParamSetHandle ps;e->getParamSet(fx,&ps);int mode=choiceValue(ps,"Preset",t);
 OfxImageClipHandle clip;OfxPropertySetHandle cp;e->clipGetHandle(fx,"Source",&clip,&cp);double fps=24;p->propGetDouble(cp,kOfxImageEffectPropFrameRate,0,&fps);
 if(!choiceValue(ps,"Time enabled",t)||mode==0)return t;
 std::array<double,16> steps{};if(mode==5)for(int i=0;i<16;i++){char name[32];std::snprintf(name,sizeof(name),"Step %02d",i+1);steps[i]=value(ps,name,t);}
 const double bpm=std::max(1.0,value(ps,"BPM",t)),beats=std::max(0.0625,value(ps,"Beats",t)),origin=value(ps,"Origin frame",t);
 if(mode==5 && choiceValue(ps,"Custom interpolation",t)!=0){
  const double length=std::max(1.0,fps*60/bpm*beats),cycle=std::floor((t-origin)/length);
  const double phase=(t-origin)/length-cycle;
  steps.fill(gross::curve(steps,phase,choiceValue(ps,"Custom interpolation",t)));
 }
 double result=gross::frame(t,fps,bpm,beats,origin,mode,steps);
 double range[2];if(p->propGetDoubleN(cp,kOfxImageEffectPropFrameRange,2,range)==kOfxStatOK)result=gross::clampFrame(result,range[0],range[1]);return result;
}
struct Image {OfxPropertySetHandle h=nullptr;~Image(){if(h)e->clipReleaseImage(h);}};
static OfxStatus entry(const char* action,const void* handle,OfxPropertySetHandle in,OfxPropertySetHandle out){
 try {
 auto fx=(OfxImageEffectHandle)handle;
 if(!std::strcmp(action,kOfxActionLoad)){
 p=(OfxPropertySuiteV1*)host->fetchSuite(host->host,kOfxPropertySuite,1);e=(OfxImageEffectSuiteV1*)host->fetchSuite(host->host,kOfxImageEffectSuite,1);a=(OfxParameterSuiteV1*)host->fetchSuite(host->host,kOfxParameterSuite,1);messages=(OfxMessageSuiteV1*)host->fetchSuite(host->host,kOfxMessageSuite,1);return p&&e&&a?kOfxStatOK:kOfxStatErrMissingHostFeature;}
 if(!std::strcmp(action,kOfxActionDescribe)){
 OfxPropertySetHandle q;e->getPropertySet(fx,&q);str(q,kOfxPropLabel,"Gross Video");str(q,kOfxImageEffectPluginPropGrouping,"Gross Video");str(q,kOfxImageEffectPropSupportedContexts,kOfxImageEffectContextFilter);str(q,kOfxImageEffectPropSupportedPixelDepths,kOfxBitDepthFloat);integer(q,kOfxImageEffectPropTemporalClipAccess,1);integer(q,kOfxImageEffectPropSupportsTiles,0);integer(q,kOfxImageEffectPropSupportsMultiResolution,0);str(q,kOfxImageEffectPluginRenderThreadSafety,kOfxImageEffectRenderFullySafe);return kOfxStatOK;}
 if(!std::strcmp(action,kOfxImageEffectActionDescribeInContext)){
 OfxPropertySetHandle q;for(auto name:{"Source","Output"}){e->clipDefine(fx,name,&q);str(q,kOfxImageEffectPropSupportedComponents,kOfxImageComponentRGBA);integer(q,kOfxImageEffectPropTemporalClipAccess,1);}
 OfxParamSetHandle ps;e->getParamSet(fx,&ps);
 for(auto group:{"Timing","Custom time","Video fade","Custom fade","Music sync"}){defineParam(ps,kOfxParamTypeGroup,group,&q);integer(q,kOfxParamPropGroupOpen,0);}
 defineParam(ps,kOfxParamTypeChoice,"Preset",&q);for(int i=0;i<int(gross::presetNames.size());i++)str(q,kOfxParamPropChoiceOption,gross::presetNames[i],i);
 toggle(ps,"Time enabled",1);toggle(ps,"Safe frame fallback",1);choice(ps,"Custom interpolation",{"Hold","Linear","Smooth"});
 number(ps,"BPM",120,1,400);number(ps,"Beats",4,1,16);number(ps,"Origin frame",0,-1000000,1000000);
 for(int i=0;i<16;i++){char name[32];std::snprintf(name,sizeof(name),"Step %02d",i+1);number(ps,name,i/16.0,0,1);}
 toggle(ps,"Fade enabled",0);
 defineParam(ps,kOfxParamTypeChoice,"Fade preset",&q);for(int i=0;i<int(gross::fadeNames.size());i++)str(q,kOfxParamPropChoiceOption,gross::fadeNames[i],i);
 choice(ps,"Fade target",{"Black","Opacity"});
 number(ps,"Fade beats",1,0.0625,16);number(ps,"Fade phase",0,0,1);
 number(ps,"Fade depth",1,0,1);number(ps,"Fade minimum",0,0,1);number(ps,"Fade duty",0.5,0.01,0.99);
 choice(ps,"Fade interpolation",{"Hold","Linear","Smooth"});
 for(int i=0;i<16;i++){char name[32];std::snprintf(name,sizeof(name),"Fade point %02d",i+1);number(ps,name,1,0,1);}
 defineParam(ps,kOfxParamTypeString,"Music WAV",&q);str(q,kOfxParamPropStringMode,kOfxParamStringIsFilePath);integer(q,kOfxParamPropAnimates,0);
 number(ps,"Music start frame",0,-1000000,1000000);
 defineParam(ps,kOfxParamTypePushButton,"Analyze music",&q);
 str(q,kOfxParamPropHint,"Analyze the first 30 seconds of a WAV; estimate BPM and first strong onset. Set Music start frame to align the file with the effect timeline.");
 return kOfxStatOK;}
 if(!std::strcmp(action,kOfxActionInstanceChanged)){
  char *name=nullptr,*reason=nullptr;
  p->propGetString(in,kOfxPropName,0,&name);p->propGetString(in,kOfxPropChangeReason,0,&reason);
  if(!name||std::strcmp(name,"Analyze music")||!reason||std::strcmp(reason,kOfxChangeUserEdited))return kOfxStatReplyDefault;
  OfxParamSetHandle ps;e->getParamSet(fx,&ps);double t=0;p->propGetDouble(in,kOfxPropTime,0,&t);
  try {
   OfxParamHandle path;char* file=nullptr;
   if(a->paramGetHandle(ps,"Music WAV",&path,nullptr)!=kOfxStatOK||a->paramGetValue(path,&file)!=kOfxStatOK||!file)throw std::runtime_error("Select a WAV file first.");
   auto tempo=gross::analyzeWav(file);OfxImageClipHandle source;OfxPropertySetHandle cp;
   e->clipGetHandle(fx,"Source",&source,&cp);double fps=24;p->propGetDouble(cp,kOfxImageEffectPropFrameRate,0,&fps);
   OfxParamHandle bpm,origin;
   if(a->paramGetHandle(ps,"BPM",&bpm,nullptr)!=kOfxStatOK||a->paramGetHandle(ps,"Origin frame",&origin,nullptr)!=kOfxStatOK)throw std::runtime_error("Cannot access timing controls.");
   double anchor=value(ps,"Music start frame",t)+tempo.firstBeat*fps;
   a->paramEditBegin(ps,"Analyze music");auto bs=a->paramSetValueAtTime(bpm,t,tempo.bpm);auto os=a->paramSetValueAtTime(origin,t,anchor);a->paramEditEnd(ps);
   if(bs!=kOfxStatOK||os!=kOfxStatOK)throw std::runtime_error("Could not update timing; check BPM and Origin manually.");
   if(messages)messages->message(fx,kOfxMessageMessage,"music","Estimated %.1f BPM. First onset at %.2f seconds. Check half/double tempo and beat alignment.",tempo.bpm,tempo.firstBeat);
  }catch(const std::exception& ex){if(messages)messages->message(fx,kOfxMessageError,"music","%s",ex.what());else std::fprintf(stderr,"Gross Video music: %s\n",ex.what());}
  return kOfxStatOK;
 }
 // Resolve may bypass temporal remapping for identity responses. Always render
 // through clipGetImage(mapped time), including when fade is disabled.
 if(!std::strcmp(action,kOfxImageEffectActionIsIdentity))return kOfxStatReplyDefault;
 if(!std::strcmp(action,kOfxImageEffectActionGetFramesNeeded)){double t;p->propGetDouble(in,kOfxPropTime,0,&t);double sourceTime=mapped(fx,t);OfxParamSetHandle ps;e->getParamSet(fx,&ps);
 double r[4]={sourceTime,sourceTime,t,t};int count=sourceTime!=t&&choiceValue(ps,"Safe frame fallback",t)?4:2;
 p->propSetDoubleN(out,"OfxImageClipPropFrameRange_Source",count,r);return kOfxStatOK;}
 if(!std::strcmp(action,kOfxImageEffectActionRender)){
 double t;p->propGetDouble(in,kOfxPropTime,0,&t);OfxImageClipHandle source,dest;e->clipGetHandle(fx,"Source",&source,nullptr);e->clipGetHandle(fx,"Output",&dest,nullptr);Image s,d;
 OfxParamSetHandle ps;e->getParamSet(fx,&ps);
 const double requested=mapped(fx,t);bool usedFallback=false;
 int sourceStatus=gross::fetchTemporal(requested,t,choiceValue(ps,"Safe frame fallback",t)!=0,[&](double frame){
  if(s.h){e->clipReleaseImage(s.h);s.h=nullptr;}
  return e->clipGetImage(source,frame,nullptr,&s.h);
 },usedFallback);
 if(sourceStatus!=kOfxStatOK){
  if(e->abort(fx))return kOfxStatOK;
  std::fprintf(stderr,"Gross Video: source request failed, output=%.9g requested=%.9g fallback=%d status=%d\n",t,requested,int(usedFallback),sourceStatus);
  return sourceStatus;
 }
 if(usedFallback)std::fprintf(stderr,"Gross Video: unavailable temporal frame %.9g; using output frame %.9g\n",requested,t);
 if(e->clipGetImage(dest,t,nullptr,&d.h)!=kOfxStatOK)return kOfxStatFailed;
 void *sp=nullptr,*dp=nullptr;int sb[4]={},db[4]={},win[4]={},sr=0,dr=0;p->propGetPointer(s.h,kOfxImagePropData,0,&sp);p->propGetPointer(d.h,kOfxImagePropData,0,&dp);p->propGetIntN(s.h,kOfxImagePropBounds,4,sb);p->propGetIntN(d.h,kOfxImagePropBounds,4,db);p->propGetIntN(in,kOfxImageEffectPropRenderWindow,4,win);p->propGetInt(s.h,kOfxImagePropRowBytes,0,&sr);p->propGetInt(d.h,kOfxImagePropRowBytes,0,&dr);
 if(!gross::validImage(sp,sb,sr)||!gross::validImage(dp,db,dr))return kOfxStatErrImageFormat;
 for(auto image:{s.h,d.h}){
  char *depth=nullptr,*components=nullptr;
  if(p->propGetString(image,kOfxImageEffectPropPixelDepth,0,&depth)!=kOfxStatOK||!depth||std::strcmp(depth,kOfxBitDepthFloat))return kOfxStatErrImageFormat;
  if(p->propGetString(image,kOfxImageEffectPropComponents,0,&components)!=kOfxStatOK||!components||std::strcmp(components,kOfxImageComponentRGBA))return kOfxStatErrImageFormat;
 }
 float gain=1;bool opacity=false,premultiplied=true;
 if(choiceValue(ps,"Fade enabled",t)&&value(ps,"Fade depth",t)!=0&&value(ps,"Fade minimum",t)!=1){
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
  if((y&31)==0&&e->abort(fx))break;
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
 }catch(OfxStatus status){return status;}catch(...){return kOfxStatFailed;}
}
static OfxPlugin plugin={kOfxImageEffectPluginApi,1,"com.tristokoni.grossvideo",1,3,setHost,entry};
extern "C" __attribute__((visibility("default"))) int OfxGetNumberOfPlugins(){return 1;}
extern "C" __attribute__((visibility("default"))) OfxPlugin* OfxGetPlugin(int n){return n==0?&plugin:nullptr;}
