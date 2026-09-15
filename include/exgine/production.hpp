#pragma once
#include "exgine/animation.hpp"
#include "exgine/asset.hpp"
#include "exgine/compiler.hpp"
#include "exgine/game.hpp"
#include "exgine/geometry.hpp"
#include "exgine/gpu_residency.hpp"
#include "exgine/import.hpp"
#include "exgine/vehicle.hpp"
#include "exgine/physics.hpp"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>
namespace exgine {
enum class ImagePixelFormat : std::uint8_t { R8=1, RGB8=3, RGBA8=4 };
struct ImageData { std::uint32_t width=0,height=0; ImagePixelFormat format=ImagePixelFormat::RGBA8; std::vector<std::uint8_t> pixels; [[nodiscard]] std::uint32_t channels() const noexcept{return static_cast<std::uint32_t>(format);} [[nodiscard]] bool valid() const noexcept; };
using ImageDecoder=std::function<bool(std::string_view,const std::vector<std::uint8_t>&,ImageData&,std::string&)>;
class ImageDecoderRegistry { public: ImageDecoderRegistry(); bool register_decoder(std::string extension,ImageDecoder decoder); [[nodiscard]] bool decode(std::string_view uri,const std::vector<std::uint8_t>& bytes,ImageData& out,std::string& error) const; private: mutable std::mutex mutex_; std::unordered_map<std::string,ImageDecoder> decoders_; };
struct MaterialTextureResource { std::string slot,uri; std::shared_ptr<const ImageData> image; AssetId asset_id=invalid_asset; [[nodiscard]] bool valid() const noexcept{return !slot.empty()&&!uri.empty()&&image&&image->valid();} };
struct MaterialPipelineResult { bool success=false; std::vector<MaterialTextureResource> textures; std::string error; };
using AssetBytesLoader=std::function<bool(std::string_view,std::vector<std::uint8_t>&,std::string&)>;
class GltfMaterialPipeline { public: [[nodiscard]] MaterialPipelineResult build(std::string_view gltf_uri,std::string_view json_or_glb,const AssetBytesLoader& loader,const ImageDecoderRegistry& decoders) const; };
enum class StreamPriority : std::uint8_t { Background=0,Normal=1,High=2,Critical=3 };
struct AssetStreamRequest { std::uint64_t request_id=0; std::string uri; StreamPriority priority=StreamPriority::Normal; [[nodiscard]] bool valid() const noexcept{return request_id!=0&&!uri.empty();} };
struct AssetStreamResult { AssetStreamRequest request; bool success=false; std::vector<std::uint8_t> bytes; std::string error; };
struct AssetStreamingStats { std::uint64_t submitted=0,completed=0,failed=0,cancelled=0,active=0; };
class AsyncAssetStreamer { public: using Loader=std::function<bool(std::string_view,std::vector<std::uint8_t>&,std::string&)>; explicit AsyncAssetStreamer(Loader loader={},std::uint32_t workers=2); ~AsyncAssetStreamer(); AsyncAssetStreamer(const AsyncAssetStreamer&)=delete; AsyncAssetStreamer& operator=(const AsyncAssetStreamer&)=delete; [[nodiscard]] std::uint64_t submit(std::string uri,StreamPriority priority=StreamPriority::Normal); [[nodiscard]] std::vector<AssetStreamResult> poll(std::size_t max_results=32); bool cancel(std::uint64_t request_id); void shutdown() noexcept; [[nodiscard]] AssetStreamingStats stats() const noexcept; private: struct Task{AssetStreamRequest request;}; mutable std::mutex mutex_; std::condition_variable cv_; std::priority_queue<Task,std::vector<Task>,std::function<bool(const Task&,const Task&)>> queue_; std::queue<AssetStreamResult> completed_; std::unordered_map<std::uint64_t,bool> cancelled_; std::vector<std::thread> workers_; Loader loader_; std::atomic<std::uint64_t> next_id_{1}; AssetStreamingStats stats_{}; bool stopping_=false; void worker_loop(); };
using NavNodeId=std::uint32_t;
struct NavCell{NavNodeId id=0;std::int32_t x=0,z=0;float height=0;bool walkable=false;};
struct NavMeshConfig{std::uint32_t width=128,depth=128;float cell_size=1,max_slope=.8f,agent_radius=.35f;};
struct NavPath{bool success=false;std::vector<Vec3> points;float cost=0;};
class NavigationMesh{public:explicit NavigationMesh(NavMeshConfig config={});[[nodiscard]]const NavMeshConfig& config()const noexcept{return config_;}bool build(const std::function<float(float,float)>& height,const std::function<bool(float,float)>& walkable);bool set_cell(std::int32_t x,std::int32_t z,bool walkable,float height);[[nodiscard]]const NavCell* cell(std::int32_t x,std::int32_t z)const noexcept;[[nodiscard]]NavPath find_path(Vec3 start,Vec3 goal)const;private:NavMeshConfig config_{};std::vector<NavCell> cells_;[[nodiscard]]NavNodeId index(std::int32_t x,std::int32_t z)const noexcept;};
struct NavigationAgent{EntityId entity=invalid_entity;Vec3 destination{};float speed=2.5f,stopping_distance=.2f;NavPath path{};std::size_t waypoint=0;bool active=false;};
class NavigationSystem{public:explicit NavigationSystem(NavMeshConfig config={});bool build(const std::function<float(float,float)>& height,const std::function<bool(float,float)>& walkable);bool set_destination(NavigationAgent& agent,Vec3 destination);bool update(NavigationAgent& agent,Vec3& position,float dt)const noexcept;[[nodiscard]]const NavigationMesh& mesh()const noexcept{return mesh_;}private:NavigationMesh mesh_;};
typedef std::uint32_t AnimationStateId;
struct AnimationState{AnimationStateId id=0;std::string name;AnimationClipId clip=0;float speed=1,blend=.15f;};
struct AnimationTransition{AnimationStateId from=0,to=0;std::string parameter;float threshold=0;bool greater=true;float blend=.15f;};
struct AnimationParameters{std::unordered_map<std::string,float> floats;std::unordered_map<std::string,bool> bools;};
class AnimationStateMachine{public:bool add_state(AnimationState state);bool add_transition(AnimationTransition transition);bool set_initial(AnimationStateId id);bool set_float(std::string name,float value);bool set_bool(std::string name,bool value);[[nodiscard]]AnimationStateId state()const noexcept{return state_;}[[nodiscard]]const AnimationState* current()const noexcept;[[nodiscard]]bool update(float dt,std::function<void(AnimationClipId,float)> play)noexcept;void reset()noexcept;private:std::vector<AnimationState> states_;std::vector<AnimationTransition> transitions_;AnimationParameters parameters_;AnimationStateId state_=0;float elapsed_=0;};
struct SkyState{Color3 zenith{.12f,.25f,.5f};Color3 horizon{.75f,.82f,.9f};float star_visibility=0,sun_disk=1;};
struct WeatherVisualState{float cloud_cover=0,cloud_density=0,precipitation=0,wind_speed=0,fog_density=0,snow_cover=0;};
class EnvironmentRendererState{public:void update(const EnvironmentState& environment)noexcept;[[nodiscard]]const SkyState& sky()const noexcept{return sky_;}[[nodiscard]]const WeatherVisualState& weather()const noexcept{return weather_;}private:SkyState sky_{};WeatherVisualState weather_{};};
using AudioSourceId=std::uint64_t;
struct AudioListener{Vec3 position{};Vec3 velocity{};Vec3 forward{0,0,1};Vec3 up{0,1,0};};
struct AudioSource{AudioSourceId id=0;AssetId clip=invalid_asset;Vec3 position{};Vec3 velocity{};float gain=1,pitch=1,min_distance=1,max_distance=50;bool looping=false,playing=false,spatial=true;};
class AudioWorld{public:AudioSourceId create_source(AudioSource source={});bool destroy_source(AudioSourceId id)noexcept;bool play(AudioSourceId id,bool looping=false)noexcept;bool stop(AudioSourceId id)noexcept;bool set_listener(AudioListener listener)noexcept;[[nodiscard]]const AudioListener& listener()const noexcept{return listener_;}[[nodiscard]]const AudioSource* source(AudioSourceId id)const noexcept;[[nodiscard]]std::vector<AudioSource> mix(float dt)const;private:AudioListener listener_{};std::vector<AudioSource> sources_;AudioSourceId next_id_=1;};
enum class UiWidgetType:std::uint8_t{Panel,Label,Button,Image,Progress,Inventory,Dialogue};
using UiWidgetId=std::uint64_t; struct UiRect{float x=0,y=0,w=0,h=0;};
struct UiWidget{UiWidgetId id=0,parent=0;UiWidgetType type=UiWidgetType::Panel;UiRect rect{};std::string text;float value=0;bool visible=true,enabled=true;};
class UiWorld{public:UiWidgetId create(UiWidgetType type,UiWidgetId parent=0);bool destroy(UiWidgetId id)noexcept;bool set_text(UiWidgetId id,std::string text);bool set_value(UiWidgetId id,float value)noexcept;bool set_visible(UiWidgetId id,bool visible)noexcept;[[nodiscard]]const UiWidget* widget(UiWidgetId id)const noexcept;[[nodiscard]]std::vector<UiWidget> visible_widgets()const;void clear()noexcept;private:std::vector<UiWidget> widgets_;UiWidgetId next_id_=1;};
struct GameStateBlob{std::string key;std::vector<std::uint8_t> bytes;};
struct ProductionSave{std::uint32_t version=1;GameSaveState game;std::vector<GameStateBlob> blobs;[[nodiscard]]bool valid()const noexcept;};
class SaveStore{public:[[nodiscard]]static std::vector<std::uint8_t> encode(const ProductionSave& save);[[nodiscard]]static bool decode(const std::vector<std::uint8_t>& bytes,ProductionSave& save,std::string& error);};
struct GameSessionConfig{std::uint32_t max_stream_results_per_frame=8;float navigation_update_interval=1.0f/30.0f;bool enable_audio=true,enable_ui=true;};
struct GameSessionStats{std::uint64_t frames=0,stream_results=0,navigation_updates=0,save_operations=0;};
using ProjectSourceLoader=std::function<bool(std::string_view,std::string&)>;
class GameSession{public:explicit GameSession(GameSessionConfig config={},ProjectSourceLoader loader={});[[nodiscard]]bool open_project(std::string_view project_source);[[nodiscard]]bool update(double dt)noexcept;[[nodiscard]]std::vector<std::uint8_t> save()const;[[nodiscard]]bool restore(const std::vector<std::uint8_t>& bytes)noexcept;bool close()noexcept;[[nodiscard]]GameRuntime& game()noexcept{return game_;}[[nodiscard]]const GameRuntime& game()const noexcept{return game_;}[[nodiscard]]NavigationSystem& navigation()noexcept{return navigation_;}[[nodiscard]]AsyncAssetStreamer& streaming()noexcept{return streaming_;}[[nodiscard]]AudioWorld& audio()noexcept{return audio_;}[[nodiscard]]UiWorld& ui()noexcept{return ui_;}[[nodiscard]]EnvironmentRendererState& environment_renderer()noexcept{return environment_renderer_;}[[nodiscard]]const GameSessionStats& stats()const noexcept{return stats_;}[[nodiscard]]bool open()const noexcept{return open_;}private:GameSessionConfig config_{};ProjectSourceLoader project_loader_{};GameRuntime game_{};NavigationSystem navigation_{};AsyncAssetStreamer streaming_{};AudioWorld audio_{};UiWorld ui_{};EnvironmentRendererState environment_renderer_{};GameSessionStats stats_{};float nav_timer_=0;bool open_=false;};
} // namespace exgine
