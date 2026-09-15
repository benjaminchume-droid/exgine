#include "exgine/ast.hpp"
#include "exgine/character.hpp"
#include "exgine/compiler.hpp"
#include "exgine/diagnostic.hpp"
#include "exgine/engine.hpp"
#include "exgine/geometry.hpp"
#include "exgine/ir.hpp"
#include "exgine/lexer.hpp"
#include "exgine/material.hpp"
#include "exgine/resources.hpp"
#include "exgine/runtime.hpp"
#include "exgine/source.hpp"
#include "exgine/texture.hpp"
#include "exgine/token.hpp"
#include "exgine/version.hpp"
#include "exgine/world.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
namespace {
const exgine::SourceText valid_source(R"( game "Green World" { world { terrain { type = procedural height = 20 } building "house" { floors = 2 enabled = true } vehicle "car" { type = "sports_car" } } })");
void test_version(){assert(exgine::version_string=="0.1.0");}
void test_diagnostics(){exgine::DiagnosticBag d;d.note("parser note",{4,2,5});d.warning("unused property",{10,3,2});d.error("invalid value",{20,4,1});assert(d.has_errors()&&d.all().size()==3);}
void test_ir(){exgine::IR game;game.add_property("terrain",std::string{"procedural"});auto& b=game.add_child(exgine::NodeKind::Building,"house");b.properties.push_back({"floors",std::int64_t{2}});assert(std::get<std::int64_t>(game.root.children[0].find_property("floors")->value)==2);assert(exgine::NodeKind::NPC!=exgine::NodeKind::Player);}
void test_world_determinism(){exgine::TerrainConfig c;c.amplitude=80;c.seed=1337;c.chunk_size=32;exgine::ProceduralWorld a(c),b(c);assert(std::isfinite(a.sample_height(32,64))&&a.sample_height(32,64)==b.sample_height(32,64));const auto ca=a.generate_chunk({0,0}),cb=b.generate_chunk({0,0});assert(ca.valid()&&cb.valid()&&ca.terrain.vertices.size()==cb.terrain.vertices.size()&&ca.terrain.indices==cb.terrain.indices);assert(ca.vegetation.size()==cb.vegetation.size());}
void test_biomes_water_and_streaming(){exgine::TerrainConfig c;c.amplitude=70;c.seed=99;c.chunk_size=24;c.resolution=16;c.vegetation_density=32;exgine::ProceduralWorld w(c);const auto b=w.sample_biome(100,200);assert(b.moisture>=0&&b.moisture<=1&&b.temperature>=0&&b.temperature<=1&&b.slope>=0&&b.slope<=1);assert(w.sample_water_depth(0,0)>=0);exgine::WorldStreamer s(w);s.update({0,0,0},2);assert(s.loaded_count()>0&&s.find({0,0})!=nullptr);const auto loaded=s.loaded_count();s.update({c.chunk_size*10,0,0},1);assert(s.loaded_count()!=loaded&&s.find({10,0})!=nullptr);s.clear();assert(s.loaded_count()==0);}
void test_language_pipeline(){exgine::DiagnosticBag d;const auto tokens=exgine::Lexer(valid_source).tokenize(d);assert(!d.has_errors()&&tokens.front().kind==exgine::TokenKind::Game);const auto r=exgine::Compiler{}.compile(valid_source);assert(r.succeeded()&&r.ir.has_value());assert(r.ir->root.kind==exgine::NodeKind::World&&r.ir->root.children.size()==3);assert(r.ir->root.children[1].name=="house");}
void test_runtime(){const auto c=exgine::Compiler{}.compile(valid_source);assert(c.succeeded());exgine::Runtime r;assert(r.load(*c.ir));assert(r.state().entities.size()==4&&r.world()!=nullptr&&r.world_streamer()!=nullptr);const auto id=r.state().world_entity+1;assert(r.attach_geometry(id,exgine::MeshAssembly{{{"house_mesh",exgine::make_box({{8,3,8}}),"concrete",{},{1,1,1}}}}));assert(r.stream_world({0,0,0},1));assert(r.world_streamer()->loaded_count()>0);r.update(.5);r.update(.25);assert(r.state().tick==2&&r.state().elapsed_seconds==.75);r.reset();assert(r.state().entities.size()==0&&r.world()==nullptr);}
void test_geometry(){const auto b=exgine::make_box({{2,3,4}});const auto s=exgine::make_sphere({1,16,8});const auto c=exgine::make_cylinder({.5f,2,16});const auto k=exgine::make_capsule();assert(b.valid()&&s.valid()&&c.valid()&&k.valid()&&b.vertices.size()==24&&s.vertices.size()==16*9);}
void test_characters(){exgine::CharacterDefinition n;n.type=exgine::CharacterType::NPC;n.appearance.seed=42;n.appearance.watch=true;n.appearance.backpack=true;const auto a=exgine::generate_character(n),b=exgine::generate_character(n);assert(a.valid()&&b.valid()&&a.parts.size()==10&&a.parts[0].mesh.vertices.size()==b.parts[0].mesh.vertices.size());n.appearance.seed=43;assert(exgine::generate_character(n).parts[0].position.x!=a.parts[0].position.x);}
void test_materials(){auto steel=exgine::make_real_world_material("steel",42);assert(steel.valid()&&steel.metallic>.9f);exgine::MaterialLibrary lib;assert(lib.define(steel)&&lib.find("steel")!=nullptr);assert(!lib.define(exgine::Material{}));auto bad=steel;bad.roughness=2;assert(!lib.define(bad));}
void test_textures(){exgine::TextureGenerationSettings s;s.width=16;s.height=12;s.noise.seed=123;s.noise.type=exgine::NoiseType::Fbm;auto t=exgine::generate_noise_texture(s,1);assert(t.valid()&&t.data.size()==16u*12u);assert(exgine::sample_noise(.2f,.7f,s.noise)==exgine::sample_noise(.2f,.7f,s.noise));auto m=exgine::make_real_world_material("wood",7);auto set=exgine::generate_material_textures(m,s);assert(set.valid()&&set.base_color->width==16&&set.base_color->channels==3);}
void test_resources_and_runtime_materials(){const auto c=exgine::Compiler{}.compile(valid_source);assert(c.succeeded());exgine::Runtime r;assert(r.load(*c.ir));auto m=exgine::make_real_world_material("concrete",99);assert(r.define_material(m));assert(r.material("concrete")!=nullptr);assert(r.material_resource("concrete")!=nullptr);assert(r.material_resource("concrete")->textures->valid());const auto key=exgine::material_generation_key(m,{});auto resource=exgine::build_material_resource(m,{});assert(resource&&resource->generation_key==key);}
void test_engine(){exgine::Engine e;assert(e.load(valid_source));e.update(1.0/60.0);assert(e.runtime().state().tick==1);assert(!e.load(exgine::SourceText("game \"Broken\" { world { terrain_height = 0 } }")));assert(e.runtime().state().entities.size()==0&&e.diagnostics().has_errors());}
}
int main(){test_version();test_diagnostics();test_ir();test_world_determinism();test_biomes_water_and_streaming();test_language_pipeline();test_runtime();test_geometry();test_characters();test_materials();test_textures();test_resources_and_runtime_materials();test_engine();return 0;}
