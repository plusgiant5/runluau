#pragma once

#include <string>
#include <filesystem>
namespace fs = std::filesystem;
#include <fstream>

#include <Windows.h>

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
read_file_info read_script(const std::string& path) ;
read_file_info read_plugin(const std::string& path);
read_file_info read_require(const std::string& path);