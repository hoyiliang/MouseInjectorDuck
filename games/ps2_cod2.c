//===========================================================
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
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

#define COD2_BASE 0x48FC64 //useless
#define COD2_CAMY 0x39A748 //(-90.f - 90.f)
#define COD2_CAMX 0x39A74C //(0.f - 360.f)
#define COD2_AIM_ASSIST_1 0x180470 //not here
#define COD2_AIM_ASSIST_2 0x180454 //not here
#define COD2_FOV 0x3C07E4
#define COD2_CUTSCENE_FLAG 0x49D858
#define COD2_PAUSE_FLAG 0x4A5CD0

static uint8_t PS2_COD2_Status(void);
static void PS2_COD2_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Call of Duty 2: Big Red One",
	PS2_COD2_Status,
	PS2_COD2_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway not supported for driver
};

const GAMEDRIVER *GAME_PS2_COD2 = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_COD2_Status(void)
{
	// SLUS_214.26
	return (PS2_MEM_ReadWord(0x00015B90) == 0x534C5553 && // SLUS_205.79; 53 4C 55 53
			PS2_MEM_ReadWord(0x00015B94) == 0x5F323132 && // 5F 32 31 32
			PS2_MEM_ReadWord(0x00015B98) == 0x2E32383B);  // 2E 32 38 3B
}

// static uint8_t PS2_COD2_DetectCam(void)
// {
// 	uint32_t tempCamBase = PS2_MEM_ReadUInt(RTCW_ACTUAL_CAMY_BASE_PTR);
// 	if (tempCamBase)
// 	{
// 		actualCamYBase = tempCamBase;
// 		return 1;
// 	}
// 	return 0;
// }

static void PS2_COD2_Inject(void)
{
	// disable aim-assist
	PS2_MEM_WriteUInt(COD2_AIM_ASSIST_1, 0x0);
	PS2_MEM_WriteUInt(COD2_AIM_ASSIST_2, 0x0);

	// PS2_MEM_WriteUInt(COD2_CAN_MOVE_JEEP_3RD_PERSON_CAM, 0x1);
	/*
	if (PS2_MEM_ReadUInt(COD2_IS_IN_GAME_CUTSCENE))
		return;

	if (PS2_MEM_ReadUInt(COD2_IS_PAUSED))
		return;

	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	// if (!PS2_RTCW_DetectCam())
	// 	return;
	*/

	float looksensitivity = (float)sensitivity / 40.f;
	float scale = 6.f;
	float fov = PS2_MEM_ReadFloat(COD2_FOV) / 55.f;

	float camX = PS2_MEM_ReadFloat(COD2_CAMX);
	camX -= (float)xmouse * looksensitivity / scale * fov;
	PS2_MEM_WriteFloat(COD2_CAMX, (float)camX);

	float camY = PS2_MEM_ReadFloat(COD2_CAMY);
	camY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale * fov;
	// game clamps internally to actual camY
	PS2_MEM_WriteFloat(COD2_CAMY, (float)camY);
}