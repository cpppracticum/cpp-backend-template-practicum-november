#include "json_loader.h"
#include <fstream>
#include <sstream>
#include <boost/json.hpp>

namespace json_loader {

namespace json = boost::json;

namespace {

void ValidateCoordinates(model::Coord x, model::Coord y, const std::string& context) {
    if (x < 0 || y < 0) {
        throw std::runtime_error(context + ": coordinates cannot be negative");
    }
}

void ValidateDimensions(model::Dimension w, model::Dimension h, const std::string& context) {
    if (w <= 0 || h <= 0) {
        throw std::runtime_error(context + ": dimensions must be positive");
    }
}

}  // namespace

model::Game LoadGame(const std::filesystem::path& json_path) {
    std::ifstream file(json_path);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + json_path.string());
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    
    if (content.empty()) {
        throw std::runtime_error("Config file is empty: " + json_path.string());
    }
    
    json::value value;
    try {
        value = json::parse(content);
    } catch (const std::exception& e) {
        throw std::runtime_error("JSON parsing failed: " + std::string(e.what()));
    }
    
    if (!value.is_object()) {
        throw std::runtime_error("Root must be a JSON object");
    }
    
    auto& obj = value.as_object();
    model::Game game;
    
    if (obj.contains("maps")) {
        if (!obj.at("maps").is_array()) {
            throw std::runtime_error("Field 'maps' must be an array");
        }
        
        auto& maps_array = obj.at("maps").as_array();
        for (size_t i = 0; i < maps_array.size(); ++i) {
            auto& map_value = maps_array[i];
            
            if (!map_value.is_object()) {
                throw std::runtime_error("Map #" + std::to_string(i) + " must be an object");
            }
            
            auto& map_obj = map_value.as_object();
            
            if (!map_obj.contains("id") || !map_obj.contains("name") || !map_obj.contains("roads")) {
                throw std::runtime_error("Map #" + std::to_string(i) + " missing required fields (id, name, or roads)");
            }
            
            std::string id = json::value_to<std::string>(map_obj.at("id"));
            std::string name = json::value_to<std::string>(map_obj.at("name"));
            
            if (id.empty()) {
                throw std::runtime_error("Map #" + std::to_string(i) + " has empty id");
            }
            
            model::Map map(model::Map::Id(id), name);
            
            if (map_obj.contains("roads")) {
                if (!map_obj.at("roads").is_array()) {
                    throw std::runtime_error("Map '" + id + "': roads must be an array");
                }
                
                auto& roads_array = map_obj.at("roads").as_array();
                if (roads_array.empty()) {
                    throw std::runtime_error("Map '" + id + "': roads array cannot be empty");
                }
                
                for (size_t j = 0; j < roads_array.size(); ++j) {
                    auto& road_value = roads_array[j];
                    
                    if (!road_value.is_object()) {
                        throw std::runtime_error("Map '" + id + "', road #" + std::to_string(j) + " must be an object");
                    }
                    
                    auto& road_obj = road_value.as_object();
                    
                    // Проверить наличие обязательных полей для дороги
                    if (!road_obj.contains("x0") || !road_obj.contains("y0")) {
                        throw std::runtime_error("Map '" + id + "', road #" + std::to_string(j) + " missing x0 or y0");
                    }
                    
                    try {
                        if (road_obj.contains("x1")) {
                            // Горизонтальная дорога
                            model::Coord x0 = road_obj.at("x0").as_int64();
                            model::Coord y0 = road_obj.at("y0").as_int64();
                            model::Coord x1 = road_obj.at("x1").as_int64();
                            
                            ValidateCoordinates(x0, y0, "Map '" + id + "', road #" + std::to_string(j));
                            if (x1 < x0) {
                                throw std::runtime_error("Map '" + id + "', road #" + std::to_string(j) + ": x1 must be >= x0 for horizontal road");
                            }
                            
                            map.AddRoad(model::Road(model::Road::HORIZONTAL, {x0, y0}, x1));
                        } else if (road_obj.contains("y1")) {
                            
                            model::Coord x0 = road_obj.at("x0").as_int64();
                            model::Coord y0 = road_obj.at("y0").as_int64();
                            model::Coord y1 = road_obj.at("y1").as_int64();
                            
                            ValidateCoordinates(x0, y0, "Map '" + id + "', road #" + std::to_string(j));
                            if (y1 < y0) {
                                throw std::runtime_error("Map '" + id + "', road #" + std::to_string(j) + ": y1 must be >= y0 for vertical road");
                            }
                            
                            map.AddRoad(model::Road(model::Road::VERTICAL, {x0, y0}, y1));
                        } else {
                            throw std::runtime_error("Map '" + id + "', road #" + std::to_string(j) + " must have either x1 or y1");
                        }
                    } catch (const std::exception& e) {
                        throw std::runtime_error(std::string(e.what()));
                    }
                }
            }
            
            if (map_obj.contains("buildings")) {
                if (!map_obj.at("buildings").is_array()) {
                    throw std::runtime_error("Map '" + id + "': buildings must be an array");
                }
                
                auto& buildings_array = map_obj.at("buildings").as_array();
                for (size_t j = 0; j < buildings_array.size(); ++j) {
                    auto& building_value = buildings_array[j];
                    
                    if (!building_value.is_object()) {
                        throw std::runtime_error("Map '" + id + "', building #" + std::to_string(j) + " must be an object");
                    }
                    
                    auto& building_obj = building_value.as_object();
                    
                    if (!building_obj.contains("x") || !building_obj.contains("y") || 
                        !building_obj.contains("w") || !building_obj.contains("h")) {
                        throw std::runtime_error("Map '" + id + "', building #" + std::to_string(j) + " missing required fields (x, y, w, h)");
                    }
                    
                    try {
                        model::Coord x = building_obj.at("x").as_int64();
                        model::Coord y = building_obj.at("y").as_int64();
                        model::Dimension w = building_obj.at("w").as_int64();
                        model::Dimension h = building_obj.at("h").as_int64();
                        
                        ValidateCoordinates(x, y, "Map '" + id + "', building #" + std::to_string(j));
                        ValidateDimensions(w, h, "Map '" + id + "', building #" + std::to_string(j));
                        
                        map.AddBuilding(model::Building({{x, y}, {w, h}}));
                    } catch (const std::exception& e) {
                        throw std::runtime_error(std::string(e.what()));
                    }
                }
            }
            if (map_obj.contains("offices")) {
                if (!map_obj.at("offices").is_array()) {
                    throw std::runtime_error("Map '" + id + "': offices must be an array");
                }
                
                auto& offices_array = map_obj.at("offices").as_array();
                for (size_t j = 0; j < offices_array.size(); ++j) {
                    auto& office_value = offices_array[j];
                    
                    if (!office_value.is_object()) {
                        throw std::runtime_error("Map '" + id + "', office #" + std::to_string(j) + " must be an object");
                    }
                    
                    auto& office_obj = office_value.as_object();
                    
                    if (!office_obj.contains("id") || !office_obj.contains("x") || !office_obj.contains("y") ||
                        !office_obj.contains("offsetX") || !office_obj.contains("offsetY")) {
                        throw std::runtime_error("Map '" + id + "', office #" + std::to_string(j) + " missing required fields");
                    }
                    
                    try {
                        std::string office_id = json::value_to<std::string>(office_obj.at("id"));
                        model::Coord x = office_obj.at("x").as_int64();
                        model::Coord y = office_obj.at("y").as_int64();
                        model::Offset offsetX = office_obj.at("offsetX").as_int64();
                        model::Offset offsetY = office_obj.at("offsetY").as_int64();
                        
                        ValidateCoordinates(x, y, "Map '" + id + "', office #" + std::to_string(j));
                        
                        map.AddOffice(model::Office(model::Office::Id(office_id), {x, y}, {offsetX, offsetY}));
                    } catch (const std::exception& e) {
                        throw std::runtime_error(std::string(e.what()));
                    }
                }
            }
            
            try {
                game.AddMap(std::move(map));
            } catch (const std::exception& e) {
                throw std::runtime_error("Failed to add map '" + id + "': " + e.what());
            }
        }
    } else {
        throw std::runtime_error("Missing required field: maps");
    }
    
    return game;
}

}  // namespace json_loader
