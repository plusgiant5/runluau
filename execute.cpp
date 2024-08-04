#include "execute.h"

#include <Windows.h>

#include <lualib.h>

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
	return state;
}

std::string runluau::compile(const std::string& source, uint8_t O, uint8_t g) {
	return Luau::compile(source, { .optimizationLevel = O, .debugLevel = g }, {});
}
void runluau::execute_bytecode(const std::string& bytecode) {
	lua_State* state = create_state();
	struct lua_State* thread = lua_newthread(state);
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
	status = lua_resume(thread, NULL, 0);
	if (status != LUA_OK) {
		printf("Script errored:\n");
		if (status == LUA_YIELD) [[unlikely]] {
			printf("Thread yielded unexpectedly\n");
		} else if (const char* str = lua_tostring(thread, -1)) {
			printf("%s\n", str);
		}

		printf("Stack trace:\n");

		printf("%s\n", lua_debugtrace(thread));
	}
}
void runluau::execute(const std::string& source, uint8_t O, uint8_t g) {
	execute_bytecode(compile(source, O, g));
}