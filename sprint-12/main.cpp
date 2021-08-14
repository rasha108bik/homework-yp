#include <iostream>
#include <optional>

#include "json_reader.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

using namespace std;
using namespace transport_catalogue;
using namespace renderer;
using namespace transport_router;

int main() {
    TransportCatalogue transportCatalogue;
    MapRenderer mapRenderer;

    json_reader::JsonReader jsonReader(transportCatalogue, mapRenderer);
    auto node = jsonReader.Load(cin).GetRoot();

    jsonReader.PostBaseRequestsInTransportCatalogue(node);

    auto get_render_settings = jsonReader.ParseRenderSettings(node);
    MapRenderer::RenderSettings render_settings;
    if (get_render_settings != nullopt) {
        render_settings = get_render_settings.value();
    }

    auto get_routing_settings = jsonReader.routingSettings(node);
    TransportRouter::RoutingSettings routingSettings{};
    if (get_routing_settings != nullopt) {
        routingSettings = get_routing_settings.value();
    }

    auto array_stat_req = jsonReader.GetStatRequests(node, render_settings, routingSettings);
    std::cout << json_reader::JsonReader::Print(array_stat_req);

    return 0;
}