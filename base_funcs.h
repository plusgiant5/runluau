#pragma once

#include "luau.h"

std::optional<fs::path> get_state_path(lua_State* state);
void set_state_path(lua_State* state, fs::path path);

void register_base_funcs(lua_State* state);