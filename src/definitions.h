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

#ifndef EMULATOR_BUILD
#define EMULATOR_BUILD "undefined"
#endif

#define GEARCOLECO_TITLE "Gearcoleco"
#define GEARCOLECO_VERSION EMULATOR_BUILD
#define GEARCOLECO_TITLE_ASCII "" \
"   ____                          _                 \n" \
"  / ___| ___  __ _ _ __ ___ ___ | | ___  ___ ___   \n" \
" | |  _ / _ \\/ _` | '__/ __/ _ \\| |/ _ \\/ __/ _ \\  \n" \
" | |_| |  __/ (_| | | | (_| (_) | |  __/ (_| (_) | \n" \
"  \\____|\\___|\\__,_|_|  \\___\\___/|_|\\___|\\___\\___/  \n"


#ifdef DEBUG
#define DEBUG_GEARCOLECO 1
#endif

#if defined(PS2) || defined(PSP)
#define PERFORMANCE
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

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(value, min, max) MIN(MAX(value, min), max)
#define UNUSED(expr) (void)(expr)

#if defined(__GNUC__) || defined(__clang__)
    #define likely(x)   __builtin_expect(!!(x), 1)
    #define unlikely(x) __builtin_expect(!!(x), 0)
#else
    #define likely(x)   (x)
    #define unlikely(x) (x)
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
#define GC_VIDEO_MAX_WIDTH 512

#define GC_MAX_GAMEPADS 2
#define GC_MAX_SPRITES 32

#define GC_RESOLUTION_WIDTH_WITH_OVERSCAN 320
#define GC_RESOLUTION_HEIGHT_WITH_OVERSCAN 288
#define GC_RESOLUTION_SMS_OVERSCAN_H_320_L 32
#define GC_RESOLUTION_SMS_OVERSCAN_H_320_R 32
#define GC_RESOLUTION_SMS_OVERSCAN_H_284_L 14
#define GC_RESOLUTION_SMS_OVERSCAN_H_284_R 14
#define GC_RESOLUTION_OVERSCAN_V 24
#define GC_RESOLUTION_OVERSCAN_V_PAL 48
#define GC_VIDEO_MAX_HEIGHT GC_RESOLUTION_HEIGHT_WITH_OVERSCAN

#define GC_CYCLES_PER_LINE 228

#define GC_MASTER_CLOCK_NTSC 3579545
#define GC_LINES_PER_FRAME_NTSC 262
#define GC_FRAMES_PER_SECOND_NTSC 60

#define GC_MASTER_CLOCK_PAL 3579545
#define GC_LINES_PER_FRAME_PAL 313
#define GC_FRAMES_PER_SECOND_PAL 50

#define GC_AUDIO_SAMPLE_RATE 44100
#define GC_AUDIO_BUFFER_SIZE 2048
#define GC_AUDIO_BUFFER_SIZE_V1 8192
#define GC_AUDIO_QUEUE_SIZE 1792

#define GC_SAVESTATE_MAGIC 0x09200902
#define GC_SAVESTATE_VERSION 107
#define GC_SAVESTATE_MIN_VERSION 100
#define GC_SAVESTATE_VERSION_V1 1
#define GC_LIBRETRO_SAVESTATE_SIZE_COLECOVISION 0x3A000
#define GC_LIBRETRO_SAVESTATE_SIZE_ADAM 0x180000
#define GC_LIBRETRO_SAVESTATE_SIZE GC_LIBRETRO_SAVESTATE_SIZE_ADAM

struct GC_SaveState_Header
{
    u32 magic;
    u32 version;
    u32 size;
    s64 timestamp;
    char rom_name[128];
    u32 rom_crc;
    u32 screenshot_size;
    u16 screenshot_width;
    u16 screenshot_height;
    char emu_build[32];
};

struct GC_SaveState_Header_Libretro
{
    u32 magic;
    u32 version;
};

struct GC_SaveState_Screenshot
{
    u32 width;
    u32 height;
    u32 size;
    u8* data;
};

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
    GC_PIXEL_RGBA8888,
    GC_PIXEL_BGR565,
    GC_PIXEL_BGR555,
    GC_PIXEL_BGRA8888
};

enum GC_VideoChip
{
    GC_VIDEO_CHIP_AUTO = 0,
    GC_VIDEO_CHIP_TMS9918A,
    GC_VIDEO_CHIP_F18A
};

enum GC_Machine
{
    GC_MACHINE_AUTO = 0,
    GC_MACHINE_COLECOVISION,
    GC_MACHINE_ADAM
};

enum GC_ContentType
{
    GC_CONTENT_NONE = 0,
    GC_CONTENT_CARTRIDGE,
    GC_CONTENT_ADAM_DATA_PACK,
    GC_CONTENT_ADAM_DISK
};

enum GC_AdamBootMode
{
    GC_ADAM_BOOT_COMPUTER = 0,
    GC_ADAM_BOOT_CARTRIDGE
};

enum GC_AdamFirmware
{
    GC_ADAM_FIRMWARE_OS7 = 0,
    GC_ADAM_FIRMWARE_EOS,
    GC_ADAM_FIRMWARE_SMARTWRITER,
    GC_ADAM_FIRMWARE_COUNT
};

enum GC_AdamMediaType
{
    GC_ADAM_MEDIA_NONE = 0,
    GC_ADAM_MEDIA_DATA_PACK,
    GC_ADAM_MEDIA_DISK
};

