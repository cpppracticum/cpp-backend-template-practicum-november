#include "json_loader.h"

#include <boost/json.hpp>
#include <boost/json/src.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

namespace json_loader {

namespace json = boost::json;

model::Game LoadGame(const std::filesystem::path& json_path) {
    std::cout << "Loading config from: " << json_path << std::endl;
    
    std::ifstream file(json_path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open config file: " + json_path.string());
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    json::error_code ec;
    auto value = json::parse(content, ec);
    if (ec) {
        throw std::runtime_error("JSON parse error: " + ec.message());
    }
    
    auto& obj = value.as_object();
    model::Game game;
    
    if (obj.contains("maps")) {
        auto& maps_array = obj.at("maps").as_array();
        std::cout << "Maps array size: " << maps_array.size() << std::endl;
        
        for (const auto& map_value : maps_array) {
            auto& map_obj = map_value.as_object();
            
            model::Map::Id map_id(std::string(map_obj.at("id").as_string()));
            std::string map_name = std::string(map_obj.at("name").as_string());
            model::Map map(map_id, map_name);
            
            // Парсинг roads
            if (map_obj.contains("roads")) {
                auto& roads_array = map_obj.at("roads").as_array();
                for (const auto& road_value : roads_array) {
                    auto& road_obj = road_value.as_object();
                    int x0 = road_obj.at("x0").as_int64();
                    int y0 = road_obj.at("y0").as_int64();
                    
                    if (road_obj.contains("x1")) {
                        int x1 = road_obj.at("x1").as_int64();
                        map.AddRoad(model::Road(model::Road::HORIZONTAL, {x0, y0}, x1));
                    } else if (road_obj.contains("y1")) {
                        int y1 = road_obj.at("y1").as_int64();
                        map.AddRoad(model::Road(model::Road::VERTICAL, {x0, y0}, y1));
                    }
                }
            }
            
            // Парсинг buildings
            if (map_obj.contains("buildings")) {
                auto& buildings_array = map_obj.at("buildings").as_array();
                for (const auto& building_value : buildings_array) {
                    auto& building_obj = building_value.as_object();
                    int x = building_obj.at("x").as_int64();
                    int y = building_obj.at("y").as_int64();
                    int w = building_obj.at("w").as_int64();
                    int h = building_obj.at("h").as_int64();
                    
                    model::Rectangle rect;
                    rect.position = {x, y};
                    rect.size = {w, h};
                    map.AddBuilding(model::Building(rect));
                }
            }
            
            // Парсинг offices
            if (map_obj.contains("offices")) {
                auto& offices_array = map_obj.at("offices").as_array();
                for (const auto& office_value : offices_array) {
                    auto& office_obj = office_value.as_object();
                    std::string id = std::string(office_obj.at("id").as_string());
                    int x = office_obj.at("x").as_int64();
                    int y = office_obj.at("y").as_int64();
                    int offsetX = office_obj.at("offsetX").as_int64();
                    int offsetY = office_obj.at("offsetY").as_int64();
                    
                    map.AddOffice(model::Office(
                        model::Office::Id(id),
                        {x, y},
                        {offsetX, offsetY}
                    ));
                }
            }
            
            game.AddMap(std::move(map));
        }
    }
    
    return game;
}

}  // namespace json_loader
