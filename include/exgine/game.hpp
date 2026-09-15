#pragma once

#include "exgine/environment.hpp"
#include "exgine/ir.hpp"
#include "exgine/runtime.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace exgine {

enum class GameAssetKind : std::uint8_t { Scene, World, Script, Material, Generic };

struct GameAssetRef {
    std::string name;
    std::string uri;
    GameAssetKind kind = GameAssetKind::Generic;
    [[nodiscard]] bool valid() const noexcept { return !name.empty() && !uri.empty(); }
};

struct GameProject {
    std::string name = "EXGINE Game";
    std::string version = "1.0";
    std::string startup_scene;
    std::uint32_t tick_rate = 60;
    CalendarConfig calendar{};
    std::vector<GameAssetRef> assets;
    std::vector<std::string> scene_names;
    std::unordered_map<std::string, std::string> settings;
    [[nodiscard]] bool valid() const noexcept;
};

struct GameProjectLoadResult {
    bool success = false;
    GameProject project;
    std::string error;
};

[[nodiscard]] GameProjectLoadResult parse_project(std::string_view source);
[[nodiscard]] std::string serialize_project(const GameProject& project);

struct GameScene {
    std::string name;
    IR ir;
    [[nodiscard]] bool valid() const noexcept { return !name.empty() && ir.root.kind == NodeKind::World; }
};

struct GameSaveState {
    std::string project_name;
    std::string scene_name;
    double environment_seconds = 0.0;
    std::uint64_t runtime_tick = 0;
    std::unordered_map<std::string, std::string> variables;
    std::vector<std::string> active_entities;
    [[nodiscard]] bool valid() const noexcept { return !project_name.empty() && environment_seconds >= 0.0; }
};

[[nodiscard]] std::string serialize_save(const GameSaveState& state);
[[nodiscard]] GameSaveState parse_save(std::string_view source);

class GameRuntime {
public:
    using SceneLoader = std::function<bool(std::string_view, IR&)>;

    GameRuntime() = default;
    [[nodiscard]] bool load_project(const GameProject& project, SceneLoader loader = {});
    [[nodiscard]] bool add_scene(GameScene scene);
    [[nodiscard]] bool activate_scene(std::string_view name);
    [[nodiscard]] bool update(double real_seconds) noexcept;
    void reset() noexcept;

    [[nodiscard]] bool set_variable(std::string name, std::string value);
    [[nodiscard]] std::string_view variable(std::string_view name) const noexcept;
    [[nodiscard]] bool remove_variable(std::string_view name) noexcept;

    [[nodiscard]] GameSaveState save() const;
    [[nodiscard]] bool restore(const GameSaveState& state) noexcept;

    [[nodiscard]] const GameProject& project() const noexcept { return project_; }
    [[nodiscard]] const std::string& active_scene() const noexcept { return active_scene_; }
    [[nodiscard]] const EnvironmentSystem& environment() const noexcept { return environment_; }
    [[nodiscard]] EnvironmentSystem& environment() noexcept { return environment_; }
    [[nodiscard]] const Runtime& runtime() const noexcept { return runtime_; }
    [[nodiscard]] Runtime& runtime() noexcept { return runtime_; }
    [[nodiscard]] bool loaded() const noexcept { return loaded_; }
private:
    GameProject project_{};
    EnvironmentSystem environment_{};
    Runtime runtime_{};
    std::unordered_map<std::string, GameScene> scenes_;
    std::unordered_map<std::string, std::string> variables_;
    std::string active_scene_;
    bool loaded_ = false;
    double accumulator_ = 0.0;
    SceneLoader scene_loader_{};
};

} // namespace exgine
