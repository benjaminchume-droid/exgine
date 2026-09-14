#pragma once

#include <cstddef>
#include <string_view>

namespace exgine {

class SourceText {
public:
    explicit SourceText(std::string_view text) noexcept : text_(text) {}

    [[nodiscard]] std::string_view text() const noexcept { return text_; }
    [[nodiscard]] std::size_t size() const noexcept { return text_.size(); }
    [[nodiscard]] char at(std::size_t offset) const noexcept {
        return offset < text_.size() ? text_[offset] : '\0';
    }

private:
    std::string_view text_;
};

} // namespace exgine
