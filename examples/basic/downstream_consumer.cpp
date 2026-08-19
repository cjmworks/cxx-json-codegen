#include "user.hpp"
#include "cjm/nlohmann/user.cjm.hpp"

#include <nlohmann/json.hpp>

int main() {
    User user;
    user.name = "Downstream";
    user.age = 7;

    nlohmann::json json = user;
    const auto round_trip = json.get<User>();

    return round_trip.name == "Downstream" && round_trip.age == 7 ? 0 : 1;
}
