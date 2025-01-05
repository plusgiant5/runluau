#include "env.h"

DWORD SetPermanentEnvironmentVariable(const char* value, const char* data) {
	HKEY hKey;
	LSTATUS lOpenStatus = RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_SET_VALUE, &hKey);
	if (lOpenStatus == ERROR_SUCCESS) {
		LSTATUS lSetStatus = RegSetValueExA(hKey, value, 0, REG_SZ, (LPBYTE)data, strlen(data) + 1);
		RegCloseKey(hKey);

		if (lSetStatus == ERROR_SUCCESS) {
			SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)"Environment", SMTO_BLOCK, 100, NULL);
			return ERROR_SUCCESS;
		}
		return lSetStatus | 0x10000000;
	}

	return lOpenStatus;
}