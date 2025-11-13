#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"

#include <istream>
#include <ostream>

namespace transport::json_reader {

void ProcessRequests(std::istream& in, std::ostream& out, transport::catalogue::TransportCatalogue& catalogue);

transport::renderer::RenderSettings ReadRenderSettings(const json::Dict& render_settings_map);

svg::Color ReadColor(const json::Node& color_node);

} // namespace transport::json_reader