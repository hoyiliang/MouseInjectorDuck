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
#include "../main.h"
#include "../memory.h"
#include "../mouse.h"
#include "game.h"

#define PI 3.14159265f
#define TAU 6.2831853f

//check the instructions to determine what elf is loaded
// ON FOOT SECTIONS
#define PS2_007_NF_CAMERA_PTR 0x2D88E0
#define PS2_007_NF_CAMERA_BASE_OFFSET_X 0x54
#define PS2_007_NF_CAMERA_BASE_OFFSET_Y 0x9B8
#define PS2_007_NF_AIM_FLAG 0x240
#define PS2_007_NF_AIM_OFFSET_X 0x228
#define PS2_007_NF_AIM_OFFSET_Y 0x22C
#define PS2_007_NF_SENTRY_FLAG 0xF6
#define PS2_007_NF_SENTRY_X_SP 0x2EED4 //seems hardcoded to one level, too far from the base
#define PS2_007_NF_SENTRY_Y_SP 0x2EED0 //same
#define PS2_007_NF_SENTRY_X_MP 0x1424
#define PS2_007_NF_SENTRY_Y_MP 0x1420
#define PS2_007_NF_FOV 0x9E0
#define PS2_007_NF_MENU_AND_PAUSE_FLAG 0x2E3D7C
#define PS2_007_NF_MP_FLAG 0x2800
#define PS2_007_NF_SCOPE 0x9E8

// VEHICLE SECTION - USE FOV OP AS A CHECK - NO HOR FOV > NO VEH TURRET
#define PS2_007_NF_VEHICLE_FLAG 0x1A506C
#define PS2_007_NF_VEHICLE_CAMERA_PTR 0x437E20
#define PS2_007_NF_VEHICLE_OFFSET_FOV 0xA4
#define PS2_007_NF_VEHICLE_OFFSET_X 0x1DC
#define PS2_007_NF_VEHICLE_OFFSET_Y 0x1D8

static uint8_t PS2_007_NF_Status(void);
static void PS2_007_NF_Inject(void);

void printdebug(uint64_t val);

static uint32_t Cam_Base = 0;
static uint32_t Veh_Cam_Base = 0;
static uint16_t On_Foot_Flag = 0;
static uint16_t Sentry_Flag = 0;
static uint16_t MP_Flag = 0;
static uint8_t Menu_Flag = 0;
static uint8_t Aim_Flag = 0;
static uint32_t Diff_ELF_Flag = 0;
static uint32_t Scope = 0;

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
	{
		"007: Nightfire",
		PS2_007_NF_Status,
		PS2_007_NF_Inject,
		1, // 1000 Hz tickrate
		0  // crosshair sway supported for driver
};

