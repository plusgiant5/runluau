#pragma once

#include <optional>

#include <luau.h>

namespace runluau {
	struct settings {
		uint8_t O = 1;
		uint8_t g = 1;
		std::optional<std::vector<std::string>> script_args = std::nullopt;

		std::optional<std::vector<std::string>> plugins = std::nullopt;
	};
	std::string compile(const std::string& source, settings& settings);
	void execute_bytecode(const std::string& bytecode, settings& settings);
	void execute(const std::string& source, settings& settings);
}