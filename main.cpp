#include <stdio.h>
#include <Windows.h>

#include <cerrno>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

#include "execute.h"

void help_then_exit() {
	printf(R"(Invalid arguments. Help is below:

runluau mode options...

Modes:
	- Run a script. `args` is optional. If specified, everything after it will be passed into the script:
	runluau run path\to\script options... args ...
	- Build a script to an executable:
	runluau build path\to\script options...
Options:
	- [default: 1] Optimization level 0-2:
	-On
	- [default: 1] Debug level 0-2:
	-gn
)");
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

std::vector<std::string> read_options(std::vector<std::string> args) {
	std::vector<std::string> script_args;
	for (const auto& arg : args) {
		
	}
	return script_args;
}

int main(int argc, char* argv[]) {
	std::vector<std::string> args(argv + 1, argv + argc);
	if (args.size() < 2) [[unlikely]] {
		help_then_exit();
	}
	std::string mode = args[0];
	if (mode == "run") {
		std::string source = read_script(args[1]);
		runluau::execute(source, 1, 1);
	} else if (mode == "build") {
		std::string source = read_script(args[1]);
	} else [[unlikely]] {
		help_then_exit();
	}
	return ERROR_SUCCESS;
}