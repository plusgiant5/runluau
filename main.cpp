#include <stdio.h>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <csignal>
#include <signal.h>
#endif

#include <cerrno>
#include <string>
#include <vector>
#include <unordered_set>
#include <fstream>
#include <format>

#include "errors.h"
#include "file.hpp"
#include "execute.h"
#include "plugins.h"
#include "luau.h"

void help_then_exit(std::string notice_message) {
	printf(R"(%s Help is below:

runluau mode options...

Modes:
	- Run a script. `args` is optional. If specified, everything after it will be passed into the script:
	runluau run path\to\script <options> --args <...>
Options:
	- [default: 1] Optimization level 0-2:
	-O <n>
	- [default: 1] Debug level 0-2:
	-g <n>
)", notice_message.c_str());
	exit(ERROR_INVALID_PARAMETER);
}


runluau::settings read_args(std::vector<std::string>& args, size_t starting_point) {
	std::vector<std::string> script_args;
	std::vector<std::string> plugins;
	runluau::settings settings;
	std::unordered_set<std::string> used_args;
	bool reading_args = false;
	bool reading_plugins = false;
	for (size_t i = starting_point; i < args.size(); i++) {
		const auto& arg = args[i];
		if (reading_args) [[likely]] {
			script_args.push_back(arg);
		} else if (reading_plugins) [[unlikely]] {
			plugins.push_back(arg);
		} else if (arg == "--args") [[unlikely]] {
			reading_args = true;
		} else if (arg == "--plugins") [[unlikely]] {
			reading_plugins = true;
		} else {
			if (used_args.find(arg) != used_args.end()) {
				printf("Argument `%s` specified twice.", arg.c_str());
				exit(ERROR_INVALID_PARAMETER);
			}
			if (arg == "-O" || arg == "-g") {
				if (++i >= args.size())
					help_then_exit(std::format("Expected value after argument `{}`.", arg));
				int value;
				try {
					value = std::stoi(args[i]);
				} catch (...) {
					help_then_exit(std::format("Value \"{}\" is not a valid integer.", args[i]));
				}
				if (value < 0 || value > 2)
					help_then_exit(std::format("Value \"{}\" not in between 0 and 2.", value));
				if (arg == "-O")
					settings.O = value;
				else
					settings.g = value;
				used_args.insert(arg);
			} else {
				help_then_exit(std::format("Unknown argument `{}`.", arg));
			}
		}
	}
	if (reading_args)
		settings.script_args = script_args;
	if (reading_plugins)
		settings.plugins = plugins;
	return settings;
}

inline uintptr_t align(uintptr_t value, uintptr_t alignment) {
	return (value + (alignment - 1)) & ~(alignment - 1);
}
#ifdef _WIN32
BOOL WINAPI ctrl_handler(unsigned long type) {
#else
int ctrl_handler(int type) {
#endif
	switch (type) {
	#ifdef _WIN32
	case CTRL_C_EVENT:
	#else
	case SIGINT:
	#endif
		printf("Exiting\n");
		#ifdef _WIN32
		__fastfail(ERROR_PROCESS_ABORTED);
		#else
		raise(SIGABRT);
		#endif
		return 1;
	default:
		return 0;
	}
}
int main(int argc, char* argv[]) {
#ifdef _WIN32
	SetConsoleCtrlHandler(ctrl_handler, TRUE);
#else
	signal(SIGINT, (void(*)(int))ctrl_handler);
#endif
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	if (console) {
		DWORD mode;
		GetConsoleMode(console, &mode);
		mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(console, mode);
	}
	std::vector<std::string> args(argv + 1, argv + argc);
	if (args.size() < 2) [[unlikely]]
		help_then_exit("Not enough arguments.");
	std::string mode = args[0];
	if (mode == "run") {
		read_file_info script = read_script(args[1]);
		runluau::settings settings = read_args(args, 2);
		if (settings.plugins != std::nullopt) [[unlikely]]
			help_then_exit("Cannot specify `--plugins` in `run` mode.");

		runluau::execute(script.contents, settings);
	} else [[unlikely]] {
		help_then_exit(std::format("Unknown mode `{}`.", mode));
	}
	return 0;
}