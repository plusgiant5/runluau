#include <stdio.h>
#include <Windows.h>

#include <cerrno>
#include <string>
#include <vector>
#include <unordered_set>
#include <fstream>
#include <format>

#include "file.hpp"
#include "execute.h"
#include "plugins.h"

void help_then_exit(std::string notice_message) {
	printf(R"(%s Help is below:

runluau mode options...

Modes:
	- Run a script. `args` is optional. If specified, everything after it will be passed into the script:
	runluau run path\to\script <options> --args <...>
	- Build a script to an executable, with an optional list of plugins to embed into the output. If `plugins` isn't specified, it will use every plugin installed:
	runluau build path\to\script path\to\output <options> --plugins <...>
Options:
	- [default: 1] Optimization level 0-2:
	-O <n>
	- [default: 1] Debug level 0-2:
	-g <n>
)", notice_message.c_str());
	exit(ERROR_INVALID_PARAMETER);
}


runluau::settings read_args(std::vector<std::string>& args, size_t starting_point) {
	std::vector<std::string> script_args;
	std::vector<std::string> plugins;
	runluau::settings settings;
	std::unordered_set<std::string> used_args;
	bool reading_args = false;
	bool reading_plugins = false;
	for (size_t i = starting_point; i < args.size(); i++) {
		const auto& arg = args[i];
		if (reading_args) [[likely]] {
			script_args.push_back(arg);
		} else if (reading_plugins) [[unlikely]] {
			plugins.push_back(arg);
		} else if (arg == "--args") [[unlikely]] {
			reading_args = true;
		} else if (arg == "--plugins") [[unlikely]] {
			reading_plugins = true;
		} else {
			if (used_args.find(arg) != used_args.end()) {
				printf("Argument `%s` specified twice.", arg.c_str());
				exit(ERROR_INVALID_PARAMETER);
			}
			if (arg == "-O" || arg == "-g") {
				if (++i >= args.size())
					help_then_exit(std::format("Expected value after argument `{}`.", arg));
				int value;
				try {
					value = std::stoi(args[i]);
				} catch (...) {
					help_then_exit(std::format("Value \"{}\" is not a valid integer.", args[i]));
				}
				if (value < 0 || value > 2)
					help_then_exit(std::format("Value \"{}\" not in between 0 and 2.", value));
				if (arg == "-O")
					settings.O = value;
				else
					settings.g = value;
				used_args.insert(arg);
			} else {
				help_then_exit(std::format("Unknown argument `{}`.", arg));
			}
		}
	}
	if (reading_args)
		settings.script_args = script_args;
	if (reading_plugins)
		settings.plugins = plugins;
	return settings;
}

