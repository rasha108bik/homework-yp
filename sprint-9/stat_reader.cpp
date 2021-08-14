#include "stat_reader.h"

#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std::literals;
using namespace transport_catalogue;

namespace stat_reader {

TransportCatalogue::Bus GetRequestBusParse() {
    std::string name;
    std::cin.ignore();
    getline(std::cin, name, '\n');

    TransportCatalogue::Bus bus;
    bus.name = name;

    return bus;
}

TransportCatalogue::Stop GetRequestStopNameParse() {
    std::string name;
    std::cin.ignore();
    getline(std::cin, name, '\n');

    TransportCatalogue::Stop stop;
    stop.name = name;

    return stop;
}

void PrintBusResult(const BusResult& busResult, std::ostream& output) {
    std::stringstream myString;
    if (busResult.number_stops == 0) {
        myString << "Bus "s << busResult.bus_number << ": not found"s;
    } else {
        myString << "Bus "s << busResult.bus_number
                 << ": "s << busResult.number_stops << " stops on route, "s
                 << busResult.unique_stop << " unique stops, "s
                 << busResult.route << " route length, "s
                 << std::setprecision(6)
                 << busResult.curvature << " curvature"s;
    }

    output << myString.str() << std::endl;
}

void PrintStopResult(bool flag_no_buses, const std::string &stop_name, const std::set<std::string> &stop_to_buses, std::ostream& output) {
    std::stringstream myString;
    if (flag_no_buses) {
        myString << "Stop "s << stop_name << ": no buses"s;
    } else if (stop_to_buses.empty()) {
        myString << "Stop "s << stop_name << ": not found"s;
    } else {
        myString << "Stop "s << stop_name << ": buses "s;
        for (const auto &bus : stop_to_buses) {
            myString << bus;
            if (bus != *stop_to_buses.end()) {
                myString << ' ';
            }
        }
    }

    output << myString.str() << std::endl;
}

void ParseRequestData(TransportCatalogue& transportCatalogue, std::ostream& out) {
    int base_get_request_count = 0;
    std::cin >> base_get_request_count;
    for (int i = 0; i < base_get_request_count; ++i) {
        std::string command;
        std::cin >> command;

        if (command == "Bus") {
            TransportCatalogue::Bus bus = stat_reader::GetRequestBusParse();
            auto get = transportCatalogue.GetInformationRoute(bus);

            stat_reader::BusResult busResult{get.bus_number, get.number_stops, get.unique_stop, get.route,
                                             get.curvature};
            stat_reader::PrintBusResult(busResult, out);
        }

        if (command == "Stop") {
            TransportCatalogue::Stop stop = stat_reader::GetRequestStopNameParse();
            auto get = transportCatalogue.GetBusForStop(stop);

            stat_reader::PrintStopResult(get.flag_no_buses, get.stop_name, get.stop_to_buses, out);
        }
    }
}

}