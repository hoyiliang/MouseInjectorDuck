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
#endif
#include "mouse.h"
#include "./manymouse/manymouse.h"

int32_t xmouse, ymouse; // holds mouse input data (used for gamedrivers)

#ifdef _WIN32
static POINT mouselock; // center screen X and Y var for mouse
#else
static int mouse_grabbed = 0;
static int uinput_fd = -1; // virtual mouse for forwarding buttons while grabbed
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
	// Create uinput virtual mouse for forwarding buttons/scroll while grabbed
	uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if (uinput_fd >= 0)
	{
		ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);
		ioctl(uinput_fd, UI_SET_KEYBIT, BTN_LEFT);
		ioctl(uinput_fd, UI_SET_KEYBIT, BTN_RIGHT);
		ioctl(uinput_fd, UI_SET_KEYBIT, BTN_MIDDLE);
		ioctl(uinput_fd, UI_SET_KEYBIT, BTN_SIDE);
		ioctl(uinput_fd, UI_SET_KEYBIT, BTN_EXTRA);
		ioctl(uinput_fd, UI_SET_EVBIT, EV_REL);
		ioctl(uinput_fd, UI_SET_RELBIT, REL_WHEEL);
		ioctl(uinput_fd, UI_SET_RELBIT, REL_HWHEEL);

		struct uinput_setup usetup = {0};
		usetup.id.bustype = BUS_USB;
		usetup.id.vendor = 0x1234;
		usetup.id.product = 0x5678;
		snprintf(usetup.name, UINPUT_MAX_NAME_SIZE, "MI Button Fwd");

		ioctl(uinput_fd, UI_DEV_SETUP, &usetup);
		ioctl(uinput_fd, UI_DEV_CREATE);
		fprintf(stderr, "[MOUSE] uinput button forwarder created\n");
	}
#endif
	return (ManyMouse_Init() > 0);
}
//==========================================================================
// Purpose: safely quit manymouse
//==========================================================================
void MOUSE_Quit(void)
{
#ifndef _WIN32
	if (mouse_grabbed)
	{
		ManyMouse_GrabMice(0);
		mouse_grabbed = 0;
	}
	if (uinput_fd >= 0)
	{
		ioctl(uinput_fd, UI_DEV_DESTROY);
		close(uinput_fd);
		uinput_fd = -1;
	}
#endif
	ManyMouse_Quit();
}
//==========================================================================
// Purpose: update cursor lock position
//==========================================================================
void MOUSE_Lock(void)
{
#ifdef _WIN32
	GetCursorPos(&mouselock);
#else
	// Toggle exclusive grab on the physical mice
	// When grabbed, the compositor never sees mouse events = cursor stays put
	mouse_grabbed = !mouse_grabbed;
	ManyMouse_GrabMice(mouse_grabbed);
	fprintf(stderr, "[MOUSE] Grab %s\n", mouse_grabbed ? "ON" : "OFF");
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
	(void)tickrate;
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
#ifndef _WIN32
		// Forward buttons and scroll through uinput so PCSX2 still sees clicks
		if (mouse_grabbed && uinput_fd >= 0)
		{
			struct input_event ev[2];
			memset(ev, 0, sizeof(ev));
			int send = 0;

			if (event.type == MANYMOUSE_EVENT_BUTTON)
			{
				ev[0].type = EV_KEY;
				ev[0].code = BTN_LEFT + event.item; // BTN_LEFT=0x110, +1=RIGHT, +2=MIDDLE...
				ev[0].value = event.value;
				send = 1;
			}
			else if (event.type == MANYMOUSE_EVENT_SCROLL)
			{
				ev[0].type = EV_REL;
				ev[0].code = (event.item == 0) ? REL_WHEEL : REL_HWHEEL;
				ev[0].value = event.value;
				send = 1;
			}

			if (send)
			{
				ev[1].type = EV_SYN;
				ev[1].code = SYN_REPORT;
				ev[1].value = 0;
				write(uinput_fd, ev, sizeof(ev));
			}
		}
#endif
	}
}