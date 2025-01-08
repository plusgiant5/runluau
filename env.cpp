#include "env.h"

#include <stdexcept>
#include <string>

void SetPermanentEnvironmentVariable(const char* name, const char* value, bool system) {
	HKEY key;
	LSTATUS open_status;
	if (system) {
		open_status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Control\\Session Manager\\Environment", 0, KEY_SET_VALUE, &key);
	} else {
		open_status = RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_SET_VALUE, &key);
	}
	if (open_status != ERROR_SUCCESS) {
		throw std::runtime_error(std::string("RegOpenKeyExA inside SetPermanentEnvironmentVariable failed with ") + std::to_string(open_status));
	}
	LSTATUS set_status = RegSetValueExA(key, name, 0, REG_SZ, (LPBYTE)value, (DWORD)(strlen(value) + 1));
	RegCloseKey(key);
	if (set_status != ERROR_SUCCESS) {
		throw std::runtime_error(std::string("RegSetValueExA inside SetPermanentEnvironmentVariable failed with ") + std::to_string(set_status));
	}
	SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)"Environment", SMTO_BLOCK, 100, NULL);
}