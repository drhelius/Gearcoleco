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

#ifndef TMS9918A_H
#define	TMS9918A_H

#include "Video.h"

class Memory;
class Processor;
class TraceLogger;

class TMS9918A : public Video
{
public:
    TMS9918A(Memory* pMemory, Processor* pProcessor);
    ~TMS9918A();
    void Init();
    void Reset(bool bPAL);
    bool Tick(unsigned int clockCycles);
    u8 GetDataPort();
    u8 GetStatusFlags();
    void WriteData(u8 data);
    void WriteControl(u8 control);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream, u32 version = GC_SAVESTATE_VERSION);
    u8* GetVRAM();
    u8* GetRegisters();
    u16* GetFrameBuffer();
    int GetMode();
    void Render32bit(u16* srcFrameBuffer, u8* dstFrameBuffer, GC_Color_Format pixelFormat, int size, bool overscan = false);
    void Render16bit(u16* srcFrameBuffer, u8* dstFrameBuffer, GC_Color_Format pixelFormat, int size, bool overscan = false);
    void SetOverscan(Overscan overscan);
    Overscan GetOverscan();
    void SetCustomPalette(GC_Color* palette);
    void SetPredefinedPalette(int palette);
    void SetNoSpriteLimit(bool noSpriteLimit);
    bool IsPAL();
    u8 GetBufferReg();
    u16 GetAddressReg();
    u8 GetStatusReg();
    int GetRenderLine();
    int GetCycleCounter();
    bool GetLatch();
    void SetTraceLogger(TraceLogger* pTraceLogger);
    bool IsF18AHardware() const;
    int GetScreenWidth() const;
    int GetScreenHeight() const;

private:
    INLINE void TraceVDPEvent(u8 event, u8 reg = 0xFF, u8 raw = 0, int sprite = 0xFF, int auxiliary = 0);
    void LogVDPEvent(u8 event, u8 reg, u8 raw, int sprite, int auxiliary);
    void ScanLine(int line);
    void LatchSpriteAttributes();
    void RenderBackground(int line);
    void RenderSprites(int line);
    void InitPalettes();

private:
    Memory* m_pMemory;
    Processor* m_pProcessor;
    TraceLogger* m_pTraceLogger;
    u8* m_pInfoBuffer;
    u16* m_pFrameBuffer;
    u8* m_pVdpVRAM;
    bool m_bFirstByteInSequence;
    u8 m_VdpRegister[8];
    u8 m_VdpBuffer;
    u16 m_VdpAddress;
    int m_iCycleCounter;
    u8 m_VdpStatus;
    int m_iLinesPerFrame;
    bool m_bPAL;
    int m_iMode;
    int m_iRenderLine;
    u8 m_SpriteAttribLatch[GC_MAX_SPRITES * 4];
    Overscan m_Overscan;

    struct LineEvents 
    {
        bool vint;
        bool render;
        bool display;
    };

    LineEvents m_LineEvents;

    enum Timing
    {
        TIMING_VINT = 0,
        TIMING_RENDER = 1,
        TIMING_DISPLAY = 2
    };

    int m_Timing[3];
    bool m_bDisplayEnabled;
    bool m_bSpriteOvrRequest;
    bool m_bNoSpriteLimit;

    u16 m_palette_565_rgb[16];
    u16 m_palette_555_rgb[16];
    u16 m_palette_565_bgr[16];
    u16 m_palette_555_bgr[16];

    u8 m_CustomPalette[48];
    u8* m_pCurrentPalette;
};

#include "TraceLogger.h"

INLINE void TMS9918A::TraceVDPEvent(u8 event, u8 reg, u8 raw, int sprite, int auxiliary)
{
    if (IsValidPointer(m_pTraceLogger) && m_pTraceLogger->IsEventEnabled(TRACE_VDP, event))
        LogVDPEvent(event, reg, raw, sprite, auxiliary);
}

inline u8* TMS9918A::GetVRAM()
{
    return m_pVdpVRAM;
}

inline u8* TMS9918A::GetRegisters()
{
    return m_VdpRegister;
}

inline int TMS9918A::GetMode()
{
    return m_iMode;
}

inline u16* TMS9918A::GetFrameBuffer()
{
    return m_pFrameBuffer;
}

#endif	/* TMS9918A_H */
