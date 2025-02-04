#include "luau.h"
#include "pch.h"

#include "scheduler.h"

#pragma push_macro("max")
#undef max
#include <Luau/CodeGen.h>
#pragma pop_macro("max")

#define API __declspec(dllexport)

void* l_alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
	(void)ud;
	(void)osize;
	if (nsize == 0) {
		free(ptr);
		return NULL;
	} else {
		return realloc(ptr, nsize);
	}
}

int specified_O = -1;
int specified_g = -1;
API void luau::set_O_g(int O, int g) {
	specified_O = O;
	specified_g = g;
}
API int luau::get_O() {
	if (specified_O == -1) {
		throw std::runtime_error("O not specified");
	}
	return specified_O;
}
API int luau::get_g() {
	if (specified_g == -1) {
		throw std::runtime_error("g not specified");
	}
	return specified_g;
}

API std::recursive_mutex luau::luau_operation_mutex;

API const char* luau::get_error_message(lua_State* thread) {
	const char* message = lua_tostring(thread, -1);
	if (message == nullptr) {
		return "(none)";
	}
	return message;
}
API const char* luau::get_stack_trace(lua_State* thread) {
	return lua_debugtrace(thread);
}
API std::string luau::beautify_stack_trace(std::string stack_trace) {
	std::string colored;
	size_t current = 0;
	while (current < stack_trace.size()) {
		size_t newline = stack_trace.find_first_of('\n', current);
		size_t colon = stack_trace.find_first_of(':', current);
		if (stack_trace.substr(current, 3) == "[C]") {
			size_t space = stack_trace.find_first_of(' ', current + 3);
			size_t function = stack_trace.find_first_of(" function ", current + 3);
			colored += FUNCTION_COLOR + "function[C]:";
			if (function == std::string::npos) {
				colored += "<unnamed>";
			} else {
				current = function + strlen(" function ");
				size_t ending = newline == std::string::npos ? stack_trace.size() : newline + 1;
				colored += stack_trace.substr(current, ending - current);
			}
			if (newline == std::string::npos) {
				break;
			}
			current = newline + 1;
			continue;
		}
		colored += MAGENTA + stack_trace.substr(current, colon - current + 1);
		current = colon + 1;
		size_t space = stack_trace.find_first_of(' ', current);
		if (space == std::string::npos) {
			colored += NUMBER_COLOR + stack_trace.substr(current);
			break;
		} else if (newline != std::string::npos && space > newline) {
			space = newline;
		}
		colored += NUMBER_COLOR + stack_trace.substr(current, space - current) + MAGENTA + ": ";
		current = space + 1;
		size_t function = stack_trace.find_first_of(" function ", current);
		colored += FUNCTION_COLOR + "function[Lua]:";
		if (function == std::string::npos || function > newline) {
			colored += "<unnamed>";
		} else {
			current = function + strlen(" function ") - 1;
			size_t ending = newline == std::string::npos ? stack_trace.size() : newline;
			colored += stack_trace.substr(current, ending - current);
		}
		if (newline == std::string::npos) {
			break;
		}
		colored += '\n';
		current = newline + 1;
	}
	return colored.substr(0, colored.size() - 1) + RESET;
}
API std::string luau::beautify_syntax_error(std::string syntax_error) {
	size_t space = syntax_error.find_first_of(' ');
	if (space == std::string::npos) {
		return syntax_error;
	}
	std::string errorpart = syntax_error.substr(space);
	std::string location = syntax_error.substr(0, space);
	size_t first_colon = syntax_error.find_first_of(':');
	if (first_colon == std::string::npos) {
		return syntax_error;
	}
	std::string chunk_name = syntax_error.substr(0, first_colon);
	size_t second_colon = syntax_error.find_first_of(':', first_colon + 1);
	if (second_colon == std::string::npos || second_colon + 1 != location.size()) {
		return syntax_error;
	}
	std::string line_number = syntax_error.substr(first_colon + 1, second_colon - first_colon - 1);
	return MAGENTA + chunk_name + ":" + NUMBER_COLOR + line_number + MAGENTA + ":" + RED + errorpart + RESET;
}

std::unordered_map<lua_State*, lua_State*> thread_to_parent;
API size_t luau::thread_count = 0;
void userthread_callback(lua_State* parent_thread, lua_State* thread) {
	if (parent_thread) {
		//printf("Thread created: %lld -> %lld\n", luau::thread_count, luau::thread_count + 1);
		thread_to_parent.insert({thread, parent_thread});
		luau::thread_count++;
	} else {
		//printf("Thread destroyed: %lld -> %lld\n", luau::thread_count, luau::thread_count - 1);
		luau::thread_count--;
	}
}
void set_callbacks(lua_State* thread) {
	lua_Callbacks* callbacks = lua_callbacks(thread);
	callbacks->userthread = userthread_callback;
}
API lua_State* luau::get_parent_state(lua_State* child) {
	if (thread_to_parent.find(child) == thread_to_parent.end()) {
		return nullptr;
	}
	return thread_to_parent.at(child);
}

