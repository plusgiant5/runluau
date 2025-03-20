#pragma once

#include <string>
#include <unordered_map>
#include <filesystem>
namespace fs = std::filesystem;
#include <fstream>

#ifdef _WIN32
#include <Windows.h>
#else
#include "errors.h"
#endif

#define PLUGINS_FOLDER_NAME "plugins"

struct read_file_info {
	std::string contents;
	fs::path path;
};

const fs::path get_self_path();
fs::path get_parent_folder();
fs::path get_plugins_folder();

std::string read_file(const fs::path path);
read_file_info read_paths(const std::vector<fs::path>& paths);
read_file_info read_script(const std::string& path);
read_file_info read_plugin(const std::string& path);

struct require_info {
	fs::path root;
	std::unordered_map<std::string, fs::path> aliases;
};
void set_global_require_info(require_info require_info);
read_file_info read_require(const std::string& raw_path, std::optional<fs::path> caller_path = std::nullopt);

void write_file(const fs::path path, const std::string& content);