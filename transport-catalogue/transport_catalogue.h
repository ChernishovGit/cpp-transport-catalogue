#pragma once
#include <deque>
#include <vector>
#include <unordered_map>

#include <string_view>
#include <string>
#include <optional>

#include "geo.h"
#include <set>

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
	double length = 0.0;
};

class TransportCatalogue {
	// Реализуйте класс самостоятельно
	/*
	Класс транспортного справочника назовите TransportCatalogue.
	Он должен иметь методы для выполнения следующих задач:
	добавление маршрута в базу,
	добавление остановки в базу,
	поиск маршрута по имени,
	поиск остановки по имени,
	получение информации о маршруте.

	Методы классаTransportCatalogue не должны выполнять никакого ввода-вывода.
	В будущих версиях программы формат входных и выходных данных программы будет изменён.
	Отделение логики от ввода-вывода позволит легко изменить формат входных и выходных данных,
	не затрагивая логику приложения.
	*/
public:
	void AddStop(std::string name, double lat, double lon);
	void AddStop(std::string name, transport::geo::Coordinates coords);
	void AddBus(std::string name, std::vector<std::string> stop_names, bool is_circle);
	const Stop* FindStop(std::string_view name) const;
	const Bus* FindBus(std::string_view name) const;
	std::optional<BusInfo> GetBusInfo(std::string_view bus_name) const;

	std::optional<std::vector<std::string>> GetBusesByStop(std::string_view stop_name) const;

private:
	std::deque<Stop> stops_{};
	std::unordered_map<std::string_view, const Stop*> stops_index_{};

	std::deque<Bus> buses_{};
	std::unordered_map<std::string_view, const Bus*> bus_index_{};

	std::unordered_map<std::string_view, std::set<std::string_view>> stop_to_bus_;
};
} // namespace transport::catalogue