#pragma once

#include <string>
#include <vector>
#include <iosfwd>
#include "core/ir/model.hpp"

namespace cjm::generator::simdjson::detail {
// Return the first unsupported-field diagnostic, or an empty string.
std::string validate_decode_project(const metadata::ProjectModel& project);

// Generate the experimental decode error and structured path types.
void generate_decode_error_model(std::ostringstream& out);

// Generate the internal object decoder for one model.
void generate_object_decode_function(
    std::ostringstream& out, const metadata::TypeModel& type,
    const std::vector<metadata::EnumModel>& enums);

// Generate the public root decoder for one model.
void generate_root_decode_function(std::ostringstream& out,
                                   const metadata::TypeModel& type);

} // namespace cjm::generator::simdjson::detail
