#pragma once

#include <string>
#include <sstream>
#include <optional>

#include "json.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "graph.h"

namespace json_reader {

    class JsonReader {
    public:

        explicit JsonReader(transport_catalogue::TransportCatalogue &transportCatalogue,
                            renderer::MapRenderer &mapRenderer);

        json::Document LoadJSON(const std::string& s);
        json::Document Load(std::istream& input);
        static std::string Print(const json::Node &node);

        void PostBaseRequestsInTransportCatalogue(const json::Node& root_);

        std::optional<renderer::MapRenderer::RenderSettings> ParseRenderSettings(const json::Node& root_);
        std::optional<transport_router::TransportRouter::RoutingSettings> routingSettings(const json::Node& root_);

        json::Array GetStatRequests(const json::Node& root_, const renderer::MapRenderer::RenderSettings &renderSettings,
                                    const transport_router::TransportRouter::RoutingSettings& routingSettings);

    private:
        transport_catalogue::TransportCatalogue &transportCatalogue_;
        renderer::MapRenderer &mapRenderer_;
        request_handler::RequestHandler requestHandler_;

        std::vector<std::pair<std::string, bool>> buses_route;
    };

}