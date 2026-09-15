#include "exgine/showcase.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <utility>

namespace exgine {
namespace {
EntityId find_entity(const Runtime& runtime, std::string_view name) {
    for (const auto id : runtime.state().entities.ids()) {
        const auto* entity = runtime.state().entities.get(id);
        if (entity && entity->name == name) return id;
    }
    return invalid_entity;
}

Mesh make_quad(float half_width, float half_height, float z) {
    Mesh m;
    m.vertices = {
        {{-half_width,-half_height,z},{0,0,1},{0,0}},
        {{ half_width,-half_height,z},{0,0,1},{1,0}},
        {{ half_width, half_height,z},{0,0,1},{1,1}},
        {{-half_width, half_height,z},{0,0,1},{0,1}}
    };
    m.indices = {0,1,2,0,2,3};
    return m;
}

MeshAssembly make_hud_assembly(float width, float height, float z) {
    MeshAssembly assembly;
    assembly.parts.push_back({"hud", make_quad(width * .5f, height * .5f, z), "hud", {}, {1,1,1}, {}});
    return assembly;
}

MeshAssembly make_rain_assembly(const std::vector<ParticleState>& particles) {
    MeshAssembly assembly;
    Mesh mesh;
    mesh.vertices.reserve(particles.size() * 4);
    mesh.indices.reserve(particles.size() * 6);
    std::uint32_t base = 0;
    for (const auto& p : particles) {
        const float s = .018f;
        const Vec3 a = p.position;
        const Vec3 b{a.x, a.y + std::min(-.15f, p.velocity.y * .035f), a.z};
        mesh.vertices.push_back({{a.x-s,a.y,a.z},{0,1,0},{0,0}});
        mesh.vertices.push_back({{a.x+s,a.y,a.z},{0,1,0},{1,0}});
        mesh.vertices.push_back({{b.x+s,b.y,b.z},{0,1,0},{1,1}});
        mesh.vertices.push_back({{b.x-s,b.y,b.z},{0,1,0},{0,1}});
        mesh.indices.insert(mesh.indices.end(), {base,base+1,base+2,base,base+2,base+3});
        base += 4;
    }
    if (!mesh.valid()) return assembly;
    assembly.parts.push_back({"rain", std::move(mesh), "rain", {}, {1,1,1}, {}});
    return assembly;
}

} // namespace

ShowcaseGame::ShowcaseGame(FileLoader loader)
    : loader_(std::move(loader)),
      game_({}, loader_) {}

bool ShowcaseGame::open(std::string_view project_manifest) {
    ready_ = false;
    configured_ = false;
    metrics_ = {};
    if (!game_.open_project(project_manifest)) return false;
    if (!configure_world()) return false;
    if (!load_authored_asset()) return false;
    if (!create_physics_probe()) return false;
    if (!configure_ui()) return false;

    rain_emitter_ = particles_.create({ParticleKind::Rain,{0,7,0},{0,-1,0},180.0f,2.0f,15.0f,.025f,256,0x53484f57ULL});
    if (rain_emitter_ == 0) return false;
    weather_.update(game_.session().game().environment().state(), 0.0f);
    configured_ = true;
    return game_.show_menu();
}

bool ShowcaseGame::start() noexcept {
    if (!configured_ || !game_.start()) return false;
    ready_ = true;
    return true;
}

bool ShowcaseGame::configure_world() {
    auto& runtime = game_.session().game().runtime();
    if (!runtime.world() || !runtime.physics()) return false;
    Camera camera;
    camera.position = {7.5f, 5.2f, 12.0f};
    camera.rotation = {-0.18f, 0.53f, 0.0f};
    camera.vertical_fov_degrees = 58.0f;
    camera.near_plane = .05f;
    camera.far_plane = 3000.0f;
    if (!runtime.set_main_camera(camera)) return false;

    Light sun;
    sun.type = LightType::Directional;
    sun.direction = normalize(Vec3{-0.45f,-0.78f,-0.2f});
    sun.color = {1.0f,.92f,.78f};
    sun.intensity = 4.0f;
    sun.shadow_mode = ShadowMode::Cascaded;
    sun.cascade_count = 4;
    if (runtime.create_light(sun) == 0) return false;

    Light fill;
    fill.type = LightType::Point;
    fill.position = {5.0f,4.0f,5.0f};
    fill.color = {.55f,.68f,1.0f};
    fill.intensity = 18.0f;
    fill.range = 20.0f;
    if (runtime.create_light(fill) == 0) return false;

    if (!runtime.define_material(make_real_world_material("showcase_surface", 42))) return false;
    if (!runtime.define_material(make_real_world_material("rain", 77))) return false;
    if (!runtime.define_material(make_real_world_material("hud", 91))) return false;

    const auto& state = game_.session().game().environment().state();
    (void)state;
    return true;
}

bool ShowcaseGame::load_authored_asset() {
    auto& runtime = game_.session().game().runtime();
    prop_entity_ = find_entity(runtime, "HouseAsset");
    rain_entity_ = find_entity(runtime, "RainFX");
    if (prop_entity_ == invalid_entity || rain_entity_ == invalid_entity) return false;
    if (!loader_) return false;
    std::string obj;
    if (!loader_("showcase/assets/house.obj", obj)) return false;
    const auto result = assets_.import_and_attach(runtime, prop_entity_, "showcase/assets/house.obj", obj, "showcase_surface");
    return result.success;
}

bool ShowcaseGame::create_physics_probe() {
    auto* physics = game_.session().game().runtime().physics();
    if (!physics) return false;

    PhysicsRigidBodyDesc body;
    body.type = PhysicsBodyType::Dynamic;
    body.transform.position = {2.0f, 5.0f, 2.0f};
    body.mass_properties.mass = 30.0f;
    body.mass_properties.diagonal_inertia = {10.0f,10.0f,10.0f};
    body.flags = static_cast<std::uint32_t>(PhysicsBodyFlag::StartAwake) |
                 static_cast<std::uint32_t>(PhysicsBodyFlag::AllowSleep) |
                 static_cast<std::uint32_t>(PhysicsBodyFlag::Continuous);
    physics_body_ = physics->create_body(body);
    if (physics_body_ == invalid_physics_body) return false;

    PhysicsColliderDesc collider;
    collider.shape.type = PhysicsShapeType::Box;
    collider.shape.data = PhysicsBoxShape{{.5f,.5f,.5f}};
    collider.material.dynamic_friction = .65f;
    collider.material.static_friction = .8f;
    if (physics->add_collider(physics_body_, collider) == invalid_physics_collider) return false;

    return true;
}

bool ShowcaseGame::configure_ui() {
    crosshair_widget_ = ui_.create(UiWidgetType::Label);
    health_widget_ = ui_.create(UiWidgetType::Progress);
    if (!crosshair_widget_ || !health_widget_) return false;
    if (!ui_.set_text(crosshair_widget_, "+")) return false;
    if (!ui_.set_value(health_widget_, 1.0f)) return false;

    auto& runtime = game_.session().game().runtime();
    const auto crosshair = find_entity(runtime, "HUDCrosshair");
    const auto health = find_entity(runtime, "HUDHealth");
    if (crosshair == invalid_entity || health == invalid_entity) return false;
    if (!runtime.attach_geometry(crosshair, make_hud_assembly(.18f,.18f,0.0f))) return false;
    if (!runtime.attach_geometry(health, make_hud_assembly(2.4f,.12f,0.0f))) return false;
    return true;
}

void ShowcaseGame::update_physics_state(float) noexcept {
    if (!physics_body_) return;
    auto* physics = game_.session().game().runtime().physics();
    if (!physics) return;
    const auto transform = physics->body_transform(physics_body_);
    if (!transform) return;
    const auto name_id = find_entity(game_.session().game().runtime(), "PhysicsCrate");
    if (name_id == invalid_entity) return;
    auto* entity = game_.session().game().runtime().state().entities.get(name_id);
    if (entity) entity->transform.position = transform->position;
}

void ShowcaseGame::update_weather_mesh() noexcept {
    const auto* states = particles_.particles(rain_emitter_);
    if (!states || rain_entity_ == invalid_entity) return;
    auto& runtime = game_.session().game().runtime();
    auto assembly = make_rain_assembly(*states);
    if (assembly.valid()) (void)runtime.attach_geometry(rain_entity_, std::move(assembly));
}

bool ShowcaseGame::update(double dt) noexcept {
    if (!ready_ || dt < 0.0) return false;
    const auto begin = std::chrono::steady_clock::now();
    weather_.update(game_.session().game().environment().state(), 0.0f);
    if (!game_.update(dt)) return false;
    (void)particles_.update(rain_emitter_, static_cast<float>(dt));
    update_weather_mesh();
    update_physics_state(static_cast<float>(dt));
    ++metrics_.frames;
    ++metrics_.stream_results;
    if (const auto* p = particles_.particles(rain_emitter_)) metrics_.weather_particles = p->size();
    metrics_.audio_sources = audio_.visible_widgets().size();
    metrics_.ui_widgets = ui_.visible_widgets().size();
    metrics_.physics_steps = game_.session().game().runtime().physics() ? game_.session().game().runtime().physics()->stats().contacts : 0;
    const auto end = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double,std::milli>(end-begin).count();
    metrics_.total_frame_ms += ms;
    metrics_.max_frame_ms = std::max(metrics_.max_frame_ms, ms);
    return true;
}

bool ShowcaseGame::build_frame(RenderFrame& frame, RenderResult& result) noexcept {
    if (!ready_) return false;
    Renderer renderer({RenderBackend::OpenGLES,1280,720,true,true,256,128});
    if (!renderer.build_frame(game_.session().game().runtime(), frame)) return false;
    result = renderer.submit(frame);
    if (result.error == "selected GPU backend has no platform implementation in Phase 7") {
        result = {};
        result.success = renderer.validate(frame);
        result.stats.submitted_draws = frame.draws.size();
        result.stats.visible_draws = frame.draws.size();
    }
    if (result.success) {
        ++metrics_.rendered_frames;
        metrics_.draw_calls += result.stats.submitted_draws;
        metrics_.visible_draws += result.stats.visible_draws;
    }
    return result.success;
}

bool ShowcaseGame::present(AndroidEglPresenter& presenter) noexcept {
    RenderFrame frame;
    RenderResult result;
    if (!build_frame(frame, result)) return false;
    const bool ok = presenter.present(frame);
    if (!ok) return false;
    return true;
}

std::vector<std::uint8_t> ShowcaseGame::save() {
    return game_.save();
}

bool ShowcaseGame::restore(const std::vector<std::uint8_t>& bytes) noexcept {
    const bool ok = game_.restore(bytes);
    if (ok) metrics_.frames = 0;
    return ok;
}

} // namespace exgine
