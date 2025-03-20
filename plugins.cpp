#include "plugins.h"

#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <vector>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <dlfcn.h>
#endif
#include "errors.h"
#include "macros.h"

#include "luau.h"

typedef void(__fastcall* register_library_t)(lua_State* state);
typedef const char**(__fastcall* get_dependencies_t)();


#ifdef _WIN32
HMODULE load_plugin(const fs::path& path) {
	HMODULE plugin_module = LoadLibraryW(path.wstring().c_str());
	if (!plugin_module) [[unlikely]] {
		wprintf(L"Failed to load plugin %s\n", path.wstring().c_str());
		exit(ERROR_MOD_NOT_FOUND);
	}
	return plugin_module;
}
#else
void* load_plugin(const fs::path& path) {
	void* plugin_module = dlopen(path.c_str(), RTLD_LAZY);
	if (!plugin_module) [[unlikely]] {
		fprintf(stderr, "Failed to load plugin %s\n", dlerror());
		exit(ERROR_MOD_NOT_FOUND);
	}
	return plugin_module;
}
#endif
std::unordered_map<std::string, std::vector<std::string>> collect_dependencies(const fs::path& folder) {
	std::unordered_map<std::string, std::vector<std::string>> dependencies;
	for (const auto& file : fs::directory_iterator(folder)) {
		if (file.is_directory()) {
			auto subdir_dependencies = collect_dependencies(file);
			dependencies.insert(subdir_dependencies.begin(), subdir_dependencies.end());
		} else if (file.path().extension() == ".dll") {
			auto plugin_module = load_plugin(file.path());
#ifdef _WIN32
			get_dependencies_t get_dependencies = (get_dependencies_t)GetProcAddress(plugin_module, "get_dependencies");
#else
			get_dependencies_t get_dependencies = (get_dependencies_t)dlsym(plugin_module, "get_dependencies");
			if (!get_dependencies) [[unlikely]] {
				fprintf(stderr, "Failed to find get_dependencies export in plugin %s\n", file.path().filename().c_str());
				exit(EINVAL);
			}
#endif
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

void apply_plugins(lua_State* state, std::optional<std::unordered_set<std::string>> plugins_to_load) {
	auto folder = get_plugins_folder();
	auto dependencies = collect_dependencies(folder);
	auto sorted_plugins = topological_sort(dependencies);
	std::unordered_set<std::string> loaded_plugins_to_load;
	for (const auto& plugin_name : sorted_plugins) {
		if (plugins_to_load.has_value()) {
			if (plugins_to_load.value().find(plugin_name) == plugins_to_load.value().end()) {
				continue;
			}
			loaded_plugins_to_load.insert(plugin_name);
		}
		luau::add_loaded_plugin(plugin_name);
	}
	if (plugins_to_load.has_value()) {
		bool found_missing = false;
		for (const auto& plugin : plugins_to_load.value()) {
			if (loaded_plugins_to_load.find(plugin) == loaded_plugins_to_load.end()) {
				printf("Plugin %s was listed in `plugins` in `.luaurc`, but was not found in your `plugins` folder\n", plugin.c_str());
				found_missing = true;
			}
		}
		if (found_missing) {
			exit(ERROR_FILE_NOT_FOUND);
		}
	}
	for (const auto& plugin_name : sorted_plugins) {
		if (plugins_to_load.has_value()) {
			if (plugins_to_load.value().find(plugin_name) == plugins_to_load.value().end()) {
				continue;
			}
		}
		fs::path plugin_path = folder / plugin_name;
		auto plugin_module = load_plugin(plugin_path);
		#ifdef _WIN32
		register_library_t register_library = (register_library_t)GetProcAddress(plugin_module, "register_library");
		#else
		register_library_t register_library = (register_library_t)dlsym(plugin_module, "register_library");
		#endif
		if (!register_library) {
			wprintf(L"Invalid plugin %s (missing register_library export)\n", plugin_path.wstring().c_str());
			exit(ERROR_NOT_FOUND);
		}
		register_library(state);
	}
}