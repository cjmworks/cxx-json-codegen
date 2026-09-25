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

struct Profile {
    int id;
    User user; // json:"owner"
    bool enabled;
};

struct OmittedAddress {
    std::optional<std::string> city; // json:"city,omitempty"
};

struct UserWithOmittedAddress {
    OmittedAddress address; // json:"home"
};

struct OptionalUser {
    std::optional<Address> address; // json:"home"
};

} // namespace app
