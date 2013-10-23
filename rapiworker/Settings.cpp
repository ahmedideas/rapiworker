#include "stdafx.h"
#include "regext.h"

volatile DWORD dwRunRapi = 1;
volatile DWORD dwRunDebugger = 0;

void UpdateSettings()
{
	RegistryGetDWORD(HKEY_LOCAL_MACHINE, L"Software\\OEM\\RemoteAccess", L"RunRapi", (DWORD*)&dwRunRapi);
	RegistryGetDWORD(HKEY_LOCAL_MACHINE, L"Software\\OEM\\RemoteAccess", L"RunDebugger", (DWORD*)&dwRunDebugger);
}
