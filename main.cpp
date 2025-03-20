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
#include "file.h"
#include "execute.h"
#include "plugins.h"
#include "build.h"
#include "luaurc.h"
#include "luau.h"

void help_then_exit(std::string notice_message) {
	printf(R"(%s Help is below:

runluau <mode> <...>

Modes:
	- Run a script. `args` is optional. If specified, everything after it will be passed into the script. If not specified, `script` will be set to "init".
	runluau run <path>? <options> --args <...>
Options:
	- [default: 1] Optimization level 0-2:
	-O/-o <n>
	- [default: 1] Debug level 0-2:
	-g/-d <n>
More modes:
	- Creates a folder in the current directory with a `.luaurc` and `init.luau` inside.
	runluau new <name>

Script paths don't have to include `.luau` or `.lua`. `file.luau` can optionally be shortened to `file`.
)", notice_message.c_str());
	exit(ERROR_INVALID_PARAMETER);
}


runluau::settings_run_build read_args_run_build(std::vector<std::string>& args, size_t starting_point) {
	std::vector<std::string> script_args;
	std::vector<std::string> plugins;
	runluau::settings_run_build settings;
	std::unordered_set<std::string> used_args;
	bool reading_args = false;
	bool reading_plugins = false;
	for (size_t i = starting_point; i < args.size(); i++) {
		const auto& arg = args[i];
		if (reading_args) {
			script_args.push_back(arg);
		} else if (reading_plugins) {
			plugins.push_back(arg);
		} else if (arg == "--args") {
			reading_args = true;
		} else if (arg == "--plugins") {
			reading_plugins = true;
		} else {
			if (used_args.find(arg) != used_args.end()) {
				printf("Argument `%s` specified twice.", arg.c_str());
				exit(ERROR_INVALID_PARAMETER);
			}
			used_args.insert(arg);
			if (arg == "-O" || arg == "-g" || arg == "-o" || arg == "-d") {
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
				if (arg == "-O" || arg == "-o")
					settings.O = value;
				else
					settings.g = value;
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
#ifdef _WIN32
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	if (console) {
		DWORD mode;
		GetConsoleMode(console, &mode);
		mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(console, mode);
	}
#endif
	std::vector<std::string> args(argv + 1, argv + argc);
	const auto minimum_arguments = [&args](size_t amount) -> void {
		if (args.size() - 1 < amount) {
			help_then_exit("Not enough arguments.");
		}
	};
	if (args.size() == 0) {
		help_then_exit("Mode not specified.");
	}
	std::string mode = args[0];
	if (mode == "run") {
		read_file_info script;
		runluau::settings_run_build settings;
		bool script_specified = args.size() > 1;
		if (script_specified) {
			script_specified = args[1].substr(0, 1) != "-";
		}
		if (script_specified) {
			script = read_script(args[1]);
			settings = read_args_run_build(args, 2);
		} else {
			script = read_script("init");
			settings = read_args_run_build(args, 1);
		}
		luau::set_O_g(settings.O, settings.g);
		if (settings.plugins != std::nullopt)
			help_then_exit("Cannot specify `--plugins` in `run` mode.");

		fs::path root = fs::absolute(script.path).parent_path();
		fs::path luaurc_path = root / ".luaurc";
		
		std::optional<std::unordered_set<std::string>> plugins_to_load = std::nullopt;
		require_info require_info;
		require_info.root = root;
		require_info.aliases = {};
		if (fs::is_regular_file(luaurc_path)) {
			luaurc luaurc;
			try {
				read_luaurc(&luaurc, read_file(luaurc_path));
			} catch (int err) {
				printf("Access denied when reading luaurc \"%s\"\n", luaurc_path.string().c_str());
				return err;
			} catch (std::runtime_error err) {
				printf("Failed to read luaurc \"%s\": %s\n", luaurc_path.string().c_str(), err.what());
				return ERROR_INTERNAL_ERROR;
			}
			require_info.aliases = luaurc.aliases;
			plugins_to_load = luaurc.plugins_to_load;
		}
		set_global_require_info(require_info);

		runluau::execute(script.contents, settings, script.path, plugins_to_load);
	} else if (mode == "build") {
		minimum_arguments(2);
		std::string source = read_script(args[1]).contents;
		fs::path output_path = args[2];
		runluau::settings_run_build settings = read_args_run_build(args, 3);
		luau::set_O_g(settings.O, settings.g);
		if (settings.script_args != std::nullopt)
			help_then_exit("Cannot specify `--args` in `build` mode.");

		return build(source, output_path, settings);
	} else if (mode == "new") {
		minimum_arguments(1);
		std::string name = args[1];
		
		fs::path folder_path = fs::absolute(fs::path(name));
		printf("%s\n", folder_path.string().c_str());
		if (fs::exists(folder_path)) {
			if (fs::is_regular_file(folder_path)) {
				printf("A file exists at `%s`!\n", folder_path.string().c_str());
				return ERROR_FILE_EXISTS;
			} else if (fs::is_directory(folder_path)) {
				printf("A folder already exists at `%s`!\n", folder_path.string().c_str());
				return ERROR_FILE_EXISTS;
			} else {
				printf("Something already exists at `%s`!\n", folder_path.string().c_str());
				return ERROR_FILE_EXISTS;
			}
		}
		try {
			fs::create_directory(name);
		} catch (...) {
			printf("Failed to create directory at path `%s`.", folder_path.string().c_str());
			return ERROR_INVALID_NAME;
		}
		std::string content;
		fs::path template_luaurc = get_parent_folder() / "template.luaurc";
		if (fs::is_regular_file(template_luaurc)) {
			content = read_file(template_luaurc);
		} else {
			content = R"({
	"languageMode": "strict",
	"aliases": {
	}
})";
			write_file(template_luaurc, content);
		}
		write_file(folder_path / ".luaurc", content);
		write_file(folder_path / "init.luau", "print(\"Hello world!\")");
		printf("Created `%s`.", name.c_str());
	} else {
		help_then_exit(std::format("Unknown mode `{}`.", mode));
	}
	return 0;
}