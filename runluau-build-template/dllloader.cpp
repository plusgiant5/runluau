#include "dllloader.h"

#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

HMODULE load_embedded_dll(const char* name, const char* data, size_t size) {
	fs::path temp_path = fs::temp_directory_path() / name;
	std::ofstream dll_file(temp_path, std::ios::binary | std::ios::trunc | std::ios::out);
	if (!dll_file) {
		wprintf(L"Failed to open temp file \"%s\"\n", temp_path.c_str());
		exit(ERROR_INTERNAL_ERROR);
	}
	dll_file.write(data, size);
	dll_file.close();
	HMODULE module_handle = LoadLibraryW(temp_path.c_str());
	if (!module_handle) {
		wprintf(L"Failed to load library \"%s\"\n", temp_path.c_str());
		exit(ERROR_FILE_CORRUPT);
	}
	return module_handle;
}