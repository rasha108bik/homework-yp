#pragma once

#include <string_view>
#include <string>
#include <map>
#include <unordered_map>

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"
#include "domain.h"

namespace transport_router {
    using namespace std::literals;

    class TransportRouter {
    public:
        struct RoutingSettings {
            int bus_wait_time_;
            double bus_velocity_;
        };

        struct GraphInfoStops {
            std::string_view bus_num;
            std::string_view stop_from;
            std::string_view stop_to;
            graph::VertexId stop_from_id;
            graph::VertexId stop_to_id;
            double width = 0.0;
            int span_count = 0;
        };

        template <typename Weight>
        explicit TransportRouter(Weight vertex_size, const TransportRouter::RoutingSettings &routingSettings)
                : directedWeightedGraph_(vertex_size),
                  routingSettings_(routingSettings) {
        }

        void CreateGraph(transport_catalogue::TransportCatalogue& transportCatalogue, std::string_view stop_from, std::string_view stop_to);
        void AddEdges(std::string_view bus_num, std::string_view stop_from, std::string_view stop_to, int distance, int span_count);

        struct FindRoute{
            std::string bus_num_;
            std::string stop_name_;
            int span_count_ = 0;
            double time_ = 0.0;
            std::string type_;
        };

        struct ResponseFindRoute {
            double weight_ = 0.0;
            FindRoute findRoute;
        };

        std::optional<std::vector<ResponseFindRoute>> FindRoute(std::string_view stop_from, std::string_view stop_to);

    private:
        enum ItemType {
            Bus,
            Stop
        };

        std::string getItemType(ItemType itemType) {
            if (itemType == Bus)
                return "Bus"s;
            if (itemType == Stop)
                return "Stop"s;
            return "failed item type"s;
        }

        graph::DirectedWeightedGraph<double> directedWeightedGraph_;
        const RoutingSettings routingSettings_;

        std::vector<GraphInfoStops> data_;
        std::unordered_map<std::string, graph::VertexId> stop_name_vertex_id_;
    };

}