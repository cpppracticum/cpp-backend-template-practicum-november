#include "request_handler.h"

#include <sstream>
#include <iostream>

#include <boost/json.hpp>

#include "constants.h"
#include "model/game.h"
#include "model/map.h"

namespace json = boost::json;

// Вспомогательные функции для сериализации
json::array SerializeRoads(const std::vector<model::Road>& roads) {
    json::array result;
    for (const auto& road : roads) {
        json::object road_obj;
        road_obj["x0"] = road.GetStart().x;
        road_obj["y0"] = road.GetStart().y;
        
        if (road.GetType() == model::Road::Horizontal) {
            road_obj["x1"] = road.GetEnd().x;
        } else {
            road_obj["y1"] = road.GetEnd().y;
        }
        
        result.push_back(std::move(road_obj));
    }
    return result;
}

json::array SerializeBuildings(const std::vector<model::Building>& buildings) {
    json::array result;
    for (const auto& building : buildings) {
        const auto& rect = building.GetBounds();
        json::object building_obj;
        building_obj["x"] = rect.x;
        building_obj["y"] = rect.y;
        building_obj["w"] = rect.w;
        building_obj["h"] = rect.h;
        result.push_back(std::move(building_obj));
    }
    return result;
}

json::array SerializeOffices(const std::vector<model::Office>& offices) {
    json::array result;
    for (const auto& office : offices) {
        json::object office_obj;
        office_obj["id"] = office.GetId();
        office_obj["x"] = office.GetPosition().x;
        office_obj["y"] = office.GetPosition().y;
        office_obj["offsetX"] = office.GetOffset().x;
        office_obj["offsetY"] = office.GetOffset().y;
        result.push_back(std::move(office_obj));
    }
    return result;
}

// Основной обработчик запросов
http::response<http::string_body> HandleRequest(
    const http::request<http::string_body>& req,
    const model::Game& game) {
    
    http::response<http::string_body> response;
    response.version(req.version());
    response.keep_alive(false);
    
    std::string target(req.target());
    
    // Обработка эндпоинта /api/v1/maps
    if (target == endpoints::MAPS) {
        json::array maps_array;
        
        for (const auto& map : game.GetMaps()) {
            json::object map_obj;
            map_obj["id"] = map.GetId();
            map_obj["name"] = map.GetName();
            maps_array.push_back(std::move(map_obj));
        }
        
        response.result(http::status::ok);
        response.set(http::field::content_type, "application/json");
        response.body() = json::serialize(maps_array);
    }
    // Обработка эндпоинта /api/v1/maps/{id}
    else if (target.starts_with(endpoints::MAP_BY_ID)) {
        std::string map_id = target.substr(endpoints::MAP_BY_ID.size());
        const model::Map* found_map = nullptr;
        
        for (const auto& map : game.GetMaps()) {
            if (map.GetId() == map_id) {
                found_map = &map;
                break;
            }
        }
        
        if (!found_map) {
            response.result(http::status::not_found);
            response.body() = R"({"code":"mapNotFound","message":"Map not found"})";
        } else {
            json::object map_obj;
            map_obj["id"] = found_map->GetId();
            map_obj["name"] = found_map->GetName();
            map_obj["roads"] = SerializeRoads(found_map->GetRoads());
            map_obj["buildings"] = SerializeBuildings(found_map->GetBuildings());
            map_obj["offices"] = SerializeOffices(found_map->GetOffices());
            
            response.result(http::status::ok);
            response.set(http::field::content_type, "application/json");
            response.body() = json::serialize(map_obj);
        }
    }
    // Неизвестный эндпоинт
    else {
        response.result(http::status::not_found);
        response.body() = R"({"code":"notFound","message":"Endpoint not found"})";
    }
    
    response.set(http::field::content_length, response.body().size());
    response.prepare_payload();
    
    return response;
}
