#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

namespace cjm::generator::simdjson::detail {

// Generate a C++ string literal preserving the input bytes.
inline std::string cpp_string_literal(std::string_view input) {
    std::string result = "\"";
    for (unsigned char byte : input) {
        if (byte < 0x20 || byte > 0x7e) {
            result.push_back('\\');
            result.push_back(static_cast<char>('0' + byte / 64));
            result.push_back(static_cast<char>('0' + (byte / 8) % 8));
            result.push_back(static_cast<char>('0' + byte % 8));
            continue;
        }
        if (byte == '"' || byte == '\\') {
            result.push_back('\\');
        }
        result.push_back(static_cast<char>(byte));
    }
    result.push_back('"');
    return result;
}

// Generate a C++ string_view expression preserving the input byte length.
inline std::string cpp_string_view_expression(std::string_view input) {
    return "std::string_view{" + cpp_string_literal(input) + ", " +
           std::to_string(input.size()) + "}";
}

} // namespace cjm::generator::simdjson::detail
