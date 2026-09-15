#include "exgine/import.hpp"
#include <cassert>
#include <string>

using namespace exgine;
namespace {
std::string gltf(){return R"({"asset":{"version":"2.0"},"buffers":[{"byteLength":42,"uri":"data:application/octet-stream;base64,AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAABAAIA"}],"bufferViews":[{"buffer":0,"byteOffset":0,"byteLength":36},{"buffer":0,"byteOffset":36,"byteLength":6}],"accessors":[{"bufferView":0,"componentType":5126,"count":3,"type":"VEC3"},{"bufferView":1,"componentType":5123,"count":3,"type":"SCALAR"}],"materials":[{"name":"Paint","pbrMetallicRoughness":{"baseColorFactor":[0.2,0.4,0.6,1],"metallicFactor":0.8,"roughnessFactor":0.25}}],"meshes":[{"name":"Triangle","primitives":[{"attributes":{"POSITION":0},"indices":1,"material":0}]}],"nodes":[{"name":"Root","mesh":0}],"scenes":[{"nodes":[0]}],"scene":0})";}
void formats(){assert(detect_import_format("a.gltf",gltf())==ImportFormat::Gltf);assert(detect_import_format("a.glb",std::string("glTF",4))==ImportFormat::Glb);}
void parse(){auto r=import_gltf("triangle.gltf",gltf());assert(r.success&&r.meshes.size()==1&&r.meshes[0].mesh.vertices.size()==3&&r.meshes[0].mesh.indices.size()==3);assert(r.materials.size()==1&&r.materials[0].material.metallic>.79f);assert(r.nodes.size()==1&&r.nodes[0].mesh_index==0);}
void rejects(){assert(!import_gltf("bad.gltf","{").success);assert(!import_gltf("x.gltf",R"({"asset":{"version":"1.0"}})").success);}
}
int main(){formats();parse();rejects();return 0;}
