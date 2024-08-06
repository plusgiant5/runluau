#pragma once

#include <Luau/Compiler.h>
#include <lualib.h>

namespace runluau {
	std::string compile(const std::string& source, uint8_t O, uint8_t g);
	void execute_bytecode(const std::string& bytecode, std::vector<std::string> args);
	void execute(const std::string& source, uint8_t O, uint8_t g, std::vector<std::string> args);
}