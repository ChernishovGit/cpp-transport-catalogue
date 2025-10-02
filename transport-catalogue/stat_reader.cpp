#include "stat_reader.h"
#include <iomanip>
#include <string>
#include <vector>
#include <optional>

namespace transport::stat {

using std::string;
using std::string_view;
using std::vector;
using std::optional;
using std::ostream;
using transport::catalogue::TransportCatalogue;

void ParseAndPrintStat(const TransportCatalogue& catalogue, string_view request, ostream& output) {
    if (request.size() >= 5 && request.substr(0, 4) == "Bus ") {
        string bus_name(request.substr(4));
        if (auto info = catalogue.GetBusInfo(bus_name)) {
            output << std::setprecision(6)
                   << "Bus " << bus_name << ": "
                   << info->total_stops << " stops on route, "
                   << info->unique_stops << " unique stops, "
                   << info->length << " route length\n";
        } else {
            output << "Bus " << bus_name << ": not found\n";
        }
    }
    else if (request.size() >= 6 && request.substr(0, 5) == "Stop ") {
        string stop_name(request.substr(5));
        if (auto buses = catalogue.GetBusesByStop(stop_name)) {
            if (buses->empty()) {
                output << "Stop " << stop_name << ": no buses\n";
            } else {
                output << "Stop " << stop_name << ": buses";
                for (const auto& bus : *buses) {
                    output << " " << bus;
                }
                output << "\n";
            }
        } else {
            output << "Stop " << stop_name << ": not found\n";
        }
    }
}
} // namespace transport::stat