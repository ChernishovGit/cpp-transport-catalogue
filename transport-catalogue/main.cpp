#include "json_reader.h"
#include "transport_catalogue.h"
#include <iostream>

int main() {
    transport::catalogue::TransportCatalogue catalogue;
    transport::json_reader::JSONReader request(catalogue);
    request.ProcessRequests(std::cin, std::cout);
    return 0;
}