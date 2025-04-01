#pragma once

#define SCHEDULER_RATE 240




#include <Windows.h>

#include <thread>
#include <functional>
#include <mutex>
#include <filesystem>
namespace fs = std::filesystem;

#include <lualib.h>

#ifdef PROJECT_EXPORTS
#define API __declspec(dllexport)
#else
#define API __declspec(dllimport)
#endif

namespace luau {
	namespace settings {
		API extern bool use_native_codegen;
	}

	API extern std::recursive_mutex luau_operation_mutex;
	API lua_State* create_state();
	API lua_State* create_thread(lua_State* thread);
	API void load_and_handle_status(lua_State* thread, const std::string& bytecode, std::string chunk_name, bool beautify_syntax_error = false);
	// Must call this with main thread before starting scheduler, and within functions passed into `create_windows_thread_for_luau`
	// `setup_func` is to avoid desync when modifying the state before resuming, check `task.wait` for an example
	API void add_thread_to_resume_queue(lua_State* thread, lua_State* from, int args, std::function<void()> setup_func = [&](){});
	API lua_State* get_parent_state(lua_State* child);
	API bool resume_and_handle_status(lua_State* thread, lua_State* from, int args, std::function<void()> setup_func = [&](){});
	API extern size_t thread_count;
	API void start_scheduler();
	API const char* get_error_message(lua_State* thread);
	API const char* get_stack_trace(lua_State* thread);
	API std::string beautify_stack_trace(std::string stack_trace);
	API std::string beautify_syntax_error(std::string syntax_error);
	API void on_thread_error(lua_State* thread);

	API void set_O_g(int O, int g);
	API int get_O();
	API int get_g();

	API std::string checkstring(lua_State* thread, int arg);
	API std::string optstring(lua_State* thread, int arg, std::string def);
	API void pushstring(lua_State* thread, std::string str);

	API void add_loaded_plugin(std::string name);
	API std::vector<std::string> get_loaded_plugins();
	API bool is_plugin_loaded(std::string name);
}

#define wanted_arg_count(n) \
if (n > 0 && lua_gettop(thread) < n) { \
	lua_pushfstring(thread, __FUNCTION__" expects at least " #n " arguments, received %d", lua_gettop(thread)); \
	lua_error(thread); \
	return 0; \
}
// Too many slots isn't that big of a deal, too little can lead to random crashes
#define stack_slots_needed(n) lua_rawcheckstack(thread, n);

// Yielding for custom functions
typedef HANDLE yield_ready_event_t;
typedef void(*yield_thread_func_t)(lua_State* thread, yield_ready_event_t yield_ready_event, void* ud);
API void signal_yield_ready(yield_ready_event_t yield_ready_event);
API void create_windows_thread_for_luau(lua_State* thread, yield_thread_func_t func, void* ud = nullptr);