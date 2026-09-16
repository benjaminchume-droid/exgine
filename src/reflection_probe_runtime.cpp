#include "exgine/reflection_probe_runtime.hpp"
#include <algorithm>
#include <cmath>
namespace exgine {
ReflectionProbeGpuRuntime::ReflectionProbeGpuRuntime(OpenGLESRenderer&r)noexcept:renderer_(r){}
ReflectionProbeGpuRuntime::~ReflectionProbeGpuRuntime(){clear();}
bool ReflectionProbeGpuRuntime::capture_and_build(std::uint64_t id,const RenderFrame&frame,std::uint32_t size,std::string&e)noexcept{auto*p=registry_.find(id);auto*g=registry_.gpu_find(id);if(!p||!g){e="reflection probe id not found";return false;}auto&target=captures_[id];if(!capture_probe_six_faces(renderer_,frame,p->position,size,target,[&](OpenGLESRenderer&r,const RenderFrame&f,const Mat4&view,const Mat4&proj,std::uint32_t,std::string&err){RenderFrame face=f;face.view=view;face.projection=proj;face.view_projection=multiply(proj,view);auto result=r.submit(face);if(!result.success)err=result.error;return result.success;},e))return false;GlesCubeResource env{target.color_cube,size,1,true};GpuIblResources ibl;if(!build_gpu_ibl(renderer_,env,std::max(16u,size/8u),size,std::max(1u,static_cast<std::uint32_t>(std::log2(std::max(1u,size)))+1),ibl,e))return false;g->environment=ibl.environment;g->irradiance=ibl.irradiance;g->specular=ibl.specular;g->depth_rbo=target.depth_rbo;g->last_capture_frame=frame.frame_id;g->valid=true;target.color_cube=0;target.depth_rbo=0;destroy_probe_capture_target(const_cast<OpenGLESApi&>(renderer_.api()),target);return true;}
ProbeMaterialSet ReflectionProbeGpuRuntime::material_set(Vec3 p,std::uint32_t max)const noexcept{return select_probe_material_set(registry_,p,max);}
void ReflectionProbeGpuRuntime::clear()noexcept{auto&a=const_cast<OpenGLESApi&>(renderer_.api());for(auto&[id,t]:captures_){(void)id;destroy_probe_capture_target(a,t);}captures_.clear();registry_.clear(a);}
}