const GAMEDRIVER *GAME_PS2_007_NF = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: return 1 if game is detected
//==========================================================================
static uint8_t PS2_007_NF_Status(void)
{
	return (PS2_MEM_ReadWord(0x00093390) == 0x534C5553 && // SLUS_205.79; 53 4C 55 53
			PS2_MEM_ReadWord(0x00093394) == 0x5F323035 && // 5F 32 30 35
			PS2_MEM_ReadWord(0x00093398) == 0x2E37393B);  // 2E 37 39 3B
}
//==========================================================================
// Purpose: calculate mouse look and inject into current game
//==========================================================================
static void PS2_007_NF_Inject(void)
{
	Cam_Base = PS2_MEM_ReadPointer(PS2_007_NF_CAMERA_PTR);
	Veh_Cam_Base = PS2_MEM_ReadPointer(PS2_007_NF_VEHICLE_CAMERA_PTR);
	float fov = PS2_MEM_ReadFloat(Cam_Base + PS2_007_NF_FOV);
	float vfov = PS2_MEM_ReadFloat(Veh_Cam_Base + PS2_007_NF_VEHICLE_OFFSET_FOV);
	float looksensitivity = (float)sensitivity;
	float scale = 10000.f;
	uint32_t Scope = PS2_MEM_ReadUInt(Cam_Base + PS2_007_NF_SCOPE);

	//Flags
	On_Foot_Flag = (PS2_MEM_ReadUInt(Cam_Base) != 0x98989898) && (PS2_MEM_ReadUInt(Cam_Base+0x10) == 0x00000000);
	Menu_Flag = PS2_MEM_ReadUInt16(PS2_007_NF_MENU_AND_PAUSE_FLAG);
	Aim_Flag = PS2_MEM_ReadUInt8(Cam_Base + PS2_007_NF_AIM_FLAG);
	Sentry_Flag = PS2_MEM_ReadUInt16(Cam_Base + PS2_007_NF_SENTRY_FLAG);
	MP_Flag = PS2_MEM_ReadUInt16(Cam_Base + PS2_007_NF_MP_FLAG);
	// Homing_Missile_Flag - couldnt find it in the memory - needs op patching
	Diff_ELF_Flag = (PS2_MEM_ReadUInt(PS2_007_NF_VEHICLE_FLAG) == 0xE680000C);
	// Equinox_Flag - needs patching as well

	if (xmouse == 0 && ymouse == 0)
		return;

	if (On_Foot_Flag == 1 && (Sentry_Flag != 0xC && Sentry_Flag != 0x6 && Sentry_Flag != 0xA && Sentry_Flag != 0xD)) //Sentry flag is for player status - 000D - death, 0006 - climb, 000C - sentry - only in SP - playerbase has slightly different layout in MP
	{
		if ((Menu_Flag == 0x0000))
		{
			float aimX = PS2_MEM_ReadFloat(Cam_Base + PS2_007_NF_AIM_OFFSET_X);
			float aimY = PS2_MEM_ReadFloat(Cam_Base + PS2_007_NF_AIM_OFFSET_Y);

			if ((Aim_Flag == 0x00) &&
				(Scope <= 0x40400000) &&
				(aimX >= -0.296f && aimX <= 0.296f) &&
				(aimY >= -0.4f && aimY <= 0.4f))
			{
				aimX += (float)xmouse * looksensitivity / scale * (1.f / fov);
				aimY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale * (1.f / fov);

				PS2_MEM_WriteFloat(Cam_Base + PS2_007_NF_AIM_OFFSET_X, (float)aimX);
				PS2_MEM_WriteFloat(Cam_Base + PS2_007_NF_AIM_OFFSET_Y, (float)aimY);
			}
			else
			{
				if ((Aim_Flag == 0x02) || (Aim_Flag == 0x03)) // this disables autocentering
				{
					PS2_MEM_WriteUInt8(Cam_Base + PS2_007_NF_AIM_FLAG, 0x01);
				}
				float camX = PS2_MEM_ReadFloat(Cam_Base + PS2_007_NF_CAMERA_BASE_OFFSET_X);
				float camY = PS2_MEM_ReadFloat(Cam_Base + PS2_007_NF_CAMERA_BASE_OFFSET_Y);

				camX -= (float)xmouse * looksensitivity / scale * (1.f / fov);
				camY -= (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale * (1.f / fov);

				while (camX <= -PI)
					camX += TAU;
				while (camX >= PI)
					camX -= TAU;

				if (camY >= 1.0f)
					camY = 1.0f;
				if (camY <= -1.0f)
					camY = -1.0f;

				PS2_MEM_WriteFloat(Cam_Base + PS2_007_NF_CAMERA_BASE_OFFSET_X, (float)camX);
				PS2_MEM_WriteFloat(Cam_Base + PS2_007_NF_CAMERA_BASE_OFFSET_Y, (float)camY);
			}
		}
	}
	if (Sentry_Flag == 0xC && MP_Flag != 0xFFFF) //there must be a check, if i write to this area I break the bots in MP
	{
		float sentryX = PS2_MEM_ReadFloat(Cam_Base + PS2_007_NF_SENTRY_X_SP);
		float sentryY = PS2_MEM_ReadFloat(Cam_Base + PS2_007_NF_SENTRY_Y_SP);

		sentryX -= (float)xmouse * looksensitivity / scale / 5.f * (1.f / fov);
		sentryY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale / 2.f * (1.f / fov);
		
		if (sentryY >= PI/2)
			sentryY = PI/2;
		if (sentryY <= -PI/2)
			sentryY = -PI/2;

		PS2_MEM_WriteFloat(Cam_Base + PS2_007_NF_SENTRY_X_SP, (float)sentryX);
		PS2_MEM_WriteFloat(Cam_Base + PS2_007_NF_SENTRY_Y_SP, (float)sentryY);
	}
	if (Sentry_Flag == 0xC && MP_Flag == 0xFFFF)
	{
		float sentryX_MP = PS2_MEM_ReadFloat(Cam_Base + PS2_007_NF_SENTRY_X_MP);
		float sentryY_MP = PS2_MEM_ReadFloat(Cam_Base + PS2_007_NF_SENTRY_Y_MP);

		sentryX_MP -= (float)xmouse * looksensitivity / scale / 2.f * (1.f / fov);
		sentryY_MP += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale / 2.f * (1.f / fov);
		
		if (sentryY_MP >= PI/2)
			sentryY_MP = PI/2;
		if (sentryY_MP <= -PI/2)
			sentryY_MP = -PI/2;

		PS2_MEM_WriteFloat(Cam_Base + PS2_007_NF_SENTRY_X_MP, (float)sentryX_MP);
		PS2_MEM_WriteFloat(Cam_Base + PS2_007_NF_SENTRY_Y_MP, (float)sentryY_MP);
	}
	if (Diff_ELF_Flag = 1)
	{
		float vcamX = PS2_MEM_ReadFloat(Veh_Cam_Base + PS2_007_NF_VEHICLE_OFFSET_X);
		float vcamY = PS2_MEM_ReadFloat(Veh_Cam_Base + PS2_007_NF_VEHICLE_OFFSET_Y);

		vcamX += (float)xmouse * looksensitivity / scale / 10.f * (vfov / 33.f);
		vcamY += (float)(invertpitch ? -ymouse : ymouse) * looksensitivity / scale / 10.f * (vfov / 33.f);

		PS2_MEM_WriteFloat(Veh_Cam_Base + PS2_007_NF_VEHICLE_OFFSET_X, (float)vcamX);
		PS2_MEM_WriteFloat(Veh_Cam_Base + PS2_007_NF_VEHICLE_OFFSET_Y, (float)vcamY);
	}
}