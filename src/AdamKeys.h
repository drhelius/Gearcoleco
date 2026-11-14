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


#ifndef ADAM_KEYS_H
#define ADAM_KEYS_H

#include "definitions.h"

struct GC_AdamKeyInfo
{
    GC_AdamKey key;
    const char* name;
    const char* label;
};

struct GC_DefaultKey
{
    u16 usage;
    GC_AdamKey key;
};

static const GC_AdamKeyInfo kAdamKeyInfos[] =
{
    { GC_ADAM_KEY_A, "a", "A" }, { GC_ADAM_KEY_B, "b", "B" }, { GC_ADAM_KEY_C, "c", "C" },
    { GC_ADAM_KEY_D, "d", "D" }, { GC_ADAM_KEY_E, "e", "E" }, { GC_ADAM_KEY_F, "f", "F" },
    { GC_ADAM_KEY_G, "g", "G" }, { GC_ADAM_KEY_H, "h", "H" }, { GC_ADAM_KEY_I, "i", "I" },
    { GC_ADAM_KEY_J, "j", "J" }, { GC_ADAM_KEY_K, "k", "K" }, { GC_ADAM_KEY_L, "l", "L" },
    { GC_ADAM_KEY_M, "m", "M" }, { GC_ADAM_KEY_N, "n", "N" }, { GC_ADAM_KEY_O, "o", "O" },
    { GC_ADAM_KEY_P, "p", "P" }, { GC_ADAM_KEY_Q, "q", "Q" }, { GC_ADAM_KEY_R, "r", "R" },
    { GC_ADAM_KEY_S, "s", "S" }, { GC_ADAM_KEY_T, "t", "T" }, { GC_ADAM_KEY_U, "u", "U" },
    { GC_ADAM_KEY_V, "v", "V" }, { GC_ADAM_KEY_W, "w", "W" }, { GC_ADAM_KEY_X, "x", "X" },
    { GC_ADAM_KEY_Y, "y", "Y" }, { GC_ADAM_KEY_Z, "z", "Z" },
    { GC_ADAM_KEY_0, "0", "0" }, { GC_ADAM_KEY_1, "1", "1" }, { GC_ADAM_KEY_2, "2", "2" },
    { GC_ADAM_KEY_3, "3", "3" }, { GC_ADAM_KEY_4, "4", "4" }, { GC_ADAM_KEY_5, "5", "5" },
    { GC_ADAM_KEY_6, "6", "6" }, { GC_ADAM_KEY_7, "7", "7" }, { GC_ADAM_KEY_8, "8", "8" },
    { GC_ADAM_KEY_9, "9", "9" },
    { GC_ADAM_KEY_SPACE, "space", "Space" },
    { GC_ADAM_KEY_MINUS, "minus", "-" },
    { GC_ADAM_KEY_PLUS, "plus", "+" },
    { GC_ADAM_KEY_CARET, "caret", "^" },
    { GC_ADAM_KEY_SEMICOLON, "semicolon", ";" },
    { GC_ADAM_KEY_QUOTE, "quote", "'" },
    { GC_ADAM_KEY_OPEN_BRACKET, "open_bracket", "[" },
    { GC_ADAM_KEY_CLOSE_BRACKET, "close_bracket", "]" },
    { GC_ADAM_KEY_BACKSLASH, "backslash", "\\" },
    { GC_ADAM_KEY_COMMA, "comma", "," },
    { GC_ADAM_KEY_PERIOD, "period", "." },
    { GC_ADAM_KEY_SLASH, "slash", "/" },
    { GC_ADAM_KEY_RETURN, "return", "Return" },
    { GC_ADAM_KEY_ESCAPE, "escape", "Escape/WP" },
    { GC_ADAM_KEY_BACKSPACE, "backspace", "Backspace" },
    { GC_ADAM_KEY_TAB, "tab", "Tab" },
    { GC_ADAM_KEY_HOME, "home", "Home" },
    { GC_ADAM_KEY_SMART_1, "smart_1", "SmartKey I" },
    { GC_ADAM_KEY_SMART_2, "smart_2", "SmartKey II" },
    { GC_ADAM_KEY_SMART_3, "smart_3", "SmartKey III" },
    { GC_ADAM_KEY_SMART_4, "smart_4", "SmartKey IV" },
    { GC_ADAM_KEY_SMART_5, "smart_5", "SmartKey V" },
    { GC_ADAM_KEY_SMART_6, "smart_6", "SmartKey VI" },
    { GC_ADAM_KEY_WILD_CARD, "wild_card", "Wild Card" },
    { GC_ADAM_KEY_UNDO, "undo", "Undo" },
    { GC_ADAM_KEY_MOVE, "move", "Move/Copy" },
    { GC_ADAM_KEY_STORE, "store", "Store/Get" },
    { GC_ADAM_KEY_INSERT, "insert", "Insert" },
    { GC_ADAM_KEY_PRINT, "print", "Print" },
    { GC_ADAM_KEY_CLEAR, "clear", "Clear" },
    { GC_ADAM_KEY_DELETE, "delete", "Delete" },
    { GC_ADAM_KEY_UP, "up", "Up" },
    { GC_ADAM_KEY_RIGHT, "right", "Right" },
    { GC_ADAM_KEY_DOWN, "down", "Down" },
    { GC_ADAM_KEY_LEFT, "left", "Left" },
    { GC_ADAM_KEY_SHIFT, "shift", "Shift" },
    { GC_ADAM_KEY_CONTROL, "control", "Control" },
    { GC_ADAM_KEY_LOCK, "lock", "Lock" }
};

