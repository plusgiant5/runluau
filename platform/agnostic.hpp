#ifdef _WIN32
#include <Windows.h>
#define PLATFORM_HANDLE HANDLE
#else
#define PLATFORM_HANDLE void*
#endif