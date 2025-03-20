#pragma once

#include <unordered_set>
#include <filesystem>
namespace fs = std::filesystem;

#include "Luau/VM/include/lua.h"

fs::path get_plugins_folder();
void apply_plugins(lua_State* state, std::optional<std::unordered_set<std::string>> plugins_to_load = std::nullopt);