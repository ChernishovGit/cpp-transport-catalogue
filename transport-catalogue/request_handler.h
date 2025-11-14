#pragma once

#include "transport_catalogue.h"

#include <optional>
#include <string_view>
#include <set>

namespace transport::request_handler {

std::optional<transport::catalogue::BusInfo> GetBusStat(std::string_view bus_name, const transport::catalogue::TransportCatalogue& catalogue);
std::optional<const std::set<std::string>*> GetBusesByStop(std::string_view stop_name, const transport::catalogue::TransportCatalogue& catalogue);

} // namespace transport::request_handler
