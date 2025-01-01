#include "file.h"

const fs::path get_self_path() {
	wchar_t self_path[1024];
	GetModuleFileNameW(NULL, self_path, 1024);
	return fs::path(self_path);
}
fs::path get_parent_folder() {
	return get_self_path().parent_path();
}
fs::path get_plugins_folder() {
	fs::path plugins_folder = get_parent_folder() / PLUGINS_FOLDER_NAME;
	if (!fs::exists(plugins_folder)) {
		fs::create_directory(plugins_folder);
	}
	return plugins_folder;
}


std::string read_file(const fs::path path) {
	std::ifstream file(path, std::ios::binary);
	if (!file) {
		throw errno;
	}
	std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();
	return std::string(buffer.begin(), buffer.end());
}
read_file_info read_paths(const std::vector<fs::path>& paths) {
	if (paths.size() == 0) {
		printf("No paths!\n");
		exit(ERROR_INTERNAL_ERROR);
	}
	int last_errno{};
	for (const auto& path : paths) {
		try {
			return {read_file(path), path};
		} catch (int err) {
			last_errno = err;
		}
	}
	throw last_errno;
}
read_file_info read_script(const std::string& path) {
	try {
		return read_paths({path, path + ".luau", path + ".lua", path + "\\init.luau", path + "\\init.lua", path + "\\init"});
	} catch (int err) {
		if (err == ENOENT) {
			printf("No script found at \"%s\"\n", path.c_str());
			exit(ERROR_FILE_NOT_FOUND);
		} else {
			printf("Access denied when reading script \"%s\"\n", path.c_str());
			exit(err);
		}
	}
}
read_file_info read_plugin(const std::string& path) {
	fs::path plugins_folder = get_plugins_folder();
	try {
		return read_paths({path, plugins_folder / path, plugins_folder / ("runluau-" + path), plugins_folder / (path + ".dll"), plugins_folder / ("runluau-" + path + ".dll")});
	} catch (int err) {
		if (err == ENOENT) {
			printf("No plugin found at \"%s\"\n", path.c_str());
			exit(ERROR_FILE_NOT_FOUND);
		} else {
			printf("Access denied when reading plugin \"%s\"\n", path.c_str());
			exit(err);
		}
	}
}
read_file_info read_require(const std::string& path) {
	try {
		std::vector<fs::path> possible_paths;
		for (const auto& suffix : {".luau", "", "/init.luau", "/init", ".lua", "/init.lua"}) {
			possible_paths.push_back(path + suffix);
			possible_paths.push_back(get_parent_folder() / (path + suffix));
		}
		return read_paths(possible_paths);
	} catch (int err) {
		if (err == ENOENT) {
			throw std::runtime_error(std::format("No file found at \"{}\"", path));
		} else {
			throw std::runtime_error(std::format("Access denied when reading file \"{}\"", path));
		}
	}
}