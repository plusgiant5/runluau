#include <Windows.h>
// analagous to dlopen
PLATFORM_HANDLE get_module_handle(const char* path, int __flags) {
    return GetModuleHandleA(path);
}

// analagous to dlclose
int close_handle(PLATFORM_HANDLE handle) {
    return CloseHandle(handle);
}