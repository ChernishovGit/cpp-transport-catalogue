#include "map_renderer.h"
#include "transport_catalogue.h"

#include <algorithm>
#include <vector>
#include <string>
#include <set>

namespace transport::renderer {

using namespace std::literals;

MapRenderer::MapRenderer(const RenderSettings& settings, const transport::catalogue::TransportCatalogue& catalogue)
    : settings_(settings), catalogue_(catalogue) {
}

svg::Document MapRenderer::Render() const {
    svg::Document doc;
    
    auto stops_coords = GetAllStopCoordinates();
    SphereProjector projector(stops_coords.begin(), stops_coords.end(),
                             settings_.width, settings_.height, settings_.padding);
    
    RenderBusLines(doc, projector);
    RenderBusLabels(doc, projector);
    RenderStopPoints(doc, projector);
    RenderStopLabels(doc, projector);
    
    return doc;
}

void MapRenderer::RenderBusLines(svg::Document& doc, const SphereProjector& projector) const {
    auto buses = GetSortedBuses();
    
    for (size_t i = 0; i < buses.size(); ++i) {
        const auto* bus = buses[i];
        if (bus->stops.empty()) continue;
        
        svg::Polyline line;
        line.SetStrokeColor(settings_.color_palette[i % settings_.color_palette.size()])
            .SetStrokeWidth(settings_.line_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
            .SetFillColor("none"s);
        
        for (const auto* stop : bus->stops) {
            line.AddPoint(projector(stop->coordinates));
        }
        
        if (!bus->is_circle) {
            for (auto it = bus->stops.rbegin() + 1; it != bus->stops.rend(); ++it) {
                line.AddPoint(projector((*it)->coordinates));
            }
        }
        
        doc.Add(std::move(line));
    }
}

void MapRenderer::RenderBusLabels(svg::Document& doc, const SphereProjector& projector) const {
    auto buses = GetSortedBuses();
    
    for (size_t i = 0; i < buses.size(); ++i) {
        const auto* bus = buses[i];
        if (bus->stops.empty()) continue;
        
        auto add_bus_label = [&](const domain::Stop* stop, const std::string& bus_name) {
            // Подложка
            svg::Text underlayer;
            underlayer.SetPosition(projector(stop->coordinates))
                     .SetOffset(settings_.bus_label_offset)
                     .SetFontSize(settings_.bus_label_font_size)
                     .SetFontFamily("Verdana"s)
                     .SetFontWeight("bold"s)
                     .SetData(bus_name)
                     .SetFillColor(settings_.underlayer_color)
                     .SetStrokeColor(settings_.underlayer_color)
                     .SetStrokeWidth(settings_.underlayer_width)
                     .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                     .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
            
            // Основной текст
            svg::Text text;
            text.SetPosition(projector(stop->coordinates))
                .SetOffset(settings_.bus_label_offset)
                .SetFontSize(settings_.bus_label_font_size)
                .SetFontFamily("Verdana"s)
                .SetFontWeight("bold"s)
                .SetData(bus_name)
                .SetFillColor(settings_.color_palette[i % settings_.color_palette.size()]);
            
            doc.Add(std::move(underlayer));
            doc.Add(std::move(text));
        };
        
        add_bus_label(bus->stops.front(), bus->name);
        
        if (!bus->is_circle && bus->stops.front() != bus->stops.back()) {
            add_bus_label(bus->stops.back(), bus->name);
        }
    }
}

void MapRenderer::RenderStopPoints(svg::Document& doc, const SphereProjector& projector) const {
    auto used_stops = catalogue_.GetStopsUsedInRoutes();
    
    std::sort(used_stops.begin(), used_stops.end(),
              [](const domain::Stop* lhs, const domain::Stop* rhs) {
                  return lhs->name < rhs->name;
              });
    
    for (const auto* stop : used_stops) {
        svg::Circle circle;
        circle.SetCenter(projector(stop->coordinates))
              .SetRadius(settings_.stop_radius)
              .SetFillColor("white"s);
        
        doc.Add(std::move(circle));
    }
}

void MapRenderer::RenderStopLabels(svg::Document& doc, const SphereProjector& projector) const {
    auto used_stops = catalogue_.GetStopsUsedInRoutes();
    
    std::sort(used_stops.begin(), used_stops.end(),
              [](const domain::Stop* lhs, const domain::Stop* rhs) {
                  return lhs->name < rhs->name;
              });
    
    for (const auto* stop : used_stops) {
        // Подложка
        svg::Text underlayer;
        underlayer.SetPosition(projector(stop->coordinates))
                 .SetOffset(settings_.stop_label_offset)
                 .SetFontSize(settings_.stop_label_font_size)
                 .SetFontFamily("Verdana"s)
                 .SetData(stop->name)
                 .SetFillColor(settings_.underlayer_color)
                 .SetStrokeColor(settings_.underlayer_color)
                 .SetStrokeWidth(settings_.underlayer_width)
                 .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                 .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        
        // Основной текст
        svg::Text text;
        text.SetPosition(projector(stop->coordinates))
            .SetOffset(settings_.stop_label_offset)
            .SetFontSize(settings_.stop_label_font_size)
            .SetFontFamily("Verdana"s)
            .SetData(stop->name)
            .SetFillColor("black"s);
        
        doc.Add(std::move(underlayer));
        doc.Add(std::move(text));
    }
}

std::vector<const domain::Bus*> MapRenderer::GetSortedBuses() const {
    auto all_buses = catalogue_.GetAllBuses();
    
    std::vector<const domain::Bus*> buses_with_stops;
    for (const auto* bus : all_buses) {
        if (!bus->stops.empty()) {
            buses_with_stops.push_back(bus);
        }
    }
    
    std::sort(buses_with_stops.begin(), buses_with_stops.end(),
              [](const domain::Bus* lhs, const domain::Bus* rhs) {
                  return lhs->name < rhs->name;
              });
    
    return buses_with_stops;
}

std::vector<geo::Coordinates> MapRenderer::GetAllStopCoordinates() const {
    auto used_stops = catalogue_.GetStopsUsedInRoutes();
    std::vector<geo::Coordinates> coords;
    
    for (const auto* stop : used_stops) {
        coords.push_back(stop->coordinates);
    }
    
    return coords;
}

} // namespace transport::renderer