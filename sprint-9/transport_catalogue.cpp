#include "transport_catalogue.h"

#include <algorithm>

using namespace transport_catalogue;

void TransportCatalogue::AddBusRoute(const Bus& bus) {
    if (get_buses_to_stop_.find(bus.name) == get_buses_to_stop_.end()) {
        size_t size_unique_route = 0;
        for (const auto& stop_name : bus.stop_names) {
            if (stop_to_buses_[stop_name].find(bus.name) == stop_to_buses_[stop_name].end()) {
                ++size_unique_route;
            }

            stop_to_buses_[stop_name].insert(bus.name);
        }

        buses_to_stop_.push_back({bus.name, bus.stop_names});
        get_buses_to_stop_[buses_to_stop_.back().name] = &buses_to_stop_.back();
        buses_unique_route_[buses_to_stop_.back().name] = size_unique_route;
    }
}

const TransportCatalogue::Bus* TransportCatalogue::FindBus(std::string_view bus) {
    auto search = get_buses_to_stop_.find(bus);
    if (search != get_buses_to_stop_.end()) {
        return get_buses_to_stop_[bus];
    }
    return nullptr;
}

void TransportCatalogue::AddStop(const std::string& stop_name, const geo::Coordinates& coordinates) {
    if (FindStop(stop_name) == nullptr) {
        stops_.push_back({stop_name, {coordinates.lat, coordinates.lng}});
        get_stops_[stops_.back().name] = &stops_.back();
    }
}

const TransportCatalogue::Stop* TransportCatalogue::FindStop(std::string_view stop_name) {
    auto search = get_stops_.find(stop_name);
    if (search != get_stops_.end()) {
        return get_stops_[stop_name];
    }
    return nullptr;
}

std::set<std::string> TransportCatalogue::FindBusByStop(const std::string& stop_name) {
    std::set<std::string> result;

    auto stop_to_bus = stop_to_buses_.find(stop_name);
    if (stop_to_bus != stop_to_buses_.end()) {
        result = stop_to_buses_[stop_name];
    }

    return result;
}

void TransportCatalogue::AddDistance(const std::string&  from, std::string_view to, int distance) {
    const Stop* a = FindStop(from);
    const Stop* b = FindStop(to);
    if (a != nullptr && b != nullptr) {
        if (distance_between_stop_.find({a, b}) == distance_between_stop_.end()) {
            distance_between_stop_[{a, b}] = distance;
        }
    }
}

TransportCatalogue::InformationRoute TransportCatalogue::GetInformationRoute(const Bus& bus) {
    InformationRoute getInform{};

    if (FindBus(bus.name) == nullptr) {
        getInform.bus_number = bus.name;
        return getInform;
    }

    int actual_route = 0;
    double coordinates_route = 0.0;
    std::string last_stop_name;

    auto bus_num = FindBus(bus.name);
    for (const auto& stop_name : bus_num->stop_names) {
        if (!last_stop_name.empty()) {
            coordinates_route += ComputeDistance(FindStop(last_stop_name)->coordinates, FindStop(stop_name)->coordinates);
            actual_route += GetCalculateDistance(FindStop(last_stop_name), FindStop(stop_name));
        }
        last_stop_name = stop_name;
    }

    getInform.number_stops = bus_num->stop_names.size();
    getInform.bus_number = bus.name;
    getInform.unique_stop = buses_unique_route_[bus.name];
    getInform.route = actual_route;
    getInform.curvature = (static_cast<double>(actual_route) / coordinates_route);

    return getInform;
}

int TransportCatalogue::GetCalculateDistance(const Stop* first_route,
                                             const Stop* second_route) {
    if (distance_between_stop_.find({first_route, second_route}) != distance_between_stop_.end()) {
        return distance_between_stop_[{first_route, second_route}];
    } else {
        return distance_between_stop_[{second_route, first_route}];
    }
}

TransportCatalogue::StopInform TransportCatalogue::GetBusForStop(const Stop& stop) {
    StopInform stopInform{};
    stopInform.stop_name = stop.name;

    auto get_stop = FindStop(stop.name);
    if (get_stop == nullptr) {
        return stopInform;
    }

    auto stop_to_bus = FindBusByStop(stop.name);
    if (stop_to_bus.empty()) {
        stopInform.flag_no_buses = true;
        return stopInform;
    }

    stopInform.stop_to_buses = stop_to_buses_[stop.name];
    return stopInform;
}