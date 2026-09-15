#include "exgine/production.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <condition_variable>
#include <cstring>
#include <limits>
#include <queue>
#include <thread>
#include <unordered_set>

namespace exgine {
namespace {

std::string lower_ext(std::string_view uri) {
    const auto dot = uri.find_last_of('.');
    std::string out(dot == std::string_view::npos ? uri : uri.substr(dot + 1));
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

bool decode_ppm(std::string_view, const std::vector<std::uint8_t>& bytes, ImageData& out, std::string& error) {
    const std::string text(bytes.begin(), bytes.end());
    std::size_t p=0;
    auto token=[&]() -> std::string {
        while (p<text.size()) {
            if (std::isspace(static_cast<unsigned char>(text[p]))) { ++p; continue; }
            if (text[p]=='#') { while (p<text.size() && text[p]!='\n') ++p; continue; }
            break;
        }
        const std::size_t s=p; while (p<text.size() && !std::isspace(static_cast<unsigned char>(text[p]))) ++p; return text.substr(s,p-s);
    };
    const auto magic=token();
    if (magic!="P3" && magic!="P6") { error="unsupported PPM variant"; return false; }
    std::uint32_t w=0,h=0,maxv=0;
    try { w=static_cast<std::uint32_t>(std::stoul(token())); h=static_cast<std::uint32_t>(std::stoul(token())); maxv=static_cast<std::uint32_t>(std::stoul(token())); }
    catch (...) { error="invalid PPM header"; return false; }
    if (w==0 || h==0 || maxv==0 || maxv>255) { error="invalid PPM dimensions"; return false; }
    out.width=w; out.height=h; out.format=ImagePixelFormat::RGB8; out.pixels.resize(static_cast<std::size_t>(w)*h*3);
    if (magic=="P3") {
        for (std::size_t i=0;i<out.pixels.size();++i) { try { out.pixels[i]=static_cast<std::uint8_t>(std::min<std::uint32_t>(255,std::stoul(token())*255/maxv)); } catch (...) { error="truncated PPM"; return false; } }
    } else {
        while (p<bytes.size() && std::isspace(bytes[p])) ++p;
        const std::size_t remaining=bytes.size()-p;
        if (remaining<out.pixels.size()) { error="truncated PPM payload"; return false; }
        std::memcpy(out.pixels.data(),bytes.data()+p,out.pixels.size());
        if (maxv!=255) for (auto& v:out.pixels) v=static_cast<std::uint8_t>(static_cast<unsigned>(v)*255/maxv);
    }
    return true;
}

std::uint16_t u16(const std::vector<std::uint8_t>& b,std::size_t p) { return static_cast<std::uint16_t>(b[p] | (b[p+1]<<8)); }
std::uint32_t u32(const std::vector<std::uint8_t>& b,std::size_t p) { return static_cast<std::uint32_t>(b[p] | (b[p+1]<<8) | (b[p+2]<<16) | (b[p+3]<<24)); }

bool decode_bmp(std::string_view, const std::vector<std::uint8_t>& b, ImageData& out, std::string& error) {
    if (b.size()<54 || b[0]!='B' || b[1]!='M') { error="invalid BMP"; return false; }
    const std::uint32_t off=u32(b,10), w=u32(b,18), rawh=u32(b,22); const std::uint16_t bits=u16(b,28); const std::uint32_t compression=u32(b,30);
    if (w==0 || rawh==0 || (bits!=24 && bits!=32) || compression!=0) { error="unsupported BMP"; return false; }
    const std::uint32_t row=((w*bits+31)/32)*4; if (b.size()<static_cast<std::size_t>(off)+static_cast<std::size_t>(row)*rawh) { error="truncated BMP"; return false; }
    const std::uint32_t channels=bits/8; out.width=w; out.height=rawh; out.format=channels==4?ImagePixelFormat::RGBA8:ImagePixelFormat::RGB8; out.pixels.resize(static_cast<std::size_t>(w)*rawh*channels);
    for (std::uint32_t y=0;y<rawh;++y) for (std::uint32_t x=0;x<w;++x) {
        const std::size_t s=off+static_cast<std::size_t>(y)*row+static_cast<std::size_t>(x)*channels; const std::size_t d=(static_cast<std::size_t>(rawh-1-y)*w+x)*channels;
        out.pixels[d]=b[s+2]; out.pixels[d+1]=b[s+1]; out.pixels[d+2]=b[s]; if (channels==4) out.pixels[d+3]=b[s+3];
    }
    return true;
}

bool decode_tga(std::string_view, const std::vector<std::uint8_t>& b, ImageData& out, std::string& error) {
    if (b.size()<18) { error="truncated TGA"; return false; }
    const std::uint8_t idlen=b[0], type=b[2]; const std::uint16_t w=u16(b,12), h=u16(b,14); const std::uint8_t depth=b[16], desc=b[17];
    if (w==0 || h==0 || (type!=2 && type!=3) || (depth!=24 && depth!=32)) { error="unsupported TGA"; return false; }
    const std::size_t channels=depth/8, start=18+idlen, count=static_cast<std::size_t>(w)*h; if (b.size()<start+count*channels) { error="truncated TGA pixels"; return false; }
    out.width=w; out.height=h; out.format=channels==4?ImagePixelFormat::RGBA8:ImagePixelFormat::RGB8; out.pixels.resize(count*channels);
    const bool flip_y=(desc&0x20U)==0; const bool flip_x=(desc&0x10U)!=0;
    for (std::uint32_t y=0;y<h;++y) for (std::uint32_t x=0;x<w;++x) {
        const std::uint32_t sx=flip_x?(w-1-x):x, sy=flip_y?(h-1-y):y; const std::size_t s=start+(static_cast<std::size_t>(sy)*w+sx)*channels; const std::size_t d=(static_cast<std::size_t>(y)*w+x)*channels;
        if (type==3) { for (std::size_t c=0;c<channels;++c) out.pixels[d+c]=b[s+c]; }
        else { out.pixels[d]=b[s+2]; out.pixels[d+1]=b[s+1]; out.pixels[d+2]=b[s]; if(channels==4) out.pixels[d+3]=b[s+3]; }
    }
    return true;
}

std::uint64_t hash_bytes(std::string_view a,const std::vector<std::uint8_t>& b) {
    std::uint64_t h=1469598103934665603ULL; for(unsigned char c:a){h^=c;h*=1099511628211ULL;} for(auto c:b){h^=c;h*=1099511628211ULL;} return h;
}

std::vector<std::string> json_uri_values(std::string_view json) {
    std::vector<std::string> result; std::size_t p=0;
    while ((p=json.find("\"uri\"",p))!=std::string_view::npos) {
        p=json.find(':',p+5); if(p==std::string_view::npos) break; ++p; while(p<json.size()&&std::isspace(static_cast<unsigned char>(json[p])))++p;
        if(p>=json.size()||json[p]!='\"') continue; ++p; std::string s;
        while(p<json.size()) { if(json[p]=='\"') {++p;break;} if(json[p]=='\\'&&p+1<json.size()){s.push_back(json[p+1]);p+=2;} else s.push_back(json[p++]); }
        result.push_back(std::move(s));
    } return result;
}

void put_u32(std::vector<std::uint8_t>& b,std::uint32_t v){ for(int i=0;i<4;++i)b.push_back(static_cast<std::uint8_t>((v>>(i*8))&255U)); }
void put_u64(std::vector<std::uint8_t>& b,std::uint64_t v){ for(int i=0;i<8;++i)b.push_back(static_cast<std::uint8_t>((v>>(i*8))&255U)); }
void put_f64(std::vector<std::uint8_t>& b,double v){ std::uint64_t u=0; std::memcpy(&u,&v,sizeof(u));put_u64(b,u); }
bool get_u32(const std::vector<std::uint8_t>&b,std::size_t&p,std::uint32_t&v){if(p+4>b.size())return false;v=u32(b,p);p+=4;return true;}
bool get_u64(const std::vector<std::uint8_t>&b,std::size_t&p,std::uint64_t&v){if(p+8>b.size())return false;v=0;for(int i=0;i<8;++i)v|=static_cast<std::uint64_t>(b[p+i])<<(i*8);p+=8;return true;}
bool get_f64(const std::vector<std::uint8_t>&b,std::size_t&p,double&v){std::uint64_t u;if(!get_u64(b,p,u))return false;std::memcpy(&v,&u,sizeof(v));return true;}
void put_str(std::vector<std::uint8_t>&b,std::string_view s){put_u32(b,static_cast<std::uint32_t>(s.size()));b.insert(b.end(),s.begin(),s.end());}
bool get_str(const std::vector<std::uint8_t>&b,std::size_t&p,std::string&s){std::uint32_t n;if(!get_u32(b,p,n)||p+n>b.size())return false;s.assign(reinterpret_cast<const char*>(b.data()+p),n);p+=n;return true;}

} // namespace

bool ImageData::valid() const noexcept { return width>0 && height>0 && pixels.size()==static_cast<std::size_t>(width)*height*channels(); }
ImageDecoderRegistry::ImageDecoderRegistry() = default;
bool ImageDecoderRegistry::register_decoder(std::string extension, ImageDecoder decoder) {
    if(extension.empty()||!decoder)return false; extension=lower_ext(extension); static std::mutex mutex; static std::unordered_map<std::string,ImageDecoder> registry;
    std::lock_guard<std::mutex> lock(mutex); registry[std::move(extension)]=std::move(decoder); return true;
}
bool ImageDecoderRegistry::decode(std::string_view uri,const std::vector<std::uint8_t>& bytes,ImageData& out,std::string& error) const {
    static std::mutex mutex; static std::unordered_map<std::string,ImageDecoder> registry;
    { std::lock_guard<std::mutex> lock(mutex); if(registry.empty()){ registry["ppm"]=decode_ppm;registry["pnm"]=decode_ppm;registry["bmp"]=decode_bmp;registry["tga"]=decode_tga; } }
    ImageDecoder decoder; { std::lock_guard<std::mutex> lock(mutex); auto it=registry.find(lower_ext(uri)); if(it!=registry.end())decoder=it->second; }
    if(!decoder){error="no image decoder registered for extension";return false;} return decoder(uri,bytes,out,error);
}

MaterialPipelineResult GltfMaterialPipeline::build(std::string_view gltf_uri,std::string_view json_or_glb,const AssetBytesLoader& loader,const ImageDecoderRegistry& decoders) const {
    MaterialPipelineResult result; const auto uris=json_uri_values(json_or_glb); std::size_t slot=0;
    static constexpr const char* slots[]={"base_color","metallic_roughness","normal","occlusion","emission"};
    for(const auto& rel:uris){ if(rel.rfind("data:",0)==0) continue; std::vector<std::uint8_t> bytes; std::string error; std::string resolved=rel; const auto slash=gltf_uri.find_last_of('/'); if(slash!=std::string_view::npos && rel.find("/")==std::string::npos) resolved=std::string(gltf_uri.substr(0,slash+1))+rel; if(!loader || !loader(resolved,bytes,error)){result.error=error.empty()?"failed to load texture":error; return result;} ImageData image; if(!decoders.decode(resolved,bytes,image,error)){result.error=error;return result;} auto ptr=std::make_shared<ImageData>(std::move(image)); MaterialTextureResource t; t.slot=slots[std::min(slot,static_cast<std::size_t>(4))];t.uri=resolved;t.asset_id=make_asset_id(resolved,bytes);t.image=std::move(ptr);result.textures.push_back(std::move(t)); ++slot; }
    result.success=true; return result;
}

AsyncAssetStreamer::AsyncAssetStreamer(Loader loader,std::uint32_t workers): loader_(std::move(loader)), queue_([](const Task&a,const Task&b){return static_cast<int>(a.request.priority)<static_cast<int>(b.request.priority);}) { workers=std::max<std::uint32_t>(1,workers); workers_.reserve(workers); for(std::uint32_t i=0;i<workers;++i)workers_.emplace_back(&AsyncAssetStreamer::worker_loop,this); }
AsyncAssetStreamer::~AsyncAssetStreamer(){shutdown();}
std::uint64_t AsyncAssetStreamer::submit(std::string uri,StreamPriority priority){if(uri.empty())return 0; std::lock_guard<std::mutex> lock(mutex_); if(stopping_)return 0; const auto id=next_id_++; queue_.push(Task{AssetStreamRequest{id,std::move(uri),priority}});++stats_.submitted;stats_.active=queue_.size();cv_.notify_one();return id;}
std::vector<AssetStreamResult> AsyncAssetStreamer::poll(std::size_t max_results){std::lock_guard<std::mutex> lock(mutex_);std::vector<AssetStreamResult> out;while(!completed_.empty()&&out.size()<max_results){out.push_back(std::move(completed_.front()));completed_.pop();}return out;}
bool AsyncAssetStreamer::cancel(std::uint64_t id){std::lock_guard<std::mutex> lock(mutex_);if(id==0)return false;cancelled_[id]=true;++stats_.cancelled;return true;}
void AsyncAssetStreamer::shutdown() noexcept { {std::lock_guard<std::mutex> lock(mutex_);if(stopping_)return;stopping_=true;}cv_.notify_all();for(auto&t:workers_)if(t.joinable())t.join();workers_.clear(); }
AssetStreamingStats AsyncAssetStreamer::stats() const noexcept {std::lock_guard<std::mutex> lock(mutex_);auto s=stats_;s.active=queue_.size();return s;}
void AsyncAssetStreamer::worker_loop(){for(;;){Task task;{std::unique_lock<std::mutex> lock(mutex_);cv_.wait(lock,[&]{return stopping_||!queue_.empty();});if(stopping_&&queue_.empty())return;task=std::move(const_cast<Task&>(queue_.top()));queue_.pop();stats_.active=queue_.size();if(cancelled_.erase(task.request.request_id)>0)continue;}AssetStreamResult r;r.request=task.request;std::string error;std::vector<std::uint8_t> bytes;bool ok=loader_?loader_(task.request.uri,bytes,error):false;r.success=ok;r.bytes=std::move(bytes);r.error=std::move(error);{std::lock_guard<std::mutex> lock(mutex_);completed_.push(std::move(r));++stats_.completed;if(!ok)++stats_.failed;}}}

NavigationMesh::NavigationMesh(NavMeshConfig config):config_(config){config_.width=std::max(1U,config_.width);config_.depth=std::max(1U,config_.depth);config_.cell_size=std::max(0.05f,config_.cell_size);config_.max_slope=std::max(0.01f,config_.max_slope);cells_.resize(static_cast<std::size_t>(config_.width)*config_.depth);for(std::uint32_t z=0;z<config_.depth;++z)for(std::uint32_t x=0;x<config_.width;++x){auto&c=cells_[static_cast<std::size_t>(z)*config_.width+x];c.id=static_cast<NavNodeId>(z*config_.width+x+1);c.x=static_cast<std::int32_t>(x);c.z=static_cast<std::int32_t>(z);}}
NavNodeId NavigationMesh::index(std::int32_t x,std::int32_t z)const noexcept{if(x<0||z<0||x>=static_cast<std::int32_t>(config_.width)||z>=static_cast<std::int32_t>(config_.depth))return 0;return static_cast<NavNodeId>(z*config_.width+x+1);}
bool NavigationMesh::build(const std::function<float(float,float)>& height,const std::function<bool(float,float)>& walkable){if(!height||!walkable)return false;for(auto&c:cells_){const float wx=(static_cast<float>(c.x)+0.5f)*config_.cell_size,cwz=(static_cast<float>(c.z)+0.5f)*config_.cell_size;c.height=height(wx,cwz);c.walkable=walkable(wx,cwz);}return true;}
bool NavigationMesh::set_cell(std::int32_t x,std::int32_t z,bool w,float h){const auto id=index(x,z);if(!id)return false;auto&c=cells_[id-1];c.walkable=w;c.height=h;return true;}
const NavCell* NavigationMesh::cell(std::int32_t x,std::int32_t z)const noexcept{const auto id=index(x,z);return id?&cells_[id-1]:nullptr;}
NavPath NavigationMesh::find_path(Vec3 start,Vec3 goal)const{NavPath result;auto tocell=[&](Vec3 p){return std::pair<int,int>{static_cast<int>(std::floor(p.x/config_.cell_size)),static_cast<int>(std::floor(p.z/config_.cell_size))};};auto s=tocell(start),g=tocell(goal);auto sc=cell(s.first,s.second),gc=cell(g.first,g.second);if(!sc||!gc||!sc->walkable||!gc->walkable)return result;struct Item{NavNodeId id;float f;};struct Cmp{bool operator()(const Item&a,const Item&b)const{return a.f>b.f;}};std::priority_queue<Item,std::vector<Item>,Cmp> open;std::vector<float> cost(cells_.size(),std::numeric_limits<float>::infinity());std::vector<NavNodeId> parent(cells_.size(),0);auto heuristic=[&](const NavCell& a,const NavCell& b){return std::fabs(static_cast<float>(a.x-b.x))+std::fabs(static_cast<float>(a.z-b.z));};cost[sc->id-1]=0;open.push({sc->id,heuristic(*sc,*gc)});while(!open.empty()){const auto cur=open.top();open.pop();if(cur.id==gc->id)break;const auto&cc=cells_[cur.id-1];for(const auto dz:{-1,0,1})for(const auto dx:{-1,0,1}){if(dx==0&&dz==0)continue;const auto*nc=cell(cc.x+dx,cc.z+dz);if(!nc||!nc->walkable)continue;const float step=(dx!=0&&dz!=0)?1.41421356f:1.0f;const float ng=cost[cur.id-1]+step+std::fabs(nc->height-cc.height)*0.05f;if(ng<cost[nc->id-1]){cost[nc->id-1]=ng;parent[nc->id-1]=cc.id;open.push({nc->id,ng+heuristic(*nc,*gc)});}}}if(sc->id!=gc->id&&parent[gc->id-1]==0)return result;for(NavNodeId id=gc->id;id!=0;id=parent[id-1]){const auto&c=cells_[id-1];result.points.push_back({(c.x+0.5f)*config_.cell_size,c.height,(c.z+0.5f)*config_.cell_size});if(id==sc->id)break;}std::reverse(result.points.begin(),result.points.end());result.success=true;result.cost=cost[gc->id-1];return result;}

NavigationSystem::NavigationSystem(NavMeshConfig config):mesh_(config){}
bool NavigationSystem::build(const std::function<float(float,float)>& h,const std::function<bool(float,float)>& w){return mesh_.build(h,w);}
bool NavigationSystem::set_destination(NavigationAgent& a,Vec3 d){a.destination=d;a.path=mesh_.find_path(a.path.points.empty()?Vec3{}:a.path.points.front(),d);if(!a.path.success)a.path=mesh_.find_path(d,d);a.waypoint=0;a.active=a.path.success;return a.active;}
bool NavigationSystem::update(NavigationAgent& a,Vec3& p,float dt)const noexcept{if(!a.active||dt<=0)return false;if(a.waypoint>=a.path.points.size()){a.active=false;return true;}const Vec3 t=a.path.points[a.waypoint];const Vec3 d{t.x-p.x,t.y-p.y,t.z-p.z};const float len=std::sqrt(d.x*d.x+d.y*d.y+d.z*d.z);if(len<=a.stopping_distance){++a.waypoint;if(a.waypoint>=a.path.points.size()){a.active=false;return true;}}else{const float step=std::min(len,a.speed*dt);p.x+=d.x/len*step;p.y+=d.y/len*step;p.z+=d.z/len*step;}return true;}

bool AnimationStateMachine::add_state(AnimationState s){if(s.id==0||s.name.empty()||s.clip==0)return false;if(current()!=nullptr && std::any_of(states_.begin(),states_.end(),[&](const auto&x){return x.id==s.id;}))return false;states_.push_back(std::move(s));if(state_==0)state_=states_.front().id;return true;}
bool AnimationStateMachine::add_transition(AnimationTransition t){if(t.from==0||t.to==0||t.parameter.empty())return false;transitions_.push_back(std::move(t));return true;}
bool AnimationStateMachine::set_initial(AnimationStateId id){if(!std::any_of(states_.begin(),states_.end(),[&](const auto&s){return s.id==id;}))return false;state_=id;elapsed_=0;return true;}
bool AnimationStateMachine::set_float(std::string n,float v){if(n.empty()||!std::isfinite(v))return false;parameters_.floats[std::move(n)]=v;return true;}
bool AnimationStateMachine::set_bool(std::string n,bool v){if(n.empty())return false;parameters_.bools[std::move(n)]=v;return true;}
const AnimationState* AnimationStateMachine::current()const noexcept{for(const auto&s:states_)if(s.id==state_)return &s;return nullptr;}
bool AnimationStateMachine::update(float dt,std::function<void(AnimationClipId,float)> play)noexcept{if(dt<0)return false;elapsed_+=dt;const auto* cur=current();if(!cur)return false;for(const auto&t:transitions_)if(t.from==state_){bool hit=false;auto f=parameters_.floats.find(t.parameter);if(f!=parameters_.floats.end())hit=t.greater?f->second>=t.threshold:f->second<=t.threshold;auto b=parameters_.bools.find(t.parameter);if(b!=parameters_.bools.end())hit=b->second;if(hit){state_=t.to;elapsed_=0;if(play){const auto*n=current();if(n)play(n->clip,t.blend);}return true;}}if(play&&elapsed_==dt)play(cur->clip,cur->blend);return false;}
void AnimationStateMachine::reset()noexcept{parameters_={};elapsed_=0;if(!states_.empty())state_=states_.front().id;else state_=0;}

void EnvironmentRendererState::update(const EnvironmentState& e)noexcept{const float sun=std::clamp((e.sun_elevation+0.15f)/1.15f,0.0f,1.0f);sky_.zenith={0.02f+0.12f*sun,0.04f+0.28f*sun,0.12f+0.48f*sun};sky_.horizon={0.16f+0.62f*sun,0.20f+0.68f*sun,0.28f+0.70f*sun};sky_.star_visibility=1.0f-sun;sky_.sun_disk=sun;switch(e.weather){case WeatherType::Cloudy:weather_.cloud_cover=.65f;break;case WeatherType::Rain:weather_.cloud_cover=.8f;weather_.cloud_density=.8f;weather_.precipitation=e.weather_intensity;break;case WeatherType::Storm:weather_.cloud_cover=1;weather_.cloud_density=1;weather_.precipitation=e.weather_intensity;break;case WeatherType::Snow:weather_.cloud_cover=.75f;weather_.precipitation=e.weather_intensity;weather_.snow_cover=.8f;break;case WeatherType::Fog:weather_.fog_density=e.weather_intensity;break;case WeatherType::Wind:weather_.wind_speed=e.weather_intensity*20;break;default:break;}weather_.fog_density=std::max(weather_.fog_density,e.weather_intensity*(e.weather==WeatherType::Fog?1.0f:0.0f));}

AudioSourceId AudioWorld::create_source(AudioSource s){if(s.id==0)s.id=next_id_++;if(s.id==0||std::any_of(sources_.begin(),sources_.end(),[&](const auto&x){return x.id==s.id;}))return 0;sources_.push_back(std::move(s));return sources_.back().id;}
bool AudioWorld::destroy_source(AudioSourceId id)noexcept{auto it=std::find_if(sources_.begin(),sources_.end(),[&](const auto&s){return s.id==id;});if(it==sources_.end())return false;sources_.erase(it);return true;}
bool AudioWorld::play(AudioSourceId id,bool loop)noexcept{for(auto&s:sources_)if(s.id==id){s.playing=true;s.looping=loop;return true;}return false;}
bool AudioWorld::stop(AudioSourceId id)noexcept{for(auto&s:sources_)if(s.id==id){s.playing=false;return true;}return false;}
bool AudioWorld::set_listener(AudioListener l)noexcept{if(!std::isfinite(l.position.x)||!std::isfinite(l.position.y)||!std::isfinite(l.position.z))return false;listener_=l;return true;}
const AudioSource* AudioWorld::source(AudioSourceId id)const noexcept{for(const auto&s:sources_)if(s.id==id)return &s;return nullptr;}
std::vector<AudioSource> AudioWorld::mix(float dt)const{(void)dt;std::vector<AudioSource> out;for(const auto&s:sources_)if(s.playing){auto copy=s;if(s.spatial){const Vec3 d{ s.position.x-listener_.position.x,s.position.y-listener_.position.y,s.position.z-listener_.position.z};const float dist=std::sqrt(d.x*d.x+d.y*d.y+d.z*d.z);copy.gain*=std::clamp(1.0f-dist/std::max(0.01f,s.max_distance),0.0f,1.0f);}out.push_back(std::move(copy));}return out;}

UiWidgetId UiWorld::create(UiWidgetType t,UiWidgetId parent){UiWidget w;w.id=next_id_++;w.type=t;w.parent=parent;widgets_.push_back(std::move(w));return widgets_.back().id;}
bool UiWorld::destroy(UiWidgetId id)noexcept{auto it=std::remove_if(widgets_.begin(),widgets_.end(),[&](const auto&w){return w.id==id||w.parent==id;});if(it==widgets_.end())return false;widgets_.erase(it,widgets_.end());return true;}
bool UiWorld::set_text(UiWidgetId id,std::string t){for(auto&w:widgets_)if(w.id==id){w.text=std::move(t);return true;}return false;}
bool UiWorld::set_value(UiWidgetId id,float v)noexcept{if(!std::isfinite(v))return false;for(auto&w:widgets_)if(w.id==id){w.value=v;return true;}return false;}
bool UiWorld::set_visible(UiWidgetId id,bool v)noexcept{for(auto&w:widgets_)if(w.id==id){w.visible=v;return true;}return false;}
const UiWidget* UiWorld::widget(UiWidgetId id)const noexcept{for(const auto&w:widgets_)if(w.id==id)return &w;return nullptr;}
std::vector<UiWidget> UiWorld::visible_widgets()const{std::vector<UiWidget> out;for(const auto&w:widgets_)if(w.visible)out.push_back(w);return out;}
void UiWorld::clear()noexcept{widgets_.clear();next_id_=1;}

bool ProductionSave::valid()const noexcept{return version>0&&game.valid();}
std::vector<std::uint8_t> SaveStore::encode(const ProductionSave& s){std::vector<std::uint8_t>b;if(!s.valid())return b;b.insert(b.end(),{'X','G','S','V',1});put_str(b,s.game.project_name);put_str(b,s.game.scene_name);put_f64(b,s.game.environment_seconds);put_u64(b,s.game.runtime_tick);put_u32(b,static_cast<std::uint32_t>(s.game.variables.size()));for(const auto&[k,v]:s.game.variables){put_str(b,k);put_str(b,v);}put_u32(b,static_cast<std::uint32_t>(s.blobs.size()));for(const auto&x:s.blobs){put_str(b,x.key);put_u32(b,static_cast<std::uint32_t>(x.bytes.size()));b.insert(b.end(),x.bytes.begin(),x.bytes.end());}return b;}
bool SaveStore::decode(const std::vector<std::uint8_t>&b,ProductionSave&s,std::string&error){if(b.size()<5||std::memcmp(b.data(),"XGSV",4)!=0||b[4]!=1){error="invalid save header";return false;}std::size_t p=5;s=ProductionSave{};if(!get_str(b,p,s.game.project_name)||!get_str(b,p,s.game.scene_name)||!get_f64(b,p,s.game.environment_seconds)||!get_u64(b,p,s.game.runtime_tick)){error="truncated save";return false;}std::uint32_t n=0;if(!get_u32(b,p,n)){error="truncated variables";return false;}for(std::uint32_t i=0;i<n;++i){std::string k,v;if(!get_str(b,p,k)||!get_str(b,p,v)){error="truncated variable";return false;}s.game.variables.emplace(std::move(k),std::move(v));}if(!get_u32(b,p,n)){error="truncated blobs";return false;}for(std::uint32_t i=0;i<n;++i){GameStateBlob x;std::uint32_t size=0;if(!get_str(b,p,x.key)||!get_u32(b,p,size)||p+size>b.size()){error="truncated blob";return false;}x.bytes.assign(b.begin()+static_cast<std::ptrdiff_t>(p),b.begin()+static_cast<std::ptrdiff_t>(p+size));p+=size;s.blobs.push_back(std::move(x));}s.version=1;return s.valid();}

GameSession::GameSession(GameSessionConfig c,ProjectSourceLoader l):config_(c),project_loader_(std::move(l)),streaming_([this](std::string_view uri,std::vector<std::uint8_t>& bytes,std::string& error){if(!project_loader_){error="no project loader";return false;}std::string text;if(!project_loader_(uri,text))return false;bytes.assign(text.begin(),text.end());return true;}){}
bool GameSession::open_project(std::string_view source){auto parsed=parse_project(source);if(!parsed.success)return false;GameRuntime::SceneLoader loader=[this](std::string_view uri,IR& ir){if(!project_loader_)return false;std::string text;if(!project_loader_(uri,text))return false;const auto compiled=Compiler{}.compile(SourceText(std::string(text),std::string(uri)));if(!compiled.success)return false;ir=compiled.ir;return true;};if(!game_.load_project(parsed.project,loader))return false;open_=true;stats_={};return true;}
bool GameSession::update(double dt)noexcept{if(!open_||dt<0)return false;if(!game_.update(dt))return false;environment_renderer_.update(game_.environment().state());const auto results=streaming_.poll(config_.max_stream_results_per_frame);stats_.stream_results+=results.size();nav_timer_+=static_cast<float>(dt);if(nav_timer_>=config_.navigation_update_interval){nav_timer_=0;++stats_.navigation_updates;}if(config_.enable_audio)(void)audio_.mix(static_cast<float>(dt));++stats_.frames;return true;}
std::vector<std::uint8_t> GameSession::save()const{ProductionSave s;s.game=game_.save();return SaveStore::encode(s);}
bool GameSession::restore(const std::vector<std::uint8_t>&bytes)noexcept{ProductionSave s;std::string error;if(!SaveStore::decode(bytes,s,error)||!game_.restore(s.game))return false;return true;}
bool GameSession::close()noexcept{if(!open_)return false;streaming_.shutdown();game_.reset();ui_.clear();open_=false;return true;}

} // namespace exgine
