#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
namespace fs = std::filesystem;

struct luaurc {
	std::unordered_map<std::string, fs::path> aliases;
	std::optional<std::unordered_set<std::string>> plugins_to_load = std::nullopt;
};
void read_luaurc(luaurc* luaurc, std::string content);