#pragma once

#include "core/ir/model.hpp"

#include <iosfwd>
#include <vector>

namespace cjm::generator::simdjson::detail {

// Generate the experimental encode error and public API declarations.
void generate_encode_error_model(std::ostringstream& out);

// Generate value encoding and error reoprting for one enum field.
void generate_enum_field_encode(std::ostringstream& out,
                                const metadata::FieldModel& field,
                                const metadata::EnumModel& enum_model);

// Generate an object encoder for supported scalar fields.
void generate_scalar_object_encode_function(
    std::ostringstream& out, const metadata::TypeModel& type,
    const std::vector<metadata::EnumModel>& enums);

// Generate the public root encoder for one model.
void generate_root_encode_function(std::ostringstream& out,
                                   const metadata::TypeModel& type);

} // namespace cjm::generator::simdjson::detail
