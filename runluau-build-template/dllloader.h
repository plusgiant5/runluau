#pragma once

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include <stdint.h>

HMODULE load_embedded_dll(const char* name, const char* data, size_t size);