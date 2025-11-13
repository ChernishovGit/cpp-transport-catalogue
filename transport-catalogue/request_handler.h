#pragma once

#include "transport_catalogue.h"

#include <optional>
#include <string_view>
#include <set>

namespace transport::request_handler {

class RequestHandler {
public:
    RequestHandler(const transport::catalogue::TransportCatalogue& db);
    std::optional<transport::catalogue::BusInfo> GetBusStat(std::string_view bus_name) const;
    std::optional<const std::set<std::string>*> GetBusesByStop(std::string_view stop_name) const;

private:
    const transport::catalogue::TransportCatalogue& db_;
};

} // namespace transport::request_handler