static const int kAdamKeyInfoCount = (int)(sizeof(kAdamKeyInfos) / sizeof(kAdamKeyInfos[0]));

static const GC_DefaultKey kDefaultKeys[] =
{
    { 0x04, GC_ADAM_KEY_A }, { 0x05, GC_ADAM_KEY_B }, { 0x06, GC_ADAM_KEY_C }, { 0x07, GC_ADAM_KEY_D },
    { 0x08, GC_ADAM_KEY_E }, { 0x09, GC_ADAM_KEY_F }, { 0x0A, GC_ADAM_KEY_G }, { 0x0B, GC_ADAM_KEY_H },
    { 0x0C, GC_ADAM_KEY_I }, { 0x0D, GC_ADAM_KEY_J }, { 0x0E, GC_ADAM_KEY_K }, { 0x0F, GC_ADAM_KEY_L },
    { 0x10, GC_ADAM_KEY_M }, { 0x11, GC_ADAM_KEY_N }, { 0x12, GC_ADAM_KEY_O }, { 0x13, GC_ADAM_KEY_P },
    { 0x14, GC_ADAM_KEY_Q }, { 0x15, GC_ADAM_KEY_R }, { 0x16, GC_ADAM_KEY_S }, { 0x17, GC_ADAM_KEY_T },
    { 0x18, GC_ADAM_KEY_U }, { 0x19, GC_ADAM_KEY_V }, { 0x1A, GC_ADAM_KEY_W }, { 0x1B, GC_ADAM_KEY_X },
    { 0x1C, GC_ADAM_KEY_Y }, { 0x1D, GC_ADAM_KEY_Z },
    { 0x1E, GC_ADAM_KEY_1 }, { 0x1F, GC_ADAM_KEY_2 }, { 0x20, GC_ADAM_KEY_3 }, { 0x21, GC_ADAM_KEY_4 },
    { 0x22, GC_ADAM_KEY_5 }, { 0x23, GC_ADAM_KEY_6 }, { 0x24, GC_ADAM_KEY_7 }, { 0x25, GC_ADAM_KEY_8 },
    { 0x26, GC_ADAM_KEY_9 }, { 0x27, GC_ADAM_KEY_0 },
    { 0x28, GC_ADAM_KEY_RETURN },
    { 0x2A, GC_ADAM_KEY_BACKSPACE },
    { 0x2B, GC_ADAM_KEY_TAB },
    { 0x2C, GC_ADAM_KEY_SPACE },
    { 0x2D, GC_ADAM_KEY_MINUS },
    { 0x2E, GC_ADAM_KEY_PLUS },
    { 0x2F, GC_ADAM_KEY_OPEN_BRACKET },
    { 0x30, GC_ADAM_KEY_CLOSE_BRACKET },
    { 0x31, GC_ADAM_KEY_BACKSLASH },
    { 0x33, GC_ADAM_KEY_SEMICOLON },
    { 0x34, GC_ADAM_KEY_QUOTE },
    { 0x35, GC_ADAM_KEY_CARET },
    { 0x36, GC_ADAM_KEY_COMMA },
    { 0x37, GC_ADAM_KEY_PERIOD },
    { 0x38, GC_ADAM_KEY_SLASH },
    { 0x39, GC_ADAM_KEY_LOCK },
    { 0x4F, GC_ADAM_KEY_RIGHT },
    { 0x50, GC_ADAM_KEY_LEFT },
    { 0x51, GC_ADAM_KEY_DOWN },
    { 0x52, GC_ADAM_KEY_UP },
    { 0x58, GC_ADAM_KEY_RETURN },
    { 0xE0, GC_ADAM_KEY_CONTROL },
    { 0xE1, GC_ADAM_KEY_SHIFT },
    { 0xE4, GC_ADAM_KEY_CONTROL },
    { 0xE5, GC_ADAM_KEY_SHIFT }
};

static const int kDefaultKeyCount = (int)(sizeof(kDefaultKeys) / sizeof(kDefaultKeys[0]));

static const GC_DefaultKey kDefaultSpecialKeys[] =
{
    { 0x3A, GC_ADAM_KEY_SMART_1 }, { 0x3B, GC_ADAM_KEY_SMART_2 }, { 0x3C, GC_ADAM_KEY_SMART_3 },
    { 0x3D, GC_ADAM_KEY_SMART_4 }, { 0x3E, GC_ADAM_KEY_SMART_5 }, { 0x3F, GC_ADAM_KEY_SMART_6 },
    { 0x40, GC_ADAM_KEY_UNDO }, { 0x41, GC_ADAM_KEY_WILD_CARD },
    { 0x29, GC_ADAM_KEY_ESCAPE },
    { 0x46, GC_ADAM_KEY_PRINT },
    { 0x49, GC_ADAM_KEY_INSERT },
    { 0x4A, GC_ADAM_KEY_HOME },
    { 0x4B, GC_ADAM_KEY_MOVE },
    { 0x4C, GC_ADAM_KEY_DELETE },
    { 0x4D, GC_ADAM_KEY_CLEAR },
    { 0x4E, GC_ADAM_KEY_STORE }
};

static const int kDefaultSpecialKeyCount = (int)(sizeof(kDefaultSpecialKeys) / sizeof(kDefaultSpecialKeys[0]));

static const GC_AdamKey kModifierKeys[] =
{
    GC_ADAM_KEY_CONTROL, GC_ADAM_KEY_SHIFT
};

static const int kModifierKeyCount = (int)(sizeof(kModifierKeys) / sizeof(kModifierKeys[0]));

#endif /* ADAM_KEYS_H */
