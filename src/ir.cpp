#include "exgine/ir.hpp"

#include <utility>

namespace exgine {

const Property* Node::find_property(const std::string& property_name) const noexcept {
    for (const auto& property : properties) {
        if (property.name == property_name) {
            return &property;
        }
    }
    return nullptr;
}

Property* Node::find_property(const std::string& property_name) noexcept {
    for (auto& property : properties) {
        if (property.name == property_name) {
            return &property;
        }
    }
    return nullptr;
}

void IR::add_property(std::string name, PropertyValue value) {
    root.properties.push_back(Property{std::move(name), std::move(value)});
}

Node& IR::add_child(NodeKind kind, std::string name) {
    root.children.push_back(Node{kind, std::move(name), {}, {}});
    return root.children.back();
}

} // namespace exgine
