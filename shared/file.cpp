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

require_info global_require_info;
void set_global_require_info(require_info require_info) {
	global_require_info = require_info;
}
read_file_info read_require(const std::string& raw_path, std::optional<fs::path> caller_path) {
	fs::path base;
	fs::path relative;
	if (raw_path.substr(0, 1) == "@") {
		std::string alias;
		size_t slash = raw_path.find('/');
		if (slash == std::string::npos) {
			alias = raw_path.substr(1);
			relative = "";
		} else {
			alias = raw_path.substr(1, slash - 1);
			relative = raw_path.substr(slash + 1);
		}
		if (global_require_info.aliases.find(alias) == global_require_info.aliases.end()) {
			throw std::runtime_error(std::format("Alias \"@{}\" doesn't exist", alias));
		}
		base = global_require_info.aliases.at(alias);
	} else if (raw_path.substr(0, 2) == "./") {
		base = global_require_info.root;
		relative = raw_path;
	} else if (raw_path.substr(0, 3) == "../") {
		if (caller_path.has_value()) {
			base = caller_path.value();
		} else {
			base = global_require_info.root;
		}
		relative = raw_path;
	} else {
		base = global_require_info.root;
		relative = raw_path;
	}
	fs::path path = base / relative;
	std::string path_str = path.string();
	try {
		std::vector<fs::path> possible_paths;
		for (const auto& suffix : {".luau", "", "/init.luau", "/init", ".lua", "/init.lua"}) {
			possible_paths.push_back(path_str + suffix);
			possible_paths.push_back(get_parent_folder() / (path_str + suffix));
		}
		return read_paths(possible_paths);
	} catch (int err) {
		if (err == ENOENT) {
			throw std::runtime_error(std::format("No file found at \"{}\"", path_str));
		} else {
			throw std::runtime_error(std::format("Access denied when reading file \"{}\"", path_str));
		}
	}
}

void write_file(const fs::path path, const std::string& content) {
	std::ofstream file(path, std::ios::binary);
	if (!file) {
		throw errno;
	}
	file.write(content.data(), content.size());
	file.close();
}