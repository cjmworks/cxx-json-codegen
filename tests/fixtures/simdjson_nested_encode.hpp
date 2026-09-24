#pragma once

#include <optional>
#include <string>

namespace app {
struct Address {
    std::string city;
};

struct User {
    Address address; // json:"home"
};

struct OmittedAddress {
    std::optional<std::string> city; // json:"city,omitempty"
};

struct UserWithOmittedAddress {
    OmittedAddress address; // json:"home"
};

} // namespace app
