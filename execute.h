#pragma once

#include <optional>
#include <unordered_set>

#include <luau.h>

#include "file.h"

namespace runluau {
	struct settings_run_build {
		uint8_t O = 1;
		uint8_t g = 1;
		std::optional<std::vector<std::string>> script_args = std::nullopt;

		std::optional<std::vector<std::string>> plugins = std::nullopt;
	};
	void execute_bytecode(const std::string& bytecode, settings_run_build& settings, fs::path path, std::optional<std::unordered_set<std::string>> plugins_to_load = std::nullopt);
	void execute(const std::string& source, settings_run_build& settings, fs::path path, std::optional<std::unordered_set<std::string>> plugins_to_load = std::nullopt);
	std::string compile(const std::string& source, const int O, const int g);
}