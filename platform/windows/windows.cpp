#include <Windows.h>
PLATFORM_HANDLE open_library(const char* path, int __flags) {
    return GetModuleHandleA(path, __flags);
}

int close_handle(PLATFORM_HANDLE handle) {
    return CloseHandle(handle);
}