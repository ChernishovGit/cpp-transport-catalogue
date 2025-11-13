#pragma once

#include "domain.h"

#include <deque>
#include <vector>
#include <unordered_map>
#include <string_view>
#include <string>
#include <optional>
#include <set>
#include <functional>
#include <algorithm>

namespace transport::catalogue {

struct BusInfo {
    size_t total_stops = 0;
    size_t unique_stops = 0;
    int length = 0; 
    double curvature = 0.0;
};

struct StopPairHash {
    size_t operator()(const std::pair<const domain::Stop*, const domain::Stop*>& p) const {
        auto h1 = std::hash<const void*>{}(static_cast<const void*>(p.first));
        auto h2 = std::hash<const void*>{}(static_cast<const void*>(p.second));
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};

class TransportCatalogue {
public:
    void AddStop(std::string name, double lat, double lon);
    void AddStop(std::string name, transport::geo::Coordinates coords);
    void AddBus(std::string name, const std::vector<std::string>& stop_names, bool is_circle);

    const domain::Stop* FindStop(std::string_view name) const;
    const domain::Bus* FindBus(std::string_view name) const;

    std::optional<BusInfo> GetBusInfo(std::string_view bus_name) const;
    std::optional<const std::set<std::string>*> GetBusesByStop(std::string_view stop_name) const;

    void SetDistance(const domain::Stop* from, const domain::Stop* to, int distance);
    int GetDistance(const domain::Stop* from, const domain::Stop* to) const;

    std::vector<const domain::Bus*> GetAllBuses() const;
    std::vector<const domain::Stop*> GetAllStops() const;
    std::vector<const domain::Stop*> GetStopsUsedInRoutes() const;

private:
    std::deque<domain::Stop> stops_{};
    std::unordered_map<std::string_view, const domain::Stop*> stops_index_{};

    std::deque<domain::Bus> buses_{};
    std::unordered_map<std::string_view, const domain::Bus*> bus_index_{};

    std::unordered_map<std::string, std::set<std::string>> stop_to_bus_;

    std::unordered_map<std::pair<const domain::Stop*, const domain::Stop*>, int, StopPairHash> distances_;
};

} // namespace transport::catalogue