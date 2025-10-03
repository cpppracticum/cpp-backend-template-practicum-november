#include "request_handler.h"
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <functional>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

namespace {

template <typename Request>
http::response<http::string_body> MakeErrorResponse(const Request& req, http::status status, 
                                                   std::string code, 
                                                   std::string message) {
    json::object error_json;
    error_json["code"] = code;
    error_json["message"] = message;

    http::response<http::string_body> response{status, req.version()};
    response.set(http::field::content_type, "application/json");
    response.body() = json::serialize(error_json);
    response.prepare_payload();

    return response;
}

std::string ExtractMapId(std::string_view path) {
    const std::string prefix = "/api/v1/maps/";
    if (path.size() <= prefix.size()) {
        return "";
    }
    
    std::string map_id = std::string(path.substr(prefix.size()));
    size_t pos = map_id.find('/');
    if (pos != std::string::npos) {
        map_id = map_id.substr(0, pos);
    }
    
    pos = map_id.find('?');
    if (pos != std::string::npos) {
        map_id = map_id.substr(0, pos);
    }
    
    return map_id;
}

json::object SerializeRoad(const model::Road& road) {
    json::object road_json;
    road_json["x0"] = road.GetStart().x;
    road_json["y0"] = road.GetStart().y;
    
    if (road.IsHorizontal()) {
        road_json["x1"] = road.GetEnd().x;
    } else {
        road_json["y1"] = road.GetEnd().y;
    }
    
    return road_json;
}

json::object SerializeBuilding(const model::Building& building) {
    json::object building_json;
    const auto& bounds = building.GetBounds();
    building_json["x"] = bounds.position.x;
    building_json["y"] = bounds.position.y;
    building_json["w"] = bounds.size.width;
    building_json["h"] = bounds.size.height;
    return building_json;
}

json::object SerializeOffice(const model::Office& office) {
    json::object office_json;
    office_json["id"] = *office.GetId();
    office_json["x"] = office.GetPosition().x;
    office_json["y"] = office.GetPosition().y;
    office_json["offsetX"] = office.GetOffset().dx;
    office_json["offsetY"] = office.GetOffset().dy;
    return office_json;
}

json::array SerializeRoads(const model::Map::Roads& roads) {
    json::array roads_json;
    for (const auto& road : roads) {
        roads_json.push_back(SerializeRoad(road));
    }
    return roads_json;
}

json::array SerializeBuildings(const model::Map::Buildings& buildings) {
    json::array buildings_json;
    for (const auto& building : buildings) {
        buildings_json.push_back(SerializeBuilding(building));
    }
    return buildings_json;
}

json::array SerializeOffices(const model::Map::Offices& offices) {
    json::array offices_json;
    for (const auto& office : offices) {
        offices_json.push_back(SerializeOffice(office));
    }
    return offices_json;
}

json::object SerializeMap(const model::Map& map) {
    json::object map_json;
    map_json["id"] = *map.GetId();
    map_json["name"] = map.GetName();
    map_json["roads"] = SerializeRoads(map.GetRoads());
    map_json["buildings"] = SerializeBuildings(map.GetBuildings());
    map_json["offices"] = SerializeOffices(map.GetOffices());
    return map_json;
}

}  // namespace


template <typename Body, typename Allocator, typename Send>
void RequestHandler::operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
    if (req.target().starts_with("/api/")) {
        HandleApiRequest(std::forward<decltype(req)>(req), std::forward<Send>(send));
    } else {
        send(MakeErrorResponse(req, http::status::not_found, "notFound", "Not found"));
    }
}

template <typename Body, typename Allocator, typename Send>
void RequestHandler::HandleApiRequest(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
    if (req.method() != http::verb::get) {
        send(MakeErrorResponse(req, http::status::method_not_allowed, "methodNotAllowed", "Only GET method is allowed"));
        return;
    }

    std::string_view target = req.target();

    if (target == "/api/v1/maps") {
        HandleGetMapsList(std::forward<decltype(req)>(req), std::forward<Send>(send));
    } else if (target.starts_with("/api/v1/maps/")) {
        HandleGetMap(std::forward<decltype(req)>(req), std::forward<Send>(send));
    } else {
        send(MakeErrorResponse(req, http::status::bad_request, "badRequest", "Bad request"));
    }
}

template <typename Body, typename Allocator, typename Send>
void RequestHandler::HandleGetMapsList(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
    json::array maps_json;

    for (const auto& map : game_.GetMaps()) {
        json::object map_info;
        map_info["id"] = *map.GetId();
        map_info["name"] = map.GetName();
        maps_json.push_back(std::move(map_info));
    }

    http::response<http::string_body> response{http::status::ok, req.version()};
    response.set(http::field::content_type, "application/json");
    response.body() = json::serialize(maps_json);
    response.prepare_payload();

    send(std::move(response));
}

template <typename Body, typename Allocator, typename Send>
void RequestHandler::HandleGetMap(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
    std::string map_id = ExtractMapId(req.target());

    if (map_id.empty()) {
        send(MakeErrorResponse(req, http::status::bad_request, "badRequest", "Invalid map ID"));
        return;
    }

    const model::Map* map = game_.FindMap(model::Map::Id(map_id));
    if (!map) {
        send(MakeErrorResponse(req, http::status::not_found, "mapNotFound", "Map not found"));
        return;
    }

    json::object map_json = SerializeMap(*map);
    
    http::response<http::string_body> response{http::status::ok, req.version()};
    response.set(http::field::content_type, "application/json");
    response.body() = json::serialize(map_json);
    response.prepare_payload();
    
    send(std::move(response));
}

template void RequestHandler::operator()<http::string_body, std::allocator<char>, std::function<void(http::response<http::string_body>&&)>>(
    http::request<http::string_body, http::basic_fields<std::allocator<char>>>&&, 
    std::function<void(http::response<http::string_body>&&)>&&);

template void RequestHandler::HandleApiRequest<http::string_body, std::allocator<char>, std::function<void(http::response<http::string_body>&&)>>(
    http::request<http::string_body, http::basic_fields<std::allocator<char>>>&&, 
    std::function<void(http::response<http::string_body>&&)>&&);

template void RequestHandler::HandleGetMapsList<http::string_body, std::allocator<char>, std::function<void(http::response<http::string_body>&&)>>(
    http::request<http::string_body, http::basic_fields<std::allocator<char>>>&&, 
    std::function<void(http::response<http::string_body>&&)>&&);

template void RequestHandler::HandleGetMap<http::string_body, std::allocator<char>, std::function<void(http::response<http::string_body>&&)>>(
    http::request<http::string_body, http::basic_fields<std::allocator<char>>>&&, 
    std::function<void(http::response<http::string_body>&&)>&&);

}  // namespace http_handler
