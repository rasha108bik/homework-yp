#include "transport_router.h"

using namespace transport_router;
using namespace graph;
using namespace std;

set<const transport_catalogue::Bus*> AddAllBusRoute(transport_catalogue::TransportCatalogue catalogue,
                                                                         string_view from, string_view to) {
    auto bus_from = catalogue.FindBusByStop(std::string(from));
    auto bus_to = catalogue.FindBusByStop(std::string(to));

    set<const transport_catalogue::Bus*> bus_routes;
    for (const auto& b_f_ : bus_from) {
        bus_routes.insert(catalogue.FindBus(b_f_));
    }
    for (const auto& b_t_ : bus_to) {
        bus_routes.insert(catalogue.FindBus(b_t_));
    }

    return bus_routes;
}

void TransportRouter::AddEdges(std::string_view bus_num, std::string_view stop_from, std::string_view stop_to, int distance, int span_count) {
    VertexId stop_from_vertex_id = stop_name_vertex_id_[std::string(stop_from)];
    VertexId stop_to_vertex_id = stop_name_vertex_id_[std::string(stop_to)];

    Edge<double> edge_{};
    edge_.from = stop_from_vertex_id;
    edge_.to = stop_to_vertex_id;
    edge_.weight = distance / routingSettings_.bus_velocity_ + routingSettings_.bus_wait_time_;

    auto edge_id = directedWeightedGraph_.AddEdge(edge_);
    data_.push_back({bus_num, stop_from, stop_to, stop_from_vertex_id, stop_to_vertex_id, edge_.weight, span_count});

    assert(edge_id == data_.size() - 1);
}

void TransportRouter::CreateGraph(transport_catalogue::TransportCatalogue &transportCatalogue, string_view stop_from, string_view stop_to) {
    auto get_all_bus_route = AddAllBusRoute(transportCatalogue, stop_from, stop_to);

    for (size_t i = 0; i < transportCatalogue.GetAllStops().size(); ++i) {
        stop_name_vertex_id_.insert({transportCatalogue.GetAllStops()[i].name, i});
    }

    for (const transport_catalogue::Bus* item : get_all_bus_route) {
        size_t size = item->stop_names.size();
        for (size_t i = 0; i < size - 1; ++i) {
            int distance = 0;
            for (size_t j = i + 1; j < size; ++j) {
                auto find_stop_first = transportCatalogue.FindStop(item->stop_names[j - 1u]);
                auto find_stop_second = transportCatalogue.FindStop(item->stop_names[j]);

                if (item->stop_names[i] == item->stop_names[j]) {
                    continue;                                       // hardcode  ???
                }

                distance += transportCatalogue.GetCalculateDistance(find_stop_first, find_stop_second);
                AddEdges(item->name, item->stop_names[i], item->stop_names[j], distance, static_cast<int>(j - i));
            }
        }
    }

}

std::optional<std::vector<TransportRouter::ResponseFindRoute>> TransportRouter::FindRoute(string_view stop_from, string_view stop_to) {
    Router router(directedWeightedGraph_);

    VertexId vertex_from = stop_name_vertex_id_[std::string(stop_from)];
    VertexId vertex_to = stop_name_vertex_id_[std::string(stop_to)];
    auto get_build = router.BuildRoute(vertex_from, vertex_to);
    if (get_build == nullopt) {
        return nullopt;
    }

    vector<ResponseFindRoute> result;
    ResponseFindRoute responseFindRoute;
    responseFindRoute.weight_ = get_build->weight;

    for (const auto& v : get_build->edges) {
        auto get_info_route = data_[v];
        responseFindRoute.findRoute.bus_num_ = get_info_route.bus_num;
        responseFindRoute.findRoute.stop_name_ = get_info_route.stop_from;
        responseFindRoute.findRoute.time_ = get_info_route.width;
        responseFindRoute.findRoute.type_ = getItemType(ItemType::Bus);
        responseFindRoute.findRoute.span_count_ = get_info_route.span_count;

        result.push_back(responseFindRoute);
    }

    return result;
}
