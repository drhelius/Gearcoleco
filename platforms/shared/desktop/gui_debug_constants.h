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
#include "config.h"

struct GuiDebugColor
{
    ImVec4 dark;
    ImVec4 light;

    operator ImVec4() const
    {
        return (config_emulator.theme == config_Theme_Light) ? light : dark;
    }
};

struct GuiDebugTextColor
{
    const char* dark;
    const char* light;

    const char* c_str() const
    {
        return (config_emulator.theme == config_Theme_Light) ? light : dark;
    }

    operator const char*() const
    {
        return c_str();
    }
};

static inline ImVec4 gui_debug_color(unsigned int rgb)
{
    return ImVec4(((rgb >> 16) & 0xFF) / 255.0f, ((rgb >> 8) & 0xFF) / 255.0f, (rgb & 0xFF) / 255.0f, 1.0f);
}

static const GuiDebugColor cyan = { gui_debug_color(0x1AE6E6), gui_debug_color(0x007C91) };
static const GuiDebugColor dark_cyan = { gui_debug_color(0x004D4D), gui_debug_color(0xCBEFF3) };
static const GuiDebugColor magenta = { gui_debug_color(0xFF80F5), gui_debug_color(0xB42375) };
static const GuiDebugColor dark_magenta = { gui_debug_color(0x4D2E45), gui_debug_color(0xF3D6E8) };
static const GuiDebugColor yellow = { gui_debug_color(0xFFE60D), gui_debug_color(0x8A6000) };
static const GuiDebugColor dark_yellow = { gui_debug_color(0x4D4000), gui_debug_color(0xF7E7B2) };
static const GuiDebugColor orange = { gui_debug_color(0xFF8000), gui_debug_color(0xC44D00) };
static const GuiDebugColor dark_orange = { gui_debug_color(0x993300), gui_debug_color(0xF8D4B6) };
static const GuiDebugColor red = { gui_debug_color(0xFA2673), gui_debug_color(0xC7254E) };
static const GuiDebugColor dark_red = { gui_debug_color(0x4D0A29), gui_debug_color(0xF6CDD8) };
static const GuiDebugColor green = { gui_debug_color(0x1AE61A), gui_debug_color(0x17823B) };
static const GuiDebugColor dim_green = { gui_debug_color(0x0D660D), gui_debug_color(0x4D7438) };
static const GuiDebugColor dark_green = { gui_debug_color(0x083305), gui_debug_color(0xD5E8D6) };
static const GuiDebugColor violet = { gui_debug_color(0xAD82FF), gui_debug_color(0x7047C2) };
static const GuiDebugColor dark_violet = { gui_debug_color(0x3D264D), gui_debug_color(0xE4D9F7) };
static const GuiDebugColor blue = { gui_debug_color(0x3366FF), gui_debug_color(0x0969DA) };
static const GuiDebugColor dark_blue = { gui_debug_color(0x121A4D), gui_debug_color(0xD7E5FA) };
static const GuiDebugColor white = { gui_debug_color(0xFFFFFF), gui_debug_color(0x21201C) };
static const GuiDebugColor gray = { gui_debug_color(0x808080), gui_debug_color(0x69645D) };
static const GuiDebugColor mid_gray = { gui_debug_color(0x666666), gui_debug_color(0x756F67) };
static const GuiDebugColor dark_gray = { gui_debug_color(0x1A1A1A), gui_debug_color(0x4B4842) };
static const GuiDebugColor black = { gui_debug_color(0x000000), gui_debug_color(0x21201C) };
static const GuiDebugColor brown = { gui_debug_color(0xAD805C), gui_debug_color(0x87502C) };
static const GuiDebugColor dark_brown = { gui_debug_color(0x61330F), gui_debug_color(0xE8D6C8) };

