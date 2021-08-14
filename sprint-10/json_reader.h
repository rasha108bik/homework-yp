#pragma once

#include <string>
#include <sstream>
#include <optional>

#include "json.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"

/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */

namespace json_reader {

class JsonReader {
public:

    explicit JsonReader(transport_catalogue::TransportCatalogue &transportCatalogue, renderer::MapRenderer &mapRenderer);

    json::Document LoadJSON(const std::string& s);
    json::Document Load(std::istream& input);
    static std::string Print(const json::Node &node);

    void PostBaseRequestsInTransportCatalogue(const json::Node& root_);

    std::optional<renderer::MapRenderer::RenderSettings> ParseRenderSettings(const json::Node& root_);

    json::Array GetStatRequests(const json::Node& root_, const renderer::MapRenderer::RenderSettings &renderSettings);

private:
    transport_catalogue::TransportCatalogue &transportCatalogue_;
    renderer::MapRenderer &mapRenderer_;
    request_handler::RequestHandler requestHandler_;

    std::vector<std::pair<std::string, bool>> buses_route;
};

}