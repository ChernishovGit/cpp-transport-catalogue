#include "json_reader.h"
#include "transport_catalogue.h"
#include <iostream>

int main() {
    //transport::catalogue::TransportCatalogue catalogue;
    //transport::json_reader::ProcessRequests(std::cin, std::cout, catalogue);

    transport::catalogue::TransportCatalogue catalogue;
    transport::json_reader::JSONReader request(catalogue);
    request.ProcessRequests(input_stream, std::cout);
    return 0;
}