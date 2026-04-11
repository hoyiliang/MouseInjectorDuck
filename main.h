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
#include <string.h>

#define DOLPHINVERSION "emulators"
#define BUILDINFO "(v0.43 - "__DATE__")"
#define LINE "____________________________________________________________________________"

// cross-platform sleep macro (milliseconds)
#ifdef _WIN32
#define MI_SLEEP(ms) Sleep(ms)
#else
#include <unistd.h>
#define MI_SLEEP(ms) usleep((ms) * 1000)
#endif

// input for interface
#ifdef _WIN32
#define K_1 GetAsyncKeyState(0x31) // key '1'
#define K_2 GetAsyncKeyState(0x32) // key '2'
#define K_3 GetAsyncKeyState(0x33) // key '3'
#define K_4 GetAsyncKeyState(0x34) // key '4'
#define K_5 GetAsyncKeyState(0x35) // key '5'
#define K_6 GetAsyncKeyState(0x36) // key '6'
#define K_7 GetAsyncKeyState(0x37) // key '7'
#define K_8 GetAsyncKeyState(0x38) // key '8'
#define K_9 GetAsyncKeyState(0x39) // key '9'
#define K_0 GetAsyncKeyState(0x30) // key '0'
#define K_CTRL0 (GetAsyncKeyState(0x11) && GetAsyncKeyState(0x30) || GetAsyncKeyState(0x30) && GetAsyncKeyState(0x11)) // key combo control + '0'
#define K_CTRL1 (GetAsyncKeyState(0x11) && GetAsyncKeyState(0x31) || GetAsyncKeyState(0x31) && GetAsyncKeyState(0x11)) // key combo control + '1'
#define K_CTRL2 (GetAsyncKeyState(0x11) && GetAsyncKeyState(0x32) || GetAsyncKeyState(0x32) && GetAsyncKeyState(0x11)) // key combo control + '1'
#define K_PLUS (GetAsyncKeyState(0x6B) || GetAsyncKeyState(0xBB)) // key '+'
#define K_MINUS (GetAsyncKeyState(0x6D) || GetAsyncKeyState(0xBD)) // key '-'
#define K_INSERT GetAsyncKeyState(0x2D) // key 'Insert'
#else
// Linux: key input via non-blocking terminal read (set up in main.c)
// These check a global lastkey var that gets updated each loop iteration
extern int linux_lastkey;
extern int linux_ctrl_held;
extern void linux_poll_key(void);
#define K_1 (linux_lastkey == '1')
#define K_2 (linux_lastkey == '2')
#define K_3 (linux_lastkey == '3')
#define K_4 (linux_lastkey == '4')
#define K_5 (linux_lastkey == '5')
#define K_6 (linux_lastkey == '6')
#define K_7 (linux_lastkey == '7')
#define K_8 (linux_lastkey == '8')
#define K_9 (linux_lastkey == '9')
#define K_0 (linux_lastkey == '0')
#define K_CTRL0 (linux_ctrl_held && linux_lastkey == '0')
#define K_CTRL1 (linux_ctrl_held && linux_lastkey == '1')
#define K_CTRL2 (linux_ctrl_held && linux_lastkey == '2')
#define K_PLUS (linux_lastkey == '+' || linux_lastkey == '=')
#define K_MINUS (linux_lastkey == '-' || linux_lastkey == '_')
#define K_INSERT (linux_lastkey == 0x100)
#endif
#if _MSC_VER && !__INTEL_COMPILER // here because some MSVC versions only support __inline :/
#define inline __inline
#endif

inline float ClampFloat(const float value, const float min, const float max)
{
	const float test = value < min ? min : value;
	return test > max ? max : test;
}

inline int32_t ClampInt(const int32_t value, const int32_t min, const int32_t max)
{
	const int32_t test = value < min ? min : value;
	return test > max ? max : test;
}

inline uint16_t ClampHalfword(const uint16_t value, const uint16_t min, const uint16_t max)
{
	const int16_t test = value < min ? min : value;
	return test > max ? max : test;
}

inline uint8_t FloatsEqual(const float f1, const float f2)
{
	const float epsilon = 0.0001;
	return (f1 - f2) < epsilon;
}

extern void AccumulateAddRemainder(float *value, float *accumulator, float dir, float dx);

extern uint8_t sensitivity;
extern uint8_t crosshair;
extern uint8_t invertpitch;
extern uint8_t altPCSX2hook;
extern uint8_t optionToggle;
extern float out;
extern float out2;
extern float out3;
extern float preSinOut;
extern float preCosOut;
extern float totalAngleOut;
extern uint32_t uIntOut1;
extern uint32_t uIntOut2;
extern char titleOut[256];
extern uint64_t emuoffsetOut;