static const GuiDebugTextColor c_cyan = { "{1AE6E6}", "{007C91}" };
static const GuiDebugTextColor c_dark_cyan = { "{004D4D}", "{CBEFF3}" };
static const GuiDebugTextColor c_magenta = { "{FF80F5}", "{B42375}" };
static const GuiDebugTextColor c_dark_magenta = { "{4D2E45}", "{F3D6E8}" };
static const GuiDebugTextColor c_yellow = { "{FFE60D}", "{8A6000}" };
static const GuiDebugTextColor c_dark_yellow = { "{4D4000}", "{F7E7B2}" };
static const GuiDebugTextColor c_orange = { "{FF8000}", "{C44D00}" };
static const GuiDebugTextColor c_dark_orange = { "{993300}", "{F8D4B6}" };
static const GuiDebugTextColor c_red = { "{FA2673}", "{C7254E}" };
static const GuiDebugTextColor c_dark_red = { "{4D0A29}", "{F6CDD8}" };
static const GuiDebugTextColor c_green = { "{1AE61A}", "{17823B}" };
static const GuiDebugTextColor c_dim_green = { "{0D660D}", "{4D7438}" };
static const GuiDebugTextColor c_dark_green = { "{083305}", "{D5E8D6}" };
static const GuiDebugTextColor c_violet = { "{AD82FF}", "{7047C2}" };
static const GuiDebugTextColor c_dark_violet = { "{3D264D}", "{E4D9F7}" };
static const GuiDebugTextColor c_blue = { "{3366FF}", "{0969DA}" };
static const GuiDebugTextColor c_dark_blue = { "{121A4D}", "{D7E5FA}" };
static const GuiDebugTextColor c_white = { "{FFFFFF}", "{21201C}" };
static const GuiDebugTextColor c_gray = { "{808080}", "{69645D}" };
static const GuiDebugTextColor c_mid_gray = { "{666666}", "{756F67}" };
static const GuiDebugTextColor c_dark_gray = { "{1A1A1A}", "{4B4842}" };
static const GuiDebugTextColor c_black = { "{000000}", "{21201C}" };
static const GuiDebugTextColor c_brown = { "{AD805C}", "{87502C}" };
static const GuiDebugTextColor c_dark_brown = { "{61330F}", "{E8D6C8}" };

static inline ImVec4 gui_debug_lerp_color(const ImVec4& a, const ImVec4& b, float t)
{
    return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
}

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

struct stDebugPortLabel
{
    u8 mask;
    u8 port;
    const char* label;
    int direction;
};

static const stDebugPortLabel k_debug_port_labels[] = 
{
    // VDP ports (TMS9918A)
    { 0xE1, 0xA0, "VDP_DATA", IO_BOTH },
    { 0xE1, 0xA1, "VDP_STATUS", IO_IN },
    { 0xE1, 0xA1, "VDP_CTRL", IO_OUT },
    // PSG (SN76489A)
    { 0xE0, 0xE0, "PSG_DATA", IO_OUT },
    // Controller strobe and read ports
    { 0xE0, 0x80, "CTRL_STROBE_SET", IO_OUT },
    { 0xE0, 0xC0, "CTRL_STROBE_RST", IO_OUT },
    { 0xE2, 0xE0, "CTRL_1", IO_IN },
    { 0xE2, 0xE2, "CTRL_2", IO_IN },
    // SGM AY-3-8910 sound
    { 0xFF, 0x50, "AY_REG_SELECT", IO_OUT },
    { 0xFF, 0x51, "AY_DATA", IO_OUT },
    { 0xFF, 0x52, "AY_DATA", IO_IN },
    // SGM memory control
    { 0xFF, 0x53, "SGM_UPPER_RAM", IO_OUT },
    { 0xFF, 0x7F, "SGM_RAM_CTRL", IO_OUT },
};

static const int k_debug_port_label_count = (int)(sizeof(k_debug_port_labels) / sizeof(k_debug_port_labels[0]));

