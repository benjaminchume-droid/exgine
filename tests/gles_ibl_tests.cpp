#include "exgine/gles_ibl.hpp"
#include <cassert>
#include <string>
int main(){std::string v=exgine::gpu_ibl_vertex_shader(),i=exgine::gpu_irradiance_fragment_shader(),g=exgine::gpu_ggx_prefilter_fragment_shader();assert(v.find("#version 310 es")!=std::string::npos);assert(i.find("samplerCube")!=std::string::npos);assert(i.find("texture")!=std::string::npos);assert(g.find("Hammersley")!=std::string::npos||g.find("RI(uint")!=std::string::npos);assert(g.find("textureLod")!=std::string::npos);assert(g.find("u_roughness")!=std::string::npos);return 0;}