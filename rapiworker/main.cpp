// rapiworker.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "pm.h"
#include "notify.h"
#include "Settings.h"
#include "regext.h"
#include "..\..\common\Process.hpp"
#include "..\..\common\ProcessFunctions.h"
#include "..\..\common\APIReady.h"
#include "..\..\common\XnaMessageBox.hpp"

const UINT  WM_STOP_RAPI        = (WM_USER + 124);

bool skipOnce = false;
HREGNOTIFY hNotify = NULL;

void StopRapiConnection()
{
	if (HWND hWnd = FindWindow(L"RapiClnt", NULL))
		SendMessage(hWnd, WM_STOP_RAPI, NULL, NULL);
}

BOOL CreateProcess7(LPWSTR path, LPWSTR args, DWORD wait)
{
	PROCESS_INFORMATION procInfo;
	if (::CreateProcess(path, args, NULL, NULL, FALSE, 0, NULL, NULL, NULL, &procInfo) == TRUE)
	{
		CloseHandle(procInfo.hThread);

		if (wait)
			WaitForSingleObject(procInfo.hProcess, wait);
		CloseHandle(procInfo.hProcess);
	}

	return FALSE;
}

void RunApps()
{
	UpdateSettings();
	if (dwRunRapi)
	{
		
        if (Diagnostics::Process::IsRunning(L"rapiclnt.exe"))
		{
			StopRapiConnection();

			Sleep(4000);
			// -q signal stops rapi client
			CreateProcess7(L"\\Windows\\rapiclnt.exe", L"-q", 10000);
		}

		int retries = 0;
		while (Diagnostics::Process::IsRunning(L"rapiclnt.exe") == true && retries < 10)
		{
			Sleep(500);
			retries++;
		}
		Diagnostics::Process::TerminateIfRunning(L"rapiclnt.exe");

		// -drndis_peer is an argument that directly runs active sync connection
		Diagnostics::Process::Start(L"\\Windows\\rapiclnt.exe", L"-drndis_peer", NULL, true);
	}
	if (dwRunDebugger)
	{

		// we don't need more than one connection manager. 
		HANDLE mutex = CreateMutex(NULL, TRUE, L"RapiWorker\\ConManLaunchedEvent");
		if (mutex)
		{
			if (GetLastError() == ERROR_ALREADY_EXISTS)
			{
				CloseHandle(mutex);
				return;
			}

			// ConManClient is proxy exe.
			Diagnostics::Process::Start(L"\\Windows\\ConManClient2.exe", NULL, NULL, true);

			CloseHandle(mutex);
		}
	}
}


ULONG RapiStartThread(LPVOID pParam)
{
	RunApps();
	return 0;
}


void DeviceAdded()
{
	RunApps();
}

void DeviceRemoved()
{
	StopRapiConnection();
}

VOID HandleAddDevice(DEVDETAIL *pd)
{
	if (wcsicmp(pd->szName, L"USB_STATUS") == 0)
	{
		//XnaMessageBox(NULL, pd->szName, L"Device added", NULL);
		DeviceAdded();
		
	}
}

VOID HandleRemoveDevice(DEVDETAIL *pd)
{
	if (wcsicmp(pd->szName, L"USB_STATUS") == 0)
	{
		//MessageBox(NULL, L"USB removed\n", L"RapiWorker", 0);
		DeviceRemoved();
	}
}

#define MAXTIMEOUT 120000

bool WaitForApis()
{
	if (WaitForAPIReady(SH_FILESYS_APIS, MAXTIMEOUT) != WAIT_OBJECT_0)
		return false;
	if (WaitForAPIReady(SH_SHELL, MAXTIMEOUT) != WAIT_OBJECT_0)
		return false;
	if (WaitForAPIReady(SH_DEVMGR_APIS, MAXTIMEOUT) != WAIT_OBJECT_0)
		return false;
	if (WaitForAPIReady(SH_SERVICES, MAXTIMEOUT) != WAIT_OBJECT_0)
		return false;
	if (WaitForAPIReady(SH_COMM, MAXTIMEOUT) != WAIT_OBJECT_0)
		return false;
	if (WaitForAPIReady(SH_WMGR, MAXTIMEOUT) != WAIT_OBJECT_0)
		return false;
	
	return true;
}

DWORD prevState = -1;
void Callback(HREGNOTIFY hNotify, DWORD dwUserData, PBYTE pData, UINT cbData)
{
	DWORD *data = (DWORD*)pData;
	DWORD dwData = *data;

	DWORD normalizedState = (dwData == 7) ? 1 : 0;
	if (prevState != normalizedState)
	{
		prevState = normalizedState;

		if (normalizedState)
			DeviceAdded();
		else
			DeviceRemoved();

	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	bool runned = false;
	if (WaitForApis() == false)
	{
		return -1;
	}
	if (argc > 1)
	{
		if (wcsicmp(argv[1], L"-now") == 0)
		{
			RunApps();
			runned = true;
		}
		else
		{
			wchar_t *stopSymbol;
			int num = wcstoul(argv[1], &stopSymbol, 10);
			if (num > 0 && num < 200)
			{
				SignalStarted(num);
				Sleep(40 * 1000);
			}
		}
	}
	HANDLE mutex = CreateMutex(NULL, TRUE, L"RapiWorkerMutex2");
	
	if (mutex)
	{
		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			CloseHandle(mutex);
			return -1;
		}
		
		if (runned == false)
		{
			DWORD dwCurState = 0;
			RegistryGetDWORD(HKEY_LOCAL_MACHINE, L"System\\State\\Connections\\DTPT", L"State", &dwCurState);
			Callback(NULL, 0, (PBYTE)&dwCurState, 4);
		}
		RegistryNotifyCallback(HKEY_LOCAL_MACHINE, L"System\\State\\Connections\\DTPT", L"State", Callback, 0, NULL, &hNotify);

		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		CloseHandle(mutex);
	}
	return 0;
}

