#include "input_reader.h"
#include <algorithm>
#include <iterator>
#include <string>
#include <vector>
#include <string_view>
#include <cmath>

namespace transport::input::detail {

using std::string;
using std::string_view;
using std::vector;
using transport::geo::Coordinates;

Coordinates ParseCoordinates(string_view str) {
    static const double nan = std::nan("");
    auto not_space = str.find_first_not_of(' ');
    auto comma = str.find(',');
    if (comma == string_view::npos) {
        return { nan, nan };
    }
    auto not_space2 = str.find_first_not_of(' ', comma + 1);
    double lat = std::stod(string(str.substr(not_space, comma - not_space)));
    double lng = std::stod(string(str.substr(not_space2)));
    return { lat, lng };
}

string_view Trim(string_view str) {
    const auto start = str.find_first_not_of(' ');
    if (start == string_view::npos) {
        return {};
    }
    return str.substr(start, str.find_last_not_of(' ') + 1 - start);
}

vector<string_view> Split(string_view str, char delim) {
    vector<string_view> result;
    size_t pos = 0;
    while ((pos = str.find_first_not_of(' ', pos)) < str.length()) {
        auto delim_pos = str.find(delim, pos);
        if (delim_pos == string_view::npos) {
            delim_pos = str.size();
        }
        if (auto substr = Trim(str.substr(pos, delim_pos - pos)); !substr.empty()) {
            result.push_back(substr);
        }
        pos = delim_pos + 1;
    }
    return result;
}

transport::input::CommandDescription ParseCommandDescription(string_view line) {
    auto colon_pos = line.find(':');
    if (colon_pos == string_view::npos) {
        return {};
    }
    auto space_pos = line.find(' ');
    if (space_pos >= colon_pos) {
        return {};
    }
    auto not_space = line.find_first_not_of(' ', space_pos);
    if (not_space >= colon_pos) {
        return {};
    }
    return {
        string(line.substr(0, space_pos)),
        string(line.substr(not_space, colon_pos - not_space)),
        string(line.substr(colon_pos + 1))
    };
}

} // namespace transport::input::detail

namespace transport::input {

using std::string;
using std::string_view;
using std::vector;
using transport::catalogue::TransportCatalogue;

void Reader::ParseLine(string_view line) {
    auto cmd = detail::ParseCommandDescription(line);
    if (cmd) {
        commands_.push_back(std::move(cmd));
    }
}

void Reader::ApplyCommands(TransportCatalogue& catalogue) const {
    // Сначала все остановки
    for (const auto& cmd : commands_) {
        if (cmd.command == "Stop") {
            auto coords = detail::ParseCoordinates(cmd.description);
            catalogue.AddStop(cmd.id, coords);
        }
    }
    // Потом все маршруты
    for (const auto& cmd : commands_) {
        if (cmd.command == "Bus") {
            string_view route = cmd.description;
            bool is_circle = (route.find('>') != string_view::npos);
            vector<string> stops;
            auto parts = is_circle
                ? detail::Split(route, '>')
                : detail::Split(route, '-');
            stops.reserve(parts.size());
            for (const auto& p : parts) {
                stops.emplace_back(p);
            }
            catalogue.AddBus(cmd.id, std::move(stops), is_circle);
        }
    }
}

} // namespace transport::input