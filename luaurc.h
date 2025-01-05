#pragma once

#include <string>
#include <unordered_map>
#include <filesystem>
namespace fs = std::filesystem;

struct luaurc {
	std::unordered_map<std::string, fs::path> aliases;
};
void read_luaurc(luaurc* luaurc, std::string content);