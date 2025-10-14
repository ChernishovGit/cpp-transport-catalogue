#pragma once
#include <deque>
#include <vector>
#include <unordered_map>
#include <string_view>
#include <string>
#include <optional>
#include <set>
#include <functional>

#include "geo.h"

namespace transport::catalogue {

struct Stop {
	std::string name;
	transport::geo::Coordinates coordinates;
};

struct Bus {
	std::string name;
	std::vector<const Stop*> stops;
	bool is_circle = false;
};

struct BusInfo {
	size_t total_stops = 0;
	size_t unique_stops = 0;
	int length = 0;
	double curvature = 0.0;
};

struct StopPairHash {
	size_t operator()(const std::pair<const Stop*, const Stop*>& p) const {
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

	const Stop* FindStop(std::string_view name) const;
	const Bus* FindBus(std::string_view name) const;

	std::optional<BusInfo> GetBusInfo(std::string_view bus_name) const;
	std::optional<const std::set<std::string>*> GetBusesByStop(std::string_view stop_name) const;

	void SetDistance(const Stop* from, const Stop* to, int distance);
	int GetDistance(const Stop* from, const Stop* to) const;

private:
	std::deque<Stop> stops_{};
	std::unordered_map<std::string_view, const Stop*> stops_index_{};

	std::deque<Bus> buses_{};
	std::unordered_map<std::string_view, const Bus*> bus_index_{};

	std::unordered_map<std::string, std::set<std::string>> stop_to_bus_;
	std::unordered_map<std::pair<const Stop*, const Stop*>, int, StopPairHash> distances_;
};
} // namespace transport::catalogue