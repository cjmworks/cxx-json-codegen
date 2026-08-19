#include "user.hpp"
#include "cjm/simdjson/user.cjm.hpp"

#include <cassert>

int main() {
    cjm::simdjson::DecodeError error;
    const auto result =
        cjm::simdjson::from_json<User>(R"({"name": "Ada", "age":42})", error);

    assert(result.has_value());
    assert(result->name == "Ada");
    assert(result->age == 42);
    assert(error.code == cjm::simdjson::DecodeErrorCode::none);
    assert(error.path.empty());
    assert(error.runtime_error == simdjson::SUCCESS);

    return 0;
}
