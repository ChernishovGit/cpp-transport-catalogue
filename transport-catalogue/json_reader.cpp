#include "json_reader.h"
#include <iostream>
#include <string>
#include <vector>
#include <string_view>
#include <set>
#include <algorithm>
#include <sstream>

namespace transport::json_reader {

using namespace std::literals;

void JSONReader::ProcessRequests(std::istream& in, std::ostream& out) {
    auto document = json::Load(in);
    const auto& root = document.GetRoot();
    const auto& root_map = root.AsMap();

    const auto& base_requests = root_map.at("base_requests"s).AsArray();
    
    // Читаем настройки рендеринга, если они есть
    transport::renderer::RenderSettings render_settings;
    if (root_map.count("render_settings"s)) {
        render_settings = ReadRenderSettings(root_map.at("render_settings"s).AsMap());
    }

    ProcessStops(base_requests);
    ProcessDistances(base_requests);
    ProcessBusses(base_requests);


    // Обрабатываем stat_requests если они есть
    if (root_map.count("stat_requests"s)) {
        const auto& stat_requests = root_map.at("stat_requests"s).AsArray();
        
        json::Array responses;

        for (const auto& request_node : stat_requests) {
            const auto& req_map = request_node.AsMap();
            int id = req_map.at("id"s).AsInt();
            std::string_view req_type = req_map.at("type"s).AsString();

            json::Builder builder;
            builder.StartDict();
            builder.Key("request_id").Value(id);

            if (req_type == "Bus"sv) {
                RequestBus(builder, req_map);
            } else if (req_type == "Stop"sv) {
                RequestStop(builder, req_map);
            } else if (req_type == "Map"sv) {
                ProcessMap(builder, render_settings);
            } else {
                builder.Key("error_message").Value("unknown request type"s);
            }

            builder.EndDict();
            responses.push_back(builder.Build());
        }

        json::Print(json::Document(json::Node(std::move(responses))), out);
    }
}
void JSONReader::ProcessStops(const json::Array& base_requests)
{
    // Этап 1: все остановки
    for (const auto& request_node : base_requests) {
        const auto& req_map = request_node.AsMap();
        std::string_view type = req_map.at("type"s).AsString();
        if (type == "Stop"sv) {
            std::string name = req_map.at("name"s).AsString();
            double lat = req_map.at("latitude"s).AsDouble();
            double lon = req_map.at("longitude"s).AsDouble();
            catalogue_.AddStop(std::move(name), lat, lon);
        }
    }
}
void JSONReader::ProcessDistances(const json::Array& base_requests)
{
    // Этап 2: расстояния
    for (const auto& request_node : base_requests) {
        const auto& req_map = request_node.AsMap();
        std::string_view type = req_map.at("type"s).AsString();
        if (type == "Stop"sv) {
            std::string_view from_name = req_map.at("name"s).AsString();
            const auto* from_stop = catalogue_.FindStop(from_name);
            if (!from_stop) continue;

            const auto& dist_map = req_map.at("road_distances"s).AsMap();
            for (const auto& [to_name, dist_node] : dist_map) {
                const auto* to_stop = catalogue_.FindStop(to_name);
                if (to_stop) {
                    catalogue_.SetDistance(from_stop, to_stop, dist_node.AsInt());
                }
            }
        }
    }
}
void JSONReader::ProcessBusses(const json::Array& base_requests)
{
    // Этап 3: автобусы
    for (const auto& request_node : base_requests) {
        const auto& req_map = request_node.AsMap();
        std::string_view type = req_map.at("type"s).AsString();
        if (type == "Bus"sv) {
            std::string name = req_map.at("name"s).AsString();
            bool is_circle = req_map.at("is_roundtrip"s).AsBool();

            std::vector<std::string> stop_names;
            for (const auto& stop_node : req_map.at("stops"s).AsArray()) {
                stop_names.emplace_back(stop_node.AsString());
            }

            catalogue_.AddBus(std::move(name), stop_names, is_circle);
        }
    }
}

void JSONReader::RequestBus(json::Builder& builder, const json::Dict& req_map) const {
    std::string_view bus_name = req_map.at("name"s).AsString();
    if (auto info = transport::request_handler::GetBusStat(bus_name, catalogue_)) {
        builder.Key("curvature").Value(info->curvature);
        builder.Key("route_length").Value(info->length);
        builder.Key("stop_count").Value(static_cast<int>(info->total_stops));
        builder.Key("unique_stop_count").Value(static_cast<int>(info->unique_stops));
    }
    else {
        builder.Key("error_message").Value("not found"s);
    }
}

void JSONReader::RequestStop(json::Builder& builder, const json::Dict& req_map) const {
    std::string_view stop_name = req_map.at("name"s).AsString();
    if (auto buses_opt = transport::request_handler::GetBusesByStop(stop_name, catalogue_)) {
        const auto& buses = **buses_opt;
        std::vector<std::string> names(buses.begin(), buses.end());
        std::sort(names.begin(), names.end());

        json::Array arr;
        for (const auto& name : names) {
            arr.push_back(json::Node(name));
        }
        builder.Key("buses").Value(std::move(arr));
    }
    else {
        builder.Key("error_message").Value("not found"s);
    }
}

void JSONReader::ProcessMap(json::Builder& builder, renderer::RenderSettings render_settings) const {
    // Генерируем SVG карту и включаем ее в JSON ответ
    transport::renderer::MapRenderer renderer(render_settings, catalogue_);
    auto svg_doc = renderer.Render();

    // Сохраняем SVG в строку
    std::ostringstream svg_stream;
    svg_doc.Render(svg_stream);
    builder.Key("map").Value(svg_stream.str());
}

transport::renderer::RenderSettings JSONReader::ReadRenderSettings(const json::Dict& render_settings_map) {
    transport::renderer::RenderSettings settings;
    
    settings.width = render_settings_map.at("width"s).AsDouble();
    settings.height = render_settings_map.at("height"s).AsDouble();
    settings.padding = render_settings_map.at("padding"s).AsDouble();
    settings.line_width = render_settings_map.at("line_width"s).AsDouble();
    settings.stop_radius = render_settings_map.at("stop_radius"s).AsDouble();
    settings.bus_label_font_size = render_settings_map.at("bus_label_font_size"s).AsInt();
    
    const auto& bus_label_offset = render_settings_map.at("bus_label_offset"s).AsArray();
    settings.bus_label_offset = {bus_label_offset[0].AsDouble(), bus_label_offset[1].AsDouble()};
    
    settings.stop_label_font_size = render_settings_map.at("stop_label_font_size"s).AsInt();
    
    const auto& stop_label_offset = render_settings_map.at("stop_label_offset"s).AsArray();
    settings.stop_label_offset = {stop_label_offset[0].AsDouble(), stop_label_offset[1].AsDouble()};
    
    settings.underlayer_color = ReadColor(render_settings_map.at("underlayer_color"s));
    settings.underlayer_width = render_settings_map.at("underlayer_width"s).AsDouble();
    
    const auto& color_palette = render_settings_map.at("color_palette"s).AsArray();
    for (const auto& color_node : color_palette) {
        settings.color_palette.push_back(ReadColor(color_node));
    }
    
    return settings;
}

svg::Color JSONReader::ReadColor(const json::Node& color_node) {
    if (color_node.IsString()) {
        return color_node.AsString();
    } else if (color_node.IsArray()) {
        const auto& color_array = color_node.AsArray();
        if (color_array.size() == 3) {
            return svg::Rgb{
                static_cast<uint8_t>(color_array[0].AsInt()),
                static_cast<uint8_t>(color_array[1].AsInt()),
                static_cast<uint8_t>(color_array[2].AsInt())
            };
        } else if (color_array.size() == 4) {
            return svg::Rgba{
                static_cast<uint8_t>(color_array[0].AsInt()),
                static_cast<uint8_t>(color_array[1].AsInt()),
                static_cast<uint8_t>(color_array[2].AsInt()),
                color_array[3].AsDouble()
            };
        }
    }
    
    // По умолчанию возвращаем черный цвет
    return "black"s;
}

} // namespace transport::json_reader