#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

namespace cjm::generator::simdjson::detail {

// Generate a C++ string literal preserving the input bytes.
inline std::string cpp_string_literal(std::string_view input) {
    throw std::logic_error("cpp_string_literal: not implemented");
}

} // namespace cjm::generator::simdjson::detail
