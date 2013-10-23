#include "stdafx.h"
#include "notify.h"

DEVICECHANGEHANDLER DeviceAdded = NULL;
DEVICECHANGEHANDLER DeviceRemoved = NULL;

static HANDLE g_hTerminateEvent;
static HANDLE g_hThread;

DWORD WINAPI DeviceNotifyThread(LPVOID lpParam)
{
    DWORD dwSize, dwFlags;
    HANDLE hReg = NULL;
    MSGQUEUEOPTIONS msgopts = {0};
    BYTE pPNPBuf[sizeof(DEVDETAIL) + 256] = {0};
    DEVDETAIL *pd = (DEVDETAIL*)pPNPBuf;
    GUID guid = {0};
    HANDLE pHandles[2] = {0};

    msgopts.dwSize = sizeof(MSGQUEUEOPTIONS);
    msgopts.dwFlags = 0;
    msgopts.dwMaxMessages = 0;
    msgopts.cbMaxMessage = sizeof(pPNPBuf);
    msgopts.bReadAccess = TRUE;
    
    pHandles[0] = CreateMsgQueue(NULL, &msgopts);
    pHandles[1] = g_hTerminateEvent;
    
    hReg = RequestDeviceNotifications(&guid, pHandles[0], TRUE);

    while (TRUE)
    {
        DWORD dwWaitCode = WaitForMultipleObjects(2, pHandles, FALSE, INFINITE);
        if (WAIT_OBJECT_0 == dwWaitCode)
        {
            if (ReadMsgQueue(pHandles[0], pd, sizeof(pPNPBuf), &dwSize, INFINITE, &dwFlags))
            {
                if (pd->fAttached)
				{
					if (DeviceAdded)
					{
						DeviceAdded(pd);
					}
				}
                else
				{
					if (DeviceRemoved)
					{
						DeviceRemoved(pd);
					}
				}
            }
        }
        else if ((WAIT_OBJECT_0 + 1) == dwWaitCode)
        {
            break;
        }
    }
    StopDeviceNotifications(hReg);
    CloseHandle(pHandles[0]);

    return 0;
}


BOOL InitDevNotify(DEVICECHANGEHANDLER tDeviceAdded, DEVICECHANGEHANDLER tDeviceRemoved)
{
	DeviceAdded = tDeviceAdded;
	DeviceRemoved = tDeviceRemoved;

    g_hTerminateEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    g_hThread = CreateThread(NULL, 0, DeviceNotifyThread, NULL, 0, NULL);
    return TRUE;
}
