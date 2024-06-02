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

#ifndef DEFINITIONS_H
#define	DEFINITIONS_H

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <sstream>

#ifdef DEBUG
#define DEBUG_GEARCOLECO 1
#endif

#if defined(PS2) || defined(PSP)
#define PERFORMANCE
#endif

#define GEARCOLECO_TITLE "Gearcoleco"
#define GEARCOLECO_VERSION "1.1.0"

#ifndef EMULATOR_BUILD
#define EMULATOR_BUILD "undefined"
#endif

#ifndef NULL
#define NULL 0
#endif

#ifdef _WIN32
#define BLARGG_USE_NAMESPACE 1
#endif

//#define GEARCOLECO_DISABLE_DISASSEMBLER

#define MAX_ROM_SIZE 0x800000

#define SafeDelete(pointer) if(pointer != NULL) {delete pointer; pointer = NULL;}
#define SafeDeleteArray(pointer) if(pointer != NULL) {delete [] pointer; pointer = NULL;}

#define InitPointer(pointer) ((pointer) = NULL)
#define IsValidPointer(pointer) ((pointer) != NULL)

#if defined(MSB_FIRST) || defined(__BIG_ENDIAN__) || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define IS_BIG_ENDIAN
#else
#define IS_LITTLE_ENDIAN
#endif

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;

typedef void (*RamChangedCallback) (void);

#define FLAG_CARRY 0x01
#define FLAG_NEGATIVE 0x02
#define FLAG_PARITY 0x04
#define FLAG_X 0x08
#define FLAG_HALF 0x10
#define FLAG_Y 0x20
#define FLAG_ZERO 0x40
#define FLAG_SIGN 0x80
#define FLAG_NONE 0

#define GC_RESOLUTION_WIDTH 256
#define GC_RESOLUTION_HEIGHT 192

#define GC_RESOLUTION_WIDTH_WITH_OVERSCAN 320
#define GC_RESOLUTION_HEIGHT_WITH_OVERSCAN 288
#define GC_RESOLUTION_SMS_OVERSCAN_H_320_L 32
#define GC_RESOLUTION_SMS_OVERSCAN_H_320_R 32
#define GC_RESOLUTION_SMS_OVERSCAN_H_284_L 14
#define GC_RESOLUTION_SMS_OVERSCAN_H_284_R 14
#define GC_RESOLUTION_OVERSCAN_V 24
#define GC_RESOLUTION_OVERSCAN_V_PAL 48

#define GC_CYCLES_PER_LINE 228

#define GC_MASTER_CLOCK_NTSC 3579545
#define GC_LINES_PER_FRAME_NTSC 262
#define GC_FRAMES_PER_SECOND_NTSC 60

#define GC_MASTER_CLOCK_PAL 3546893
#define GC_LINES_PER_FRAME_PAL 313
#define GC_FRAMES_PER_SECOND_PAL 50

#define GC_AUDIO_SAMPLE_RATE 44100
#define GC_AUDIO_BUFFER_SIZE 8192

#define GC_SAVESTATE_MAGIC 0x09200902

struct GC_Color
{
    u8 red;
    u8 green;
    u8 blue;
};

enum GC_Color_Format
{
    GC_PIXEL_RGB565,
    GC_PIXEL_RGB555,
    GC_PIXEL_RGB888,
    GC_PIXEL_BGR565,
    GC_PIXEL_BGR555,
    GC_PIXEL_BGR888
};

enum GC_Keys
{
    Keypad_8 = 0x01,
    Keypad_4 = 0x02,
    Keypad_5 = 0x03,
    Key_Blue = 0x04,
    Keypad_7 = 0x05,
    Keypad_Hash = 0x06,
    Keypad_2 = 0x07,
    Key_Purple = 0x08,
    Keypad_Asterisk = 0x09,
    Keypad_0 = 0x0A,
    Keypad_9 = 0x0B,
    Keypad_3 = 0x0C,
    Keypad_1 = 0x0D,
    Keypad_6 = 0x0E,
    Key_Up = 0x10,
    Key_Right = 0x11,
    Key_Down = 0x12,
    Key_Left = 0x13,
    Key_Left_Button = 0x14,
    Key_Right_Button = 0x15
};

enum GC_Controllers
{
    Controller_1 = 0,
    Controller_2 = 1
};

enum GC_Region
{
    Region_NTSC,
    Region_PAL
};

struct GC_RuntimeInfo
{
    int screen_width;
    int screen_height;
    GC_Region region;
};

#ifdef DEBUG_GEARCOLECO

#ifdef __ANDROID__
#include <android/log.h>
#define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "GEARCOLECO", __VA_ARGS__);
#endif

#define Log(msg, ...) (Log_func(msg, ##__VA_ARGS__))

inline void Log_func(const char* const msg, ...)
{
    static int count = 1;
    char szBuf[512];

    va_list args;
    va_start(args, msg);
    vsprintf(szBuf, msg, args);
    va_end(args);

    printf("%d: %s\n", count, szBuf);
    fflush(stdout);

    count++;
}

#else
#define Log(msg, ...)
#endif

inline u8 SetBit(const u8 value, const u8 bit)
{
    return value | (0x01 << bit);
}

inline u8 UnsetBit(const u8 value, const u8 bit)
{
    return value & (~(0x01 << bit));
}

inline bool IsSetBit(const u8 value, const u8 bit)
{
    return (value & (0x01 << bit)) != 0;
}

inline u8 FlipBit(const u8 value, const u8 bit)
{
    return value ^ (0x01 << bit);
}

inline u8 ReverseBits(const u8 value)
{
    u8 ret = value;
    ret = (ret & 0xF0) >> 4 | (ret & 0x0F) << 4;
    ret = (ret & 0xCC) >> 2 | (ret & 0x33) << 2;
    ret = (ret & 0xAA) >> 1 | (ret & 0x55) << 1;
    return ret;
}

inline int AsHex(const char c)
{
   return c >= 'A' ? c - 'A' + 0xA : c - '0';
}

inline unsigned int Pow2Ceil(u16 n)
{
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    ++n;
    return n;
}

#endif	/* DEFINITIONS_H */
