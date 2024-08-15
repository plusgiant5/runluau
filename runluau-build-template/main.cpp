#include <Windows.h>
#include <stdio.h>
#include <shlobj.h>

#include <lualib.h>

int main(int argc, char* argv[]) {
	HMODULE module_handle = GetModuleHandleW(NULL);
	HRSRC resource_handle = FindResourceA(module_handle, MAKEINTRESOURCEA(101), "BINARY");
	if (!resource_handle) [[unlikely]] {
		DWORD last_error = GetLastError();
		printf("Failed to FindResourceA (0x%.8X)\n", last_error);
		return last_error;
	}

	HGLOBAL loaded = LoadResource(module_handle, resource_handle);
	DWORD resource_size = SizeofResource(module_handle, resource_handle);
	if (!loaded) [[unlikely]] {
		DWORD last_error = GetLastError();
		printf("Failed to LoadResource (0x%.8X)\n", last_error);
		return last_error;
	}
	void* resource_buffer = LockResource(loaded);

	char temp_path[MAX_PATH];
	if (GetTempPathA(MAX_PATH, temp_path) == 0) {
		DWORD last_error = GetLastError();
		printf("Failed to GetTempPathA (0x%.8X)\n", last_error);
		return last_error;
	}

	// Create a unique temporary file name in the temp folder
	char dll_path[MAX_PATH];
	if (GetTempFileNameA(temp_path, "runluau-luau-dll", 0, dll_path) == 0) {
		DWORD last_error = GetLastError();
		printf("Failed to GetTempFileNameA (0x%.8X)\n", last_error);
		return last_error;
	}

	HANDLE dll_file = CreateFileA(
		dll_path,
		GENERIC_WRITE | GENERIC_READ,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_TEMPORARY,
		NULL
	);
	if (!dll_file) [[unlikely]] {
		DWORD last_error = GetLastError();
		printf("File not found at \"%s\" (0x%.8X)\n", dll_path, last_error);
		return GetLastError();
	}
	DWORD written;
	WriteFile(dll_file, resource_buffer, resource_size, &written, NULL);
	CloseHandle(dll_file);

	if (!LoadLibraryA(dll_path)) {
		DWORD last_error = GetLastError();
		printf("Failed to load library (0x%.8X)\n", last_error);
		return GetLastError();
	}

	

	return 0;
}