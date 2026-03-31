#include "json_loader.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

#include <boost/json.hpp>

namespace json = boost::json;
namespace model {

// Вспомогательные функции для парсинга отдельных объектов
Road ParseRoad(const json::object& road_obj) {
    int x0 = road_obj.at("x0").as_int64();
    int y0 = road_obj.at("y0").as_int64();
    
    if (road_obj.contains("x1")) {
        int x1 = road_obj.at("x1").as_int64();
        return Road{Road::Horizontal, {x0, y0}, x1};
    } else if (road_obj.contains("y1")) {
        int y1 = road_obj.at("y1").as_int64();
        return Road{Road::Vertical, {x0, y0}, y1};
    }
    
    throw std::runtime_error("Invalid road format");
}

Building ParseBuilding(const json::object& building_obj) {
    int x = building_obj.at("x").as_int64();
    int y = building_obj.at("y").as_int64();
    int w = building_obj.at("w").as_int64();
    int h = building_obj.at("h").as_int64();
    
    return Building{Rectangle{x, y, w, h}};
}

Office ParseOffice(const json::object& office_obj) {
    std::string id = std::string(office_obj.at("id").as_string());
    int x = office_obj.at("x").as_int64();
    int y = office_obj.at("y").as_int64();
    int offset_x = office_obj.at("offsetX").as_int64();
    int offset_y = office_obj.at("offsetY").as_int64();
    
    return Office{id, {x, y}, {offset_x, offset_y}};
}

// Основная функция загрузки карт
void LoadMaps(const std::string& config_path, Game& game) {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + config_path);
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    
    json::value config = json::parse(content);
    const auto& maps_array = config.as_object().at("maps").as_array();
    
    for (const auto& map_obj_value : maps_array) {
        const auto& map_obj = map_obj_value.as_object();
        
        std::string map_id = std::string(map_obj.at("id").as_string());
        std::string map_name = std::string(map_obj.at("name").as_string());
        Map map(map_id, map_name);
        
        // Парсинг дорог
        if (map_obj.contains("roads")) {
            const auto& roads_array = map_obj.at("roads").as_array();
            for (const auto& road_obj : roads_array) {
                map.AddRoad(ParseRoad(road_obj.as_object()));
            }
        }
        
        // Парсинг зданий
        if (map_obj.contains("buildings")) {
            const auto& buildings_array = map_obj.at("buildings").as_array();
            for (const auto& building_obj : buildings_array) {
                map.AddBuilding(ParseBuilding(building_obj.as_object()));
            }
        }
        
        // Парсинг офисов
        if (map_obj.contains("offices")) {
            const auto& offices_array = map_obj.at("offices").as_array();
            for (const auto& office_obj : offices_array) {
                map.AddOffice(ParseOffice(office_obj.as_object()));
            }
        }
        
        game.AddMap(std::move(map));
    }
}

} // namespace model
