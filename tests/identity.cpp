// Exercise the plugin's actual OFX entry point. Identity probing must never
// delegate time-only effects to the host: Resolve can drop the temporal offset.
#include "../src/plugin.cpp"
#include <cassert>
#include <iostream>
int main(){
 auto plugin=OfxGetPlugin(0);
 assert(plugin && OfxGetNumberOfPlugins()==1);
 // No property or parameter suite reads are required: the result must be
 // render-required for every parameter state, regardless of fade settings.
 assert(plugin->mainEntry(kOfxImageEffectActionIsIdentity,nullptr,nullptr,nullptr)==kOfxStatReplyDefault);
 std::cout<<"OFX identity regression passed: host must call Render.\n";
}
