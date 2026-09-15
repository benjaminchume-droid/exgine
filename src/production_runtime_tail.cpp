#include "exgine/production.hpp"
namespace exgine {
bool SaveStore::decode(const std::vector<std::uint8_t>& b,ProductionSave& save,std::string& error){
    if(b.size()<5||std::memcmp(b.data(),"XGSV",4)!=0||b[4]!=1){error="invalid save header";return false;}
    std::size_t p=5;save=ProductionSave{};
    auto read32=[&](std::uint32_t&v){if(p+4>b.size())return false;v=static_cast<std::uint32_t>(b[p])|static_cast<std::uint32_t>(b[p+1])<<8|static_cast<std::uint32_t>(b[p+2])<<16|static_cast<std::uint32_t>(b[p+3])<<24;p+=4;return true;};
    auto read64=[&](std::uint64_t&v){if(p+8>b.size())return false;v=0;for(int i=0;i<8;++i)v|=static_cast<std::uint64_t>(b[p+i])<<(8*i);p+=8;return true;};
    auto readstr=[&](std::string&s){std::uint32_t n=0;if(!read32(n)||p+n>b.size())return false;s.assign(reinterpret_cast<const char*>(b.data()+p),n);p+=n;return true;};
    if(!readstr(save.game.project_name)||!readstr(save.game.scene_name)){error="truncated save strings";return false;}
    std::uint64_t dbits=0;if(!read64(dbits)){error="truncated save time";return false;}std::memcpy(&save.game.environment_seconds,&dbits,sizeof(double));
    if(!read64(save.game.runtime_tick)){error="truncated save tick";return false;}
    std::uint32_t n=0;if(!read32(n)){error="truncated save variables";return false;}
    for(std::uint32_t i=0;i<n;++i){std::string k,v;if(!readstr(k)||!readstr(v)){error="truncated variable";return false;}save.game.variables.emplace(std::move(k),std::move(v));}
    if(!read32(n)){error="truncated save blobs";return false;}
    for(std::uint32_t i=0;i<n;++i){GameStateBlob blob;std::uint32_t size=0;if(!readstr(blob.key)||!read32(size)||p+size>b.size()){error="truncated save blob";return false;}blob.bytes.assign(b.begin()+static_cast<std::ptrdiff_t>(p),b.begin()+static_cast<std::ptrdiff_t>(p+size));p+=size;save.blobs.push_back(std::move(blob));}
    return save.valid();
}
GameSession::GameSession(GameSessionConfig config,ProjectSourceLoader loader):config_(config),project_loader_(std::move(loader)),streaming_([this](std::string_view uri,std::vector<std::uint8_t>&bytes,std::string&error){if(!project_loader_){error="no project loader";return false;}std::string text;if(!project_loader_(uri,text)){error="asset load failed";return false;}bytes.assign(text.begin(),text.end());return true;}){}
bool GameSession::open_project(std::string_view source){
    const auto parsed=parse_project(source);if(!parsed.success)return false;
    GameRuntime::SceneLoader scene_loader=[this](std::string_view uri,IR& ir){if(!project_loader_)return false;std::string text;if(!project_loader_(uri,text))return false;const auto compiled=Compiler{}.compile(SourceText(text));if(!compiled.succeeded()||!compiled.ir)return false;ir=*compiled.ir;return true;};
    if(!game_.load_project(parsed.project,scene_loader))return false;open_=true;stats_={};nav_timer_=0;return true;
}
bool GameSession::update(double dt)noexcept{if(!open_||dt<0)return false;if(!game_.update(dt))return false;environment_renderer_.update(game_.environment().state());const auto results=streaming_.poll(config_.max_stream_results_per_frame);stats_.stream_results+=results.size();nav_timer_+=static_cast<float>(dt);if(nav_timer_>=config_.navigation_update_interval){nav_timer_=0;stats_.navigation_updates++;}if(config_.enable_audio)(void)audio_.mix(static_cast<float>(dt));stats_.frames++;return true;}
std::vector<std::uint8_t> GameSession::save()const{ProductionSave s;s.game=game_.save();return SaveStore::encode(s);}
bool GameSession::restore(const std::vector<std::uint8_t>& bytes)noexcept{ProductionSave s;std::string error;if(!SaveStore::decode(bytes,s,error))return false;return game_.restore(s.game);}
bool GameSession::close()noexcept{if(!open_)return false;game_.reset();ui_.clear();open_=false;return true;}
}
