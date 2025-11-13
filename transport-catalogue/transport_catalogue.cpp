#include "transport_catalogue.h"
#include "geo.h"

#include <unordered_set>
#include <limits>

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
    if (FindStop(name)) return;
    stops_.push_back(domain::Stop{ std::move(name), transport::geo::Coordinates{lat, lon} });
    const domain::Stop& stop = stops_.back();
    stops_index_[stop.name] = &stop;
}

void TransportCatalogue::AddStop(string name, transport::geo::Coordinates coords) {
    if (FindStop(name)) return;
    stops_.push_back(domain::Stop{ std::move(name), coords });
    const domain::Stop& stop = stops_.back();
    stops_index_[stop.name] = &stop;
}

void TransportCatalogue::AddBus(string name, const vector<string>& stop_names, bool is_circle) {
    buses_.emplace_back();
    domain::Bus& bus = buses_.back();
    bus.name = std::move(name);
    bus.is_circle = is_circle;

    for (const auto& stop_name : stop_names) {
        const domain::Stop* stop = FindStop(stop_name);
        if (stop) {
            bus.stops.push_back(stop);
            stop_to_bus_[string(stop->name)].insert(bus.name);
        }
    }

    bus_index_[bus.name] = &bus;
}

const domain::Stop* TransportCatalogue::FindStop(string_view name) const {
    auto it = stops_index_.find(name);
    return it != stops_index_.end() ? it->second : nullptr;
}

const domain::Bus* TransportCatalogue::FindBus(string_view name) const {
    auto it = bus_index_.find(name);
    return it != bus_index_.end() ? it->second : nullptr;
}

optional<BusInfo> TransportCatalogue::GetBusInfo(string_view bus_name) const {
    const domain::Bus* bus = FindBus(bus_name);
    if (!bus || bus->stops.empty()) {
        return std::nullopt;
    }

    BusInfo info;
    const auto& stops = bus->stops;

    unordered_set<const domain::Stop*> unique_stops(stops.begin(), stops.end());
    info.unique_stops = unique_stops.size();

    int road_length = 0;
    double geo_length = 0.0;

    if (bus->is_circle) {
        // кольцо
        info.total_stops = stops.size();
        
        for (size_t i = 1; i < stops.size(); ++i) {
            road_length += GetDistance(stops[i-1], stops[i]);
            geo_length += transport::geo::ComputeDistance(stops[i-1]->coordinates, stops[i]->coordinates);
        }
    } else {
        // не кольцо
        info.total_stops = 2 * stops.size() - 1;
        
        // туда
        for (size_t i = 1; i < stops.size(); ++i) {
            road_length += GetDistance(stops[i-1], stops[i]);
            geo_length += transport::geo::ComputeDistance(stops[i-1]->coordinates, stops[i]->coordinates);
        }
        // обратно
        for (size_t i = stops.size() - 1; i > 0; --i) {
            road_length += GetDistance(stops[i], stops[i-1]);
            geo_length += transport::geo::ComputeDistance(stops[i]->coordinates, stops[i-1]->coordinates);
        }
    }

    info.length = road_length;
    info.curvature = (geo_length == 0.0) ? 1.0 : static_cast<double>(road_length) / geo_length;
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

void TransportCatalogue::SetDistance(const domain::Stop* from, const domain::Stop* to, int distance) {
    distances_[{from, to}] = distance;
}

int TransportCatalogue::GetDistance(const domain::Stop* from, const domain::Stop* to) const {

    auto it = distances_.find({ from, to });
    if (it != distances_.end()) {
        return it->second;
    }

    it = distances_.find({to, from});
    if (it != distances_.end()) {
        return it->second;
    }

    return transport::geo::ComputeDistance(from->coordinates, to->coordinates);
}

std::vector<const domain::Bus*> TransportCatalogue::GetAllBuses() const {
    std::vector<const domain::Bus*> result;
    for (const auto& bus : buses_) {
        result.push_back(&bus);
    }
    return result;
}

std::vector<const domain::Stop*> TransportCatalogue::GetAllStops() const {
    std::vector<const domain::Stop*> result;
    for (const auto& stop : stops_) {
        result.push_back(&stop);
    }
    return result;
}

std::vector<const domain::Stop*> TransportCatalogue::GetStopsUsedInRoutes() const {
    std::vector<const domain::Stop*> result;
    for (const auto& stop : stops_) {
        auto it = stop_to_bus_.find(stop.name);
        if (it != stop_to_bus_.end() && !it->second.empty()) {
            result.push_back(&stop);
        }
    }
    return result;
}

} // namespace transport::catalogue
