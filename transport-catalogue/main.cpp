#include <iostream>
#include <string>

#include "input_reader.h"
#include "stat_reader.h"

int main() {
    using namespace std;
    using namespace transport;

    catalogue::TransportCatalogue catalogue;

    ReadStream(cin, &catalogue)

    StatRequest(cin, cout, &catalogue)
}