#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <unordered_map>
#include <sstream>

#include "geo.h"
#include "transport_catalogue.h"

namespace input_reader {

class InputReared {
public:
    struct TypeRouteAndData {
        std::string type_command;
        char identifier;
        std::string data;
    };

    void ParseRouteData(transport_catalogue::TransportCatalogue &transportCatalogue, std::istream &input);

    struct DataRoute {
        std::string type_command;
        std::string name;
        geo::Coordinates coordinates;
        std::vector<std::string> stop_or_bus;
        std::vector<std::pair<std::string, int>> stop_distance;
    };

    DataRoute ParseData(const TypeRouteAndData &data_for_route);

private:
    TypeRouteAndData stopAndBusData;
    std::vector<TypeRouteAndData> stops_and_buses_data;
    std::map<std::string, std::vector<std::pair<std::string, int>>> distance_stop;
    std::map<std::string, std::vector<std::string>> busRoutes;

    void ParseTypeRouteData(std::istream &input);
    void AddRouteDataInTransportCatalogue(transport_catalogue::TransportCatalogue& transportCatalogue);
};

}
