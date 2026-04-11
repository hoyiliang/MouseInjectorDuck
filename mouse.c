//==========================================================================
// Mouse Injector for Dolphin
//==========================================================================
// Copyright (C) 2019-2020 Carnivorous
// All rights reserved.
//
// Mouse Injector is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, visit http://www.gnu.org/licenses/gpl-2.0.html
//==========================================================================
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/uinput.h>
#include <X11/Xlib.h>
#endif
#include "mouse.h"
#include "./manymouse/manymouse.h"

int32_t xmouse, ymouse; // holds mouse input data (used for gamedrivers)

#ifdef _WIN32
static POINT mouselock; // center screen X and Y var for mouse
#else
static int mouselock_x = 0, mouselock_y = 0;
static Display *xdisplay = NULL;
static Window xroot = 0;
static int uinput_fd = -1;
static int uinput_active = 0; // whether cursor warping is on
#endif
static ManyMouseEvent event; // hold current mouse event
static uint8_t lockmousecounter = 0; // limit SetCursorPos execution

uint8_t MOUSE_Init(void);
void MOUSE_Quit(void);
void MOUSE_Lock(void);
void MOUSE_Update(const uint16_t tickrate);

//==========================================================================
// Purpose: initialize manymouse and returns detected devices (0 = not found)
//==========================================================================
uint8_t MOUSE_Init(void)
{
#ifndef _WIN32
	// Open X11 display to query cursor position
	xdisplay = XOpenDisplay(NULL);
	if (xdisplay)
		xroot = DefaultRootWindow(xdisplay);

	// Create uinput device early so compositor detects it before we need it
	uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if (uinput_fd >= 0)
	{
		ioctl(uinput_fd, UI_SET_EVBIT, EV_REL);
		ioctl(uinput_fd, UI_SET_RELBIT, REL_X);
		ioctl(uinput_fd, UI_SET_RELBIT, REL_Y);
		ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);
		ioctl(uinput_fd, UI_SET_KEYBIT, BTN_LEFT);

		struct uinput_setup usetup = {0};
		usetup.id.bustype = BUS_USB;
		usetup.id.vendor = 0x1234;
		usetup.id.product = 0x5678;
		snprintf(usetup.name, UINPUT_MAX_NAME_SIZE, "MI Cursor Lock");

		ioctl(uinput_fd, UI_DEV_SETUP, &usetup);
		ioctl(uinput_fd, UI_DEV_CREATE);
		fprintf(stderr, "[MOUSE] uinput device created at init\n");
	}
	else
		fprintf(stderr, "[MOUSE] Failed to open /dev/uinput: %m\n");
#endif
	return (ManyMouse_Init() > 0);
}
//==========================================================================
// Purpose: safely quit manymouse
//==========================================================================
void MOUSE_Quit(void)
{
	ManyMouse_Quit();
#ifndef _WIN32
	if (uinput_fd >= 0)
	{
		ioctl(uinput_fd, UI_DEV_DESTROY);
		close(uinput_fd);
		uinput_fd = -1;
	}
	if (xdisplay)
	{
		XCloseDisplay(xdisplay);
		xdisplay = NULL;
	}
#endif
}
//==========================================================================
// Purpose: update cursor lock position
//==========================================================================
void MOUSE_Lock(void)
{
#ifdef _WIN32
	GetCursorPos(&mouselock);
#else
	if (xdisplay)
	{
		Window child;
		int win_x, win_y;
		unsigned int mask;
		XQueryPointer(xdisplay, xroot, &xroot, &child,
			&mouselock_x, &mouselock_y, &win_x, &win_y, &mask);

		uinput_active = !uinput_active;
		fprintf(stderr, "[MOUSE] Lock %s at %d,%d\n",
			uinput_active ? "ON" : "OFF", mouselock_x, mouselock_y);
	}
#endif
}
//==========================================================================
// Purpose: update xmouse/ymouse with mouse input
// Changed Globals: lockmousecounter, xmouse, ymouse, event
//==========================================================================
void MOUSE_Update(const uint16_t tickrate)
{
#ifdef _WIN32
	if(tickrate > 8) // if game driver tickrate is over 8ms, do not bother limiting SetCursorPos calls
		SetCursorPos(mouselock.x, mouselock.y); // set mouse position back to lock position
	else
	{
		if(lockmousecounter % 25 == 0) // don't execute every tick
			SetCursorPos(mouselock.x, mouselock.y); // set mouse position back to lock position
		lockmousecounter++; // overflow pseudo-counter
	}
#else
	if (uinput_fd >= 0 && uinput_active)
	{
		// Counteract physical mouse movement to keep cursor locked
		// xmouse/ymouse still hold last tick's accumulated movement
		if (xmouse != 0 || ymouse != 0)
		{
			struct input_event ev[3];
			memset(ev, 0, sizeof(ev));
			ev[0].type = EV_REL; ev[0].code = REL_X; ev[0].value = -xmouse;
			ev[1].type = EV_REL; ev[1].code = REL_Y; ev[1].value = -ymouse;
			ev[2].type = EV_SYN; ev[2].code = SYN_REPORT; ev[2].value = 0;
			write(uinput_fd, ev, sizeof(ev));
		}
	}
#endif
	xmouse = ymouse = 0; // reset mouse input
	while(ManyMouse_PollEvent(&event))
	{
		if(event.type == MANYMOUSE_EVENT_RELMOTION)
		{
			if(event.item == 0)
				xmouse += event.value;
			else
				ymouse += event.value;
		}
	}
}