#pragma once

#include "geo.h"

#include <string>
#include <vector>

namespace transport::domain {

struct Stop {
    std::string name;
    transport::geo::Coordinates coordinates; 
};

struct Bus {
    std::string name;
    std::vector<const Stop*> stops;
    bool is_circle = false;
};

} // namespace transport::domain