inline uintptr_t align(uintptr_t value, uintptr_t alignment) {
	return (value + (alignment - 1)) & ~(alignment - 1);
}
BOOL WINAPI ctrl_handler(DWORD type) {
	switch (type) {
	case CTRL_C_EVENT:
		printf("Exiting\n");
		__fastfail(ERROR_PROCESS_ABORTED);
		return TRUE;
	default:
		return FALSE;
	}
}
int main(int argc, char* argv[]) {
	SetConsoleCtrlHandler(ctrl_handler, TRUE);
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	if (console) {
		DWORD mode;
		GetConsoleMode(console, &mode);
		mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(console, mode);
	}
	std::vector<std::string> args(argv + 1, argv + argc);
	if (args.size() < 2) [[unlikely]]
		help_then_exit("Not enough arguments.");
	std::string mode = args[0];
	if (mode == "run") {
		read_file_info script = read_script(args[1]);
		runluau::settings settings = read_args(args, 2);
		if (settings.plugins != std::nullopt) [[unlikely]]
			help_then_exit("Cannot specify `--plugins` in `run` mode.");

		runluau::execute(script.contents, settings);
	} else if (mode == "build") {
		if (args.size() < 3) [[unlikely]]
			help_then_exit("Not enough arguments.");
		std::string source = read_script(args[1]).contents;
		fs::path output_path = args[2];
		runluau::settings settings = read_args(args, 3);
		if (settings.script_args != std::nullopt) [[unlikely]]
			help_then_exit("Cannot specify `--args` in `build` mode.");

		std::string bytecode = luau::wrapped_compile(source, settings.O, settings.g);
		if (bytecode[0] == '\0') {
			printf("Syntax error:\n%s\n", bytecode.data() + 1);
			return ERROR_INTERNAL_ERROR;
		}

		// Getting the template binary
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
		void* resource_buffer = LockResource(loaded); // Readonly
		void* template_exe_buffer = malloc(resource_size);
		if (template_exe_buffer == NULL) {
			printf("Allocation error (allocation size 0x%X)\n", resource_size);
			return ERROR_OUTOFMEMORY;
		}
		memcpy(template_exe_buffer, resource_buffer, resource_size);

		// Open in trunc first to clear contents
		std::ofstream pre_output_file(output_path, std::ios::binary | std::ios::out | std::ios::trunc);
		if (!pre_output_file) [[unlikely]] {
			int err = errno;
			wprintf(L"Invalid output path \"%s\"\n", output_path.c_str());
			return err;
		}
		pre_output_file.close();
		std::ofstream output_file(output_path, std::ios::binary | std::ios::out | std::ios::app);
		if (!output_file) [[unlikely]] {
			int err = errno;
			wprintf(L"Failed to open \"%s\" in append mode\n", output_path.c_str());
			return err;
		}
		output_file.write((char*)template_exe_buffer, resource_size);
		uintptr_t section_start = output_file.tellp();
		auto dos_header = (IMAGE_DOS_HEADER*)(template_exe_buffer);
		auto nt_header = (IMAGE_NT_HEADERS*)((uintptr_t)template_exe_buffer + dos_header->e_lfanew);
		auto sections = (IMAGE_SECTION_HEADER*)((uintptr_t)(nt_header)+sizeof(*nt_header));
		DWORD file_alignment = nt_header->OptionalHeader.FileAlignment;
		DWORD section_alignment = nt_header->OptionalHeader.SectionAlignment;

		// Writing the overlay
		// The length of the bytecode, followed by the bytecode
		size_t bytecode_size = bytecode.size();
		output_file.write((char*)&bytecode_size, sizeof(bytecode_size));
		output_file.write(bytecode.data(), bytecode_size);
		// For each plugin, write its name, size, and contents
		std::vector<std::string> plugins;
		if (settings.plugins.has_value()) {
			plugins = settings.plugins.value();
		} else {
			for (const auto& file : fs::directory_iterator(get_plugins_folder())) {
				plugins.push_back(file.path().filename().string());
			}
		}
		size_t plugins_count = plugins.size();
		output_file.write((char*)&plugins_count, sizeof(plugins_count));
		for (std::string short_plugin_name : plugins) {
			auto plugin_info = read_plugin(short_plugin_name);
			std::string plugin_contents = plugin_info.contents;
			std::string plugin_name = plugin_info.path.filename().string();
			printf("Plugin %s, size %llX\n", plugin_name.c_str(), plugin_contents.size());
			output_file << plugin_name << '\0';
			size_t plugin_contents_size = plugin_contents.size();
			output_file.write((char*)&plugin_contents_size, sizeof(plugin_contents_size));
			output_file.write(plugin_contents.data(), plugin_contents.size());
		}

		uintptr_t unaligned_end = output_file.tellp();
		size_t zero_count = align(unaligned_end, section_alignment) - unaligned_end;
		if (zero_count > 0) {
			void* zeroes = malloc(zero_count);
			if (zeroes == NULL) {
				printf("Allocation error 2 (allocation size 0x%llX)\n", zero_count);
				return ERROR_OUTOFMEMORY;
			}
			memset(zeroes, 0, zero_count);
			output_file.write((char*)zeroes, zero_count);
		}
		uintptr_t section_end = output_file.tellp();
		output_file.close();
		// Create section to point to the overlay
		IMAGE_SECTION_HEADER* highest_section = nullptr;
		for (size_t i = 0; i < nt_header->FileHeader.NumberOfSections; i++) {
			if (highest_section) {
				if (sections[i].PointerToRawData > highest_section->PointerToRawData) {
					highest_section = &sections[i];
				}
			} else {
				highest_section = &sections[i];
			}
		}
		if (highest_section == nullptr) {
			printf("Failed to find sections\n");
			return ERROR_NOT_FOUND;
		}
		// Create a section to point to the overlay
		WORD new_section_index = nt_header->FileHeader.NumberOfSections++;
		IMAGE_SECTION_HEADER* next_section = &sections[new_section_index];
		size_t new_section_size = section_end - section_start;
		memcpy(&next_section->Name, ".runluau", 8);
		next_section->Misc.VirtualSize = static_cast<DWORD>(new_section_size);
		next_section->VirtualAddress = (DWORD)align(highest_section->VirtualAddress + highest_section->Misc.VirtualSize, section_alignment);
		next_section->SizeOfRawData = (DWORD)align(new_section_size, file_alignment);
		next_section->PointerToRawData = highest_section->PointerToRawData + (DWORD)align(highest_section->SizeOfRawData, file_alignment);
		next_section->PointerToRelocations = NULL;
		next_section->PointerToLinenumbers = NULL;
		next_section->NumberOfRelocations = 0;
		next_section->NumberOfLinenumbers = 0;
		next_section->Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA;
		nt_header->OptionalHeader.SizeOfImage = (DWORD)align(next_section->VirtualAddress + next_section->Misc.VirtualSize, section_alignment);
		// Add in the newly made section by overwriting the headers
		std::fstream post_output_file(output_path, std::ios::binary | std::ios::in | std::ios::out);
		if (!post_output_file) [[unlikely]] {
			int err = errno;
			wprintf(L"Failed to open \"%s\" in final stage\n", output_path.c_str());
			return err;
		}
		post_output_file.seekp(0, std::ios::beg);
		post_output_file.write((char*)template_exe_buffer, 0x400);
		post_output_file.close();

		wprintf(L"Built to \"%s\"\n", fs::absolute(output_path).c_str());
	} else [[unlikely]] {
		help_then_exit(std::format("Unknown mode `{}`.", mode));
	}
	return 0;
}