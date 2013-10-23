#pragma once
#ifndef NOTIFY_H
#define NOTIFY_H

#include <msgqueue.h>
#include <pnp.h>

BOOL InitDevNotify();

typedef void (*DEVICECHANGEHANDLER)(DEVDETAIL *pd);

BOOL InitDevNotify(DEVICECHANGEHANDLER tDeviceAdded, DEVICECHANGEHANDLER tDeviceRemoved);

#endif
