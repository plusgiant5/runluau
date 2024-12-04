#pragma once

#include <filesystem>
namespace fs = std::filesystem;

#include "Luau/VM/include/lua.h"

fs::path get_plugins_folder();
void apply_plugins(lua_State* state);