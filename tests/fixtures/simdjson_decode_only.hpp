#pragma once

#include <vector>

namespace app {
struct Supported {
    int count;
    std::vector<int> ignored; // json:"-"
};

struct DecodeOnly {
    std::vector<int> scores; // json:"points"
    std::vector<int> backup;
};

struct Parent {
    DecodeOnly child; // json:"payload"
};
} // namespace app
