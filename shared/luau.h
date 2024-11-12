#pragma once

#define SCHEDULER_RATE 240




#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <thread>
#include <functional>
#include <mutex>
#include <filesystem>
namespace fs = std::filesystem;

#pragma push_macro("max")
#undef max
#include <Luau/CodeGen.h>
#pragma pop_macro("max")
#include <Luau/Compiler.h>
#include <lualib.h>

#ifdef PROJECT_EXPORTS
// https://stackoverflow.com/a/78330763
#if defined(_MSC_VER) && !defined(__clang__)
#define API __declspec(dllexport)
#else
#error "TODO: Add support for non-MSVC compilers with PROJECT_EXPORTS"
#endif
#else
// https://stackoverflow.com/a/78330763
#if defined(_MSC_VER) && !defined(__clang__)
#define API __declspec(dllimport)
#else
#define API __attribute__((visibility("default")))
#endif
#endif

namespace luau {
	API extern std::recursive_mutex luau_operation_mutex;
	API lua_State* create_state();
	API lua_State* create_thread(lua_State* thread);
	API void load_and_handle_status(lua_State* thread, const std::string& bytecode, std::string chunk_name = "runluau");
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
	API void on_thread_error(lua_State* thread);
	API std::string wrapped_compile(const std::string& source, const int O, const int g);
}

#define wanted_arg_count(n) \
if (n > 0 && lua_gettop(thread) < n) [[unlikely]] { \
	lua_pushfstring(thread, __FUNCTION__" expects at least " #n " arguments, received %d", lua_gettop(thread)); \
	lua_error(thread); \
	return 0; \
}

// Yielding for custom functions
typedef void* yield_ready_event_t;
typedef void(*yield_thread_func_t)(lua_State* thread, yield_ready_event_t yield_ready_event, void* ud);
API void signal_yield_ready(yield_ready_event_t yield_ready_event);
API void create_windows_thread_for_luau(lua_State* thread, yield_thread_func_t func, void* ud = nullptr);