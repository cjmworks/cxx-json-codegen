#pragma once

#include <optional>
#include <string>

struct OptionalEncodeValues {
    std::optional<int> count;        // json:"count,omitempty"
    std::optional<std::string> name; // json:"name,omitempty"
    std::optional<bool> enabled;
};

struct OmittedOptionalValues {
    std::optional<std::string> ignored; // json:"-"
    std::optional<int> count;           // json:"count,omitempty"
    std::optional<std::string> name;    // json:"name,omitempty"
};

enum class OptionalStatus {
    Active,
    Disabled,
};

struct OptionalEnumValues {
    std::optional<OptionalStatus> status; // json:"state"
};

struct OptionalFloatValues {
    std::optional<float> ratio; // json:"ratio,omitempty"
    std::optional<double> amount;
};
