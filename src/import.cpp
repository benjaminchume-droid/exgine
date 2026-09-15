#include "exgine/import.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace exgine {
namespace {
struct Key { int v=0, t=-1, n=-1; bool operator==(const Key& o) const noexcept{return v==o.v&&t==o.t&&n==o.n;} };
struct KeyHash { std::size_t operator()(const Key& k) const noexcept { return (static_cast<std::size_t>(k.v)*1315423911u) ^ (static_cast<std::size_t>(k.t+1)*2654435761u) ^ (static_cast<std::size_t>(k.n+1)*2246822519u); } };
int to_index(int value, std::size_t size) { if(value>0) return value-1; if(value<0) return static_cast<int>(size)+value; return -1; }
bool parse_ref(const std::string& token, Key& out) {
    std::stringstream ss(token); std::string part; int slot=0; int vals[3]{0,-1,-1};
    while(std::getline(ss,part,'/') && slot<3){ if(!part.empty()){ try{ vals[slot]=std::stoi(part); }catch(...){return false;} } ++slot; }
    if(slot==0 || vals[0]==0) return false; out={vals[0],vals[1],vals[2]}; return true;
}
Vec3 cross(Vec3 a, Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
Vec3 sub(Vec3 a, Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
float len(Vec3 a){return std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z);}
Vec3 normalize(Vec3 a){const float l=len(a);return l>1e-8f?Vec3{a.x/l,a.y/l,a.z/l}:Vec3{0,1,0};}
}
ImportFormat detect_import_format(std::string_view uri,std::string_view) noexcept {
    const auto dot=uri.find_last_of('.'); if(dot==std::string_view::npos) return ImportFormat::Unknown;
    std::string ext{uri.substr(dot+1)}; std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
    return ext=="obj"?ImportFormat::Obj:ImportFormat::Unknown;
}
ImportResult import_obj(std::string_view uri,std::string_view text) {
    ImportResult r; r.format=ImportFormat::Obj;
    std::istringstream in{std::string{text}}; std::string line; std::vector<Vec3> positions,normals; std::vector<Vec2> uvs;
    Mesh mesh; std::unordered_map<Key,std::uint32_t,KeyHash> cache;
    while(std::getline(in,line)){
        if(line.empty()) continue; std::istringstream ls(line); std::string op; ls>>op; if(op.empty()||op[0]=='#') continue;
        if(op=="v"){Vec3 p{}; if(!(ls>>p.x>>p.y>>p.z)){r.error="invalid OBJ vertex";return r;} positions.push_back(p);}
        else if(op=="vn"){Vec3 n{}; if(!(ls>>n.x>>n.y>>n.z)){r.error="invalid OBJ normal";return r;} normals.push_back(normalize(n));}
        else if(op=="vt"){Vec2 uv{}; if(!(ls>>uv.x>>uv.y)){r.error="invalid OBJ texcoord";return r;} uvs.push_back(uv);}
        else if(op=="f"){
            std::vector<Key> refs; std::string tok; while(ls>>tok){Key k; if(!parse_ref(tok,k)){r.error="invalid OBJ face";return r;} k.v=to_index(k.v,positions.size());k.t=to_index(k.t,uvs.size());k.n=to_index(k.n,normals.size());if(k.v<0||static_cast<std::size_t>(k.v)>=positions.size()||k.t>=static_cast<int>(uvs.size())||k.n>=static_cast<int>(normals.size())){r.error="OBJ face index out of range";return r;} refs.push_back(k);}
            if(refs.size()<3){r.error="OBJ face has fewer than 3 vertices";return r;}
            for(std::size_t i=1;i+1<refs.size();++i){Key tri[3]{refs[0],refs[i],refs[i+1]}; std::uint32_t idx[3]{};
                for(int j=0;j<3;++j){auto it=cache.find(tri[j]); if(it!=cache.end()) idx[j]=it->second; else {Vertex v{};v.position=positions[tri[j].v];v.uv=tri[j].t>=0?uvs[tri[j].t]:Vec2{};v.normal=tri[j].n>=0?normals[tri[j].n]:Vec3{};idx[j]=static_cast<std::uint32_t>(mesh.vertices.size());mesh.vertices.push_back(v);cache.emplace(tri[j],idx[j]);}}
                mesh.indices.insert(mesh.indices.end(),{idx[0],idx[1],idx[2]});
                if(tri[0].n<0||tri[1].n<0||tri[2].n<0){const Vec3 fn=normalize(cross(sub(mesh.vertices[idx[1]].position,mesh.vertices[idx[0]].position),sub(mesh.vertices[idx[2]].position,mesh.vertices[idx[0]].position)));for(int j=0;j<3;++j)if(tri[j].n<0)mesh.vertices[idx[j]].normal=fn;}
            }
        }
    }
    if(!mesh.valid()){r.error="OBJ produced no triangles";return r;}
    ImportedMesh m; m.uri=std::string{uri}; m.mesh=std::move(mesh); m.asset_id=make_asset_id(uri,std::vector<std::uint8_t>(text.begin(),text.end()));
    r.meshes.push_back(std::move(m)); r.success=true; return r;
}
bool ObjAssetImporter::accepts(std::string_view uri,std::string_view payload) const noexcept{return detect_import_format(uri,payload)==ImportFormat::Obj;}
ImportResult ObjAssetImporter::import(std::string_view uri,std::string_view payload) const{return import_obj(uri,payload);}
} // namespace exgine
