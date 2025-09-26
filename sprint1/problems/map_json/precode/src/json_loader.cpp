#include "json_loader.h"
#include <fstream>
#include <sstream>
#include <boost/json.hpp>

namespace json_loader {

namespace json = boost::json;

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла
    std::ifstream file(json_path);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + json_path.string());
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    // Распарсить JSON
    auto value = json::parse(content);
    auto& obj = value.as_object();
    
    model::Game game;
    
    // Обработать карты
    if (obj.contains("maps")) {
        auto& maps_array = obj.at("maps").as_array();
        for (auto& map_value : maps_array) {
            auto& map_obj = map_value.as_object();
            
            // Извлечь id и name карты
            std::string id = json::value_to<std::string>(map_obj.at("id"));
            std::string name = json::value_to<std::string>(map_obj.at("name"));
            
            model::Map map(model::Map::Id(id), name);
            
            // Добавить дороги
            if (map_obj.contains("roads")) {
                auto& roads_array = map_obj.at("roads").as_array();
                for (auto& road_value : roads_array) {
                    auto& road_obj = road_value.as_object();
                    
                    if (road_obj.contains("x1")) {
                        // Горизонтальная дорога
                        model::Coord x0 = road_obj.at("x0").as_int64();
                        model::Coord y0 = road_obj.at("y0").as_int64();
                        model::Coord x1 = road_obj.at("x1").as_int64();
                        map.AddRoad(model::Road(model::Road::HORIZONTAL, 
                                              {x0, y0}, x1));
                    } else if (road_obj.contains("y1")) {
                        // Вертикальная дорога
                        model::Coord x0 = road_obj.at("x0").as_int64();
                        model::Coord y0 = road_obj.at("y0").as_int64();
                        model::Coord y1 = road_obj.at("y1").as_int64();
                        map.AddRoad(model::Road(model::Road::VERTICAL, 
                                              {x0, y0}, y1));
                    }
                }
            }
            
            // Добавить здания
            if (map_obj.contains("buildings")) {
                auto& buildings_array = map_obj.at("buildings").as_array();
                for (auto& building_value : buildings_array) {
                    auto& building_obj = building_value.as_object();
                    
                    model::Coord x = building_obj.at("x").as_int64();
                    model::Coord y = building_obj.at("y").as_int64();
                    model::Dimension w = building_obj.at("w").as_int64();
                    model::Dimension h = building_obj.at("h").as_int64();
                    
                    map.AddBuilding(model::Building({{x, y}, {w, h}}));
                }
            }
            
            // Добавить офисы
            if (map_obj.contains("offices")) {
                auto& offices_array = map_obj.at("offices").as_array();
                for (auto& office_value : offices_array) {
                    auto& office_obj = office_value.as_object();
                    
                    std::string id = json::value_to<std::string>(office_obj.at("id"));
                    model::Coord x = office_obj.at("x").as_int64();
                    model::Coord y = office_obj.at("y").as_int64();
                    model::Offset offsetX = office_obj.at("offsetX").as_int64();
                    model::Offset offsetY = office_obj.at("offsetY").as_int64();
                    
                    map.AddOffice(model::Office(model::Office::Id(id), 
                                              {x, y}, {offsetX, offsetY}));
                }
            }
            
            game.AddMap(std::move(map));
        }
    }
    
    return game;
}

}  // namespace json_loader
