#include "exgine/gles_probe_capture.hpp"
#include "exgine/gles_probe_material.hpp"
#include <cassert>
#include <string>
int main(){assert(sizeof(exgine::ReflectionProbeCaptureTarget)>=sizeof(exgine::GlUInt)*3);assert(exgine::cube_faces().size()==6);exgine::ReflectionProbeRegistry r;auto a=r.create({0,0,0},10.f,1.f),b=r.create({8,0,0},10.f,2.f);assert(r.find(a)&&r.find(b));auto*ga=r.gpu_find(a);auto*gb=r.gpu_find(b);ga->valid=gb->valid=true;auto s=exgine::select_probe_material_set(r,{6,0,0},4);assert(s.count==2);assert(s.weights[0]+s.weights[1]>.99f&&s.weights[0]+s.weights[1]<1.01f);assert(r.select({6,0,0})!=nullptr);return 0;}