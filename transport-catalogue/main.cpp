#include "json_reader.h"
#include "transport_catalogue.h"
#include <iostream>

int main() {
    //transport::catalogue::TransportCatalogue catalogue;
    //transport::json_reader::ProcessRequests(std::cin, std::cout, catalogue);

    transport::catalogue::TransportCatalogue catalogue;
    transport::json_reader::ProcessRequests(std::cin, std::cout, catalogue);
    return 0;
}