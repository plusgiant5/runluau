#include <dlfcn.h>
#include "platform_linux.hpp"
// https://linux.die.net/man/3/dlopen
// analagous to LoadLibrary
PLATFORM_HANDLE get_module_handle(const char* path, int __flags) {
    return dlopen(path, __flags);
}

// analagous to FreeLibrary
int close_handle(PLATFORM_HANDLE handle) {
    return dlclose(handle);
}