#include "user.hpp"
#include "cjm/nlohmann/user.cjm.hpp"

#include <nlohmann/json.hpp>

int main() {
    User user;
    user.name = "Ada";
    user.age = 42;

    nlohmann::json json = user;

    User round_trip;
    json.get_to(round_trip);

    return round_trip.name == "Ada" && round_trip.age == 42 ? 0 : 1;
}
