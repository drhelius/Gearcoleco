/*
 * Gearcoleco - ColecoVision Emulator
 * Copyright (C) 2021  Ignacio Sanchez

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/
 *
 */

#ifndef GUI_DEBUG_CONSTANTS_H
#define GUI_DEBUG_CONSTANTS_H

#include "imgui.h"
#include "gearcoleco.h"

static const ImVec4 cyan =          ImVec4(0.10f, 0.90f, 0.90f, 1.0f);
static const ImVec4 dark_cyan =     ImVec4(0.00f, 0.30f, 0.30f, 1.0f);
static const ImVec4 magenta =       ImVec4(1.00f, 0.50f, 0.96f, 1.0f);
static const ImVec4 dark_magenta =  ImVec4(0.30f, 0.18f, 0.27f, 1.0f);
static const ImVec4 yellow =        ImVec4(1.00f, 0.90f, 0.05f, 1.0f);
static const ImVec4 dark_yellow =   ImVec4(0.30f, 0.25f, 0.00f, 1.0f);
static const ImVec4 orange =        ImVec4(1.00f, 0.50f, 0.00f, 1.0f);
static const ImVec4 dark_orange =   ImVec4(0.60f, 0.20f, 0.00f, 1.0f);
static const ImVec4 red =           ImVec4(0.98f, 0.15f, 0.45f, 1.0f);
static const ImVec4 dark_red =      ImVec4(0.30f, 0.04f, 0.16f, 1.0f);
static const ImVec4 green =         ImVec4(0.10f, 0.90f, 0.10f, 1.0f);
static const ImVec4 dim_green =     ImVec4(0.05f, 0.40f, 0.05f, 1.0f);
static const ImVec4 dark_green =    ImVec4(0.03f, 0.20f, 0.02f, 1.0f);
static const ImVec4 violet =        ImVec4(0.68f, 0.51f, 1.00f, 1.0f);
static const ImVec4 dark_violet =   ImVec4(0.24f, 0.15f, 0.30f, 1.0f);
static const ImVec4 blue =          ImVec4(0.20f, 0.40f, 1.00f, 1.0f);
static const ImVec4 dark_blue =     ImVec4(0.07f, 0.10f, 0.30f, 1.0f);
static const ImVec4 white =         ImVec4(1.00f, 1.00f, 1.00f, 1.0f);
static const ImVec4 gray =          ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
static const ImVec4 mid_gray =      ImVec4(0.40f, 0.40f, 0.40f, 1.0f);
static const ImVec4 dark_gray =     ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
static const ImVec4 black =         ImVec4(0.00f, 0.00f, 0.00f, 1.0f);
static const ImVec4 brown =         ImVec4(0.68f, 0.50f, 0.36f, 1.0f);
static const ImVec4 dark_brown =    ImVec4(0.38f, 0.20f, 0.06f, 1.0f);

static const char* const c_cyan = "{19E6E6}";
static const char* const c_dark_cyan = "{004C4C}";
static const char* const c_magenta = "{FF80F5}";
static const char* const c_dark_magenta = "{4C2E45}";
static const char* const c_yellow = "{FFE60D}";
static const char* const c_dark_yellow = "{4C4000}";
static const char* const c_orange = "{FF8000}";
static const char* const c_dark_orange = "{993300}";
static const char* const c_red = "{FA2673}";
static const char* const c_dark_red = "{4C0A29}";
static const char* const c_green = "{19E619}";
static const char* const c_dim_green = "{0D660D}";
static const char* const c_dark_green = "{083305}";
static const char* const c_violet = "{AD82FF}";
static const char* const c_dark_violet = "{3D274D}";
static const char* const c_blue = "{3366FF}";
static const char* const c_dark_blue = "{12194D}";
static const char* const c_white = "{FFFFFF}";
static const char* const c_gray = "{808080}";
static const char* const c_mid_gray = "{666666}";
static const char* const c_dark_gray = "{1A1A1A}";
static const char* const c_black = "{000000}";
static const char* const c_brown = "{AD805C}";
static const char* const c_dark_brown = "{61330F}";

struct stDebugLabel
{
    u16 address;
    const char* label;
};

enum eDebugIODirection
{
    IO_IN   = 1,
    IO_OUT  = 2,
    IO_BOTH = 3,
};

struct stDebugIOLabel
{
    u16 address;
    const char* label;
    int direction;
};

static const int k_debug_io_label_count = 18;
static const stDebugIOLabel k_debug_io_labels[k_debug_io_label_count] = 
{
    // VDP Ports (TMS9918)
    { 0xBE, "VDP_DATA", IO_BOTH },
    { 0xBF, "VDP_STATUS", IO_IN },
    { 0xBF, "VDP_CTRL", IO_OUT },
    // PSG (SN76489)
    { 0xFF, "PSG", IO_OUT },
    // Controller Ports
    { 0xFC, "CTRL_MODE_1", IO_OUT },
    { 0x80, "CTRL_MODE_2", IO_OUT },
    { 0xC0, "CTRL_MODE_1", IO_OUT },
    { 0xE0, "CTRL_MODE_2", IO_OUT },
    // Controller Read
    { 0xFC, "CTRL_1", IO_IN },
    { 0xFF, "CTRL_2", IO_IN },
    // SGM AY-3-8910 Sound
    { 0x50, "AY_ADDR", IO_OUT },
    { 0x51, "AY_DATA", IO_OUT },
    { 0x52, "AY_READ", IO_IN },
    // SGM Memory Control
    { 0x53, "SGM_UPPER", IO_OUT },
    { 0x7F, "SGM_LOWER", IO_OUT },
    // NMI
    { 0xBF, "VDP_NMI", IO_IN },
    // Spinner
    { 0xFC, "SPINNER_1", IO_IN },
    { 0xFF, "SPINNER_2", IO_IN },
};

static const int k_debug_symbol_count = 9;

static const stDebugLabel k_debug_symbols[k_debug_symbol_count] = 
{
    { 0x0000, "RST_00" },
    { 0x0008, "RST_08" },
    { 0x0010, "RST_10" },
    { 0x0018, "RST_18" },
    { 0x0020, "RST_20" },
    { 0x0028, "RST_28" },
    { 0x0030, "RST_30" },
    { 0x0038, "RST_38" },
    { 0x0066, "NMI_Interrupt" },
};

#endif /* GUI_DEBUG_CONSTANTS_H */