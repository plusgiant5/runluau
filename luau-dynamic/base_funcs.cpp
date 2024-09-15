#include "pch.h"

#include "base_funcs.h"
#include "file.hpp"

int specified_O{};
int specified_g{};
void set_O_g(int O, int g) {
	specified_O = O;
	specified_g = g;
}

// Mostly from Luau source
int require(lua_State* thread) {
	wanted_arg_count(1);
	std::string path = luaL_checkstring(thread, 1);

	read_file_info module_info = read_require(path);

	lua_State* main_thread = lua_mainthread(thread);
	lua_State* module_thread = luau::create_thread(main_thread);
	lua_xmove(main_thread, thread, 1);

	luaL_sandboxthread(module_thread);

	std::string bytecode = luau::wrapped_compile(module_info.contents, specified_O, specified_g);
	luau::load_and_handle_status(module_thread, bytecode, module_info.path.filename().string());

	int status = lua_resume(module_thread, thread, 0);

	const char* error = nullptr;
	if (status == 0) {
		if (lua_gettop(module_thread) == 0) {
			error = "module must return a value";
		} else if (!lua_istable(module_thread, -1) && !lua_isfunction(module_thread, -1)) {
			error = "module must return a table or function";
		}
	} else if (status == LUA_YIELD) {
		error = "module can not yield";
	} else if (!lua_isstring(module_thread, -1)) {
		error = "unknown error while running module";
	}
	if (error) {
		lua_pushstring(module_thread, error);
		lua_error(thread);
		return 0;
	}

	lua_xmove(module_thread, thread, 1);
	lua_pushvalue(thread, -1);

	return 1;
}

void register_base_funcs(lua_State* state) {
#define r(name) lua_pushcfunction(state, name, #name); lua_setglobal(state, #name);
	r(require);
}