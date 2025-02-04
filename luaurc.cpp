#include "luaurc.h"

#pragma warning ( push )
#pragma warning ( disable: 26819 )
#include "json.hpp"
#pragma warning ( pop )

void read_luaurc(luaurc* luaurc, std::string content) {
	luaurc->aliases = {};
	try {
		auto json = nlohmann::json::parse(content);
		if (json.contains("aliases")) {
			const auto& aliases = json["aliases"];
			if (!aliases.is_object()) {
				throw std::runtime_error(std::string("Expected `aliases` to be object, got ") + aliases.type_name());
			}
			for (const auto& [key, value] : aliases.items()) {
				if (!value.is_string()) {
					throw std::runtime_error(std::string("Expected alias value to be string, got ") + value.type_name());
				}
				if (luaurc->aliases.find(key) != luaurc->aliases.end()) {
					throw std::runtime_error(std::string("Alias \"@") + key + "\" was defined more than once");
				}
				luaurc->aliases.insert({key, value});
			}
		}
		if (json.contains("plugins")) {
			luaurc->plugins_to_load.emplace();
			const auto& plugins = json["plugins"];
			if (!plugins.is_object()) {
				throw std::runtime_error(std::string("Expected `plugins` to be object, got ") + plugins.type_name());
			}
			for (const auto& [key, value] : plugins.items()) {
				if (!value.is_boolean()) {
					throw std::runtime_error(std::string("Expected plugin value to be boolean, got ") + value.type_name());
				}
				if (luaurc->plugins_to_load.value().find(key) != luaurc->plugins_to_load.value().end()) {
					throw std::runtime_error(std::string("Plugin ") + key + " was listed in `plugins` more than once");
				}
				if (value) {
					luaurc->plugins_to_load.value().insert(key);
				}
			}
		}
	} catch (const nlohmann::json::parse_error& error) {
		throw std::runtime_error(std::string("JSON parsing error: ") + error.what());
	} catch (const nlohmann::json::type_error& error) {
		throw std::runtime_error(std::string("JSON type error: ") + error.what());
	} catch (const std::exception& error) {
		throw std::runtime_error(error.what());
	}
}