enum GC_AdamMediaSlot
{
    GC_ADAM_MEDIA_DISK_1 = 0,
    GC_ADAM_MEDIA_DISK_2,
    GC_ADAM_MEDIA_DATA_PACK_1,
    GC_ADAM_MEDIA_DATA_PACK_2,
    GC_ADAM_MEDIA_SLOT_COUNT
};

enum GC_AdamMediaError
{
    GC_ADAM_MEDIA_ERROR_NONE = 0,
    GC_ADAM_MEDIA_ERROR_INVALID_ARGUMENT,
    GC_ADAM_MEDIA_ERROR_INVALID_SIZE,
    GC_ADAM_MEDIA_ERROR_NO_MEDIA,
    GC_ADAM_MEDIA_ERROR_WRITE_PROTECTED,
    GC_ADAM_MEDIA_ERROR_OUT_OF_RANGE,
    GC_ADAM_MEDIA_ERROR_CHANGED
};

enum GC_AdamKey
{
    GC_ADAM_KEY_A = 0,
    GC_ADAM_KEY_B,
    GC_ADAM_KEY_C,
    GC_ADAM_KEY_D,
    GC_ADAM_KEY_E,
    GC_ADAM_KEY_F,
    GC_ADAM_KEY_G,
    GC_ADAM_KEY_H,
    GC_ADAM_KEY_I,
    GC_ADAM_KEY_J,
    GC_ADAM_KEY_K,
    GC_ADAM_KEY_L,
    GC_ADAM_KEY_M,
    GC_ADAM_KEY_N,
    GC_ADAM_KEY_O,
    GC_ADAM_KEY_P,
    GC_ADAM_KEY_Q,
    GC_ADAM_KEY_R,
    GC_ADAM_KEY_S,
    GC_ADAM_KEY_T,
    GC_ADAM_KEY_U,
    GC_ADAM_KEY_V,
    GC_ADAM_KEY_W,
    GC_ADAM_KEY_X,
    GC_ADAM_KEY_Y,
    GC_ADAM_KEY_Z,
    GC_ADAM_KEY_0,
    GC_ADAM_KEY_1,
    GC_ADAM_KEY_2,
    GC_ADAM_KEY_3,
    GC_ADAM_KEY_4,
    GC_ADAM_KEY_5,
    GC_ADAM_KEY_6,
    GC_ADAM_KEY_7,
    GC_ADAM_KEY_8,
    GC_ADAM_KEY_9,
    GC_ADAM_KEY_SPACE,
    GC_ADAM_KEY_MINUS,
    GC_ADAM_KEY_PLUS,
    GC_ADAM_KEY_CARET,
    GC_ADAM_KEY_SEMICOLON,
    GC_ADAM_KEY_QUOTE,
    GC_ADAM_KEY_OPEN_BRACKET,
    GC_ADAM_KEY_CLOSE_BRACKET,
    GC_ADAM_KEY_BACKSLASH,
    GC_ADAM_KEY_COMMA,
    GC_ADAM_KEY_PERIOD,
    GC_ADAM_KEY_SLASH,
    GC_ADAM_KEY_RETURN,
    GC_ADAM_KEY_ESCAPE,
    GC_ADAM_KEY_BACKSPACE,
    GC_ADAM_KEY_TAB,
    GC_ADAM_KEY_HOME,
    GC_ADAM_KEY_SMART_1,
    GC_ADAM_KEY_SMART_2,
    GC_ADAM_KEY_SMART_3,
    GC_ADAM_KEY_SMART_4,
    GC_ADAM_KEY_SMART_5,
    GC_ADAM_KEY_SMART_6,
    GC_ADAM_KEY_WILD_CARD,
    GC_ADAM_KEY_UNDO,
    GC_ADAM_KEY_MOVE,
    GC_ADAM_KEY_STORE,
    GC_ADAM_KEY_INSERT,
    GC_ADAM_KEY_PRINT,
    GC_ADAM_KEY_CLEAR,
    GC_ADAM_KEY_DELETE,
    GC_ADAM_KEY_UP,
    GC_ADAM_KEY_RIGHT,
    GC_ADAM_KEY_DOWN,
    GC_ADAM_KEY_LEFT,
    GC_ADAM_KEY_SHIFT,
    GC_ADAM_KEY_CONTROL,
    GC_ADAM_KEY_LOCK,
    GC_ADAM_KEY_COUNT
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
    double fps;
};

enum GC_Disassembler_Syntax
{
    GC_Disassembler_Syntax_Gearcoleco = 0,
    GC_Disassembler_Syntax_WLADX,
    GC_Disassembler_Syntax_TNIASM,
    GC_Disassembler_Syntax_Z88DK,
    GC_Disassembler_Syntax_Count
};

struct GC_Disassembler_Record
{
    u32 address;
    u8 bank;
    char name[64];
    char bytes[25];
    char segment[8];
    u8 opcodes[7];
    int size;
    bool jump;
    u16 jump_address;
    u8 jump_bank;
    bool subroutine;
    int irq;
    bool has_operand_address;
    u16 operand_address;
    bool operand_is_zp;
    int operand_offset;
    int operand_length;
    char auto_symbol[64];
};

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

#if defined(__GNUC__) || defined(__clang__)
    #define INLINE inline __attribute__((always_inline))
    #define NO_INLINE __attribute__((noinline))
#elif defined(_MSC_VER)
    #define INLINE __forceinline
    #define NO_INLINE __declspec(noinline)
#else
    #define INLINE inline
    #define NO_INLINE
#endif

#endif	/* DEFINITIONS_H */
