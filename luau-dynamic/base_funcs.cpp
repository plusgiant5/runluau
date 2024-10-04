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
	luau::load_and_handle_status(module_thread, bytecode, module_info.path.string());

	std::optional<std::string> error = std::nullopt;
	int status = lua_resume(module_thread, NULL, 0);
	switch (status) {
	case LUA_OK:
		if (lua_gettop(module_thread) != 1) {
			error = std::format("Module `{}` must return exactly one value", path);
		} else if (!lua_istable(module_thread, -1) && !lua_isfunction(module_thread, -1)) {
			error = std::format("Module `{}` must return a table or function", path);
		}
		break;
	case LUA_YIELD:
		error = std::format("Module `{}` yielded on require", path);
		break;
	case LUA_ERRRUN:
	{
		luau::on_thread_error(module_thread);
		error = std::format("Module `{}` errored on require", path);
		break;
	}	
	default:
		break;
	}
	if (error.has_value()) {
		lua_pushstring(thread, error.value().c_str());
		lua_error(thread);
		return 1;
	}

	lua_xmove(module_thread, thread, 1);
	lua_pushvalue(thread, -1);

	lua_resetthread(module_thread);

	return 1;
}

void register_base_funcs(lua_State* state) {
#define r(name) lua_pushcfunction(state, name, #name); lua_setglobal(state, #name);
	r(require);
}