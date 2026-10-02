#pragma once

#include "core/ir/model.hpp"

#include <string>
#include <vector>

namespace cjm::generator::simdjson {

struct GenerationResult {
    bool success = false;
    std::string header;
    std::string error;
    // Non-fatal limitations of successfully generated output.
    std::vector<std::string> warnings;
};

// Generate an experimental simdjson header from supported Metadata IR.
GenerationResult generate_header(const metadata::ProjectModel& project);

} // namespace cjm::generator::simdjson
