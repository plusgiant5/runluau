#include <dlfcn.h>
#include "platform_linux.hpp"
// https://linux.die.net/man/3/dlopen
PLATFORM_HANDLE open_library(const char* path, int __flags) {
    return dlopen(path, __flags);
}

int close_handle(PLATFORM_HANDLE handle) {
    return dlclose(handle);
}