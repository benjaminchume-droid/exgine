#include "exgine/showcase.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {
std::string read_text(const std::filesystem::path& path) { std::ifstream file(path,std::ios::binary); if(!file)return{}; std::ostringstream out; out<<file.rdbuf(); return out.str(); }
}
int main(int argc,char**argv){
    const std::filesystem::path root=argc>1?std::filesystem::path(argv[1]):std::filesystem::path("examples/showcase");
    auto loader=[root](std::string_view uri,std::string&out){out=read_text(root/std::string(uri));if(!out.empty())return true;out=read_text(root.parent_path()/"showcase"/std::string(uri));return !out.empty();};
    const auto manifest=read_text(root/"project.exg"); if(manifest.empty()){std::cerr<<"showcase: project.exg not found\n";return 1;}
    exgine::ShowcaseGame game(loader); if(!game.open(manifest)||!game.start()){std::cerr<<"showcase: startup failed\n";return 2;}
    exgine::RenderFrame frame;exgine::RenderResult render;constexpr int sample_frames=300;
    for(int i=0;i<sample_frames;++i)if(!game.update(1.0/60.0)||!game.build_frame(frame,render)){std::cerr<<"showcase: frame "<<i<<" failed\n";return 3;}
    const auto saved=game.save();if(saved.empty()||!game.restore(saved)){std::cerr<<"showcase: save round-trip failed\n";return 4;}
    const auto&m=game.metrics();std::cout<<"EXGINE SHOWCASE\n"<<"frames="<<m.frames<<" rendered="<<m.rendered_frames<<"\n"<<"avg_frame_ms="<<m.average_frame_ms()<<" max_frame_ms="<<m.max_frame_ms<<"\n"<<"draw_calls_sum="<<m.draw_calls<<" visible_draws_sum="<<m.visible_draws<<"\n"<<"physics_steps="<<m.physics_steps<<" stream_results="<<m.stream_results<<"\n"<<"weather_particles="<<m.weather_particles<<" audio_sources="<<m.audio_sources<<" ui_widgets="<<m.ui_widgets<<"\n"<<"save_bytes="<<saved.size()<<"\n";return m.valid()?0:5;
}
