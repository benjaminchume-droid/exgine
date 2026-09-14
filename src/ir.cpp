#include "exgine/ir.hpp"

#include <utility>

namespace exgine {

void IR::add_property(std::string name,
                      std::variant<int64_t, double, bool, std::string> value) {
    root.properties.push_back(Property{std::move(name), std::move(value)});
}

} // namespace exgine
