#include "../src/plugin.cpp"
#include <cassert>
#include <cstdarg>
#include <map>
#include <iostream>
struct Param {std::string name;int enabled=1,secret=0;};
static std::map<std::string,Param> params;
static int lookups=0,samples=0;
static int timeMode=0,fadeMode=0,fadeOn=0;
static const char* button="Align here";
static std::map<std::string,double> edits;static int begun=0,ended=0;
static OfxStatus getString(OfxPropertySetHandle,const char* key,int,char** out){*out=const_cast<char*>(!std::strcmp(key,kOfxPropName)?button:kOfxChangeUserEdited);return kOfxStatOK;}
static OfxStatus getDouble(OfxPropertySetHandle,const char*,int,double* out){*out=42;return kOfxStatOK;}
static OfxStatus getSet(OfxImageEffectHandle,OfxParamSetHandle* out){*out=(OfxParamSetHandle)1;return kOfxStatOK;}
static OfxStatus begin(OfxParamSetHandle,const char*){++begun;return kOfxStatOK;}
static OfxStatus end(OfxParamSetHandle){++ended;return kOfxStatOK;}
static OfxStatus edit(OfxParamHandle h,OfxTime t,...){assert(t==42);va_list args;va_start(args,t);edits[((Param*)h)->name]=va_arg(args,double);va_end(args);return kOfxStatOK;}

static OfxStatus handle(OfxParamSetHandle,const char* name,OfxParamHandle* h,OfxPropertySetHandle* q){
 ++lookups;auto& r=params[name];r.name=name;*h=(OfxParamHandle)&r;if(q)*q=(OfxPropertySetHandle)&r;return kOfxStatOK;
}
static OfxStatus sample(OfxParamHandle h,OfxTime t,...){
 ++samples;auto& r=*(Param*)h;va_list args;va_start(args,t);
 if(r.name=="BPM")*va_arg(args,double*)=120+t;
 else {int v=r.name=="Time enabled"?1:r.name=="Preset"?timeMode:r.name=="Fade preset"?fadeMode:r.name=="Fade enabled"?fadeOn:0;*va_arg(args,int*)=v;}
 va_end(args);return kOfxStatOK;
}
static OfxStatus setInt(OfxPropertySetHandle q,const char* key,int,int v){auto& r=*(Param*)q;if(!std::strcmp(key,kOfxParamPropEnabled))r.enabled=v;if(!std::strcmp(key,kOfxParamPropSecret))r.secret=v;return kOfxStatOK;}
int main(){
 OfxParameterSuiteV1 parameterSuite{};parameterSuite.paramGetHandle=handle;parameterSuite.paramGetValueAtTime=sample;a=&parameterSuite;
 OfxPropertySuiteV1 propertySuite{};propertySuite.propSetInt=setInt;p=&propertySuite;
 auto ps=(OfxParamSetHandle)1;
 {ReadScope scope;for(int i=0;i<100;i++)assert(value(ps,"BPM",0)==120);assert(lookups==1&&samples==1);
  {ReadScope nested;assert(value(ps,"BPM",2)==122);}assert(value(ps,"BPM",0)==120);
  assert(value(ps,"BPM",1)==121);assert(value((OfxParamSetHandle)2,"BPM",3)==123);
 }
 assert(reads==nullptr);
 {ReadScope scope;updateUI(ps,0);assert(params["Step 01"].secret==1);assert(params["Fade depth"].enabled==0);}
 timeMode=5;fadeOn=1;fadeMode=9;
 {ReadScope scope;updateUI(ps,0);assert(params["Step 01"].secret==0);assert(params["Fade point 16"].secret==0);assert(params["Fade duty"].secret==1);}
 fadeMode=0;
 {ReadScope scope;updateUI(ps,0);assert(params["Fade point 16"].secret==1);assert(params["Fade duty"].enabled==1&&params["Fade duty"].secret==0);}
 parameterSuite.paramSetValueAtTime=edit;parameterSuite.paramEditBegin=begin;parameterSuite.paramEditEnd=end;
 propertySuite.propGetString=getString;propertySuite.propGetDouble=getDouble;
 OfxImageEffectSuiteV1 effects{};effects.getParamSet=getSet;e=&effects;
 for(auto name:{"Align here","Reset time pattern","Reset fade pattern"}){button=name;assert(entry(kOfxActionInstanceChanged,(void*)1,nullptr,nullptr)==kOfxStatOK);}
 assert(edits["Origin frame"]==42&&edits["Step 16"]==15.0/16&&edits["Fade point 01"]==1);
 assert(begun==3&&ended==3);
 std::cout<<"UI/cache tests passed: visibility, 100 reads -> 1 host lookup/sample, fresh time/instance/nested actions.\n";
}
