#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstddef>

#include "backends/nlohmann/cpp_generator.hpp"
#include "backends/simdjson/cpp_generator.hpp"
#include "backends/schema/schema_generator.hpp"
#include "frontends/cxx/parser/parser.hpp"
#include "frontends/cxx/semantic/analysis.hpp"

namespace {

enum class JsonBackend { Nlohmann, Simdjson };

struct GenerateOptions {
    std::vector<std::string> inputs;
    std::string output;
    JsonBackend backend = JsonBackend::Nlohmann;
};

bool parse_json_backend(std::string_view value, JsonBackend& backend) {
    if (value == "nlohmann") {
        backend = JsonBackend::Nlohmann;
        return true;
    }
    if (value == "simdjson") {
        backend = JsonBackend::Simdjson;
        return true;
    }

    std::cerr << "cjm: unknown JSON backend: " << value << "\n"
              << "cjm: supported JSON backends: nlohmann, simdjson\n";
    return false;
}

constexpr int kExitSuccess = 0;
constexpr int kExitFailure = 1;
constexpr int kExitUsageError = 2;

void print_help(std::ostream& out) {
    out << "cjm - C++ JSON metadata code generator\n"
        << "\n"
        << "Usage:\n"
        << "    cjm --help\n"
        << "    cjm help\n"
        << "    cjm generate [--backend <nlohmann|simdjson] --input <header> "
           "[<header>...] --output <file>\n"
        << "    cjm generate-schema --input <header> [<header>...] --output "
           "<file>\n";
}

// Return true when an argument starts a CLI option such as --input or --output.
bool is_option_arg(const std::string& arg) { return arg.rfind("--", 0) == 0; }

bool parse_generate_options(int argc, char** argv, bool allow_backend_selection,
                            GenerateOptions& options) {
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "--input") {
            if (i + 1 >= argc) {
                std::cerr << "cjm: --input requires a value\n";
                return false;
            }

            // Consume all explicit header paths until the next CLI option.
            ++i;
            if (is_option_arg(argv[i])) {
                std::cerr << "cjm: --input requires at least one value\n";
                return false;
            }
            while (i < argc && !is_option_arg(argv[i])) {
                options.inputs.push_back(argv[i]);
                ++i;
            }
            --i;
            continue;
        }
        if (arg == "--output") {
            if (i + 1 >= argc) {
                std::cerr << "cjm: --output requires a value\n";
                return false;
            }
            options.output = argv[++i];
            continue;
        }
        if (arg == "--backend") {
            if (!allow_backend_selection) {
                std::cerr << "cjm: --backend is only valid for generate\n";
                return false;
            }
            if (i + 1 >= argc) {
                std::cerr << "cjm: --backend requires a value\n";
                return false;
            }
            if (!parse_json_backend(std::string_view(argv[++i]),
                                    options.backend)) {
                return false;
            }
            continue;
        }
        std::cerr << "cjm: unknown generate option: " << arg << "\n";
        return false;
    }

    if (options.inputs.empty()) {
        std::cerr << "cjm: generate requires --input <header>\n";
        return false;
    }
    if (options.output.empty()) {
        std::cerr << "cjm: generate requires --output <file>\n";
        return false;
    }
    return true;
}

bool generate_runtime_header(const GenerateOptions& options,
                             const cjm::metadata::ProjectModel& project,
                             std::string& generated) {
    switch (options.backend) {
    case JsonBackend::Nlohmann:
        generated = cjm::generator::generate_header(project);
        return true;
    case JsonBackend::Simdjson:
        auto result = cjm::generator::simdjson::generate_header(project);
        if (!result.success) {
            std::cerr << "cjm: simdjson backend: " << result.error << "\n";
            return false;
        }
        generated = result.header;
        return true;
    }
    return false;
}

std::string join_inputs(const std::vector<std::string>& inputs) {
    std::string result;
    for (std::size_t i = 0; i < inputs.size(); ++i) {
        if (i > 0) {
            result += ", ";
        }
        result += inputs[i];
    }
    return result;
}

// Write generated text to an output file.
bool write_output_file(const std::string& path, const std::string& contents) {
    std::ofstream output(path);
    if (!output.is_open()) {
        std::cerr << "cjm: failed to open output file: " << path << "\n";
        return false;
    }
    output << contents;
    return true;
}

// Parse inputs and build the validated Metadata IR project.
bool analyze_inputs(const std::vector<std::string>& inputs,
                    cjm::metadata::ProjectModel& project) {
    std::vector<cjm::parser::SourceFileSyntax> files;
    for (const auto& input : inputs) {
        const auto parse_result = cjm::parser::parse_source_file(input);
        if (!parse_result.success) {
            std::cerr << "cjm: " << parse_result.error.message << ": "
                      << parse_result.error.location.file << "\n";
            return false;
        }
        files.push_back(parse_result.file);
    }
    const auto analysis_result = cjm::semantic::analyze_source_files(files);

    if (!analysis_result.success) {
        for (const auto& diagnostic : analysis_result.diagnostics) {
            std::cerr << diagnostic.location.file << ":"
                      << diagnostic.location.line << ":"
                      << diagnostic.location.column << ": "
                      << diagnostic.message << "\n";
        }
        return false;
    }
    project = analysis_result.project;
    return true;
}

// Run the nlohmann header generation command.
int run_generate_command(const GenerateOptions& options) {
    cjm::metadata::ProjectModel project;
    if (!analyze_inputs(options.inputs, project)) {
        return kExitFailure;
    }

    std::string generated;
    if (!generate_runtime_header(options, project, generated)) {
        return kExitFailure;
    }

    if (!write_output_file(options.output, generated)) {
        return kExitFailure;
    }
    std::cout << "cjm: generated " << options.output << " from "
              << join_inputs(options.inputs) << "\n";
    return kExitSuccess;
}

// Run the JSON Schema generation command.
int run_generate_schema_command(const GenerateOptions& options) {
    cjm::metadata::ProjectModel project;
    if (!analyze_inputs(options.inputs, project)) {
        return kExitFailure;
    }

    const auto generated = cjm::generator::schema::generate_schema(project);
    if (!write_output_file(options.output, generated)) {
        return kExitFailure;
    }

    std::cout << "cjm: generated " << options.output << " from "
              << join_inputs(options.inputs) << "\n";
    return kExitSuccess;
}

} // namespace

int main(int argc, char** argv) {
    if (argc <= 1) {
        print_help(std::cout);
        return kExitSuccess;
    }

    const std::string command = argv[1];

    if (command == "--help" || command == "-h" || command == "help") {
        print_help(std::cout);
        return kExitSuccess;
    }

    if (command == "generate") {
        GenerateOptions options;
        if (!parse_generate_options(argc, argv, true, options)) {
            std::cerr << "Run 'cjm --help' for usage.\n";
            return kExitUsageError;
        }
        return run_generate_command(options);
    }
    if (command == "generate-schema") {
        GenerateOptions options;
        if (!parse_generate_options(argc, argv, false, options)) {
            std::cerr << "Run 'cjm --help' for usage.\n";
            return kExitUsageError;
        }
        return run_generate_schema_command(options);
    }

    std::cerr << "cjm: unknown command: " << command << "\n";
    return kExitUsageError;
}
