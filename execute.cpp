#include "execute.h"

#include <Windows.h>

#include <Luau/Compiler.h>

#include "colors.h"
#include "plugins.h"
#include "base_funcs.h"

using namespace runluau;

lua_State* create_state(std::optional<std::unordered_set<std::string>> plugins_to_load = std::nullopt) {
	struct lua_State* state = luau::create_state();
	register_base_funcs(state);
	apply_plugins(state, plugins_to_load);
	luaL_sandbox(state);
	return state;
}

void runluau::execute_bytecode(const std::string& bytecode, settings_run_build& settings, std::optional<fs::path> path, std::optional<std::unordered_set<std::string>> plugins_to_load) {
	auto script_args = settings.script_args.value_or(std::vector<std::string>());

	lua_State* state = create_state(plugins_to_load);
	lua_State* thread = luau::create_thread(state);

	try {
		luau::load_and_handle_status(thread, bytecode, DEFAULT_CHUNK_NAME, true);
	} catch (std::runtime_error error) {
		printf("Failed to load bytecode: %s\n", error.what());
		exit(ERROR_INTERNAL_ERROR);
	}

	for (const auto& arg : script_args) {
		lua_pushlstring(thread, arg.data(), arg.size());
	}
	luau::add_thread_to_resume_queue(thread, nullptr, (int)script_args.size());
	luau::start_scheduler();

	lua_close(state);
}
void runluau::execute(const std::string& source, settings_run_build& settings, std::optional<fs::path> path, std::optional<std::unordered_set<std::string>> plugins_to_load) {
	execute_bytecode(compile(source, luau::get_O(), luau::get_g()), settings, path, plugins_to_load);
}
std::string runluau::compile(const std::string& source, const int O, const int g) {
	return Luau::compile(source, {.optimizationLevel = O, .debugLevel = g});
}