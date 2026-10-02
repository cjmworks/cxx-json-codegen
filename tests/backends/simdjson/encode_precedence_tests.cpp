#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <new>

namespace {
thread_local bool fail_builder_allocation = false;
thread_local std::size_t injected_failures = 0;

// Enable injection only while calling the generated encoder or its helper.
struct BuilderAllocationScope {
    explicit BuilderAllocationScope(bool fail) {
        injected_failures = 0;
        fail_builder_allocation = fail;
    }
    ~BuilderAllocationScope() { fail_builder_allocation = false; }
    BuilderAllocationScope(const BuilderAllocationScope&) = delete;
    BuilderAllocationScope& operator=(const BuilderAllocationScope&) = delete;
};
} // namespace

// The pinned builder uses nothrow new[] for its buffer. This replacement is
// isolated to this executable; ordinary allocation/deallocation stays paired.
void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    if (fail_builder_allocation) {
        ++injected_failures;
        return nullptr;
    }
    try {
        return ::operator new[](size);
    } catch (...) {
        return nullptr;
    }
}

#include "tests/fixtures/simdjson_encode_precedence.hpp"
#include "cjm/simdjson/simdjson_encode_precedence.cjm.hpp"

namespace {
using ErrorCode = cjm::simdjson::EncodeErrorCode;

const struct {
    const char* name;
    bool valid_model;
    bool fail_builder;
    ErrorCode expected_code;
    ::simdjson::error_code expected_runtime;
} cases[] = {
    {"both_valid", true, false, ErrorCode::none, ::simdjson::SUCCESS},
    {"data_error", false, false, ErrorCode::invalid_utf8_string,
     ::simdjson::UTF8_ERROR},
    {"builder_error", true, true, ErrorCode::output_failure,
     ::simdjson::OUT_OF_CAPACITY},
    {"both_invalid", false, true, ErrorCode::output_failure,
     ::simdjson::OUT_OF_CAPACITY},
};

app::EncodePrecedence make_value(bool valid) {
    return {valid ? std::string{"Ada"} : std::string{"\xC3\x28", 2}};
}
} // namespace

TEST_CASE("root.helper_states", "[simdjson][encoder][precedence]") {
    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            const auto value = make_value(item.valid_model);
            cjm::simdjson::EncodeError error;
            bool model_valid = false;
            ::simdjson::error_code builder_error;
            {
                BuilderAllocationScope injection(item.fail_builder);
                ::simdjson::builder::string_builder builder;
                model_valid =
                    cjm::simdjson::detail::encode_object(builder, value, error);
                std::string_view view;
                builder_error = builder.view().get(view);
            }

            REQUIRE((injected_failures > 0) == item.fail_builder);
            REQUIRE(model_valid == item.valid_model);
            REQUIRE(builder_error == (item.fail_builder
                                          ? ::simdjson::OUT_OF_CAPACITY
                                          : ::simdjson::SUCCESS));
            // The helper reports data validity, not the builder's state.
            REQUIRE(error.code == (item.valid_model
                                       ? ErrorCode::none
                                       : ErrorCode::invalid_utf8_string));
            if (!item.valid_model) {
                REQUIRE(error.path.size() == 1);
                REQUIRE(error.path[0].field_name == "name");
                REQUIRE(error.runtime_error == ::simdjson::UTF8_ERROR);
            }
        }
    }
}

TEST_CASE("root.precedence", "[simdjson][encoder][precedence]") {
    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            const auto value = make_value(item.valid_model);
            const auto original = value.name;
            cjm::simdjson::EncodeError error;
            std::optional<std::string> output;
            {
                BuilderAllocationScope injection(item.fail_builder);
                output = cjm::simdjson::to_json(value, error);
            }

            REQUIRE((injected_failures > 0) == item.fail_builder);
            REQUIRE(output.has_value() ==
                    (item.valid_model && !item.fail_builder));
            REQUIRE(error.code == item.expected_code);
            REQUIRE(error.runtime_error == item.expected_runtime);
            REQUIRE(value.name == original);
            if (!item.valid_model && !item.fail_builder) {
                REQUIRE(error.path.size() == 1);
                REQUIRE(error.path[0].kind ==
                        cjm::simdjson::EncodePathSegmentKind::field);
                REQUIRE(error.path[0].field_name == "name");
                REQUIRE(error.path[0].index == 0);
            } else {
                REQUIRE(error.path.empty());
            }
            if (output) {
                REQUIRE(*output == R"({"name":"Ada"})");
            }

            // Reuse the same error object after every failed outcome.
            const auto recovered =
                cjm::simdjson::to_json(make_value(true), error);
            REQUIRE(recovered.has_value());
            REQUIRE(*recovered == R"({"name":"Ada"})");
            REQUIRE(error.code == ErrorCode::none);
            REQUIRE(error.path.empty());
            REQUIRE(error.runtime_error == ::simdjson::SUCCESS);
        }
    }
}
