#pragma once

#include <lua.h>

#define PLUGINS_FOLDER_NAME "plugins"

void apply_plugins(lua_State* state);