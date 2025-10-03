#include "transport_catalogue.h"
#include <unordered_set>

namespace transport::catalogue {

using std::deque;
using std::string;
using std::string_view;
using std::unordered_map;
using std::unordered_set;
using std::vector;
using std::optional;
using std::set;

void TransportCatalogue::AddStop(string name, double lat, double lon) {
    stops_.push_back(Stop{ std::move(name), transport::geo::Coordinates{lat, lon} });
    const Stop& stop = stops_.back();
    stops_index_[stop.name] = &stop;
}

void TransportCatalogue::AddStop(string name, transport::geo::Coordinates coords) {
    stops_.push_back(Stop{ std::move(name), coords });
    const Stop& stop = stops_.back();
    stops_index_[stop.name] = &stop;
}

void TransportCatalogue::AddBus(string name, const vector<string>& stop_names, bool is_circle) {
    buses_.emplace_back();
    Bus& bus = buses_.back();
    bus.name = std::move(name);
    bus.is_circle = is_circle;

    for (const auto& stop_name : stop_names) {
        const Stop* stop = FindStop(stop_name);
        if (stop) {
            bus.stops.push_back(stop);
            stop_to_bus_[string(stop->name)].insert(bus.name);
        }
    }

    bus_index_[bus.name] = &bus;
}

const Stop* TransportCatalogue::FindStop(string_view name) const {
    auto it = stops_index_.find(name);
    return it != stops_index_.end() ? it->second : nullptr;
}

const Bus* TransportCatalogue::FindBus(string_view name) const {
    auto it = bus_index_.find(name);
    return it != bus_index_.end() ? it->second : nullptr;
}

optional<BusInfo> TransportCatalogue::GetBusInfo(string_view bus_name) const {
    const Bus* bus = FindBus(bus_name);
    if (!bus || bus->stops.empty()) {
        return std::nullopt;
    }

    BusInfo info;
    info.total_stops = bus->is_circle ? bus->stops.size() : bus->stops.size() * 2 - 1;

    unordered_set<const Stop*> unique_stops(bus->stops.begin(), bus->stops.end());
    info.unique_stops = unique_stops.size();

    double total_distance = 0.0;
    for (size_t i = 1; i < bus->stops.size(); ++i) {
        total_distance += transport::geo::ComputeDistance(
            bus->stops[i - 1]->coordinates,
            bus->stops[i]->coordinates
        );
    }

    if (bus->is_circle) {
        if (bus->stops.size() > 1) {
            total_distance += transport::geo::ComputeDistance(
                bus->stops.back()->coordinates,
                bus->stops.front()->coordinates
            );
        }
    } else {
        total_distance *= 2;
    }

    info.length = total_distance;
    return info;
}

optional<const set<string>*> TransportCatalogue::GetBusesByStop(string_view stop_name) const {
    if (!FindStop(stop_name)) {
        return std::nullopt;
    }

    auto it = stop_to_bus_.find(string(stop_name));
    if (it == stop_to_bus_.end()) {
        static const std::set<std::string> empty_set;
        return &empty_set;
    }

    return &it->second;
}
} // namespace transport::catalogue