API lua_State* luau::create_state() {
	lua_State* state = lua_newstate(l_alloc, NULL);
	set_callbacks(state);
	if (Luau::CodeGen::isSupported()) {
		Luau::CodeGen::create(state);
	}
	luaL_openlibs(state);
	return state;
}
API lua_State* luau::create_thread(lua_State* thread) {
	lua_State* new_thread = lua_newthread(thread);
	return new_thread;
}
API void luau::load_and_handle_status(lua_State* thread, const std::string& bytecode, std::string chunk_name, bool beautify_syntax_error) {
	int status = luau_load(thread, ("=" + chunk_name).c_str(), bytecode.data(), bytecode.size(), 0);
	if (status != 0) {
		if (status != 1) {
			throw std::runtime_error(std::format("Unknown luau_load status `{}`", status));
		}
		std::string error_message = lua_tostring(thread, 1);
		if (beautify_syntax_error) {
			error_message = luau::beautify_syntax_error(error_message);
		}
		throw std::runtime_error(std::format("Syntax error:\n{}", error_message));
	}

	if (Luau::CodeGen::isSupported()) {
		Luau::CodeGen::CompilationOptions options{};
		Luau::CodeGen::compile(thread, -1, options);
	}
}
lua_State* main_thread = nullptr;
API void luau::add_thread_to_resume_queue(lua_State* thread, lua_State* from, int args, std::function<void()> setup_func) {
	if (!main_thread) {
		main_thread = thread;
	}
	scheduler::add_thread_to_resume_queue(thread, from, args, setup_func);
}
API void luau::on_thread_error(lua_State* thread) {
	printf("Script errored:\n" RED "%s" RESET "\nStack trace:\n%s\n", get_error_message(thread), beautify_stack_trace(get_stack_trace(thread)).c_str());
	//printf("Script errored. Stack trace:\n%s\n", lua_debugtrace(thread));
}
API bool luau::resume_and_handle_status(lua_State* thread, lua_State* from, int args, std::function<void()> setup_func) {
	std::lock_guard<std::recursive_mutex> lock(luau_operation_mutex);
	//printf("Requested %p %d %d\n", thread, lua_status(thread), lua_costatus(from, thread));
	if (lua_status(thread) == LUA_YIELD || lua_costatus(from, thread) == LUA_COSUS) {
		//printf("Beginning %d %d\n", lua_status(thread), lua_costatus(from, thread));
		lua_setreadonly(thread, LUA_GLOBALSINDEX, false);
		setup_func();
		int status = lua_resume(thread, from, args);
		//printf("End %d %d\n", status, lua_costatus(from, thread));
		switch (status) {
		case LUA_OK: case LUA_YIELD: break;
		case LUA_ERRRUN:
			luau::on_thread_error(thread);
			break;
		default: printf("Unknown status: %d\n", status);
		}
		if (status != LUA_YIELD || (from && lua_costatus(from, thread) == LUA_COSUS)) {
			lua_resetthread(thread);
		}
		return true;
	}
	return false;
}

void luau::start_scheduler() {
	HANDLE timer = CreateWaitableTimer(NULL, TRUE, NULL);
	if (!timer) {
		DWORD error = GetLastError();
		printf("Failed to CreateWaitableTimer: %d\n", error);
		exit(error);
	}
	LARGE_INTEGER frequency, start, end;
	QueryPerformanceFrequency(&frequency);
	//LARGE_INTEGER gc_start, gc_end;
	size_t i = 0;
	while (true) {
		QueryPerformanceCounter(&start);
		//printf("Frame %lld\n", ++i);
		scheduler::cycle();
		QueryPerformanceCounter(&end);
		//QueryPerformanceCounter(&gc_start);
		lua_gc(main_thread, LUA_GCSTEP, 1000);
		//QueryPerformanceCounter(&gc_end);
		//printf("GC in %f\n", static_cast<double>(gc_end.QuadPart - gc_start.QuadPart) / frequency.QuadPart);
		if (luau::thread_count <= 1) {
			//printf("Thread count: %lld\nMain status: %d\n", luau::thread_count, lua_status(main_thread));
			if (lua_status(main_thread) != LUA_YIELD) {
				break;
			}
		}
		double delta_time = static_cast<double>(end.QuadPart - start.QuadPart) / frequency.QuadPart;
		double time_left = (1.0 / (SCHEDULER_RATE * 2.0)) - delta_time;
		//printf("%f - %f = %f\n", (1.0 / SCHEDULER_RATE), delta_time, time_left);
		if (time_left > 0) {
			LARGE_INTEGER due_time = { .QuadPart = -static_cast<LONGLONG>(time_left * 10000000.0) };
			SetWaitableTimer(timer, &due_time, 0, nullptr, nullptr, false);
			WaitForSingleObject(timer, INFINITE);
		}
	}
	//printf("Scheduler stopped\n");
}

API void signal_yield_ready(yield_ready_event_t yield_ready_event) {
	if (!SetEvent(yield_ready_event)) {
		DWORD error = GetLastError();
		printf("Failed to SetEvent: %d\n", error);
		exit(error);
	}
}
API void create_windows_thread_for_luau(lua_State* thread, yield_thread_func_t func, void* ud) {
	yield_ready_event_t yield_ready_event = CreateEvent(nullptr, false, false, nullptr);
	std::thread win_thread(func, thread, yield_ready_event, ud);
	win_thread.detach();
	WaitForSingleObject(yield_ready_event, INFINITE);
}

API std::string luau::checkstring(lua_State* thread, int arg) {
	size_t len;
	const char* contents = luaL_checklstring(thread, arg, &len);
	return std::string(contents, len);
}
API std::string luau::optstring(lua_State* thread, int arg, std::string def) {
	size_t len = def.size();
	const char* contents = luaL_optlstring(thread, arg, def.data(), &len);
	return std::string(contents, len);
}
API void luau::pushstring(lua_State* thread, std::string str) {
	lua_pushlstring(thread, str.data(), str.size());
}

std::vector<std::string> loaded_plugins;
API void luau::add_loaded_plugin(std::string name) {
	loaded_plugins.push_back(name);
}
API std::vector<std::string> luau::get_loaded_plugins() {
	return loaded_plugins;
}
API bool luau::is_plugin_loaded(std::string name) {
	return std::find(loaded_plugins.begin(), loaded_plugins.end(), name) != loaded_plugins.end();;
}

bool APIENTRY DllMain(HMODULE, DWORD reason, void* reserved) {
	return true;
}