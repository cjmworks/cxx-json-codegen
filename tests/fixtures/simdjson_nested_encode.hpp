#pragma once

#include <string>

namespace app {
struct Address {
    std::string city;
};

struct User {
    Address address; // json:"home"
};

} // namespace app
