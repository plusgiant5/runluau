#include "base_funcs.h"
#include "file.h"
#include "execute.h"

#include <Luau/Compiler.h>

std::unordered_map<lua_State*, fs::path> state_paths;
std::optional<fs::path> get_state_path(lua_State* state) {
	if (state_paths.find(state) == state_paths.end()) {
		return std::nullopt;
	}
	return state_paths.at(state);
}
void set_state_path(lua_State* state, fs::path path) {
	state_paths.insert({state, path});
}

// Mostly from Luau source
int loadstring(lua_State* thread) {
	size_t l = 0;
	const char* source = luaL_checklstring(thread, 1, &l);
	const char* name = luaL_optstring(thread, 2, source);
	lua_pop(thread, 2);

	std::string bytecode = runluau::compile(source, luau::get_O(), luau::get_g());
	try {
		lua_setsafeenv(thread, LUA_ENVIRONINDEX, false);
		luau::load_and_handle_status(thread, bytecode, name);
		return 1;
	} catch (std::runtime_error error) {
		lua_pushnil(thread);
		lua_pushstring(thread, error.what());
		return 2;
	}
}
int require(lua_State* thread) {
	wanted_arg_count(1);
	stack_slots_needed(5);
	std::string path = luaL_checkstring(thread, 1);

	read_file_info module_info;
	try {
		module_info = read_require(path, get_state_path(thread));
	} catch (std::runtime_error error) {
		lua_pushstring(thread, error.what());
		lua_error(thread);
		return 1;
	}

	lua_State* main_thread = lua_mainthread(thread);
	std::string resolved_path = std::filesystem::absolute(module_info.path).string();

	luaL_findtable(thread, LUA_REGISTRYINDEX, "_MODULES", 1);
	lua_getfield(thread, -1, resolved_path.c_str());
	if (!lua_isnil(thread, -1)) {
		//printf("Using cached module %s\n", resolved_path.c_str());
		lua_remove(thread, -2);
		return 1;
	}
	lua_pop(thread, 2);

	lua_State* module_thread = luau::create_thread(main_thread);
	set_state_path(module_thread, module_info.path);
	lua_xmove(main_thread, thread, 1);

	luaL_sandboxthread(module_thread);

	std::string bytecode = runluau::compile(module_info.contents, luau::get_O(), luau::get_g());
	std::string name = module_info.path.string();
	std::replace(name.begin(), name.end(), ' ', '_');
	luau::load_and_handle_status(module_thread, bytecode, name);

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

	lua_xpush(module_thread, thread, 1);
	int module_ret_index = lua_gettop(thread);
	luaL_findtable(thread, LUA_REGISTRYINDEX, "_MODULES", 1);
	lua_pushstring(thread, resolved_path.c_str());
	lua_pushvalue(thread, module_ret_index);
	lua_settable(thread, -3);
	lua_pop(thread, 1);

	return 1;
}

void register_base_funcs(lua_State* state) {
#define reg(name) lua_pushcfunction(state, name, #name); lua_setglobal(state, #name);
	reg(require);
	reg(loadstring);
}