#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
namespace exgine {
enum class ShaderStage : std::uint8_t { Vertex, Fragment };
struct ShaderProgram { std::string name; std::string vertex_source; std::string fragment_source; [[nodiscard]] bool valid() const noexcept; };
class ShaderLibrary { public: ShaderLibrary(); [[nodiscard]] const ShaderProgram* find(std::string_view name) const noexcept; [[nodiscard]] std::size_t size() const noexcept { return programs_.size(); } private: std::unordered_map<std::string,ShaderProgram> programs_; };
[[nodiscard]] ShaderProgram make_pbr_shader();
[[nodiscard]] ShaderProgram make_unlit_shader();
[[nodiscard]] ShaderProgram make_mobile_pbr_shader();
[[nodiscard]] ShaderProgram make_mobile_skinned_pbr_shader();
} // namespace exgine
