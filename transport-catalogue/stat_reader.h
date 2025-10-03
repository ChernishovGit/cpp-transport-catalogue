#pragma once

#include <iosfwd>
#include <string_view>
#include "transport_catalogue.h"

namespace transport::stat {

void ParseAndPrintStat(const transport::catalogue::TransportCatalogue& catalogue,
    std::string_view request, std::ostream& output);

void StatRequest(std::istream& in, std::ostream& out, const TransportCatalogue& catalogue);

} // namespace transport::stat