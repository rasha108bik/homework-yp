#include "input_reader.h"

#include <iostream>
#include <utility>
#include <algorithm>

using namespace std::literals;
using namespace transport_catalogue;

namespace input_reader {

namespace detail {

void DeleteSpaces(std::string &string) {
    size_t strBegin = string.find_first_not_of(' ');
    size_t strEnd = string.find_last_not_of(' ');
    string.erase(strEnd + 1, string.size() - strEnd);
    string.erase(0, strBegin);
}

std::vector<std::string> SplitIntoWords(const std::string &text) {
    std::vector<std::string> words;
    std::string word;
    for (const char c : text) {
        if (c == '>' || c == '-') {
            if (!word.empty()) {
                DeleteSpaces(word);

                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        DeleteSpaces(word);
        words.push_back(word);
    }

    return words;
}

}

void InputReared::ParseTypeRouteData(std::istream& input) {
    int base_request_count;
    input >> base_request_count;
    for (int i = 0; i < base_request_count; ++i) {
        std::string command;
        input >> command;

        std::string str;
        std::getline(input, str, '\n');
        if (str.empty()) {
            return;
        }

        detail::DeleteSpaces(str);

        if (str.find('-') != std::string::npos) {
            stopAndBusData.identifier = '-';
        } else if (str.find('>') != std::string::npos) {
            stopAndBusData.identifier = '>';
        } else if (str.find('m') != std::string::npos) {
            stopAndBusData.identifier = 'm';
        }

        stopAndBusData.type_command = command;
        stopAndBusData.data = str;

        if (stopAndBusData.data.empty()) {
            continue;
        }

        stops_and_buses_data.push_back(stopAndBusData);
    }
}

void InputReared::AddRouteDataInTransportCatalogue(TransportCatalogue& transportCatalogue) {
    for (const auto& [bus_number, stop_names] : busRoutes) {
        TransportCatalogue::Bus bus{bus_number, stop_names};
        transportCatalogue.AddBusRoute(bus);
    }

    for (const auto& [stop_name, distances_to_stop] : distance_stop) {
        for (const auto&[name, distance] : distances_to_stop) {
            transportCatalogue.AddDistance(stop_name, name, distance);
        }
    }
}

void InputReared::ParseRouteData(TransportCatalogue& transportCatalogue, std::istream& input) {
    ParseTypeRouteData(input);

    // сортируем вектор, чтобы остановки стали первыми элементами вектора
    std::sort(stops_and_buses_data.begin(), stops_and_buses_data.end(), [](const auto& i, const auto& j) {
        return i.type_command > j.type_command;
    });

    for (const auto& stop_and_buses_data : stops_and_buses_data) {
        DataRoute parse_data = ParseData(stop_and_buses_data);

        if (parse_data.type_command == "Stop") {
            transportCatalogue.AddStop(parse_data.name, parse_data.coordinates);

            for (const auto& distances_to_stop : parse_data.stop_distance) {
                distance_stop[parse_data.name].push_back(distances_to_stop);
            }
        }

        if (parse_data.type_command == "Bus") {
            for (const auto& w : parse_data.stop_or_bus) {
                busRoutes[parse_data.name].push_back(w);
            }
        }
    }

    stops_and_buses_data.clear();

    AddRouteDataInTransportCatalogue(transportCatalogue);
}

geo::Coordinates ParseCoordinates(const std::string& stops) {
    geo::Coordinates coordinates{};

    size_t separator = stops.find(':');

    // найти latitude
    size_t separator_second = stops.find(',');
    std::string lat = stops.substr(separator + 2, separator_second - separator - 2);
    coordinates.lat = std::stod(lat);

    // найти longitude
    separator = stops.find(',', separator_second + 1);
    std::string lng = stops.substr(separator_second + 2, separator - separator_second - 2);
    coordinates.lng = std::stod(lng);

    return coordinates;
}

std::vector<std::pair<std::string, int>> ParseDistanceToStop(const std::string& stops) {
    std::vector<std::pair<std::string, int>> result;

    size_t separator_second = stops.find(',');
    size_t separator = stops.find(',', separator_second + 1);
    std::string distance = stops.substr(separator + 1);

    if (distance.find(':') == std::string::npos) {
        separator = 0;

        while (separator != std::string::npos) {

            separator_second = distance.find('m', separator);
            auto test = distance.substr(separator + 1, separator_second - separator);
            int dist = std::stoi(test);

            separator = separator_second;
            separator_second = distance.find(',', separator);
            std::string stop = distance.substr(separator + 5, separator_second - separator - 5);
            separator = separator_second;

            result.emplace_back(stop, dist);
        }
    }

    return result;
}

InputReared::DataRoute ParseStops(const std::string& stops) {
    InputReared::DataRoute dataRoute{};

    dataRoute.coordinates = ParseCoordinates(stops);
    dataRoute.stop_distance = ParseDistanceToStop(stops);

    return dataRoute;
}

InputReared::DataRoute ParseBusses(const std::string& busses, const char identifier) {
    InputReared::DataRoute stopAndBus{};

    size_t separator = busses.find(':');

    std::string stop_bus_x = busses.substr(separator + 2, busses.size());
    std::vector<std::string> bus_route_data = detail::SplitIntoWords(stop_bus_x);

    // Преобразуем тип маршрута --> "-" в кольцевой
    if (identifier == '-') {
        int size = static_cast<int>(bus_route_data.size()) - 2;
        while (size >= 0) {
            bus_route_data.push_back(bus_route_data[size]);
            size--;
        }
    }

    stopAndBus.stop_or_bus = bus_route_data;

    return stopAndBus;
}

InputReared::DataRoute InputReared::ParseData(const TypeRouteAndData &data_for_route) {
    DataRoute stopAndBus{};

    std::string type_command = data_for_route.type_command;
    std::string str_data = data_for_route.data;

    std::string name = str_data.substr(0, str_data.find(':'));

    if (type_command == "Stop"s) {
        stopAndBus = ParseStops(str_data);
    }

    if (type_command == "Bus"s) {
        stopAndBus = ParseBusses(str_data, data_for_route.identifier);
    }

    stopAndBus.type_command = type_command;
    stopAndBus.name = name;

    return stopAndBus;
}

}