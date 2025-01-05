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
				if (luaurc->aliases.find(key) != luaurc->aliases.end()) {
					throw std::runtime_error(std::string("Alias \"@") + key + "\" was defined more than once");
				}
				luaurc->aliases.insert({key, value});
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