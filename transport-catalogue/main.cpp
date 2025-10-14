#include <iostream>
#include <string>

#include "input_reader.h"
#include "stat_reader.h"

int main() {
    using namespace std;
    using namespace transport;

    catalogue::TransportCatalogue catalogue;
    input::Reader reader;

    reader.ReadStream(cin, catalogue);

    transport::stat::StatRequest(cin, cout, catalogue);
}