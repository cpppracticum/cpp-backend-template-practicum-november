#include "request_handler.h"

#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <iostream>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

void RequestHandler::operator()(http::request<http::string_body>&& req, std::function<void(http::response<http::string_body>&&)>&& send) {
    std::string target(req.target().data(), req.target().size());
    std::cout << "=== Request received ===" << std::endl;
    std::cout << "Target: " << target << std::endl;
    
    http::response<http::string_body> response;
    
    if (target == "/api/v1/maps") {
        response = HandleMapsRequest();
    }
    else if (target.size() > 13 && target.substr(0, 13) == "/api/v1/maps/") {
        std::string map_id = target.substr(13);
        response = HandleMapRequest(map_id);
    }
    else if (target.size() >= 4 && target.substr(0, 4) == "/api") {
        response = MakeErrorResponse(http::status::bad_request, "badRequest", "Bad request");
    }
    else {
        response = MakeErrorResponse(http::status::not_found, "notFound", "Not found");
    }
    
    send(std::move(response));
}

http::response<http::string_body> RequestHandler::HandleMapsRequest() {
    json::array maps_array;
    
    for (const auto& map : game_.GetMaps()) {
        json::object map_obj;
        map_obj["id"] = *map.GetId();
        map_obj["name"] = map.GetName();
        maps_array.push_back(std::move(map_obj));
    }
    
    std::string body = json::serialize(maps_array);
    
    http::response<http::string_body> response(http::status::ok, 11);
    response.set(http::field::content_type, "application/json");
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(false);
    
    return response;
}

http::response<http::string_body> RequestHandler::HandleMapRequest(const std::string& map_id_str) {
    model::Map::Id map_id(map_id_str);
    const model::Map* map = game_.FindMap(map_id);
    
    if (!map) {
        return MakeErrorResponse(http::status::not_found, "mapNotFound", "Map not found");
    }
    
    json::object map_obj;
    map_obj["id"] = *map->GetId();
    map_obj["name"] = map->GetName();
    
    json::array roads_array;
    for (const auto& road : map->GetRoads()) {
        json::object road_obj;
        auto start = road.GetStart();
        auto end = road.GetEnd();
        road_obj["x0"] = start.x;
        road_obj["y0"] = start.y;
        
        if (road.IsHorizontal()) {
            road_obj["x1"] = end.x;
        } else {
            road_obj["y1"] = end.y;
        }
        roads_array.push_back(std::move(road_obj));
    }
    map_obj["roads"] = std::move(roads_array);
    
    json::array buildings_array;
    for (const auto& building : map->GetBuildings()) {
        auto bounds = building.GetBounds();
        json::object building_obj;
        building_obj["x"] = bounds.position.x;
        building_obj["y"] = bounds.position.y;
        building_obj["w"] = bounds.size.width;
        building_obj["h"] = bounds.size.height;
        buildings_array.push_back(std::move(building_obj));
    }
    map_obj["buildings"] = std::move(buildings_array);
    
    json::array offices_array;
    for (const auto& office : map->GetOffices()) {
        json::object office_obj;
        office_obj["id"] = *office.GetId();
        office_obj["x"] = office.GetPosition().x;
        office_obj["y"] = office.GetPosition().y;
        office_obj["offsetX"] = office.GetOffset().dx;
        office_obj["offsetY"] = office.GetOffset().dy;
        offices_array.push_back(std::move(office_obj));
    }
    map_obj["offices"] = std::move(offices_array);
    
    std::string body = json::serialize(map_obj);
    
    http::response<http::string_body> response(http::status::ok, 11);
    response.set(http::field::content_type, "application/json");
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(false);
    
    return response;
}

http::response<http::string_body> RequestHandler::MakeErrorResponse(
    http::status status, 
    const std::string& code, 
    const std::string& message) {
    
    json::object error_obj;
    error_obj["code"] = code;
    error_obj["message"] = message;
    
    std::string body = json::serialize(error_obj);
    
    http::response<http::string_body> response(status, 11);
    response.set(http::field::content_type, "application/json");
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(false);
    
    return response;
}

} // namespace http_handler
