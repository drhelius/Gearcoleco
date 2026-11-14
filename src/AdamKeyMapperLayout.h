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

#ifndef ADAM_KEY_MAPPER_LAYOUT_H
#define ADAM_KEY_MAPPER_LAYOUT_H

#include "definitions.h"

struct GC_KeyMapperKey
{
    u16 x;
    u16 y;
    u16 width;
    u16 height;
    GC_AdamKey key;
    const char* label;
};

static const int kKeyMapperWidth = GC_VIDEO_MAX_WIDTH;
static const int kKeyMapperHeight = GC_RESOLUTION_HEIGHT_WITH_OVERSCAN;
static const int kKeyMapperGrid = 100;
static const int kKeyMapperUnitX = 28;
static const int kKeyMapperUnitY = 22;
static const int kKeyMapperOriginX = 5;
static const int kKeyMapperOriginY = 26;

static const GC_KeyMapperKey kKeyMapperLayout[] =
{
    { 0, 0, 100, 100, GC_ADAM_KEY_ESCAPE, "Esc\nWP" },
    { 100, 0, 100, 100, GC_ADAM_KEY_WILD_CARD, "Wld\nCrd" },
    { 280, 0, 140, 100, GC_ADAM_KEY_SMART_1, "I" },
    { 430, 0, 140, 100, GC_ADAM_KEY_SMART_2, "II" },
    { 580, 0, 140, 100, GC_ADAM_KEY_SMART_3, "III" },
    { 730, 0, 140, 100, GC_ADAM_KEY_SMART_4, "IV" },
    { 880, 0, 140, 100, GC_ADAM_KEY_SMART_5, "V" },
    { 1030, 0, 140, 100, GC_ADAM_KEY_SMART_6, "VI" },
    { 1260, 0, 100, 100, GC_ADAM_KEY_UNDO, "Und" },
    { 1360, 0, 100, 100, GC_ADAM_KEY_BACKSPACE, "Bk\nSp" },
    { 1490, 0, 100, 100, GC_ADAM_KEY_MOVE, "Mov\nCpy" },
    { 1590, 0, 100, 100, GC_ADAM_KEY_STORE, "Sto\nGet" },
    { 1690, 0, 100, 100, GC_ADAM_KEY_CLEAR, "Clr" },
    { 0, 100, 100, 100, GC_ADAM_KEY_BACKSLASH, "|\n\\" },
    { 100, 100, 100, 100, GC_ADAM_KEY_1, "!\n1" },
    { 200, 100, 100, 100, GC_ADAM_KEY_2, "@\n2" },
    { 300, 100, 100, 100, GC_ADAM_KEY_3, "#\n3" },
    { 400, 100, 100, 100, GC_ADAM_KEY_4, "$\n4" },
    { 500, 100, 100, 100, GC_ADAM_KEY_5, "%\n5" },
    { 600, 100, 100, 100, GC_ADAM_KEY_6, "_\n6" },
    { 700, 100, 100, 100, GC_ADAM_KEY_7, "&\n7" },
    { 800, 100, 100, 100, GC_ADAM_KEY_8, "*\n8" },
    { 900, 100, 100, 100, GC_ADAM_KEY_9, "(\n9" },
    { 1000, 100, 100, 100, GC_ADAM_KEY_0, ")\n0" },
    { 1100, 100, 100, 100, GC_ADAM_KEY_MINUS, "`\n-" },
    { 1200, 100, 100, 100, GC_ADAM_KEY_PLUS, "=\n+" },
    { 1300, 100, 100, 100, GC_ADAM_KEY_CARET, "~\n^" },
    { 1490, 100, 100, 100, GC_ADAM_KEY_INSERT, "Ins" },
    { 1590, 100, 100, 100, GC_ADAM_KEY_PRINT, "Prt" },
    { 1690, 100, 100, 100, GC_ADAM_KEY_DELETE, "Del" },
    { 0, 200, 145, 100, GC_ADAM_KEY_TAB, "Tab" },
    { 155, 200, 100, 100, GC_ADAM_KEY_Q, "Q" },
    { 255, 200, 100, 100, GC_ADAM_KEY_W, "W" },
    { 355, 200, 100, 100, GC_ADAM_KEY_E, "E" },
    { 455, 200, 100, 100, GC_ADAM_KEY_R, "R" },
    { 555, 200, 100, 100, GC_ADAM_KEY_T, "T" },
    { 655, 200, 100, 100, GC_ADAM_KEY_Y, "Y" },
    { 755, 200, 100, 100, GC_ADAM_KEY_U, "U" },
    { 855, 200, 100, 100, GC_ADAM_KEY_I, "I" },
    { 955, 200, 100, 100, GC_ADAM_KEY_O, "O" },
    { 1055, 200, 100, 100, GC_ADAM_KEY_P, "P" },
    { 1155, 200, 100, 100, GC_ADAM_KEY_OPEN_BRACKET, "{\n[" },
    { 1255, 200, 100, 100, GC_ADAM_KEY_CLOSE_BRACKET, "}\n]" },
    { 1360, 200, 90, 100, GC_ADAM_KEY_RETURN, "" },
    { 0, 300, 170, 100, GC_ADAM_KEY_CONTROL, "Ctrl" },
    { 185, 300, 100, 100, GC_ADAM_KEY_A, "A" },
    { 285, 300, 100, 100, GC_ADAM_KEY_S, "S" },
    { 385, 300, 100, 100, GC_ADAM_KEY_D, "D" },
    { 485, 300, 100, 100, GC_ADAM_KEY_F, "F" },
    { 585, 300, 100, 100, GC_ADAM_KEY_G, "G" },
    { 685, 300, 100, 100, GC_ADAM_KEY_H, "H" },
    { 785, 300, 100, 100, GC_ADAM_KEY_J, "J" },
    { 885, 300, 100, 100, GC_ADAM_KEY_K, "K" },
    { 985, 300, 100, 100, GC_ADAM_KEY_L, "L" },
    { 1085, 300, 100, 100, GC_ADAM_KEY_SEMICOLON, ":\n;" },
    { 1185, 300, 100, 100, GC_ADAM_KEY_QUOTE, "\"\n'" },
    { 1290, 300, 160, 100, GC_ADAM_KEY_RETURN, "Ret" },
    { 1590, 300, 100, 100, GC_ADAM_KEY_UP, "\x18" },
    { 0, 400, 220, 100, GC_ADAM_KEY_SHIFT, "Shift" },
    { 237, 400, 100, 100, GC_ADAM_KEY_Z, "Z" },
    { 337, 400, 100, 100, GC_ADAM_KEY_X, "X" },
    { 437, 400, 100, 100, GC_ADAM_KEY_C, "C" },
    { 537, 400, 100, 100, GC_ADAM_KEY_V, "V" },
    { 637, 400, 100, 100, GC_ADAM_KEY_B, "B" },
    { 737, 400, 100, 100, GC_ADAM_KEY_N, "N" },
    { 837, 400, 100, 100, GC_ADAM_KEY_M, "M" },
    { 937, 400, 100, 100, GC_ADAM_KEY_COMMA, "<\n," },
    { 1037, 400, 100, 100, GC_ADAM_KEY_PERIOD, ">\n." },
    { 1137, 400, 100, 100, GC_ADAM_KEY_SLASH, "?\n/" },
    { 1235, 400, 215, 100, GC_ADAM_KEY_SHIFT, "Shift" },
    { 1490, 400, 100, 100, GC_ADAM_KEY_LEFT, "\x1B" },
    { 1590, 400, 100, 100, GC_ADAM_KEY_HOME, "Hom" },
    { 1690, 400, 100, 100, GC_ADAM_KEY_RIGHT, "\x1A" },
    { 30, 500, 100, 100, GC_ADAM_KEY_LOCK, "Lck" },
    { 333, 500, 790, 100, GC_ADAM_KEY_SPACE, "Space" },
    { 1590, 500, 100, 100, GC_ADAM_KEY_DOWN, "\x19" }
};

static const int kKeyMapperKeyCount = (int)(sizeof(kKeyMapperLayout) / sizeof(kKeyMapperLayout[0]));

#endif /* ADAM_KEY_MAPPER_LAYOUT_H */
