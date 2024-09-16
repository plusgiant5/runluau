#pragma once

#define SCHEDULER_RATE 60

#include <thread>
#include <functional>
#include <filesystem>
namespace fs = std::filesystem;

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
	API void load_and_handle_status(lua_State* thread, const std::string& bytecode, std::string chunk_name = "runluau");
	// Must call this with main thread before starting scheduler, and within functions passed into `create_thread_for_luau`
	// `setup_func` is to avoid desync when modifying the state before resuming, check `task.wait` for an example
	API void add_thread_to_resume_queue(lua_State* thread, lua_State* from, int args, std::function<void()> setup_func = [&](){});
	API void start_scheduler();
	API bool resume_and_handle_status(lua_State* thread, lua_State* from, int args, std::function<void()> setup_func = [&](){});
	API extern size_t thread_count;
	API lua_State* get_parent_state(lua_State* child);
	API std::string wrapped_compile(const std::string& source, const int O, const int g);
}

#define wanted_arg_count(n) \
if (n > 0 && lua_gettop(thread) < n) [[unlikely]] { \
	lua_pushfstring(thread, __FUNCTION__" expects at least " #n " arguments, received %d", lua_gettop(thread)); \
	lua_error(thread); \
	return 0; \
}

// Yielding for custom functions
typedef HANDLE yield_ready_event_t;
typedef void(*yield_thread_func_t)(lua_State* thread, yield_ready_event_t yield_ready_event, void* ud);
API void signal_yield_ready(yield_ready_event_t yield_ready_event);
API void create_thread_for_luau(lua_State* thread, yield_thread_func_t func, void* ud = nullptr);