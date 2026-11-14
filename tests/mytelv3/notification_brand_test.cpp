#include <cstdio>
#include <string>
#include <windows.h>
#define APPLICATION_NAME "MyTel"
#include "notification_brand_production.inc"

int main() {
	const auto identity = L"MyTel.Tests." + std::to_wstring(GetCurrentProcessId());
	const auto key = L"Software\\Classes\\AppUserModelId\\" + identity;
	const wchar_t *icon = L"C:\\MyTel-test\\current-logo.png";
	const HRESULT result = RegisterAumidInRegistry(identity.c_str(), icon);
	wchar_t displayName[256] = {}, actualIcon[1024] = {};
	DWORD displayBytes = sizeof(displayName), iconBytes = sizeof(actualIcon);
	const auto displayResult = RegGetValueW(HKEY_CURRENT_USER, key.c_str(), L"DisplayName", RRF_RT_REG_SZ, nullptr,
	                                        displayName, &displayBytes);
	const auto iconResult =
	    RegGetValueW(HKEY_CURRENT_USER, key.c_str(), L"IconUri", RRF_RT_REG_SZ, nullptr, actualIcon, &iconBytes);
	RegDeleteTreeW(HKEY_CURRENT_USER, key.c_str());
	const bool ok = SUCCEEDED(result) && displayResult == ERROR_SUCCESS && iconResult == ERROR_SUCCESS &&
	                std::wstring(displayName) == L"MyTel" && std::wstring(actualIcon) == icon;
	std::printf(
	    "%s actual Windows notification registry: stable test identity, friendly MyTel display, current icon URI.\n",
	    ok ? "PASS" : "FAIL");
	return ok ? 0 : 1;
}
