#include "execute.h"

#include <Windows.h>

#include "plugins.h"

using namespace runluau;

static void* l_alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
	(void)ud;
	(void)osize;
	if (nsize == 0) {
		free(ptr);
		return NULL;
	} else
		return realloc(ptr, nsize);
}

lua_State* create_state() {
	struct lua_State* state = lua_newstate(l_alloc, NULL);
	luaL_openlibs(state);
	apply_plugins(state);
	luaL_sandbox(state);
	return state;
}

std::string runluau::compile(const std::string& source, uint8_t O, uint8_t g) {
	return Luau::compile(source, { .optimizationLevel = O, .debugLevel = g, .vectorLib = "Vector3", .vectorCtor = "new" }, {});
}
void runluau::execute_bytecode(const std::string& bytecode, std::vector<std::string> args) {
	lua_State* state = create_state();
	struct lua_State* thread = lua_newthread(state);
	luaL_sandboxthread(thread);
	int status = luau_load(thread, "=runluau", bytecode.data(), bytecode.size(), 0);
	if (status != 0) [[unlikely]] {
		if (status != 1) [[unlikely]] {
			printf("Unknown luau_load status: %d\n", status);
			exit(ERROR_INTERNAL_ERROR);
		}
		const char* error_message = lua_tostring(thread, 1);
		printf("Syntax error:\n%s\n", error_message);
		exit(ERROR_INTERNAL_ERROR);
	}
	for (const auto& arg : args) {
		lua_pushlstring(thread, arg.data(), arg.size());
	}
	status = lua_resume(thread, NULL, (int)args.size());
	if (status != LUA_OK) [[unlikely]] {
		printf("Script errored:\n");
		if (status == LUA_YIELD) [[unlikely]] {
			printf("Thread yielded unexpectedly\n");
		} else if (const char* str = lua_tostring(thread, -1)) {
			printf("%s\n", str);
		}

		printf("Stack trace:\n");

		printf("%s\n", lua_debugtrace(thread));
	}
	lua_close(state);
}
void runluau::execute(const std::string& source, uint8_t O, uint8_t g, std::vector<std::string> args) {
	execute_bytecode(compile(source, O, g), args);
}