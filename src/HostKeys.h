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

#ifndef HOST_KEYS_H
#define HOST_KEYS_H

#include "definitions.h"

struct GC_HostKeyInfo
{
    u16 usage;
    const char* name;
    const char* label;
};

#if defined(__APPLE__)
#define GC_HOST_LABEL_ALT(side) side " Option"
#define GC_HOST_LABEL_GUI(side) side " Cmd"
#define GC_HOST_HOTKEY_LABEL(key) "Cmd+" key
#elif defined(_WIN32)
#define GC_HOST_LABEL_ALT(side) side " Alt"
#define GC_HOST_LABEL_GUI(side) side " Win"
#define GC_HOST_HOTKEY_LABEL(key) "Ctrl+" key
#else
#define GC_HOST_LABEL_ALT(side) side " Alt"
#define GC_HOST_LABEL_GUI(side) side " Super"
#define GC_HOST_HOTKEY_LABEL(key) "Ctrl+" key
#endif

static const GC_HostKeyInfo kHostKeys[] =
{
    { 0x04, "a", "A" }, { 0x05, "b", "B" }, { 0x06, "c", "C" }, { 0x07, "d", "D" }, { 0x08, "e", "E" },
    { 0x09, "f", "F" }, { 0x0A, "g", "G" }, { 0x0B, "h", "H" }, { 0x0C, "i", "I" }, { 0x0D, "j", "J" },
    { 0x0E, "k", "K" }, { 0x0F, "l", "L" }, { 0x10, "m", "M" }, { 0x11, "n", "N" }, { 0x12, "o", "O" },
    { 0x13, "p", "P" }, { 0x14, "q", "Q" }, { 0x15, "r", "R" }, { 0x16, "s", "S" }, { 0x17, "t", "T" },
    { 0x18, "u", "U" }, { 0x19, "v", "V" }, { 0x1A, "w", "W" }, { 0x1B, "x", "X" }, { 0x1C, "y", "Y" },
    { 0x1D, "z", "Z" },
    { 0x1E, "1", "1" }, { 0x1F, "2", "2" }, { 0x20, "3", "3" }, { 0x21, "4", "4" }, { 0x22, "5", "5" },
    { 0x23, "6", "6" }, { 0x24, "7", "7" }, { 0x25, "8", "8" }, { 0x26, "9", "9" }, { 0x27, "0", "0" },
    { 0x28, "return", "Enter" },
    { 0x29, "escape", "Esc" },
    { 0x2A, "backspace", "Backspace" },
    { 0x2B, "tab", "Tab" },
    { 0x2C, "space", "Space" },
    { 0x2D, "minus", "-" },
    { 0x2E, "equals", "=" },
    { 0x2F, "left_bracket", "[" },
    { 0x30, "right_bracket", "]" },
    { 0x31, "backslash", "\\" },
    { 0x32, "non_us_hash", "# (ISO)" },
    { 0x33, "semicolon", ";" },
    { 0x34, "apostrophe", "'" },
    { 0x35, "grave", "`" },
    { 0x36, "comma", "," },
    { 0x37, "period", "." },
    { 0x38, "slash", "/" },
    { 0x39, "caps_lock", "Caps Lock" },
    { 0x3A, "f1", "F1" }, { 0x3B, "f2", "F2" }, { 0x3C, "f3", "F3" }, { 0x3D, "f4", "F4" },
    { 0x3E, "f5", "F5" }, { 0x3F, "f6", "F6" }, { 0x40, "f7", "F7" }, { 0x41, "f8", "F8" },
    { 0x42, "f9", "F9" }, { 0x43, "f10", "F10" }, { 0x44, "f11", "F11" }, { 0x45, "f12", "F12" },
    { 0x46, "print_screen", "Print Screen" },
    { 0x47, "scroll_lock", "Scroll Lock" },
    { 0x48, "pause", "Pause" },
    { 0x49, "insert", "Insert" },
    { 0x4A, "home", "Home" },
    { 0x4B, "page_up", "Page Up" },
    { 0x4C, "delete", "Delete" },
    { 0x4D, "end", "End" },
    { 0x4E, "page_down", "Page Down" },
    { 0x4F, "right", "Right" },
    { 0x50, "left", "Left" },
    { 0x51, "down", "Down" },
    { 0x52, "up", "Up" },
    { 0x53, "num_lock", "Num Lock" },
    { 0x54, "kp_divide", "Keypad /" },
    { 0x55, "kp_multiply", "Keypad *" },
    { 0x56, "kp_minus", "Keypad -" },
    { 0x57, "kp_plus", "Keypad +" },
    { 0x58, "kp_enter", "Keypad Enter" },
    { 0x59, "kp_1", "Keypad 1" }, { 0x5A, "kp_2", "Keypad 2" }, { 0x5B, "kp_3", "Keypad 3" },
    { 0x5C, "kp_4", "Keypad 4" }, { 0x5D, "kp_5", "Keypad 5" }, { 0x5E, "kp_6", "Keypad 6" },
    { 0x5F, "kp_7", "Keypad 7" }, { 0x60, "kp_8", "Keypad 8" }, { 0x61, "kp_9", "Keypad 9" },
    { 0x62, "kp_0", "Keypad 0" },
    { 0x63, "kp_period", "Keypad ." },
    { 0x64, "non_us_backslash", "\\ (ISO)" },
    { 0x65, "menu", "Menu" },
    { 0x67, "kp_equals", "Keypad =" },
    { 0x68, "f13", "F13" }, { 0x69, "f14", "F14" }, { 0x6A, "f15", "F15" }, { 0x6B, "f16", "F16" },
    { 0x6C, "f17", "F17" }, { 0x6D, "f18", "F18" }, { 0x6E, "f19", "F19" }, { 0x6F, "f20", "F20" },
    { 0x70, "f21", "F21" }, { 0x71, "f22", "F22" }, { 0x72, "f23", "F23" }, { 0x73, "f24", "F24" },
    { 0x75, "help", "Help" },
    { 0x85, "kp_comma", "Keypad ," },
    { 0x87, "international1", "Intl 1" }, { 0x88, "international2", "Intl 2" },
    { 0x89, "international3", "Intl 3" }, { 0x8A, "international4", "Intl 4" },
    { 0x8B, "international5", "Intl 5" },
    { 0x90, "lang1", "Lang 1" }, { 0x91, "lang2", "Lang 2" }, { 0x92, "lang3", "Lang 3" },
    { 0x93, "lang4", "Lang 4" }, { 0x94, "lang5", "Lang 5" },
    { 0x9A, "sysreq", "SysRq" },
    { 0xB0, "kp_00", "Keypad 00" },
    { 0xB1, "kp_000", "Keypad 000" },
    { 0xE0, "left_ctrl", "Left Ctrl" },
    { 0xE1, "left_shift", "Left Shift" },
    { 0xE2, "left_alt", GC_HOST_LABEL_ALT("Left") },
    { 0xE3, "left_gui", GC_HOST_LABEL_GUI("Left") },
    { 0xE4, "right_ctrl", "Right Ctrl" },
    { 0xE5, "right_shift", "Right Shift" },
    { 0xE6, "right_alt", GC_HOST_LABEL_ALT("Right") },
    { 0xE7, "right_gui", GC_HOST_LABEL_GUI("Right") }
};

static const int kHostKeyCount = (int)(sizeof(kHostKeys) / sizeof(kHostKeys[0]));

static const u16 kHostKeyF1 = 0x3A;
static const u16 kHostKeyF2 = 0x3B;
static const u16 kHostKeyFirstModifier = 0xE0;
static const u16 kHostKeyLeftGui = 0xE3;
static const u16 kHostKeyRightGui = 0xE7;
#if defined(__APPLE__)
static const u16 kHostKeyLeftHotkeyMod = 0xE3;
static const u16 kHostKeyRightHotkeyMod = 0xE7;
#else
static const u16 kHostKeyLeftHotkeyMod = 0xE0;
static const u16 kHostKeyRightHotkeyMod = 0xE4;
#endif

#endif /* HOST_KEYS_H */
