#include "base_funcs.h"
#include "file.h"
#include "execute.h"

#include <Luau/Compiler.h>

// Mostly from Luau source
std::unordered_map<lua_State*, std::unordered_map<std::string, lua_State*>> module_thread_cache; // [mainthread][path]
std::recursive_mutex module_thread_cache_mutex;
int require(lua_State* thread) {
	wanted_arg_count(1);
	std::string path = luaL_checkstring(thread, 1);

	read_file_info module_info;
	try {
		module_info = read_require(path);
	} catch (std::runtime_error error) {
		lua_pushstring(thread, error.what());
		lua_error(thread);
		return 1;
	}

	lua_State* main_thread = lua_mainthread(thread);

	std::lock_guard<std::recursive_mutex> lock(module_thread_cache_mutex);
	module_thread_cache.insert({main_thread, {}});
	auto& threads = module_thread_cache.at(main_thread);

	std::string resolved_path = std::filesystem::absolute(module_info.path).string();
	if (threads.find(resolved_path) != threads.end()) {
		//printf("Using cached module %s\n", resolved_path.c_str());
		lua_State* module_thread = threads.at(resolved_path);
		lua_xpush(module_thread, thread, 1);
		lua_pushvalue(thread, -1);
		if (lua_type(thread, 2) == LUA_TSTRING) {
			lua_pop(thread, 1);
			lua_pushstring(thread, "Cyclic dependency!");
		}
		return 1;
	}

	lua_State* module_thread = luau::create_thread(main_thread);
	threads.insert({resolved_path, module_thread});
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
	lua_pushvalue(thread, -1);

	lua_ref(thread, 3);

	return 1;
}

void register_base_funcs(lua_State* state) {
#define reg(name) lua_pushcfunction(state, name, #name); lua_setglobal(state, #name);
	reg(require);
}