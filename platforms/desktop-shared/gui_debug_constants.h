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
#define	GUI_DEBUG_CONSTANTS_H

static const int gui_debug_symbols_count = 63;

static const char* gui_debug_symbols[gui_debug_symbols_count] = {
    "00:0000 RST_00",
    "00:0008 RST_08",
    "00:0010 RST_10",
    "00:0018 RST_18",
    "00:0020 RST_20",
    "00:0028 RST_28",
    "00:0030 RST_30",
    "00:0038 RST_38",
    "00:0066 NMI_Interrupt",
    "00:1F61 PLAY_SONGS",
    "00:1F64 ACTIVATEP",
    "00:1F67 PUTOBJp",
    "00:1F6A REFLECT_VERTICAL",
    "00:1F6D REFLECT_HORIZONTAL",
    "00:1F70 ROTATE_90",
    "00:1F73 ENLARGE ",
    "00:1F76 CONTROLLER_SCAN",
    "00:1F79 DECODER",
    "00:1F7C GAME_OPT",
    "00:1F7F LOAD_ASCII",
    "00:1F82 FILL_VRAM",
    "00:1F85 MODE_1",
    "00:1F88 UPDATE_SPINNER",
    "00:1F8B INIT_TABLEP",
    "00:1F8E GET_VRAMP",
    "00:1F91 PUT_VRAMP",
    "00:1F94 INIT_SPR_ORDERP",
    "00:1F97 WR_SPR_NM_TBLP",
    "00:1F9A INIT_TIMERP",
    "00:1F9D FREE_SIGNALP",
    "00:1FA0 REQUEST_SIGNALP",
    "00:1FA3 TEST_SIGNALP",
    "00:1FA6 WRITE_REGISTERP",
    "00:1FA9 WRITE_VRAMP",
    "00:1FAC READ_VRAMP",
    "00:1FAF INIT_WRITERP",
    "00:1FB2 SOUND_INITP",
    "00:1FB5 PLAY_ITP",
    "00:1FB8 INIT_TABLE",
    "00:1FBB GET_VRAM",
    "00:1FBE PUT_VRAM",
    "00:1FC1 INIT_SPR_ORDER",
    "00:1FC4 WR_SPR_NM_TBL",
    "00:1FC7 INIT_TIMER",
    "00:1FCA FREE_SIGNAL",
    "00:1FCD REQUEST_SIGNAL",
    "00:1FD0 TEST_SIGNAL",
    "00:1FD3 TIME_MGR",
    "00:1FD6 TURN_OFF_SOUND",
    "00:1FD9 WRITE_REGISTER",
    "00:1FDC READ_REGISTER",
    "00:1FDF WRITE_VRAM",
    "00:1FE2 READ_VRAM",
    "00:1FE5 INIT_WRITER",
    "00:1FE8 WRITER",
    "00:1FEB POLLER",
    "00:1FEE SOUND_INIT",
    "00:1FF1 PLAY_IT",
    "00:1FF4 SOUND_MAN",
    "00:1FF7 ACTIVATE",
    "00:1FFA PUTOBJ",
    "00:1FFD RAND_GEN",
    "00:8021 NMI_INT_VECTOR"
};

#endif	/* GUI_DEBUG_CONSTANTS_H */