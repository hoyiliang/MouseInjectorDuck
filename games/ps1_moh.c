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
#include <stdio.h>
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"
#include <inttypes.h>

//#define MOH_CAMY 0xEEDA6
//#define MOH_CAMX 0xEEDB2
//#define MOH_FOV gotta find this
#define MOH_PLAYERBASE_POINTER 0x99F4C
#define MOH_CAMY_OFFSET 0x6E //6E the correct one
#define MOH_CAMX_OFFSET 0x7A
#define MOH_AIM_FLAG 0x10C
#define MOH_CAMY_PAIM_OFFSET 0x70 //6C
#define MOH_CAMX_PAIM_OFFSET 0x7C
#define MOH_RIGHT_STICK_Y_OFFSET 0x93B71
#define MOH_RIGHT_STICK_Y_OFFSET2 0xD0C89

static uint8_t PS1_MOH_Status(void);
static uint8_t PS1_MOH_DetectCam(void);
static void PS1_MOH_Inject(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"Medal of Honor",
	PS1_MOH_Status,
	PS1_MOH_Inject,
	1, // 1000 Hz tickrate
	0 // crosshair sway supported for driver
};

//==========================================================================
// TODO:
// Crouch flag; bazooka precise aim different flag?
// Turret flag/controls (seems assy);
// Precise aim Y-axis camera object scroll (this will suck the most), it requires right stick input, no stick no scroll, need to pass a value or look at the op
//==========================================================================

const GAMEDRIVER *GAME_PS1_MEDALOFHONOR = &GAMEDRIVER_INTERFACE;

static float xAccumulator = 0.f;
static float yAccumulator = 0.f;
static uint32_t tempBase = 0;
static uint32_t camBase = 0;
static uint16_t Aim_Flag = 0;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS1_MOH_Status(void)
{
	return (PS1_MEM_ReadWord(0x92D4) == 0x534C5553U && PS1_MEM_ReadWord(0x92D8) == 0x5F303039U && PS1_MEM_ReadWord(0x92DC) == 0x2E37343BU);
}

static uint8_t PS1_MOH_DetectCam(void)
{
	uint32_t tempBase = PS1_MEM_ReadPointer(MOH_PLAYERBASE_POINTER);
	if (tempBase != 0)
	{
		camBase = tempBase;
		return 1;
	}

	return 0;
}

//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS1_MOH_Inject(void)
{	
	if (!PS1_MOH_DetectCam())
		return;
	
	if(xmouse == 0 && ymouse == 0) // if mouse is idle
		return;
	
	const float looksensitivity = (float)sensitivity / 20.f;
	const float scale = 4000.f;
	
	uint32_t camBase = PS1_MEM_ReadPointer(MOH_PLAYERBASE_POINTER);
	
	int32_t paimY = PS1_MEM_ReadInt(camBase + MOH_CAMY_PAIM_OFFSET);
	int32_t paimX = PS1_MEM_ReadInt(camBase + MOH_CAMX_PAIM_OFFSET);
	
	float paimYF = (float)paimY; //oh here...
	float paimXF = (float)paimX;

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = ym * looksensitivity * scale;
	float dx = (float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&paimXF, &xAccumulator, xmouse, dx);
	AccumulateAddRemainder(&paimYF, &yAccumulator, -ym, dy);

	Aim_Flag = PS1_MEM_ReadHalfword(camBase - MOH_AIM_FLAG);

	if (Aim_Flag == 0x0020 && paimXF > -984040.f && paimXF < 984040.f) //how the hell i got floats here TODO: necessary to NOP some functions, paim also has different values with zoom aim etc or is it only 1 byte?
	{
		
	//paimXF = ClampFloat(paimXF, -3916800.f, 3916800.f);

	
	//paimYF = ClampFloat(paimYF, -2842528.f, 2842528.f);
	//paimYF = ClampFloat(paimYF, -3842528.f, 3842528.f);
	//if (paimYF > -984040.f && paimYF < 984040.f) {
	uint8_t stick_y = PS1_MEM_ReadByte(MOH_RIGHT_STICK_Y_OFFSET); // this needs a rewrite, it jumps from two states nothing in between, it sucks ass
		if (ymouse < 0)
			stick_y = 0x0;
		else
			stick_y = 0xFF;

	//PS1_MEM_WriteByte(MOH_RIGHT_STICK_Y_OFFSET2, stick_y); //i guess this input must be there to scroll, logic could be that if paimY is near the range input right stick input, or check the instructions... naahh
	//}

	//PS1_MEM_WriteInt(camBase+MOH_CAMY_PAIM_OFFSET, (int32_t)paimYF);
	PS1_MEM_WriteInt(camBase + MOH_CAMX_PAIM_OFFSET, (int32_t)paimXF);
	}
	else {
	uint16_t camX = PS1_MEM_ReadHalfword(camBase+MOH_CAMX_OFFSET);
	uint16_t camY = PS1_MEM_ReadHalfword(camBase+MOH_CAMY_OFFSET);
	float camXF = (float)camX;
	float camYF = (float)camY;

	const float looksensitivity = (float)sensitivity / 30.f;
	const float scale = 1.f;

	float dx = (float)xmouse * looksensitivity * scale;
	AccumulateAddRemainder(&camXF, &xAccumulator, xmouse, dx);

	float ym = (float)(invertpitch ? -ymouse : ymouse);
	float dy = -ym * looksensitivity * scale;
	AccumulateAddRemainder(&camYF, &yAccumulator, -ym, dy);

	// clamp y-axis
	if (camYF > 60000 && camYF < 64854)
		camYF = 64854;
	if (camYF > 682 && camYF < 4000)
		camYF = 682;

	PS1_MEM_WriteHalfword(camBase+MOH_CAMX_OFFSET, (uint16_t)camXF);
	PS1_MEM_WriteHalfword(camBase+MOH_CAMY_OFFSET, (uint16_t)camYF);
	}
}