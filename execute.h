#pragma once

#include <Luau/Compiler.h>
#include <lualib.h>

namespace runluau {
	struct settings {
		uint8_t O = 1;
		uint8_t g = 1;
		std::vector<std::string> script_args;
	};
	std::string compile(const std::string& source, settings& settings);
	void execute_bytecode(const std::string& bytecode, settings& settings);
	void execute(const std::string& source, settings& settings);
}