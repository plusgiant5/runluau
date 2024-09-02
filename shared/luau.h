#pragma once

#define SCHEDULER_RATE 60




#include <Windows.h>

#include <thread>
#include <functional>

#pragma push_macro("max")
#undef max
#include <Luau/CodeGen.h>
#pragma pop_macro("max")
#include <Luau/Compiler.h>
#include <lualib.h>

#ifdef PROJECT_EXPORTS
#define API __declspec(dllexport)
#else
#define API __declspec(dllimport)
#endif

namespace luau {
	API lua_State* create_state();
	API lua_State* create_thread(lua_State* thread);
	API void load(lua_State* thread, const std::string& bytecode);
	// Must call this with main thread before starting scheduler, and within functions passed into `create_windows_thread_for_luau`
	// `setup_func` is to avoid desync when modifying the state before resuming, check `task.wait` for an example
	API void add_thread_to_resume_queue(lua_State* thread, lua_State* from, int args, std::function<void()> setup_func = [&](){});
	API void start_scheduler();
	API void resume_and_handle_status(lua_State* thread, lua_State* from, int args, std::function<void()> setup_func = [&](){});
	API extern size_t thread_count;
}

#define wanted_arg_count(n) \
if (n > 0 && lua_gettop(thread) < n) [[unlikely]] { \
	lua_pushfstring(thread, __FUNCTION__" expects at least " #n " arguments, received %d", lua_gettop(thread)); \
	lua_error(thread); \
	return 0; \
}

// Yielding helpers for custom functions
API void signal_yield_ready(HANDLE yield_ready_event);
API void create_windows_thread_for_luau(lua_State* thread, void(*func)(lua_State* thread, HANDLE yield_ready_event, void* ud), void* ud = nullptr);