static const stDebugLabel k_debug_symbols[] = 
{
    // OS 7 vectors and global BIOS symbols
    { 0x0000, "BOOT_UP" },
    { 0x0008, "RST_8H" },
    { 0x0010, "RST_10H" },
    { 0x0018, "RST_18H" },
    { 0x0020, "RST_20H" },
    { 0x0028, "RST_28H" },
    { 0x0030, "RST_30H" },
    { 0x0038, "IRQ_INTERRUPT" },
    { 0x003B, "RAND_GEN_" },
    { 0x0066, "NMI_INTERRUPT" },
    { 0x0069, "AMERICA" },
    { 0x006A, "ASCII_TABLE" },
    { 0x006C, "NUMBER_TABLE" },
    { 0x00FC, "FREQ_SWEEP" },
    { 0x012F, "ATN_SWEEP" },
    { 0x0190, "DECLSN" },
    { 0x019B, "DECMSN" },
    { 0x01A6, "MSNTOLSN" },
    { 0x01B1, "ADD816" },
    { 0x01D5, "LEAVE_EFFECT" },
    { 0x0203, "INIT_SOUNDQ" },
    { 0x0213, "INIT_SOUND" },
    { 0x023B, "ALL_OFF" },
    { 0x0251, "JUKE_BOXQ" },
    { 0x025E, "JUKE_BOX" },
    { 0x027F, "SND_MANAGER" },
    { 0x02EE, "EFXOVER" },
    { 0x0300, "PLAY_SONGS_" },
    { 0x0488, "ACTIVATEQ" },
    { 0x04A3, "ACTIVATE_" },
    { 0x0655, "INIT_QUEUEQ" },
    { 0x0664, "INIT_QUEUE" },
    { 0x0679, "WRITER_" },
    { 0x06C7, "PUTOBJQ" },
    { 0x06D8, "PUTOBJ_" },
    { 0x07E8, "PX_TO_PTRN_POS" },
    { 0x080B, "PUT_FRAME" },
    { 0x0898, "GET_BKGRND" },
    { 0x08C0, "CALC_OFFSET" },
    { 0x0F37, "TIME_MGR_" },
    { 0x0F9A, "INIT_TIMERQ" },
    { 0x0FAA, "INIT_TIMER_" },
    { 0x0FB8, "FREE_SIGNALQ" },
    { 0x0FC4, "FREE_SIGNAL_" },
    { 0x1044, "REQUEST_SIGNALQ" },
    { 0x1053, "REQUEST_SIGNAL_" },
    { 0x10BF, "TEST_SIGNALQ" },
    { 0x10CB, "TEST_SIGNAL_" },
    { 0x114A, "CONT_SCAN" },
    { 0x116A, "UPDATE_SPINNER_" },
    { 0x118B, "DECODER_" },
    { 0x11C1, "POLLER_" },
    { 0x18D4, "FILL_VRAM_" },
    { 0x18E9, "MODE_1_" },
    { 0x1927, "LOAD_ASCII_" },
    { 0x1979, "GAME_OPT_" },
    { 0x1B08, "INIT_TABLE_" },
    { 0x1B0E, "INIT_TABLEQ" },
    { 0x1B8C, "GET_VRAMQ" },
    { 0x1BA3, "GET_VRAM_" },
    { 0x1C10, "PUT_VRAMQ" },
    { 0x1C27, "PUT_VRAM_" },
    { 0x1C5A, "INIT_SPR_ORDERQ" },
    { 0x1C66, "INIT_SPR_ORDER_" },
    { 0x1C76, "WR_SPR_NM_TBLQ" },
    { 0x1C82, "WR_SPR_NM_TBL_" },
    { 0x1CBC, "REG_WRITEQ" },
    { 0x1CCA, "REG_WRITE" },
    { 0x1CED, "WRITE_VRAMQ" },
    { 0x1D01, "VRAM_WRITE" },
    { 0x1D2A, "READ_VRAMQ" },
    { 0x1D3E, "VRAM_READ" },
    { 0x1D43, "CTRL_PORT_PTR" },
    { 0x1D47, "DATA_PORT_PTR" },
    { 0x1D57, "REG_READ" },
    { 0x1D5A, "RFLCT_VERT" },
    { 0x1D60, "RFLCT_HOR" },
    { 0x1D66, "ROT_90" },
    { 0x1D6C, "ENLRG" },

    // OS 7 jump table entry points
    { 0x1F61, "PLAY_SONGS" },
    { 0x1F64, "ACTIVATEP" },
    { 0x1F67, "PUTOBJP" },
    { 0x1F6A, "REFLECT_VERTICAL" },
    { 0x1F6D, "REFLECT_HORIZONTAL" },
    { 0x1F70, "ROTATE_90" },
    { 0x1F73, "ENLARGE" },
    { 0x1F76, "CONTROLLER_SCAN" },
    { 0x1F79, "DECODER" },
    { 0x1F7C, "GAME_OPT" },
    { 0x1F7F, "LOAD_ASCII" },
    { 0x1F82, "FILL_VRAM" },
    { 0x1F85, "MODE_1" },
    { 0x1F88, "UPDATE_SPINNER" },
    { 0x1F8B, "INIT_TABLEP" },
    { 0x1F8E, "GET_VRAMP" },
    { 0x1F91, "PUT_VRAMP" },
    { 0x1F94, "INIT_SPR_ORDERP" },
    { 0x1F97, "WR_SPR_NM_TBLP" },
    { 0x1F9A, "INIT_TIMERP" },
    { 0x1F9D, "FREE_SIGNALP" },
    { 0x1FA0, "REQUEST_SIGNALP" },
    { 0x1FA3, "TEST_SIGNALP" },
    { 0x1FA6, "WRITE_REGISTERP" },
    { 0x1FA9, "WRITE_VRAMP" },
    { 0x1FAC, "READ_VRAMP" },
    { 0x1FAF, "INIT_WRITERP" },
    { 0x1FB2, "SOUND_INITP" },
    { 0x1FB5, "PLAY_ITP" },
    { 0x1FB8, "INIT_TABLE" },
    { 0x1FBB, "GET_VRAM" },
    { 0x1FBE, "PUT_VRAM" },
    { 0x1FC1, "INIT_SPR_ORDER" },
    { 0x1FC4, "WR_SPR_NM_TBL" },
    { 0x1FC7, "INIT_TIMER" },
    { 0x1FCA, "FREE_SIGNAL" },
    { 0x1FCD, "REQUEST_SIGNAL" },
    { 0x1FD0, "TEST_SIGNAL" },
    { 0x1FD3, "TIME_MGR" },
    { 0x1FD6, "TURN_OFF_SOUND" },
    { 0x1FD9, "WRITE_REGISTER" },
    { 0x1FDC, "READ_REGISTER" },
    { 0x1FDF, "WRITE_VRAM" },
    { 0x1FE2, "READ_VRAM" },
    { 0x1FE5, "INIT_WRITER" },
    { 0x1FE8, "WRITER" },
    { 0x1FEB, "POLLER" },
    { 0x1FEE, "SOUND_INIT" },
    { 0x1FF1, "PLAY_IT" },
    { 0x1FF4, "SOUND_MAN" },
    { 0x1FF7, "ACTIVATE" },
    { 0x1FFA, "PUTOBJ" },
    { 0x1FFD, "RAND_GEN" },

    // OS 7 RAM and cartridge-header labels
    { 0x7020, "PTR_TO_LST_OF_SND_ADDRS" },
    { 0x7022, "PTR_TO_S_ON_0" },
    { 0x7024, "PTR_TO_S_ON_1" },
    { 0x7026, "PTR_TO_S_ON_2" },
    { 0x7028, "PTR_TO_S_ON_3" },
    { 0x702A, "SAVE_CTRL" },
    { 0x73B9, "STACK" },
    { 0x73BA, "PARAM_AREA" },
    { 0x73C0, "TIMER_LENGTH" },
    { 0x73C2, "TEST_SIG_NUM" },
    { 0x73C3, "VDP_MODE_WORD" },
    { 0x73C5, "VDP_STATUS_BYTE" },
    { 0x73C6, "DEFER_WRITES" },
    { 0x73C7, "MUX_SPRITES" },
    { 0x73C8, "RAND_NUM" },
    { 0x73CA, "QUEUE_SIZE" },
    { 0x73CB, "QUEUE_HEAD" },
    { 0x73CC, "QUEUE_TAIL" },
    { 0x73CD, "HEAD_ADDRESS" },
    { 0x73CF, "TAIL_ADDRESS" },
    { 0x73D1, "BUFFER" },
    { 0x73D3, "TIMER_TABLE_BASE" },
    { 0x73D5, "NEXT_TIMER_DATA_BYTE" },
    { 0x73D7, "DBNCE_BUFF" },
    { 0x73EB, "SPIN_SW0_CT" },
    { 0x73EC, "SPIN_SW1_CT" },
    { 0x73EE, "S0_C0" },
    { 0x73EF, "S0_C1" },
    { 0x73F0, "S1_C0" },
    { 0x73F1, "S1_C1" },
    { 0x73F2, "SPRITENAMETBL" },
    { 0x73F4, "SPRITEGENTBL" },
    { 0x73F6, "PATTERNNAMETBL" },
    { 0x73F8, "PATTERNGENTBL" },
    { 0x73FA, "COLORTABLE" },
    { 0x73FC, "SAVE_TEMP" },
    { 0x73FE, "SAVED_COUNT" },
    { 0x8000, "CARTRIDGE" },
    { 0x8002, "LOCAL_SPR_TABLE" },
    { 0x8004, "SPRITE_ORDER" },
    { 0x8006, "WORK_BUFFER" },
    { 0x8008, "CONTROLLER_MAP" },
    { 0x800A, "START_GAME" },
    { 0x800C, "RST_8H_RAM" },
    { 0x800F, "RST_10H_RAM" },
    { 0x8012, "RST_18H_RAM" },
    { 0x8015, "RST_20H_RAM" },
    { 0x8018, "RST_28H_RAM" },
    { 0x801B, "RST_30H_RAM" },
    { 0x801E, "IRQ_INT_VECT" },
    { 0x8021, "NMI_INT_VECT" },
    { 0x8024, "GAME_NAME" },
};

static const int k_debug_symbol_count = (int)(sizeof(k_debug_symbols) / sizeof(k_debug_symbols[0]));

#endif /* GUI_DEBUG_CONSTANTS_H */
