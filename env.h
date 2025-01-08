#pragma once

#include <Windows.h>

void SetPermanentEnvironmentVariable(const char* name, const char* value, bool system = false);