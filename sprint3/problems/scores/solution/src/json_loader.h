#pragma once

#include <filesystem>
#include <boost/json.hpp>

#include "model.h"

namespace json_loader {

	model::Game LoadGame(const std::filesystem::path& json_path);

	// Вспомогательные функции для загрузки конфигурации
	void LoadMapLootTypes(const boost::json::value& map_json, model::Map& map);
	void LoadBagCapacityConfig(const boost::json::value& config, model::Game& game);
	void LoadLootGeneratorConfig(const boost::json::value& config, model::Game& game);
	void LoadMapSpecificBagCapacity(const boost::json::value& map_json, model::Map& map);

}  // namespace json_loader