#pragma once

#include "transport_catalogue.h"
#include "graph.h"
#include "router.h"

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <unordered_map>
#include <memory>

namespace transport::routing {

struct RoutingSettings {
    int bus_wait_time = 0;        // в минутах
    double bus_velocity = 0.0;    // км/ч
};

struct RoutingItem {
    enum class Type { WAIT, BUS } type;
    std::string stop_name;        // для WAIT
    std::string bus_name;         // для BUS
    size_t span_count = 0;        // для BUS
    double time = 0.0;            // в минутах
};

struct RouteInfo {
    double total_time = 0.0;
    std::vector<RoutingItem> items;
};

class TransportRouter {
public:
    TransportRouter(const catalogue::TransportCatalogue& catalogue, const RoutingSettings& settings);
    std::optional<RouteInfo> BuildRoute(std::string_view from, std::string_view to) const;
    double ComputeTravelTime(int distance_meters) const;
    
private:
    struct EdgeInfo {
        std::string bus_name;
        size_t span_count;
        std::string from_stop_name;
        std::string to_stop_name;
    };

    const catalogue::TransportCatalogue& catalogue_;
    RoutingSettings settings_;
    std::vector<const domain::Stop*> stop_id_to_stop_;
    std::unordered_map<std::string_view, graph::VertexId> stop_name_to_id_;

    graph::DirectedWeightedGraph<double> graph_;
    std::unique_ptr<graph::Router<double>> router_;
    std::vector<EdgeInfo> edge_info_;

    void BuildGraph();
    std::vector<RoutingItem> ReconstructRoute(const std::vector<graph::EdgeId>& edge_path) const;
};

} // namespace transport::routing