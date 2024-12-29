#include "plugins.h"

#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <vector>
#include <Windows.h>

typedef void(__fastcall* register_library_t)(lua_State* state);
typedef const char**(__fastcall* get_dependencies_t)();


HMODULE load_plugin(const fs::path& path) {
	HMODULE plugin_module = LoadLibraryW(path.wstring().c_str());
	if (!plugin_module) {
		wprintf(L"Failed to load plugin %s\n", path.wstring().c_str());
		exit(ERROR_MOD_NOT_FOUND);
	}
	return plugin_module;
}

std::unordered_map<std::string, std::vector<std::string>> collect_dependencies(const fs::path& folder) {
	std::unordered_map<std::string, std::vector<std::string>> dependencies;
	for (const auto& file : fs::directory_iterator(folder)) {
		if (file.is_directory()) {
			auto subdir_dependencies = collect_dependencies(file);
			dependencies.insert(subdir_dependencies.begin(), subdir_dependencies.end());
		} else if (file.path().extension() == ".dll") {
			HMODULE plugin_module = load_plugin(file.path());
			get_dependencies_t get_dependencies = (get_dependencies_t)GetProcAddress(plugin_module, "get_dependencies");
			std::vector<std::string> wanted_dependencies;
			if (get_dependencies) {
				for (const char** wanted_dependency = get_dependencies(); *wanted_dependency; ++wanted_dependency) {
					wanted_dependencies.push_back(*wanted_dependency);
				}
			}
			dependencies[file.path().filename().string()] = wanted_dependencies;
		}
	}
	return dependencies;
}

std::vector<std::string> topological_sort(const std::unordered_map<std::string, std::vector<std::string>>& dependencies) {
	std::unordered_map<std::string, int> indegree;
	std::unordered_set<std::string> nodes;
	for (const auto& pair : dependencies) {
		nodes.insert(pair.first);
		for (const auto& dep : pair.second) {
			nodes.insert(dep);
			indegree[dep]++;
		}
	}
	std::vector<std::string> order;
	std::deque<std::string> queue;
	for (const auto& node : nodes) {
		if (indegree[node] == 0) {
			queue.push_back(node);
		}
	}
	while (!queue.empty()) {
		std::string current = queue.front();
		queue.pop_front();
		order.push_back(current);
		for (const auto& dep : dependencies.at(current)) {
			if (--indegree[dep] == 0) {
				queue.push_back(dep);
			}
		}
	}
	if (order.size() != nodes.size()) {
		wprintf(L"Circular dependency detected\n");
		exit(ERROR_CIRCULAR_DEPENDENCY);
	}
	return order;
}

void apply_plugins(lua_State* state) {
	auto folder = get_plugins_folder();
	auto dependencies = collect_dependencies(folder);
	auto sorted_plugins = topological_sort(dependencies);
	for (const auto& plugin_name : sorted_plugins) {
		fs::path plugin_path = folder / plugin_name;
		HMODULE plugin_module = load_plugin(plugin_path);
		register_library_t register_library = (register_library_t)GetProcAddress(plugin_module, "register_library");
		if (!register_library) {
			wprintf(L"Invalid plugin %s (missing register_library export)\n", plugin_path.wstring().c_str());
			exit(ERROR_PROC_NOT_FOUND);
		}
		register_library(state);
	}
}