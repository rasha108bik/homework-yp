#include <iostream>

#include "input_reader.h"
#include "stat_reader.h"
#include "transport_catalogue.h"

using namespace std;
using namespace transport_catalogue;

int main() {
    TransportCatalogue transportCatalogue;

    input_reader::InputReared inputReared;
    inputReared.ParseRouteData(transportCatalogue, cin);

    std::ostringstream out;
    stat_reader::ParseRequestData(transportCatalogue, out);
    std::cout << out.str();

    return 0;
}