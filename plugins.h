#pragma once

#include <filesystem>
namespace fs = std::filesystem;

#include <lua.h>

#define PLUGINS_FOLDER_NAME "plugins"

fs::path get_plugins_folder();
void apply_plugins(lua_State* state);