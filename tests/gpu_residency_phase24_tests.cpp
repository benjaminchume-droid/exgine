#include "exgine/gpu_residency.hpp"
#include <cassert>
using namespace exgine;
namespace {
Mesh tri(){Mesh m;m.vertices={{{0,0,0},{0,0,1},{0,0}},{{1,0,0},{0,0,1},{1,0}},{{0,1,0},{0,0,1},{0,1}}};m.indices={0,1,2};return m;}
void config(){GpuResidencyConfig c;assert(c.valid());c.budget_bytes=0;assert(!c.valid());}
void host(){OpenGLESAssetResidency r(OpenGLESApi{},GpuResidencyConfig{1024,2});assert(!r.ready());std::string e;assert(!r.upload(1,tri(),1,e));assert(!e.empty());assert(r.stats().resident_meshes==0);assert(!r.evict(1));}
}
int main(){config();host();return 0;}
