#include "../src/plugin.cpp"
#include <cassert>
#include <map>
#include <iostream>
struct Node{std::string name,type,parent;};
static std::map<std::string,Node> nodes;
static Node clip;
static int forbidden=0;
static OfxStatus define(OfxParamSetHandle,const char* type,const char* name,OfxPropertySetHandle* out){assert(!nodes.count(name));auto& n=nodes[name];n={name,type,""};*out=(OfxPropertySetHandle)&n;return kOfxStatOK;}
static OfxStatus get(OfxParamSetHandle,const char*,OfxParamHandle*,OfxPropertySetHandle*){forbidden++;return kOfxStatErrBadHandle;}
static OfxStatus setString(OfxPropertySetHandle h,const char* key,int,const char* v){
 assert(h);if(!std::strcmp(key,kOfxParamPropParent)){auto n=(Node*)h;assert(n->name!=v);assert(nodes.count(v)&&nodes[v].type==kOfxParamTypeGroup);n->parent=v;}return kOfxStatOK;
}
static OfxStatus setInt(OfxPropertySetHandle,const char*,int,int){return kOfxStatOK;}
static OfxStatus setDouble(OfxPropertySetHandle,const char*,int,double){return kOfxStatOK;}
static OfxStatus paramSet(OfxImageEffectHandle,OfxParamSetHandle* out){*out=(OfxParamSetHandle)1;return kOfxStatOK;}
static OfxStatus clipDefine(OfxImageEffectHandle,const char*,OfxPropertySetHandle* out){*out=(OfxPropertySetHandle)&clip;return kOfxStatOK;}
int main(){
 OfxParameterSuiteV1 params{};params.paramDefine=define;params.paramGetHandle=get;
 OfxPropertySuiteV1 props{};props.propSetString=setString;props.propSetInt=setInt;props.propSetDouble=setDouble;
 OfxImageEffectSuiteV1 effects{};effects.getParamSet=paramSet;effects.clipDefine=clipDefine;
 a=&params;p=&props;e=&effects;
 assert(entry(kOfxImageEffectActionDescribeInContext,(void*)1,nullptr,nullptr)==kOfxStatOK);
 assert(forbidden==0);
 for(const auto& item:nodes)if(item.second.type!=kOfxParamTypeGroup)assert(!item.second.parent.empty());
 assert(nodes.at("Preset").parent=="Timing");assert(nodes.at("Analyze music").parent=="Music sync");
 assert(nodes.at("Fade point 16").parent=="Custom fade");
 std::cout<<"Descriptor regression passed: no instance access, no cycles, valid group parents.\n";
}
