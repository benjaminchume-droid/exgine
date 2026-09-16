#include "exgine/showcase.hpp"
#include <cassert>
#include <string>
int main(){using namespace exgine;
 const std::string manifest="name = Integrated Showcase\nversion = 1.0\nstartup_scene = Main\ntick_rate = 60\nscene = Main\n";
 const std::string scene="world World {\nterrain Terrain { amplitude = 40 seed = 42 }\nplayer Player { x = 7.5 y = 2 z = 12 }\nnpc Guide { x = 11 y = 1 z = 7 }\nbuilding MainHouse { floors = 2 rooms = 4 doors = 2 windows = 6 stairs = 1 furniture = 4 seed = 42 }\nvehicle DemoCar { x = 14 y = 1 z = 3 type = SportsCar }\nprop HouseAsset { x = -7 y = 0 z = 2 }\nprop RainFX { x = 7 y = 0 z = 5 }\nprop PhysicsCrate { x = 2 y = 5 z = 2 }\nprop HUDCrosshair { x = 7.5 y = 5.2 z = 11.5 }\nprop HUDHealth { x = 7.5 y = 4.75 z = 11.5 }\n}\n";
 const std::string obj="v -1 0 -1\nv 1 0 -1\nv 1 2 -1\nv -1 2 -1\nv -1 0 1\nv 1 0 1\nv 1 2 1\nv -1 2 1\nvt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\nvn 0 0 -1\nvn 1 0 0\nvn 0 0 1\nvn -1 0 0\nvn 0 1 0\nf 1/1/1 2/2/1 3/3/1\nf 1/1/1 3/3/1 4/4/1\nf 2/1/2 6/2/2 7/3/2\nf 2/1/2 7/3/2 3/4/2\nf 6/1/3 5/2/3 8/3/3\nf 6/1/3 8/3/3 7/4/3\nf 5/1/4 1/2/4 4/3/4\nf 5/1/4 4/3/4 8/4/4\nf 4/1/5 3/2/5 7/3/5\nf 4/1/5 7/3/5 8/4/5\n";
 const std::string ppm="P3\n1 1\n255\n180 180 180\n";
 ShowcaseGame game([&](std::string_view uri,std::string&out){if(uri=="scenes/main.scene"){out=scene;return true;}if(uri=="showcase/assets/house.obj"){out=obj;return true;}if(uri=="showcase/assets/house.ppm"){out=ppm;return true;}return false;});
 assert(game.open(manifest));assert(game.start());RenderFrame frame;RenderResult render;for(int i=0;i<180;++i){assert(game.update(1.0/60.0));assert(game.build_frame(frame,render));assert(render.success&&!frame.draws.empty());}
 const auto before=game.metrics();const auto save=game.save();assert(!save.empty());assert(game.restore(save));assert(before.frames==180&&before.rendered_frames==180);assert(before.weather_particles>0&&before.physics_steps>0&&before.ui_widgets>=2);assert(before.audio_sources>=1&&before.draw_calls>0);assert(before.procedural_animation_updates==180);assert(before.procedural_audio_events>0);assert(before.valid());return 0;}
