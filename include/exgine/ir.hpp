#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace exgine {

enum class NodeKind {
    World,
    Terrain,
    Vegetation,
    Building,
    Vehicle,
    Property
};

using PropertyValue = std::variant<int64_t, double, bool, std::string>;

struct Property {
    std::string name;
    PropertyValue value;
};

struct Node {
    NodeKind kind;
    std::string name;
    std::vector<Property> properties;
    std::vector<Node> children;

    [[nodiscard]] const Property* find_property(const std::string& property_name) const noexcept;
    Property* find_property(const std::string& property_name) noexcept;
};

class IR {
public:
    IR() = default;
    IR(const IR&) = default;
    IR(IR&&) noexcept = default;
    IR& operator=(const IR&) = default;
    IR& operator=(IR&&) noexcept = default;
    ~IR() = default;

    Node root{NodeKind::World, "world", {}, {}};

    void add_property(std::string name, PropertyValue value);
    Node& add_child(NodeKind kind, std::string name);
};

} // namespace exgine
