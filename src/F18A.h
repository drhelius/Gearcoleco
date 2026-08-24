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

#ifndef F18A_H
#define	F18A_H

#include "Video.h"
#include "F18AGPU.h"

class Memory;
class Processor;
class TraceLogger;

#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER) || defined(GEARCOLECO_ENABLE_VIDEO_DEBUG_TESTS)
struct F18ADebugTileInfo
{
    int column;
    int row;
    u16 name_address;
    u16 attribute_address;
    u16 pattern_address;
    u8 name;
    u8 attribute;
    int palette_select;
    bool flip_x;
    bool flip_y;
    bool priority;
};
#endif

class F18A : public Video
{
public:
    F18A(Memory* pMemory, Processor* pProcessor);
    ~F18A();
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
    bool IsF18AUnlocked() const;
    int GetScreenWidth() const;
    int GetScreenHeight() const;
    u8 GetF18AStatusRegister(int index) const;
    const u16* GetF18APalette() const;
    F18AGPU* GetF18AGPU();
    u8 ReadF18AGPUByte(u16 address);
    void WriteF18AGPUByte(u16 address, u8 value);
    u16 ReadF18AGPUWord(u16 address);
    void WriteF18AGPUWord(u16 address, u16 value);
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER) || defined(GEARCOLECO_ENABLE_VIDEO_DEBUG_TESTS)
    void RenderDebugNameTable(u16* buffer, bool layer2);
    void RenderDebugPatternTable(u16* buffer, int palette);
    int RenderDebugSprite(u16* buffer, int index);
    bool GetDebugTileInfo(int x, int y, bool layer2, F18ADebugTileInfo& info);
#endif

private:
    INLINE void TraceVDPEvent(u8 event, u8 reg = 0xFF, u8 raw = 0,
        int sprite = 0xFF, int auxiliary = 0);
    void LogVDPEvent(u8 event, u8 reg, u8 raw, int sprite, int auxiliary);
    void ScanLine(int line);
    void LatchSpriteAttributes();
    void RenderBackground(int line);
    void RenderSprites(int line);
    void InitPalettes();
    void ResetF18A();
    void ResetF18ARegisters();
    void ResetF18APalette();
    void ResetF18AGPURAM();
    void UpdateF18APalettePixel(int index);
    void WriteF18APaletteData(u8 value);
    void WriteVDPRegister(u8 index, u8 value, bool from_gpu = false);
    void WriteF18ARegister(u8 index, u8 value, bool from_gpu);
    u8 ReadF18ARegister(u8 index) const;
    bool HandleF18AUnlockWrite(u8 index, u8 value);
    void AdvanceF18ADataPortAddress();
    void UpdateIRQLine();
    void UpdateF18AMode();
    bool UseF18ARenderer();
    void RenderF18AScanline(int line);
    void RenderF18ATileLayer(int line, bool layer2);
    void RenderF18ABitmapLine(int line);
    void RenderF18ASprites(int line);
    void ComposeF18AScanline(int line);
    u8 GetF18APatternPixel(u16 address, int x, u8 ecm, int offset) const;
    void RunF18AGPU(unsigned int clockCycles);
    void RunF18ACounter(unsigned int gpu_cycles);
    INLINE u8 ReadVRAM(u16 address) const;

    struct F18ALinePixel
    {
        u8 color;
        u8 flags;
    };

    struct F18AMode
    {
        int width;
        int height;
        int legacy_mode;
        bool layer_1_enabled;
        bool layer_2_enabled;
        bool bitmap_enabled;
        bool sprites_enabled;
        bool row30;
        bool text_mode;
        u8 tile_ecm;
        u8 sprite_ecm;
    };

private:
    Memory* m_pMemory;
    Processor* m_pProcessor;
    TraceLogger* m_pTraceLogger;
    u8* m_pInfoBuffer;
    u16* m_pFrameBuffer;
    u8* m_pVdpVRAM;
    bool m_bFirstByteInSequence;
    u8 m_VdpRegister[64];
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
    bool m_f18a_unlocked;
    u8 m_f18a_unlock_sequence;
    bool m_f18a_data_port_mode;
    u8 m_f18a_palette_index;
    u8 m_f18a_palette_latch;
    u8 m_f18a_register_read;
    bool m_f18a_palette_second_byte;
    bool m_f18a_line_interrupt_pending;
    bool m_f18a_irq_line;
    int m_screen_width;
    int m_screen_height;
    int m_pending_screen_width;
    int m_pending_screen_height;
    bool m_f18a_mode_dirty;
    F18AMode m_f18a_mode;
    u16 m_f18a_palette[64];
    u8 m_f18a_gpu_ram[0x800];
    u64 m_f18a_gpu_clock_accumulator;
    u16 m_f18a_counter_nano;
    u16 m_f18a_counter_micro;
    u16 m_f18a_counter_milli;
    u16 m_f18a_counter_seconds;
    u16 m_f18a_counter_snapshot_nano;
    u16 m_f18a_counter_snapshot_micro;
    u16 m_f18a_counter_snapshot_milli;
    u16 m_f18a_counter_snapshot_seconds;
    F18AGPU m_f18a_gpu;
    F18ALinePixel m_f18a_tile_line[GC_VIDEO_MAX_WIDTH];
    F18ALinePixel m_f18a_sprite_line[GC_VIDEO_MAX_WIDTH];
    u8 m_f18a_sprite_occupancy[GC_VIDEO_MAX_WIDTH];

    u16 m_palette_565_rgb[16];
    u16 m_palette_555_rgb[16];
    u16 m_palette_565_bgr[16];
    u16 m_palette_555_bgr[16];
    u16 m_f18a_palette_565_rgb[64];
    u16 m_f18a_palette_555_rgb[64];
    u16 m_f18a_palette_565_bgr[64];
    u16 m_f18a_palette_555_bgr[64];
    u8 m_f18a_palette_888_rgb[64 * 3];

    u8 m_CustomPalette[48];
    u8* m_pCurrentPalette;
};

#include "TraceLogger.h"

INLINE void F18A::TraceVDPEvent(u8 event, u8 reg, u8 raw, int sprite, int auxiliary)
{
    if (IsValidPointer(m_pTraceLogger) && m_pTraceLogger->IsEventEnabled(TRACE_VDP, event))
        LogVDPEvent(event, reg, raw, sprite, auxiliary);
}

inline u8* F18A::GetVRAM()
{
    return m_pVdpVRAM;
}

inline u8* F18A::GetRegisters()
{
    return m_VdpRegister;
}

inline int F18A::GetMode()
{
    return m_iMode;
}

inline u16* F18A::GetFrameBuffer()
{
    return m_pFrameBuffer;
}

INLINE u8 F18A::ReadVRAM(u16 address) const
{
    return m_pVdpVRAM[address & 0x3FFF];
}

#endif	/* F18A_H */
