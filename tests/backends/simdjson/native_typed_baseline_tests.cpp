#include <catch2/catch_test_macros.hpp>
#include <simdjson.h>

#include <cstdint>
#include <string_view>
#include <optional>
#include <string>

namespace {

struct NativeScalarValues {
    bool enabled = false;
    std::int64_t count = 0;
    std::uint64_t limit = 0;
};

struct NativeBoolValues {
    bool enabled = false;
};

bool encode_bool_object(::simdjson::builder::string_builder& builder,
                        const NativeBoolValues& value) {
    builder.start_object();
    builder.escape_and_append_with_quotes("enabled");
    builder.append_colon();
    builder.append(value.enabled);
    builder.end_object();
    return true;
}

std::optional<std::string> encode_bool_value(const NativeBoolValues& value) {
    ::simdjson::builder::string_builder builder;

    if (!encode_bool_object(builder, value)) {
        return std::nullopt;
    }

    std::string_view view;
    if (builder.view().get(view) != ::simdjson::SUCCESS) {
        return std::nullopt;
    }
    return std::string(view);
}

} // namespace

// Decode one native scalar model from the current On-Demand document.
template <>
simdjson_inline simdjson::simdjson_result<NativeScalarValues>
simdjson::ondemand::document::get<NativeScalarValues>() & noexcept {
    simdjson::ondemand::object object;
    auto error = get_object().get(object);
    if (error) {
        return error;
    }

    NativeScalarValues values;
    if ((error = object["enabled"].get_bool().get(values.enabled))) {
        return error;
    }
    if ((error = object["count"].get_int64().get(values.count))) {
        return error;
    }
    if ((error = object["limit"].get_uint64().get(values.limit))) {
        return error;
    }
    return values;
}

TEST_CASE("document_get.required_scalars", "[simdjson][baseline]") {
    const simdjson::padded_string input(
        std::string_view{R"({"limit":199,"enabled":true,"count":-15})"});
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document document;

    REQUIRE(parser.iterate(input).get(document) == simdjson::SUCCESS);

    NativeScalarValues values;
    REQUIRE(document.get<NativeScalarValues>().get(values) ==
            simdjson::SUCCESS);
    REQUIRE(values.enabled);
    REQUIRE(values.count == -15);
    REQUIRE(values.limit == 199);
}

TEST_CASE("document_get.missing_required_field", "[simdjson][baseline]") {
    const simdjson::padded_string input(
        std::string_view{R"({"enabled":true,"limit":199})"});
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document document;

    REQUIRE(parser.iterate(input).get(document) == simdjson::SUCCESS);

    const auto result = document.get<NativeScalarValues>();
    REQUIRE(result.error() == simdjson::NO_SUCH_FIELD);
}

TEST_CASE("document_get.scalar_type_mismatch", "[simdjson][baseline]") {
    const simdjson::padded_string input(
        std::string_view{R"({"enabled":true,"count":"-15","limit":199})"});
    simdjson::ondemand::parser parser;
    simdjson::ondemand::document document;

    REQUIRE(parser.iterate(input).get(document) == simdjson::SUCCESS);

    const auto result = document.get<NativeScalarValues>();
    REQUIRE(result.error() == simdjson::INCORRECT_TYPE);
}

TEST_CASE("encode_bool_value.returns_owned_json", "[simdjson][baseline]") {
    const auto output = encode_bool_value(NativeBoolValues{true});

    REQUIRE(output.has_value());
    REQUIRE(*output == R"({"enabled":true})");
}

TEST_CASE("double.values", "[simdjson][baseline]") {
    const struct {
        const char* name;
        std::string_view json;
        double expected;
    } cases[] = {
        {"fraction", R"({"value":1.5})", 1.5},
        {"integer", R"({"value":42})", 42.0},
        {"exponent", R"({"value":-2.5e2})", -250.0},
    };

    for (const auto& item : cases) {
        DYNAMIC_SECTION(item.name) {
            const simdjson::padded_string input(item.json);
            simdjson::ondemand::parser parser;
            simdjson::ondemand::document document;
            REQUIRE(parser.iterate(input).get(document) == simdjson::SUCCESS);
            double value = 0;
            REQUIRE(document["value"].get_double().get(value) ==
                    simdjson::SUCCESS);
            REQUIRE(value == item.expected);
        }
    }
}
