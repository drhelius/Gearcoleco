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

#ifndef VIDEO_H
#define VIDEO_H

#include "definitions.h"

class TraceLogger;
class F18AGPU;

class Video
{
public:
    enum Overscan
    {
        OverscanDisabled,
        OverscanTopBottom,
        OverscanFull284,
        OverscanFull320
    };

public:
    virtual ~Video()
    {
    }

    virtual void Init() = 0;
    virtual void Reset(bool pal) = 0;
    virtual bool Tick(unsigned int clock_cycles) = 0;
    virtual u8 GetDataPort() = 0;
    virtual u8 GetStatusFlags() = 0;
    virtual void WriteData(u8 data) = 0;
    virtual void WriteControl(u8 control) = 0;
    virtual void SaveState(std::ostream& stream) = 0;
    virtual void LoadState(std::istream& stream, u32 version = GC_SAVESTATE_VERSION) = 0;
    virtual u8* GetVRAM() = 0;
    virtual u8* GetRegisters() = 0;
    virtual u16* GetFrameBuffer() = 0;
    virtual int GetMode() = 0;
    virtual void Render32bit(u16* source, u8* destination, GC_Color_Format pixel_format,
        int size, bool overscan = false) = 0;
    virtual void Render16bit(u16* source, u8* destination, GC_Color_Format pixel_format,
        int size, bool overscan = false) = 0;
    virtual void SetOverscan(Overscan overscan) = 0;
    virtual Overscan GetOverscan() = 0;
    virtual void SetCustomPalette(GC_Color* palette) = 0;
    virtual void SetPredefinedPalette(int palette) = 0;
    virtual void SetNoSpriteLimit(bool no_sprite_limit) = 0;
    virtual bool IsPAL() = 0;
    virtual u8 GetBufferReg() = 0;
    virtual u16 GetAddressReg() = 0;
    virtual u8 GetStatusReg() = 0;
    virtual int GetRenderLine() = 0;
    virtual int GetCycleCounter() = 0;
    virtual bool GetLatch() = 0;
    virtual void SetTraceLogger(TraceLogger* trace_logger) = 0;
    virtual bool IsF18AHardware() const = 0;
    virtual bool IsF18AUnlocked() const
    {
        return false;
    }

    virtual int GetScreenWidth() const = 0;
    virtual int GetScreenHeight() const = 0;

    virtual u8 GetF18AStatusRegister(int index) const
    {
        UNUSED(index);
        return 0;
    }

    virtual const u16* GetF18APalette() const
    {
        return NULL;
    }

    virtual F18AGPU* GetF18AGPU()
    {
        return NULL;
    }
};

const u8 kPalette_888_coleco[48] =
{
    0, 0, 0, 0, 0, 0, 33, 200, 66, 94, 220, 120, 84, 85, 237, 125,
    118, 252, 212, 82, 77, 66, 235, 245, 252, 85, 84, 255, 121, 120, 212,
    193, 84, 230, 206, 128, 33, 176, 59, 201, 91, 186, 204, 204, 204, 255,
    255, 255
};

const u8 kPalette_888_tms9918[48] =
{
    0, 0, 0, 0, 8, 0, 0, 241, 1, 50, 251, 65, 67, 76, 255, 112, 110,
    255, 238, 75, 28, 9, 255, 255, 255, 78, 31, 255, 112, 65, 211, 213,
    0, 228, 221, 52, 0, 209, 0, 219, 79, 211, 193, 212, 190, 244, 255,
    241
};

const u8 k2bitTo8bit[4] = {0, 85, 170, 255};
const u8 k2bitTo5bit[4] = {0, 10, 21, 31};
const u8 k2bitTo6bit[4] = {0, 21, 42, 63};
const u8 k4bitTo8bit[16] =
{
    0, 17, 34, 51, 68, 86, 102, 119, 136, 153, 170, 187, 204, 221, 238, 255
};
const u8 k4bitTo5bit[16] =
{
    0, 2, 4, 6, 8, 10, 12, 14, 17, 19, 21, 23, 25, 27, 29, 31
};
const u8 k4bitTo6bit[16] =
{
    0, 4, 8, 13, 17, 21, 25, 29, 34, 38, 42, 46, 50, 55, 59, 63
};

#endif
