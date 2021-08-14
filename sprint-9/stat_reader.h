#pragma once

#include <string>
#include <set>

#include "transport_catalogue.h"

namespace stat_reader {

transport_catalogue::TransportCatalogue::Bus GetRequestBusParse();

transport_catalogue::TransportCatalogue::Stop GetRequestStopNameParse();

struct BusResult {
    const std::string& bus_number;
    size_t number_stops = 0;
    size_t unique_stop = 0;
    int route = 0;
    double curvature = 0.0;
};

void PrintBusResult(const BusResult& busResult, std::ostream& output);

void PrintStopResult(bool flag_no_buses, const std::string& stop_name, const std::set<std::string>& stop_to_buses,
                     std::ostream& output);

void ParseRequestData(transport_catalogue::TransportCatalogue& transportCatalogue, std::ostream& out);

}

