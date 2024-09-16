#include "execute.h"

#include "colors.h"
#include "plugins.h"

using namespace runluau;

lua_State* create_state() {
	struct lua_State* state = luau::create_state();
	apply_plugins(state);
	luaL_sandbox(state);
	return state;
}

void runluau::execute_bytecode(const std::string& bytecode, settings& settings) {
	auto script_args = settings.script_args.value_or(std::vector<std::string>());

	lua_State* state = create_state();
	lua_State* thread = luau::create_thread(state);

	try {
		luau::load_and_handle_status(thread, bytecode);
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
void runluau::execute(const std::string& source, settings& settings) {
	execute_bytecode(luau::wrapped_compile(source, settings.O, settings.g), settings);
}