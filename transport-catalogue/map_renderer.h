#pragma once

#include "svg.h"
#include "geo.h"
#include "domain.h"
#include "transport_catalogue.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cstdlib>
#include <optional>

namespace transport::renderer {

struct RenderSettings {
    double width = 0.0;
    double height = 0.0;
    double padding = 0.0;
    double line_width = 0.0;
    double stop_radius = 0.0;
    int bus_label_font_size = 0;
    svg::Point bus_label_offset;
    int stop_label_font_size = 0;
    svg::Point stop_label_offset;
    svg::Color underlayer_color;
    double underlayer_width = 0.0;
    std::vector<svg::Color> color_palette;
};

class SphereProjector {
public:
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding)
        : padding_(padding)
    {
        if (points_begin == points_end) {
            return;
        }

        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            zoom_coeff_ = *height_zoom;
        } else {
            zoom_coeff_ = 0;
        }
    }

    svg::Point operator()(geo::Coordinates coords) const {
        return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
        };
    }

private:
    static bool IsZero(double value) {
        return std::abs(value) < 1e-6;
    }

    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class MapRenderer {
public:
    MapRenderer(const RenderSettings& settings, const transport::catalogue::TransportCatalogue& catalogue);
    svg::Document Render() const;

private:
    RenderSettings settings_;
    const transport::catalogue::TransportCatalogue& catalogue_;
    
    void RenderBusLines(svg::Document& doc, const SphereProjector& projector) const;
    void RenderBusLabels(svg::Document& doc, const SphereProjector& projector) const;
    void RenderStopPoints(svg::Document& doc, const SphereProjector& projector) const;
    void RenderStopLabels(svg::Document& doc, const SphereProjector& projector) const;
    
    std::vector<const domain::Bus*> GetSortedBuses() const;
    std::vector<geo::Coordinates> GetAllStopCoordinates() const;
};

} // namespace transport::renderer