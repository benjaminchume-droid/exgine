#pragma once

#include <cstdint>
#include <string>
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

struct Property {
    std::string name;
    std::variant<int64_t, double, bool, std::string> value;
};

struct Node {
    NodeKind kind;
    std::string name;
    std::vector<Property> properties;
    std::vector<Node> children;
};

class IR {
public:
    Node root{NodeKind::World, "world", {}, {}};

    void add_property(std::string name, std::variant<int64_t, double, bool, std::string> value);
};

} // namespace exgine
