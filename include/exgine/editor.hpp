#pragma once

#include "exgine/ir.hpp"
#include "exgine/runtime.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace exgine {

enum class EditorAssetType : std::uint8_t { Mesh, Texture, Material, Audio, Script, Scene };

struct EditorAsset {
    std::uint64_t id = 0;
    EditorAssetType type = EditorAssetType::Mesh;
    std::string name;
    std::string source;
    bool valid() const noexcept;
};

struct EditorNode {
    std::uint64_t id = 0;
    NodeKind kind = NodeKind::Property;
    std::string name;
    std::uint64_t parent = 0;
    SceneTransform transform{};
    std::vector<Property> properties;
    bool active = true;
    bool valid() const noexcept;
};

struct EditorProject {
    static constexpr std::uint32_t format_version = 1;
    std::string name = "Untitled";
    std::string startup_scene = "Main";
    std::vector<EditorNode> nodes;
    std::vector<EditorAsset> assets;
    std::uint64_t root_id = 0;
    std::uint64_t next_node_id = 1;
    std::uint64_t next_asset_id = 1;
    bool valid() const noexcept;
};

struct EditorViewport {
    Camera camera{};
    bool grid_visible = true;
    bool gizmos_visible = true;
    bool wireframe = false;
    float grid_size = 1.0F;
    bool valid() const noexcept;
};

class EditorSession {
public:
    EditorSession();

    [[nodiscard]] EditorProject& project() noexcept { return project_; }
    [[nodiscard]] const EditorProject& project() const noexcept { return project_; }
    [[nodiscard]] EditorViewport& viewport() noexcept { return viewport_; }
    [[nodiscard]] const EditorViewport& viewport() const noexcept { return viewport_; }
    [[nodiscard]] const Runtime& preview_runtime() const noexcept { return runtime_; }
    [[nodiscard]] Runtime& preview_runtime() noexcept { return runtime_; }

    std::uint64_t create_node(NodeKind kind, std::string name, std::uint64_t parent = 0);
    bool destroy_node(std::uint64_t id) noexcept;
    bool set_parent(std::uint64_t id, std::uint64_t parent) noexcept;
    bool set_transform(std::uint64_t id, SceneTransform transform) noexcept;
    bool rename_node(std::uint64_t id, std::string name);
    bool set_active(std::uint64_t id, bool active) noexcept;
    bool set_property(std::uint64_t id, Property property);
    bool remove_property(std::uint64_t id, std::string_view name) noexcept;
    bool add_asset(EditorAssetType type, std::string name, std::string source, std::uint64_t* id = nullptr);
    bool remove_asset(std::uint64_t id) noexcept;

    bool select(std::uint64_t id) noexcept;
    void clear_selection() noexcept { selected_id_ = 0; }
    [[nodiscard]] std::uint64_t selected() const noexcept { return selected_id_; }

    bool undo() noexcept;
    bool redo() noexcept;
    [[nodiscard]] bool can_undo() const noexcept { return !undo_.empty(); }
    [[nodiscard]] bool can_redo() const noexcept { return !redo_.empty(); }
    void clear_history() noexcept;

    bool save(std::string_view path) const;
    bool load(std::string_view path);
    bool preview();
    [[nodiscard]] std::string emit_source() const;
    [[nodiscard]] bool dirty() const noexcept { return dirty_; }
    void mark_clean() noexcept { dirty_ = false; }

private:
    struct HistoryEntry { EditorProject before; EditorProject after; std::uint64_t selected_before = 0; std::uint64_t selected_after = 0; };
    EditorProject project_;
    EditorViewport viewport_;
    Runtime runtime_;
    std::vector<HistoryEntry> undo_;
    std::vector<HistoryEntry> redo_;
    std::uint64_t selected_id_ = 0;
    bool dirty_ = false;
    bool mutate(EditorProject next, std::uint64_t next_selected = 0);
    EditorNode* find(std::uint64_t id) noexcept;
    const EditorNode* find(std::uint64_t id) const noexcept;
    bool would_cycle(std::uint64_t id, std::uint64_t parent) const noexcept;
    static void append_source_node(const EditorProject&, const EditorNode&, std::string&, int depth);
    static std::string quote(std::string_view value);
};

} // namespace exgine
