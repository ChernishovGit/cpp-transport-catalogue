#include "request_handler.h"

namespace transport::request_handler {

RequestHandler::RequestHandler(const transport::catalogue::TransportCatalogue& db)
    : db_(db) {}

std::optional<transport::catalogue::BusInfo> RequestHandler::GetBusStat(std::string_view bus_name) const {
    return db_.GetBusInfo(bus_name);
}

std::optional<const std::set<std::string>*> RequestHandler::GetBusesByStop(std::string_view stop_name) const {
    return db_.GetBusesByStop(stop_name);
}

} // namespace transport::request_handler