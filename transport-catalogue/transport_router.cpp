#include "transport_router.h"
#include <algorithm>
#include <stdexcept>

namespace transport::routing {

namespace {
    constexpr size_t kVerticesPerStop = 2;
    
    inline graph::VertexId InVertexId(size_t stop_index) {
        return stop_index * kVerticesPerStop;
    }

    inline graph::VertexId OutVertexId(size_t stop_index) {
        return stop_index * kVerticesPerStop + 1;
    }
}

TransportRouter::TransportRouter(
    const catalogue::TransportCatalogue& catalogue,
    const RoutingSettings& settings)
    : catalogue_(catalogue)
    , settings_(settings)
{
    stop_id_to_stop_ = catalogue_.GetStopsUsedInRoutes();
    const size_t stop_count = stop_id_to_stop_.size();
    
    for (size_t i = 0; i < stop_count; ++i) {
        stop_name_to_id_[stop_id_to_stop_[i]->name] = i;
    }

    graph_ = graph::DirectedWeightedGraph<double>(stop_count * kVerticesPerStop);
    BuildGraph();
    router_ = std::make_unique<graph::Router<double>>(graph_);
}

double TransportRouter::ComputeTravelTime(int distance_meters) const {
    return distance_meters / ((settings_.bus_velocity * 1000.0) / 60.0);
}

void TransportRouter::BuildGraph() {
    const size_t stop_count = stop_id_to_stop_.size();
    const double wait_time = static_cast<double>(settings_.bus_wait_time);

    // Ожидание для отсановок
    for (size_t i = 0; i < stop_count; ++i) {
        graph::Edge<double> wait_edge{InVertexId(i), OutVertexId(i), wait_time};
        graph_.AddEdge(wait_edge);
        edge_info_.push_back({
            "", 0,
            std::string(stop_id_to_stop_[i]->name),
            std::string(stop_id_to_stop_[i]->name)
        });
    }

    for (const auto* bus : catalogue_.GetAllBuses()) {
        if (!bus || bus->stops.empty()) continue;
        const auto& stops = bus->stops;
        const size_t n = stops.size();

        for (size_t i = 0; i < n; ++i) {
            auto from_it = stop_name_to_id_.find(stops[i]->name);
            if (from_it == stop_name_to_id_.end()) continue;
            size_t from_idx = from_it->second;

            int accumulated_distance = 0;
            for (size_t j = i + 1; j < n; ++j) {
                accumulated_distance += catalogue_.GetDistance(stops[j-1], stops[j]);

                auto to_it = stop_name_to_id_.find(stops[j]->name);
                if (to_it == stop_name_to_id_.end()) continue;
                size_t to_idx = to_it->second;

                double weight = ComputeTravelTime(accumulated_distance);
                graph::Edge<double> bus_edge{OutVertexId(from_idx), InVertexId(to_idx), weight};
                graph_.AddEdge(bus_edge);
                edge_info_.push_back({
                    std::string(bus->name),
                    j - i,
                    std::string(stops[i]->name),
                    std::string(stops[j]->name)
                });

                if (!bus->is_circle) {
                    int rev_distance = 0;
                    for (size_t k = j; k > i; --k) {
                        rev_distance += catalogue_.GetDistance(stops[k], stops[k-1]);
                    }
                    double rev_weight = ComputeTravelTime(rev_distance);
                    graph::Edge<double> rev_bus_edge{OutVertexId(to_idx), InVertexId(from_idx), rev_weight};
                    graph_.AddEdge(rev_bus_edge);
                    edge_info_.push_back({
                        std::string(bus->name),
                        j - i,
                        std::string(stops[j]->name),
                        std::string(stops[i]->name)
                    });
                }
            }
        }
    }
}

std::vector<RoutingItem> TransportRouter::ReconstructRoute(const std::vector<graph::EdgeId>& edge_path) const
{
    std::vector<RoutingItem> items;

    for (graph::EdgeId edge_id : edge_path) {
        const auto& einfo = edge_info_.at(edge_id);
        const auto& edge = graph_.GetEdge(edge_id);

        if (einfo.bus_name.empty()) {
            items.push_back({
                RoutingItem::Type::WAIT,
                einfo.from_stop_name, "", 0, edge.weight
            });
        } else {
            items.push_back({
                RoutingItem::Type::BUS,
                "", einfo.bus_name, einfo.span_count, edge.weight
            });
        }
    }
    return items;
}

std::optional<RouteInfo> TransportRouter::BuildRoute(std::string_view from, std::string_view to) const
{
    if (from == to) {
        return RouteInfo{0.0, {}};
    }

    auto from_it = stop_name_to_id_.find(from);
    auto to_it = stop_name_to_id_.find(to);
    if (from_it == stop_name_to_id_.end() || to_it == stop_name_to_id_.end()) {
        return std::nullopt;
    }

    graph::VertexId from_in = InVertexId(from_it->second);
    graph::VertexId to_in = InVertexId(to_it->second);

    auto route = router_->BuildRoute(from_in, to_in);
    if (!route) {
        return std::nullopt;
    }

    auto items = ReconstructRoute(route->edges);
    return RouteInfo{route->weight, std::move(items)};
}

} // namespace transport::routing