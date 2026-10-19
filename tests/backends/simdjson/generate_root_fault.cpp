#include "backends/simdjson/decoder.h"
#include "backends/simdjson/encoder.h"

#include <fstream>
#include <sstream>

int main(int argc, char** argv) {
    if (argc != 2) {
        return 1;
    }

    // 1. Prepare the generated header's dependencies.
    std::ostringstream out;
    out << "#pragma once\n"
        << "#include <cstddef>\n"
        << "#include <new>\n"
        << "#include <optional>\n"
        << "#include <stdexcept>\n"
        << "#include <string>\n"
        << "#include <string_view>\n"
        << "#include <vector>\n"
        << "#include \"tests/fixtures/simdjson_root_fault.hpp\"\n\n";

    // 2. Generate the real runtime types and root encoder.
    namespace generator = cjm::generator::simdjson::detail;
    generator::generate_decode_error_model(out);
    generator::generate_encode_error_model(out);

    cjm::metadata::TypeModel model;
    model.name = "RootEncodeFault";
    model.qualified_name = "app::RootEncodeFault";
    generator::generate_root_encode_function(out, model);

    // 3. Write the header requested by the test build.
    std::ofstream file(argv[1]);
    if (!file) {
        return 1;
    }
    file << out.str();
    file.close();
    return file ? 0 : 1;
}
