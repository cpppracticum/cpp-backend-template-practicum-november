#include "json_loader.h"
#include <fstream>
#include <sstream>
#include <boost/json.hpp>

namespace json_loader {

    namespace json = boost::json;

    using namespace std::literals;

    namespace {

        model::Point ParsePoint(const json::object& obj) {
            return { obj.at("x").as_int64(), obj.at("y").as_int64() };
        }

        model::Road ParseRoad(const json::object& road_obj) {
            if (road_obj.contains("x0") && road_obj.contains("x1")) {
                // Горизонтальная дорога
                auto start = model::Point{ road_obj.at("x0").as_int64(), road_obj.at("y0").as_int64() };
                return model::Road{ model::Road::HORIZONTAL, start, road_obj.at("x1").as_int64() };
            }
            else if (road_obj.contains("y0") && road_obj.contains("y1")) {
                // Вертикальная дорога
                auto start = model::Point{ road_obj.at("x0").as_int64(), road_obj.at("y0").as_int64() };
                return model::Road{ model::Road::VERTICAL, start, road_obj.at("y1").as_int64() };
            }
            throw std::runtime_error("Invalid road format");
        }

        model::Building ParseBuilding(const json::object& building_obj) {
            auto pos = model::Point{ building_obj.at("x").as_int64(), building_obj.at("y").as_int64() };
            auto size = model::Size{ building_obj.at("w").as_int64(), building_obj.at("h").as_int64() };
            return model::Building{ model::Rectangle{pos, size} };
        }

        model::Office ParseOffice(const json::object& office_obj) {
            auto id = model::Office::Id{ std::string(office_obj.at("id").as_string()) };
            auto pos = model::Point{ office_obj.at("x").as_int64(), office_obj.at("y").as_int64() };
            auto offset = model::Offset{ office_obj.at("offsetX").as_int64(), office_obj.at("offsetY").as_int64() };
            return model::Office{ id, pos, offset };
        }

        // Новая функция для загрузки типов лута
        void LoadMapLootTypes(const json::value& map_json, model::Map& map) {
            if (!map_json.as_object().contains("lootTypes")) {
                return;
            }

            const auto& loot_types_array = map_json.at("lootTypes").as_array();

            // Пока просто создаем несколько тестовых предметов на основе типов
            // В реальной реализации здесь должна быть логика генерации предметов
            int loot_id = 0;
            for (const auto& loot_type : loot_types_array) {
                const auto& loot_obj = loot_type.as_object();

                // Создаем несколько предметов каждого типа для тестирования
                for (int i = 0; i < 3; ++i) {
                    // Случайная позиция на карте (упрощенно)
                    model::Position pos{ 10.0 + i * 5.0, 10.0 + loot_id * 3.0 };

                    // Получаем стоимость предмета из конфигурации
                    double value = 10.0; // значение по умолчанию
                    if (loot_obj.contains("value")) {
                        value = loot_obj.at("value").as_double();
                    }

                    model::LootItem item(
                        model::LootItem::Id{ loot_id++ },
                        loot_id % 5 + 1, // тип предмета (1-5)
                        value,
                        pos
                    );
                    map.AddLootItem(std::move(item));
                }
            }
        }

        // Загрузка конфигурации вместимости рюкзака для игры
        void LoadBagCapacityConfig(const json::value& config, model::Game& game) {
            if (config.as_object().contains("defaultBagCapacity")) {
                int default_capacity = config.at("defaultBagCapacity").as_int64();
                game.SetDefaultBagCapacity(default_capacity);
            }
        }

        // Загрузка конфигурации вместимости рюкзака для конкретной карты
        void LoadMapSpecificBagCapacity(const json::value& map_json, model::Map& map) {
            if (map_json.as_object().contains("bagCapacity")) {
                int capacity = map_json.at("bagCapacity").as_int64();
                map.SetBagCapacity(capacity);
            }
        }

        // Загрузка конфигурации генератора лута
        void LoadLootGeneratorConfig(const json::value& config, model::Game& game) {
            // Пока просто заглушка - в реальной реализации здесь будет логика
            // для настройки генератора предметов на основе period и probability
            if (config.as_object().contains("lootGeneratorConfig")) {
                const auto& loot_config = config.at("lootGeneratorConfig").as_object();
                // Можно сохранить эти параметры для будущего использования
                // double period = loot_config.at("period").as_double();
                // double probability = loot_config.at("probability").as_double();
            }
        }

    }  // namespace

    model::Game LoadGame(const std::filesystem::path& json_path) {
        // Загрузить содержимое файла
        std::ifstream file(json_path);
        if (!file) {
            throw std::runtime_error("Failed to open json file: " + json_path.string());
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string json_content = buffer.str();

        // Распарсить JSON
        json::value json_value = json::parse(json_content);
        const auto& config = json_value.as_object();

        model::Game game;

        // Загрузить общие настройки
        if (config.contains("defaultDogSpeed")) {
            game.SetDefaultDogSpeed(config.at("defaultDogSpeed").as_double());
        }

        // Загрузить конфигурацию вместимости рюкзака
        LoadBagCapacityConfig(config, game);

        // Загрузить конфигурацию генератора лута
        LoadLootGeneratorConfig(config, game);

        // Загрузить карты
        const auto& maps_array = config.at("maps").as_array();
        for (const auto& map_json : maps_array) {
            const auto& map_obj = map_json.as_object();

            auto map_id = model::Map::Id{ std::string(map_obj.at("id").as_string()) };
            auto map_name = std::string(map_obj.at("name").as_string());

            model::Map map{ map_id, map_name };

            // Задать скорость собаки для карты, если указана
            if (map_obj.contains("dogSpeed")) {
                map.SetDogSpeed(map_obj.at("dogSpeed").as_double());
            }

            // Задать вместимость рюкзака для карты, если указана
            LoadMapSpecificBagCapacity(map_json, map);
            // Если для карты не указана вместимость, используется значение по умолчанию из игры
            if (!map_obj.contains("bagCapacity")) {
                map.SetDefaultBagCapacity(game.GetDefaultBagCapacity());
            }

            // Загрузить дороги
            const auto& roads_array = map_obj.at("roads").as_array();
            for (const auto& road_json : roads_array) {
                map.AddRoad(ParseRoad(road_json.as_object()));
            }

            // Загрузить здания
            const auto& buildings_array = map_obj.at("buildings").as_array();
            for (const auto& building_json : buildings_array) {
                map.AddBuilding(ParseBuilding(building_json.as_object()));
            }

            // Загрузить офисы
            const auto& offices_array = map_obj.at("offices").as_array();
            for (const auto& office_json : offices_array) {
                map.AddOffice(ParseOffice(office_json.as_object()));
            }

            // Загрузить типы лута и создать предметы
            LoadMapLootTypes(map_json, map);

            // Добавить карту в игру
            game.AddMap(std::move(map));
        }

        return game;
    }

}  // namespace json_loader