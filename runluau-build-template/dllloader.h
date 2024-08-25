#pragma once

#include <Windows.h>
#include <stdint.h>

HMODULE load_embedded_dll(const char* name, const char* data, size_t size);