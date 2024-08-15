#include <stdio.h>
#include <Windows.h>

#include <cerrno>
#include <string>
#include <vector>
#include <unordered_set>
#include <fstream>
#include <filesystem>
#include <format>

#include "execute.h"

void help_then_exit(std::string notice_message) {
	printf(R"(%s Help is below:

runluau mode options...

Modes:
	- Run a script. `args` is optional. If specified, everything after it will be passed into the script:
	runluau run path\to\script options... args ...
	- Build a script to an executable:
	runluau build path\to\script options...
Options:
	- [default: 1] Optimization level 0-2:
	-O <n>
	- [default: 1] Debug level 0-2:
	-g <n>
)", notice_message.c_str());
	exit(ERROR_INVALID_PARAMETER);
}

std::string read_file(const std::string& path) {
	std::ifstream file(path);
	if (!file) [[unlikely]] {
		throw errno;
	}
	std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	return std::string(buffer.begin(), buffer.end());
}
std::string read_script(const std::string& path) {
	bool check_all_extensions = false;
	size_t last_slash = max(path.rfind('\\'), path.rfind('/'));
	size_t last_dot = path.rfind('.');
	if (last_dot == std::string::npos) {
		check_all_extensions = true;
	} else {
		if (last_slash != std::string::npos && last_dot > last_slash) {
			check_all_extensions = true;
		}
	}
	if (check_all_extensions) {
		try {
			return read_file(path + ".luau");
		} catch (...) {
			try {
				return read_file(path + ".lua");
			} catch (...) {}
		}
	}
	try {
		return read_file(path);
	} catch (int err) {
		if (err == ENOENT) {
			printf("No script found at %s\n", path.c_str());
			exit(ERROR_FILE_NOT_FOUND);
		} else {
			printf("Access denied when reading %s\n", path.c_str());
			exit(ERROR_ACCESS_DENIED);
		}
	}
}

runluau::settings read_args(std::vector<std::string>& args) {
	std::vector<std::string> script_args;
	runluau::settings settings;
	std::unordered_set<std::string> used_args;
	bool reading_args = false;
	for (size_t i = 2; i < args.size(); i++) {
		const auto& arg = args[i];
		if (reading_args) [[likely]] {
			script_args.push_back(arg);
		} else {
			if (arg == "args") {
				reading_args = true;
			} else {
				if (used_args.find(arg) != used_args.end()) {
					printf("Argument `%s` specified twice.", arg.c_str());
					exit(ERROR_INVALID_PARAMETER);
				}
				if (arg == "-O" || arg == "-g") {
					if (++i >= args.size()) {
						help_then_exit(std::format("Expected value after argument `{}`.", arg));
					}
					int value;
					try {
						value = std::stoi(args[i]);
					} catch (...) {
						help_then_exit(std::format("Value \"{}\" is not a valid integer.", args[i]));
					}
					if (value < 0 || value > 2) {
						help_then_exit(std::format("Value \"{}\" not in between 0 and 2.", value));
					}
					if (arg == "-O") {
						settings.O = value;
					} else {
						settings.g = value;
					}
					used_args.insert(arg);
				} else {
					help_then_exit(std::format("Unknown argument `{}`.", arg));
				}
			}
		}
	}
	settings.script_args = script_args;
	return settings;
}

int main(int argc, char* argv[]) {
	std::vector<std::string> args(argv + 1, argv + argc);
	if (args.size() < 2) [[unlikely]] {
		help_then_exit("Not enough arguments.");
	}
	std::string mode = args[0];
	if (mode == "run") {
		std::string source = read_script(args[1]);
		runluau::settings settings = read_args(args);
		runluau::execute(source, settings);
	} else if (mode == "build") {
		std::string source = read_script(args[1]);
	} else [[unlikely]] {
		help_then_exit(std::format("Unknown mode `{}`.", mode));
	}
	return ERROR_SUCCESS;
}