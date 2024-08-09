#include "plugins.h"

#include <Windows.h>

#include <filesystem>
namespace fs = std::filesystem;

typedef void(__fastcall* register_library_t)(lua_State* state);

void apply_plugins(lua_State* state) {
	wchar_t self_path[1024];
	GetModuleFileNameW(NULL, self_path, 1024);
	fs::path plugins_folder = fs::path(self_path).parent_path() / PLUGINS_FOLDER_NAME;
	if (!fs::exists(plugins_folder)) [[unlikely]] {
		fs::create_directory(plugins_folder);
		return;
	}
	for (const auto& file : fs::directory_iterator(plugins_folder)) {
		if (file.path().extension() == ".dll") {
			HMODULE plugin_module = LoadLibraryW(file.path().wstring().c_str());
			if (!plugin_module) [[unlikely]] {
				wprintf(L"Failed to load plugin %s\n", file.path().wstring().c_str());
				exit(ERROR_MOD_NOT_FOUND);
			}
			register_library_t register_library = (register_library_t)GetProcAddress(plugin_module, "register_library");
			if (!register_library) [[unlikely]] {
				wprintf(L"Invalid plugin %s (missing register_library export)\n", file.path().wstring().c_str());
				exit(ERROR_PROC_NOT_FOUND);
			}
			register_library(state);
		}
	}
}