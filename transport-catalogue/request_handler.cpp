#include "request_handler.h"

namespace transport::request_handler {

std::optional<transport::catalogue::BusInfo> GetBusStat(std::string_view bus_name, const transport::catalogue::TransportCatalogue& catalogue) {
    return catalogue.GetBusInfo(bus_name);
}

std::optional<const std::set<std::string>*> GetBusesByStop(std::string_view stop_name, const transport::catalogue::TransportCatalogue& catalogue) {
    return catalogue.GetBusesByStop(stop_name);
}

} // namespace transport::request_handler