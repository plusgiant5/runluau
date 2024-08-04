#pragma once

#include <Luau/Compiler.h>
#include <lua.h>

namespace runluau {
	std::string compile(const std::string& source, uint8_t O, uint8_t g);
	void execute_bytecode(const std::string& bytecode);
	void execute(const std::string& source, uint8_t O, uint8_t g);
}