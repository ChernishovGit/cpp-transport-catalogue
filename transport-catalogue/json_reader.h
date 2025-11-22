#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "json_builder.h"

#include <istream>
#include <ostream>

namespace transport::json_reader {

class JSONReader {
public:
	JSONReader(transport::catalogue::TransportCatalogue& catalogue) : catalogue_(catalogue) {};

	void ProcessRequests(std::istream& in, std::ostream& out);

private:
	void ProcessStops(const json::Array& base_requests);
	void ProcessDistances(const json::Array& base_requests);
	void ProcessBusses(const json::Array& base_requests);

	void RequestBus(json::Builder& builder, const json::Dict& req_map) const;
	void RequestStop(json::Builder& builder, const json::Dict& req_map) const;
	void ProcessMap(json::Builder& builder, renderer::RenderSettings render_settings) const;

	transport::renderer::RenderSettings ReadRenderSettings(const json::Dict& render_settings_map);
	svg::Color ReadColor(const json::Node& color_node);

private:
	transport::catalogue::TransportCatalogue& catalogue_;
};
} // namespace transport::json_reader