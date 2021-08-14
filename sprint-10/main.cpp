#include <iostream>
#include <optional>

#include "json_reader.h"
#include "transport_catalogue.h"
#include "map_renderer.h"

using namespace std;
using namespace transport_catalogue;
using namespace renderer;

int main() {
    TransportCatalogue transportCatalogue;
    MapRenderer mapRenderer;

    json_reader::JsonReader jsonReader(transportCatalogue, mapRenderer);
    auto node = jsonReader.Load(cin).GetRoot();

    jsonReader.PostBaseRequestsInTransportCatalogue(node);

    auto getRenderSettings = jsonReader.ParseRenderSettings(node);
    MapRenderer::RenderSettings renderSettings;
    if (getRenderSettings != nullopt) {
        renderSettings = getRenderSettings.value();
    }

    auto array_stat_req = jsonReader.GetStatRequests(node, renderSettings);
    std::cout << json_reader::JsonReader::Print(array_stat_req);

    return 0;
}