#pragma once

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <list>
#include <format>
#include <thread>
#include <mutex>

#include "colors.h"

#define PROJECT_EXPORTS
#include <luau.h>