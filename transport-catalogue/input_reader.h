#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "geo.h"
#include "transport_catalogue.h"

namespace transport::input {

struct CommandDescription {
    explicit operator bool() const {
        return !command.empty();
    }
    bool operator!() const {
        return !operator bool();
    }
    std::string command;
    std::string id;
    std::string description;
};

class Reader {
public:
    void ParseLine(std::string_view line);
    void ApplyCommands(transport::catalogue::TransportCatalogue& catalogue) const;
    void ReadStream(std::istream& in, TransportCatalogue& catalogue);

private:
    std::vector<CommandDescription> commands_;
};

} // namespace transport::input