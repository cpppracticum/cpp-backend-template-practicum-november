#include "request_handler.h"
#include <boost/beast/http.hpp>
#include <boost/json.hpp>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

namespace {

// Функция для создания JSON-ответа с ошибкой
http::response<http::string_body> MakeErrorResponse(http::status status, 
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

// Функция для извлечения ID карты из пути
std::string ExtractMapId(std::string_view path) {
    const std::string prefix = "/api/v1/maps/";
    if (path.size() <= prefix.size()) {
        return "";
    }
    return std::string(path.substr(prefix.size()));
}

}  // namespace

template <typename Body, typename Allocator, typename Send>
void RequestHandler::operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
    // Проверяем, что запрос начинается с /api/
    if (req.target().starts_with("/api/")) {
        HandleApiRequest(std::forward<decltype(req)>(req), std::forward<Send>(send));
    } else {
        // Для не-API запросов возвращаем 404
        send(MakeErrorResponse(http::status::not_found, "notFound", "Not found"));
    }
}

template <typename Body, typename Allocator, typename Send>
void RequestHandler::HandleApiRequest(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
    if (req.method() != http::verb::get) {
        send(MakeErrorResponse(http::status::method_not_allowed, "methodNotAllowed", "Only GET method is allowed"));
        return;
    }
    
    std::string_view target = req.target();
    
    if (target == "/api/v1/maps") {
        HandleGetMapsList(req, send);
    } else if (target.starts_with("/api/v1/maps/")) {
        HandleGetMap(req, send);
    } else {
        send(MakeErrorResponse(http::status::bad_request, "badRequest", "Bad request"));
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
        send(MakeErrorResponse(http::status::bad_request, "badRequest", "Invalid map ID"));
        return;
    }
    
    const model::Map* map = game_.FindMap(model::Map::Id(map_id));
    if (!map) {
        send(MakeErrorResponse(http::status::not_found, "mapNotFound", "Map not found"));
        return;
    }
    
    json::object map_json;
    map_json["id"] = *map->GetId();
    map_json["name"] = map->GetName();
    
    // Добавляем дороги
    json::array roads_json;
    for (const auto& road : map->GetRoads()) {
        json::object road_json;
        road_json["x0"] = road.GetStart().x;
        road_json["y0"] = road.GetStart().y;
        
        if (road.IsHorizontal()) {
            road_json["x1"] = road.GetEnd().x;
        } else {
            road_json["y1"] = road.GetEnd().y;
        }
        
        roads_json.push_back(std::move(road_json));
    }
    map_json["roads"] = roads_json;
    
    // Добавляем здания
    json::array buildings_json;
    for (const auto& building : map->GetBuildings()) {
        json::object building_json;
        const auto& bounds = building.GetBounds();
        building_json["x"] = bounds.position.x;
        building_json["y"] = bounds.position.y;
        building_json["w"] = bounds.size.width;
        building_json["h"] = bounds.size.height;
        buildings_json.push_back(std::move(building_json));
    }
    map_json["buildings"] = buildings_json;
    
    // Добавляем офисы
    json::array offices_json;
    for (const auto& office : map->GetOffices()) {
        json::object office_json;
        office_json["id"] = *office.GetId();
        office_json["x"] = office.GetPosition().x;
        office_json["y"] = office.GetPosition().y;
        office_json["offsetX"] = office.GetOffset().dx;
        office_json["offsetY"] = office.GetOffset().dy;
        offices_json.push_back(std::move(office_json));
    }
    map_json["offices"] = offices_json;
    
    http::response<http::string_body> response{http::status::ok, req.version()};
    response.set(http::field::content_type, "application/json");
    response.body() = json::serialize(map_json);
    response.prepare_payload();
    
    send(std::move(response));
}

} 
