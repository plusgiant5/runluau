#include <Windows.h>
#include <stdio.h>
#include <shlobj.h>

#include <filesystem>

#include <luau.h>

#include "dllloader.h"

typedef void(__fastcall* register_library_t)(lua_State* state);

static void* l_alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
	(void)ud;
	(void)osize;
	if (nsize == 0) {
		free(ptr);
		return NULL;
	} else
		return realloc(ptr, nsize);
}

uintptr_t find_overlay_start() {
	auto base_address = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
	auto dos_header = (IMAGE_DOS_HEADER*)(base_address);
	auto nt_header = (IMAGE_NT_HEADERS*)(base_address + dos_header->e_lfanew);
	auto sections = (IMAGE_SECTION_HEADER*)((uintptr_t)(nt_header)+sizeof(*nt_header));
	uintptr_t overlay_start = NULL;
	for (int i = nt_header->FileHeader.NumberOfSections - 1; i >= 0; i--) {
		if (strcmp((char*)sections[i].Name, ".runluau") == 0) {
			overlay_start = base_address + sections[i].VirtualAddress;
			break;
		}
	}
	if (overlay_start == NULL) {
		printf("Failed to find .runluau section!\n");
		exit(ERROR_FILE_CORRUPT);
	}
	return overlay_start;
}

int main(int argc, char* argv[]) {
	std::vector<std::string> args(argv + 1, argv + argc);

	uintptr_t overlay_start = find_overlay_start();

	HMODULE self_handle = GetModuleHandleW(NULL);
	HRSRC resource_handle = FindResourceA(self_handle, MAKEINTRESOURCEA(101), "BINARY");
	if (!resource_handle) [[unlikely]] {
		DWORD last_error = GetLastError();
		printf("Failed to FindResourceA (0x%.8X)\n", last_error);
		return last_error;
	}
	HGLOBAL loaded = LoadResource(self_handle, resource_handle);
	DWORD resource_size = SizeofResource(self_handle, resource_handle);
	if (!loaded) [[unlikely]] {
		DWORD last_error = GetLastError();
		printf("Failed to LoadResource (0x%.8X)\n", last_error);
		return last_error;
	}
	void* resource_buffer = LockResource(loaded);

	uintptr_t current_offset = overlay_start;
	size_t bytecode_size = *(size_t*)(current_offset);
	current_offset += sizeof(bytecode_size);
	const char* bytecode = (const char*)current_offset;
	current_offset += bytecode_size;
	size_t plugins_count = *(size_t*)(current_offset);
	current_offset += sizeof(plugins_count);
	load_embedded_dll("luau.dll", (const char*)resource_buffer, resource_size);
	std::vector<register_library_t> register_funcs;
	for (size_t i = 0; i < plugins_count; i++) {
		std::string plugin_name;
		while (true) {
			uint8_t character = *(uint8_t*)(current_offset++);
			if (character == NULL) {
				break;
			}
			plugin_name += character;
		}
		//printf("Plugin `%s`\n", plugin_name.c_str());
		size_t plugin_size = *(size_t*)(current_offset);
		current_offset += sizeof(plugin_size);
		HMODULE plugin = load_embedded_dll(plugin_name.c_str(), (const char*)current_offset, plugin_size);
		register_library_t register_library = (register_library_t)GetProcAddress(plugin, "register_library");
		if (!register_library) {
			printf("Plugin `%s` has no `register_library` export\n", plugin_name.c_str());
			exit(ERROR_FILE_CORRUPT);
		}
		register_funcs.push_back(register_library);
		current_offset += plugin_size;
	}

	lua_State* state = luau::create_state();
	for (const auto& register_func : register_funcs) {
		register_func(state);
	}

	luaL_sandbox(state);
	lua_State* thread = luau::create_thread(state);

	try {
		luau::load_and_handle_status(thread, std::string(bytecode, bytecode_size));
	} catch (std::runtime_error error) {
		printf("Failed to load bytecode: %s\n", error.what());
		exit(ERROR_INTERNAL_ERROR);
	}

	for (const auto& arg : args) {
		lua_pushlstring(thread, arg.data(), arg.size());
	}
	luau::add_thread_to_resume_queue(thread, nullptr, (int)args.size());
	luau::start_scheduler();

	lua_close(state);

	return 0;
}