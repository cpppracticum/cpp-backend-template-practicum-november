#pragma once
#include "http_server.h"
#include "model.h"
#include <boost/beast/http.hpp>
#include <boost/json.hpp>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        
        if (req.target().starts_with("/api/")) {
            HandleApiRequest(std::forward<decltype(req)>(req), std::forward<Send>(send));
        } else {
            send(MakeErrorResponse(req, http::status::not_found, "notFound", "Not found"));
        }
    }

private:
    template <typename Body, typename Allocator, typename Send>
    void HandleApiRequest(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
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
    void HandleGetMapsList(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
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
    void HandleGetMap(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
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
        
        json::object map_json;
        map_json["id"] = *map->GetId();
        map_json["name"] = map->GetName();
        
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

    template<typename Request>
    http::response<http::string_body> MakeErrorResponse(const Request& req, 
                                                       http::status status, 
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
        return std::string(path.substr(prefix.size()));
    }

    model::Game& game_;
};

}  // namespace http_handler
