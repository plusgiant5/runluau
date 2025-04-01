#include "build.h"

inline uintptr_t align(uintptr_t value, uintptr_t alignment) {
	return (value + (alignment - 1)) & ~(alignment - 1);
}

DWORD runluau::build(std::string source, fs::path output_path, runluau::settings_run_build settings) {
	std::string bytecode = runluau::compile(source, settings.O, settings.g);
	if (bytecode[0] == '\0') {
		printf("Syntax error:\n%s\n", luau::beautify_syntax_error(std::string(bytecode.data() + 1)).c_str());
		return ERROR_INTERNAL_ERROR;
	}

	// Getting the template binary
	HMODULE self_handle = GetModuleHandleW(NULL);
	HRSRC resource_handle = FindResourceA(self_handle, MAKEINTRESOURCEA(101), "BINARY");
	if (!resource_handle) {
		DWORD last_error = GetLastError();
		printf("Failed to FindResourceA (0x%.8X)\n", last_error);
		return last_error;
	}
	HGLOBAL loaded = LoadResource(self_handle, resource_handle);
	DWORD resource_size = SizeofResource(self_handle, resource_handle);
	if (!loaded) {
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
	if (!pre_output_file) {
		int err = errno;
		wprintf(L"Invalid output path \"%s\"\n", output_path.c_str());
		return err;
	}
	pre_output_file.close();
	std::ofstream output_file(output_path, std::ios::binary | std::ios::out | std::ios::app);
	if (!output_file) {
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
	WORD new_section_index = nt_header->FileHeader.NumberOfSections++;
	IMAGE_SECTION_HEADER* next_section = &sections[new_section_index];
	size_t new_section_size = section_end - section_start;
	memcpy(&next_section->Name, ".runluau", 8);
	next_section->Misc.VirtualSize = static_cast<DWORD>(new_section_size);
	next_section->VirtualAddress = (DWORD)align((uintptr_t)highest_section->VirtualAddress + highest_section->Misc.VirtualSize, section_alignment);
	next_section->SizeOfRawData = (DWORD)align(new_section_size, file_alignment);
	next_section->PointerToRawData = highest_section->PointerToRawData + (DWORD)align(highest_section->SizeOfRawData, file_alignment);
	next_section->PointerToRelocations = NULL;
	next_section->PointerToLinenumbers = NULL;
	next_section->NumberOfRelocations = 0;
	next_section->NumberOfLinenumbers = 0;
	next_section->Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_INITIALIZED_DATA;
	nt_header->OptionalHeader.SizeOfImage = (DWORD)align((uintptr_t)next_section->VirtualAddress + next_section->Misc.VirtualSize, section_alignment);
	// Add in the newly made section by overwriting the headers
	std::fstream post_output_file(output_path, std::ios::binary | std::ios::in | std::ios::out);
	if (!post_output_file) {
		int err = errno;
		wprintf(L"Failed to open \"%s\" in final stage\n", output_path.c_str());
		return err;
	}
	post_output_file.seekp(0, std::ios::beg);
	post_output_file.write((char*)template_exe_buffer, 0x400);
	post_output_file.close();

	wprintf(L"Built to \"%s\"\n", fs::absolute(output_path).c_str());

	return ERROR_SUCCESS;
}