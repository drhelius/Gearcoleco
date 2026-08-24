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

#include "F18A.h"
#include "Memory.h"
#include "Processor.h"
#include "TraceLogger.h"

F18A::F18A(Memory* pMemory, Processor* pProcessor)
{
    m_pMemory = pMemory;
    m_pProcessor = pProcessor;
    InitPointer(m_pTraceLogger);
    InitPointer(m_pInfoBuffer);
    InitPointer(m_pFrameBuffer);
    InitPointer(m_pVdpVRAM);
    m_bFirstByteInSequence = true;
    for (int i = 0; i < 64; i++)
        m_VdpRegister[i] = 0;
    m_VdpBuffer = 0;
    m_VdpAddress = 0;
    m_iCycleCounter = 0;
    m_VdpStatus = 0;
    m_iLinesPerFrame = 0;
    m_bPAL = false;
    m_LineEvents.vint = false;
    m_LineEvents.render = false;
    m_LineEvents.display = false;
    m_iRenderLine = 0;
    m_iMode = 0;
    m_bDisplayEnabled = false;
    m_bSpriteOvrRequest = false;
    m_bNoSpriteLimit = false;
    m_Overscan = OverscanDisabled;
    m_f18a_unlocked = false;
    m_f18a_unlock_sequence = 0;
    m_f18a_data_port_mode = false;
    m_f18a_palette_index = 0;
    m_f18a_palette_latch = 0;
    m_f18a_register_read = 0;
    m_f18a_palette_second_byte = false;
    m_f18a_line_interrupt_pending = false;
    m_f18a_irq_line = false;
    m_screen_width = GC_RESOLUTION_WIDTH;
    m_screen_height = GC_RESOLUTION_HEIGHT;
    m_pending_screen_width = GC_RESOLUTION_WIDTH;
    m_pending_screen_height = GC_RESOLUTION_HEIGHT;
    m_f18a_mode_dirty = true;
    m_f18a_gpu_clock_accumulator = 0;
    m_f18a_counter_nano = 0;
    m_f18a_counter_micro = 0;
    m_f18a_counter_milli = 0;
    m_f18a_counter_seconds = 0;
    m_f18a_counter_snapshot_nano = 0;
    m_f18a_counter_snapshot_micro = 0;
    m_f18a_counter_snapshot_milli = 0;
    m_f18a_counter_snapshot_seconds = 0;

    for (int i = 0; i < 48; i++)
        m_CustomPalette[i] = 0;
    m_pCurrentPalette = const_cast<u8*>(kPalette_888_coleco);
}

F18A::~F18A()
{
    SafeDeleteArray(m_pInfoBuffer);
    SafeDeleteArray(m_pFrameBuffer);
    SafeDeleteArray(m_pVdpVRAM);
}

void F18A::Init()
{
    m_pFrameBuffer = new u16[GC_VIDEO_MAX_WIDTH * GC_VIDEO_MAX_HEIGHT];
    m_pInfoBuffer = new u8[GC_VIDEO_MAX_WIDTH * GC_LINES_PER_FRAME_PAL];
    m_pVdpVRAM = new u8[0x4000];
    InitPalettes();
    Reset(false);
}

void F18A::SetTraceLogger(TraceLogger* pTraceLogger)
{
    m_pTraceLogger = pTraceLogger;
}

void F18A::LogVDPEvent(u8 event, u8 reg, u8 raw, int sprite, int auxiliary)
{
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    GC_Trace_Entry e = {};
    e.type = TRACE_VDP;
    e.vdp.event = event;
    e.vdp.reg = reg;
    e.vdp.raw = raw;
    e.vdp.mode = (u8)m_iMode;
    e.vdp.sprite = (u8)sprite;
    e.vdp.line = (u16)m_iRenderLine;
    e.vdp.hpos = (u16)m_iCycleCounter;
    e.vdp.auxiliary = (u16)auxiliary;

    switch (event)
    {
        case TRACE_VDP_REG_WRITE:
            e.vdp.effective = m_VdpRegister[reg];
            break;
        case TRACE_VDP_NMI_REQUEST:
            e.vdp.raw = auxiliary ? raw : m_VdpRegister[reg];
            e.vdp.effective = m_VdpRegister[reg];
            e.vdp.status_before = m_VdpStatus;
            e.vdp.status_after = m_VdpStatus;
            break;
        case TRACE_VDP_VINT_FLAG:
        case TRACE_VDP_VBLANK:
            e.vdp.status_before = m_VdpStatus;
            e.vdp.status_after = SetBit(m_VdpStatus, 7);
            break;
        case TRACE_VDP_STATUS_READ:
            e.vdp.raw = m_VdpStatus;
            e.vdp.effective = m_VdpStatus;
            e.vdp.status_before = m_VdpStatus;
            e.vdp.status_after = m_VdpStatus & 0x1F;
            break;
        case TRACE_VDP_DISPLAY_CHANGE:
        {
            bool display = IsSetBit(m_VdpRegister[1], 6);
            if (display == m_bDisplayEnabled)
                return;
            e.vdp.reg = 1;
            e.vdp.raw = m_VdpRegister[1];
            e.vdp.effective = m_VdpRegister[1];
            e.vdp.auxiliary = display ? 1 : 0;
            break;
        }
        case TRACE_VDP_DATA_READ:
            e.vdp.raw = m_VdpBuffer;
            e.vdp.effective = m_pVdpVRAM[m_VdpAddress];
            e.vdp.address = (m_VdpAddress - 1) & 0x3FFF;
            e.vdp.auxiliary = (m_VdpAddress + 1) & 0x3FFF;
            break;
        case TRACE_VDP_DATA_WRITE:
            e.vdp.effective = raw;
            e.vdp.address = m_VdpAddress;
            e.vdp.auxiliary = (m_VdpAddress + 1) & 0x3FFF;
            break;
        case TRACE_VDP_SPRITE_OVERFLOW:
            e.vdp.status_before = m_VdpStatus;
            e.vdp.status_after = SetBit(m_VdpStatus, 6);
            e.vdp.status_after = (e.vdp.status_after & 0xE0) | (u8)sprite;
            break;
        case TRACE_VDP_SPRITE_COLLISION:
            e.vdp.status_before = m_VdpStatus;
            e.vdp.status_after = SetBit(m_VdpStatus, 5);
            break;
        default:
            break;
    }

    m_pTraceLogger->TraceLog(e);
#else
    UNUSED(event);
    UNUSED(reg);
    UNUSED(raw);
    UNUSED(sprite);
    UNUSED(auxiliary);
#endif
}

void F18A::Reset(bool bPAL)
{
    m_bPAL = bPAL;
    m_iLinesPerFrame = bPAL ? GC_LINES_PER_FRAME_PAL : GC_LINES_PER_FRAME_NTSC;
    m_bFirstByteInSequence = true;
    m_VdpBuffer = 0;
    m_VdpAddress = 0;
    m_VdpStatus = 0;

    for (int i = 0; i < (GC_VIDEO_MAX_WIDTH * GC_VIDEO_MAX_HEIGHT); i++)
        m_pFrameBuffer[i] = 1;
    for (int i = 0; i < (GC_VIDEO_MAX_WIDTH * GC_LINES_PER_FRAME_PAL); i++)
        m_pInfoBuffer[i] = 0;
    for (int i = 0; i < 0x4000; i++)
        m_pVdpVRAM[i] = 0;
    for (int i = 0; i < 64; i++)
        m_VdpRegister[i] = 0;

    ResetF18A();

    m_bDisplayEnabled = false;
    m_bSpriteOvrRequest = false;
    m_iMode = 0;

    m_LineEvents.vint = false;
    m_LineEvents.display = false;
    m_LineEvents.render = false;

    m_iCycleCounter = 0;
    m_iRenderLine = 0;

    for (int i = 0; i < (GC_MAX_SPRITES * 4); i++)
        m_SpriteAttribLatch[i] = 0;

    m_Timing[TIMING_VINT] = 220;
    m_Timing[TIMING_RENDER] = 195;
    m_Timing[TIMING_DISPLAY] = 37;
}

void F18A::SetNoSpriteLimit(bool noSpriteLimit)
{
    m_bNoSpriteLimit = noSpriteLimit;
}

bool F18A::Tick(unsigned int clockCycles)
{
    bool return_vblank = false;

    RunF18AGPU(clockCycles);

    if ((m_iRenderLine == 0) &&
        ((m_screen_width != m_pending_screen_width) || (m_screen_height != m_pending_screen_height)))
    {
        m_screen_width = m_pending_screen_width;
        m_screen_height = m_pending_screen_height;
    }

    m_iCycleCounter += clockCycles;

    ///// VINT /////
    if (m_iRenderLine == m_screen_height)
    {
        if (!m_LineEvents.vint && (m_iCycleCounter >= m_Timing[TIMING_VINT]))
        {
            m_LineEvents.vint = true;

            TraceVDPEvent(TRACE_VDP_VINT_FLAG);
            TraceVDPEvent(TRACE_VDP_VBLANK);
            m_VdpStatus = SetBit(m_VdpStatus, 7);

            if (IsSetBit(m_VdpRegister[50], 5))
                m_f18a_gpu.Trigger();
            UpdateIRQLine();
        }
    }

    ///// DISPLAY ON/OFF /////
    if (!m_LineEvents.display && (m_iCycleCounter >= m_Timing[TIMING_DISPLAY]))
    {
        m_LineEvents.display = true;
        TraceVDPEvent(TRACE_VDP_DISPLAY_CHANGE);
        m_bDisplayEnabled = IsSetBit(m_VdpRegister[1], 6);
    }

    ///// RENDER /////
    if (!m_LineEvents.render && (m_iCycleCounter >= m_Timing[TIMING_RENDER]))
    {
        m_LineEvents.render = true;
        ScanLine(m_iRenderLine);
    }

    ///// END OF LINE /////
    if (m_iCycleCounter >= GC_CYCLES_PER_LINE)
    {
        LatchSpriteAttributes();
        if (IsSetBit(m_VdpRegister[50], 6))
            m_f18a_gpu.Trigger();
        if (m_iRenderLine == m_screen_height)
            return_vblank = true;
        m_iRenderLine++;
        m_iRenderLine %= m_iLinesPerFrame;
        if ((m_VdpRegister[19] != 0) && (m_iRenderLine == m_VdpRegister[19]))
        {
            m_f18a_line_interrupt_pending = true;
            UpdateIRQLine();
        }
        if (m_iRenderLine == 0)
            TraceVDPEvent(TRACE_VDP_FRAME);
        m_iCycleCounter -= GC_CYCLES_PER_LINE;
        m_LineEvents.vint = false;
        m_LineEvents.render = false;
        m_LineEvents.display = false;
    }

    return return_vblank;
}

u8 F18A::GetDataPort()
{
    m_bFirstByteInSequence = true;
    u8 ret = m_VdpBuffer;
    TraceVDPEvent(TRACE_VDP_DATA_READ);
    m_VdpBuffer = m_pVdpVRAM[m_VdpAddress];
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    m_pProcessor->CheckMemoryBreakpoints(Processor::GC_BREAKPOINT_TYPE_VRAM, m_VdpAddress, true);
#endif
    AdvanceF18ADataPortAddress();
    return ret;
}

u8 F18A::GetStatusFlags()
{
    m_bFirstByteInSequence = true;
    int selected = m_VdpRegister[15] & 0x0F;
    u8 ret = GetF18AStatusRegister(selected);
    TraceVDPEvent(TRACE_VDP_STATUS_READ);
    if (selected == 0)
        m_VdpStatus &= 0x1F;
    else if (selected == 1)
        m_f18a_line_interrupt_pending = false;

    m_f18a_palette_second_byte = false;
    m_f18a_data_port_mode = false;
    UpdateIRQLine();

    return ret;
}

void F18A::WriteData(u8 data)
{
    m_bFirstByteInSequence = true;
    TraceVDPEvent(TRACE_VDP_DATA_WRITE, 0xFF, data);

    if (m_f18a_data_port_mode)
    {
        WriteF18APaletteData(data);
        return;
    }

    m_VdpBuffer = data;
    m_pVdpVRAM[m_VdpAddress] = data;
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    m_pProcessor->CheckMemoryBreakpoints(Processor::GC_BREAKPOINT_TYPE_VRAM, m_VdpAddress, false);
#endif
    AdvanceF18ADataPortAddress();
}

void F18A::WriteControl(u8 control)
{
    if (m_bFirstByteInSequence)
    {
        m_bFirstByteInSequence = false;
        m_VdpAddress = (m_VdpAddress & 0x3F00) | control;
        m_VdpBuffer = control;
    }
    else
    {
        m_bFirstByteInSequence = true;
        m_VdpAddress = ((control & 0x3F) << 8) | m_VdpBuffer;

        if ((control & 0xC0) == 0x00)
        {
            m_VdpBuffer = m_pVdpVRAM[m_VdpAddress];
            m_f18a_register_read = ReadF18ARegister((m_VdpAddress >> 8) & 0x3F);
            AdvanceF18ADataPortAddress();
        }
        else if ((control & 0x80) != 0)
        {
            u8 reg = control & 0x3F;
            WriteVDPRegister(reg, m_VdpBuffer);
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
            m_pProcessor->CheckMemoryBreakpoints(Processor::GC_BREAKPOINT_TYPE_VDP_REGISTER, reg, false);
#endif
            TraceVDPEvent(TRACE_VDP_REG_WRITE, reg, m_VdpBuffer);
        }
    }
}

bool F18A::IsPAL()
{
    return m_bPAL;
}

u8 F18A::GetBufferReg()
{
    return m_VdpBuffer;
}

u16 F18A::GetAddressReg()
{
    return m_VdpAddress;
}

u8 F18A::GetStatusReg()
{
    return m_VdpStatus;
}

int F18A::GetRenderLine()
{
    return m_iRenderLine;
}

int F18A::GetCycleCounter()
{
    return m_iCycleCounter;
}

bool F18A::GetLatch()
{
    return m_bFirstByteInSequence;
}

void F18A::ScanLine(int line)
{
    if (UseF18ARenderer())
    {
        if (line < m_screen_height)
            RenderF18AScanline(line);
        return;
    }

    if (m_bDisplayEnabled)
    {
        if (line < GC_RESOLUTION_HEIGHT)
        {
            RenderBackground(line);

            if (m_iMode != 0x01)
                RenderSprites(line);
        }
    }
    else
    {
        if (line < GC_RESOLUTION_HEIGHT)
        {
            u16 color = m_VdpRegister[7] & 0x0F;
            int line_width = line * GC_RESOLUTION_WIDTH;

            for (int scx = 0; scx < GC_RESOLUTION_WIDTH; scx++)
            {
                int pixel = line_width + scx;
                m_pFrameBuffer[pixel] = color;
                m_pInfoBuffer[pixel] = 0;
            }
        }
    }
}

void F18A::LatchSpriteAttributes()
{
    u16 sprite_attribute_addr = (m_VdpRegister[5] & 0x7F) << 7;

    for (int i = 0; i < (GC_MAX_SPRITES * 4); i++)
        m_SpriteAttribLatch[i] = m_pVdpVRAM[sprite_attribute_addr + i];
}

void F18A::RenderBackground(int line)
{
    int line_offset = line * GC_RESOLUTION_WIDTH;

    int name_table_addr = m_VdpRegister[2] << 10;
    int color_table_addr = m_VdpRegister[3] << 6;
    int pattern_table_addr = m_VdpRegister[4] << 11;
    int region_mask = ((m_VdpRegister[4] & 0x03) << 8) | 0xFF;
    int color_mask = ((m_VdpRegister[3] & 0x7F) << 3) | 0x07;
    int backdrop_color = m_VdpRegister[7] & 0x0F;
    backdrop_color = (backdrop_color > 0) ? backdrop_color : 1;

    int tile_y = line >> 3;
    int tile_y_offset = line & 7;
    int region = 0;

    switch (m_iMode)
    {
        case 1:
        {
            int fg_color = (m_VdpRegister[7] >> 4) & 0x0F;
            int bg_color = backdrop_color;
            fg_color = (fg_color > 0) ? fg_color : backdrop_color;

            for (int i = 0; i < 8; i++)
            {
                int pixel = line_offset + i;
                m_pFrameBuffer[pixel] = bg_color;
                m_pFrameBuffer[pixel + 248] = bg_color;
                m_pInfoBuffer[pixel] = 0x00;
                m_pInfoBuffer[pixel + 248] = 0x00;
            }

            for (int tile_x = 0; tile_x < 40; tile_x++)
            {
                int tile_number = (tile_y * 40) + tile_x;
                int name_tile_addr = name_table_addr + tile_number;
                int name_tile = m_pVdpVRAM[name_tile_addr];
                u8 pattern_line = m_pVdpVRAM[pattern_table_addr + (name_tile << 3) + tile_y_offset];

                int screen_offset = line_offset + (tile_x * 6) + 8;

                for (int tile_pixel = 0; tile_pixel < 6; tile_pixel++)
                {
                    int pixel = screen_offset + tile_pixel;
                    m_pFrameBuffer[pixel] = IsSetBit(pattern_line, 7 - tile_pixel) ? fg_color : bg_color;
                    m_pInfoBuffer[pixel] = 0x00;
                }
            }
            return;
        }
        case 2:
        {
            pattern_table_addr &= 0x2000;
            color_table_addr &= 0x2000;
            region = (tile_y & 0x18) << 5;
            break;
        }
        case 4:
        {
            pattern_table_addr &= 0x2000;
            break;
        }
    }

    for (int tile_x = 0; tile_x < 32; tile_x++)
    {
        int tile_number = (tile_y << 5) + tile_x;
        int name_tile_addr = name_table_addr + tile_number;
        int name_tile = m_pVdpVRAM[name_tile_addr];
        u8 pattern_line = 0;
        u8 color_line = 0;

        if (m_iMode == 4)
        {
            int offset_color = pattern_table_addr + (name_tile << 3) + ((tile_y & 0x03) << 1) + (line & 0x04 ? 1 : 0);
            color_line = m_pVdpVRAM[offset_color];

            int left_color = color_line >> 4;
            int right_color = color_line & 0x0F;
            left_color = (left_color > 0) ? left_color : backdrop_color;
            right_color = (right_color > 0) ? right_color : backdrop_color;

            int screen_offset = line_offset + (tile_x << 3);

            for (int tile_pixel = 0; tile_pixel < 4; tile_pixel++)
            {
                int pixel = screen_offset + tile_pixel;
                m_pFrameBuffer[pixel] = left_color;
                m_pInfoBuffer[pixel] = 0x00;
            }

            for (int tile_pixel = 4; tile_pixel < 8; tile_pixel++)
            {
                int pixel = screen_offset + tile_pixel;
                m_pFrameBuffer[pixel] = right_color;
                m_pInfoBuffer[pixel] = 0x00;
            }

            continue;
        }
        else if (m_iMode == 0)
        {
            pattern_line = m_pVdpVRAM[pattern_table_addr + (name_tile << 3) + tile_y_offset];
            color_line = m_pVdpVRAM[color_table_addr + (name_tile >> 3)];
        }
        else if (m_iMode == 2)
        {
            name_tile += region;
            pattern_line = m_pVdpVRAM[pattern_table_addr + ((name_tile & region_mask) << 3) + tile_y_offset];
            color_line = m_pVdpVRAM[color_table_addr + ((name_tile & color_mask) << 3) + tile_y_offset];
        }

        int fg_color = color_line >> 4;
        int bg_color = color_line & 0x0F;
        fg_color = (fg_color > 0) ? fg_color : backdrop_color;
        bg_color = (bg_color > 0) ? bg_color : backdrop_color;

        int screen_offset = line_offset + (tile_x << 3);

        for (int tile_pixel = 0; tile_pixel < 8; tile_pixel++)
        {
            int pixel = screen_offset + tile_pixel;
            m_pFrameBuffer[pixel] = IsSetBit(pattern_line, 7 - tile_pixel) ? fg_color : bg_color;
            m_pInfoBuffer[pixel] = 0x00;
        }
    }
}

void F18A::RenderSprites(int line)
{
    int sprite_count = 0;
    int sprite_limit = m_VdpRegister[30] & 0x1F;
    int line_width = line * GC_RESOLUTION_WIDTH;
    int sprite_size = IsSetBit(m_VdpRegister[1], 1) ? 16 : 8;
    bool sprite_zoom = IsSetBit(m_VdpRegister[1], 0);

    if (sprite_zoom)
        sprite_size *= 2;

    u16 sprite_pattern_addr = (m_VdpRegister[6] & 0x07) << 11;

    int max_sprite = GC_MAX_SPRITES - 1;

    for (int sprite = 0; sprite <= max_sprite; sprite++)
    {
        int o = sprite << 2;

        if (m_SpriteAttribLatch[o] == 0xD0)
        {
            max_sprite = sprite - 1;
            break;
        }
    }

    for (int sprite = 0; sprite <= max_sprite; sprite++)
    {
        int attrib_i = sprite << 2;
        int sprite_y = (m_SpriteAttribLatch[attrib_i] + 1) & 0xFF;

        if (sprite_y >= 0xE0)
            sprite_y = -(0x100 - sprite_y);

        if ((sprite_y > line) || ((sprite_y + sprite_size) <= line))
            continue;

        sprite_count++;

        if (!IsSetBit(m_VdpStatus, 6) && (sprite_limit != 31) && (sprite_count > sprite_limit))
        {
            TraceVDPEvent(TRACE_VDP_SPRITE_OVERFLOW, 0xFF, 0, sprite);
            m_VdpStatus = SetBit(m_VdpStatus, 6);
            m_VdpStatus = (m_VdpStatus & 0xE0) | sprite;
        }

        int sprite_color = m_SpriteAttribLatch[attrib_i + 3] & 0x0F;

        if (sprite_color == 0)
            continue;

        int sprite_shift = (m_SpriteAttribLatch[attrib_i + 3] & 0x80) ? 32 : 0;
        int sprite_x = m_SpriteAttribLatch[attrib_i + 1] - sprite_shift;

        if (sprite_x >= GC_RESOLUTION_WIDTH)
            continue;

        int sprite_tile = m_SpriteAttribLatch[attrib_i + 2];
        sprite_tile &= IsSetBit(m_VdpRegister[1], 1) ? 0xFC : 0xFF;

        int sprite_line_addr = sprite_pattern_addr + (sprite_tile << 3) +
                               ((line - sprite_y) >> (sprite_zoom ? 1 : 0));

        for (int tile_x = 0; tile_x < sprite_size; tile_x++)
        {
            int sprite_pixel_x = sprite_x + tile_x;

            if (sprite_pixel_x >= GC_RESOLUTION_WIDTH)
                break;
            if (sprite_pixel_x < 0)
                continue;

            int pixel = line_width + sprite_pixel_x;

            bool sprite_pixel = false;

            int tile_x_adjusted = tile_x >> (sprite_zoom ? 1 : 0);

            if (tile_x_adjusted < 8)
                sprite_pixel = IsSetBit(m_pVdpVRAM[sprite_line_addr], 7 - tile_x_adjusted);
            else
                sprite_pixel = IsSetBit(m_pVdpVRAM[sprite_line_addr + 16], 15 - tile_x_adjusted);

            if (sprite_pixel && ((sprite_limit == 31) || (sprite_count <= sprite_limit) ||
                m_bNoSpriteLimit))
            {
                if (!IsSetBit(m_pInfoBuffer[pixel], 0) && (sprite_color > 0))
                {
                    m_pFrameBuffer[pixel] = sprite_color;
                    m_pInfoBuffer[pixel] = SetBit(m_pInfoBuffer[pixel], 0);
                }

                if (IsSetBit(m_pInfoBuffer[pixel], 1))
                {
                    if (!IsSetBit(m_VdpStatus, 5))
                    {
                        TraceVDPEvent(TRACE_VDP_SPRITE_COLLISION, 0xFF, 0,
                            sprite, sprite_pixel_x);
                        m_VdpStatus = SetBit(m_VdpStatus, 5);
                    }
                }
                else
                {
                    m_pInfoBuffer[pixel] = SetBit(m_pInfoBuffer[pixel], 1);
                }
            }
        }
    }
}


void F18A::Render32bit(u16* srcFrameBuffer, u8* dstFrameBuffer, GC_Color_Format pixelFormat, int size, bool overscan)
{
    int x = 0;
    int y = 0;
    int overscan_h_l = 0;
    int overscan_v = 0;
    int overscan_content_v = 0;
    int overscan_content_h = 0;
    int overscan_total_width = GC_RESOLUTION_WIDTH;
    int overscan_total_height = 0;
    bool overscan_enabled = false;
    int overscan_color = (m_VdpRegister[7] & 0x0F) * 3;
    int buffer_size = size * 4;
    bool bgr = (pixelFormat == GC_PIXEL_BGRA8888);

    overscan = false;

    if (overscan && (m_Overscan != OverscanDisabled))
    {
        overscan_enabled = true;
        overscan_content_v = GC_RESOLUTION_HEIGHT;
        overscan_v = m_bPAL ? GC_RESOLUTION_OVERSCAN_V_PAL : GC_RESOLUTION_OVERSCAN_V;
        overscan_total_height = overscan_content_v + (overscan_v * 2);
    }

    if (overscan && (m_Overscan == OverscanFull320))
    {
        overscan_content_h = GC_RESOLUTION_WIDTH;
        overscan_h_l = GC_RESOLUTION_SMS_OVERSCAN_H_320_L;
        overscan_total_width = overscan_content_h + overscan_h_l + GC_RESOLUTION_SMS_OVERSCAN_H_320_R;
    }

    if (overscan && (m_Overscan == OverscanFull284))
    {
        overscan_content_h = GC_RESOLUTION_WIDTH;
        overscan_h_l = GC_RESOLUTION_SMS_OVERSCAN_H_284_L;
        overscan_total_width = overscan_content_h + overscan_h_l + GC_RESOLUTION_SMS_OVERSCAN_H_284_R;
    }

    for (int i = 0, j = 0; j < buffer_size; j += 4)
    {
        u16 src_color = 0;
        if (overscan_enabled)
        {
            bool is_h_overscan = (overscan_h_l > 0) && (x < overscan_h_l || x >= (overscan_h_l + overscan_content_h));
            bool is_v_overscan = (overscan_v > 0) && (y < overscan_v || y >= (overscan_v + overscan_content_v));

            if (is_h_overscan || is_v_overscan)
                src_color = overscan_color;
            else
                src_color = srcFrameBuffer[i++] * 3;

            if (++x == overscan_total_width)
            {
                x = 0;
                if (++y == overscan_total_height)
                {
                    y = 0;
                }
            }
        }
        else
            src_color = srcFrameBuffer[i++] * 3;

        const u8* palette = m_f18a_palette_888_rgb;
        dstFrameBuffer[j + 0] = bgr ? palette[src_color + 2] : palette[src_color];
        dstFrameBuffer[j + 1] = palette[src_color + 1];
        dstFrameBuffer[j + 2] = bgr ? palette[src_color] : palette[src_color + 2];
        dstFrameBuffer[j + 3] = 0xFF;
    }
}

void F18A::Render16bit(u16* srcFrameBuffer, u8* dstFrameBuffer, GC_Color_Format pixelFormat, int size, bool overscan)
{
    int x = 0;
    int y = 0;
    int overscan_h_l = 0;
    int overscan_v = 0;
    int overscan_content_v = 0;
    int overscan_content_h = 0;
    int overscan_total_width = GC_RESOLUTION_WIDTH;
    int overscan_total_height = 0;
    bool overscan_enabled = false;
    int overscan_color = m_VdpRegister[7] & 0x0F;
    int buffer_size = size * 2;
    bool bgr = ((pixelFormat == GC_PIXEL_BGR555) || (pixelFormat == GC_PIXEL_BGR565));
    bool green_6bit = (pixelFormat == GC_PIXEL_RGB565) || (pixelFormat == GC_PIXEL_BGR565);
    const u16* pal;

    if (bgr)
        pal = green_6bit ? m_f18a_palette_565_bgr : m_f18a_palette_555_bgr;
    else
        pal = green_6bit ? m_f18a_palette_565_rgb : m_f18a_palette_555_rgb;

    overscan = false;

    if (overscan && (m_Overscan != OverscanDisabled))
    {
        overscan_enabled = true;
        overscan_content_v = GC_RESOLUTION_HEIGHT;
        overscan_v = m_bPAL ? GC_RESOLUTION_OVERSCAN_V_PAL : GC_RESOLUTION_OVERSCAN_V;
        overscan_total_height = overscan_content_v + (overscan_v * 2);
    }

    if (overscan && (m_Overscan == OverscanFull320))
    {
        overscan_content_h = GC_RESOLUTION_WIDTH;
        overscan_h_l = GC_RESOLUTION_SMS_OVERSCAN_H_320_L;
        overscan_total_width = overscan_content_h + overscan_h_l + GC_RESOLUTION_SMS_OVERSCAN_H_320_R;
    }

    if (overscan && (m_Overscan == OverscanFull284))
    {
        overscan_content_h = GC_RESOLUTION_WIDTH;
        overscan_h_l = GC_RESOLUTION_SMS_OVERSCAN_H_284_L;
        overscan_total_width = overscan_content_h + overscan_h_l + GC_RESOLUTION_SMS_OVERSCAN_H_284_R;
    }

    for (int i = 0, j = 0; j < buffer_size; j += 2)
    {
        u16 src_color = 0;
        if (overscan_enabled)
        {
            bool is_h_overscan = (overscan_h_l > 0) && (x < overscan_h_l || x >= (overscan_h_l + overscan_content_h));
            bool is_v_overscan = (overscan_v > 0) && (y < overscan_v || y >= (overscan_v + overscan_content_v));

            if (is_h_overscan || is_v_overscan)
                src_color = overscan_color;
            else
                src_color = srcFrameBuffer[i++];

            if (++x == overscan_total_width)
            {
                x = 0;
                if (++y == overscan_total_height)
                {
                    y = 0;
                }
            }
        }
        else
            src_color = srcFrameBuffer[i++];

        *(u16*)(&dstFrameBuffer[j]) = pal[src_color];
    }
}

void F18A::SetCustomPalette(GC_Color* palette)
{
    for (int i = 0; i < 16; i++)
    {
        int p = i * 3;
        m_CustomPalette[p] = palette[i].red;
        m_CustomPalette[p + 1] = palette[i].green;
        m_CustomPalette[p + 2] = palette[i].blue;
    }

    m_pCurrentPalette = m_CustomPalette;
    InitPalettes();
}

void F18A::SetPredefinedPalette(int palette)
{
    const u8* predefined;

    switch (palette)
    {
        case 0:
            predefined = kPalette_888_coleco;
            break;
        case 1:
            predefined = kPalette_888_tms9918;
            break;
        default:
            predefined = NULL;
    }

    if (IsValidPointer(predefined))
    {
        m_pCurrentPalette = const_cast<u8*>(predefined);
        InitPalettes();
    }
}

void F18A::InitPalettes()
{
    for (int i=0,j=0; i<16; i++,j+=3)
    {
        u8 red = m_pCurrentPalette[j];
        u8 green = m_pCurrentPalette[j+1];
        u8 blue = m_pCurrentPalette[j+2];

        u8 red_5 = red * 31 / 255;
        u8 green_5 = green * 31 / 255;
        u8 green_6 = green * 63 / 255;
        u8 blue_5 = blue * 31 / 255;

        m_palette_565_rgb[i] = red_5 << 11 | green_6 << 5 | blue_5;
        m_palette_555_rgb[i] = red_5 << 10 | green_5 << 5 | blue_5;
        m_palette_565_bgr[i] = blue_5 << 11 | green_6 << 5 | red_5;
        m_palette_555_bgr[i] = blue_5 << 10 | green_5 << 5 | red_5;
    }
}

void F18A::SetOverscan(Overscan overscan)
{
    m_Overscan = overscan;
}

F18A::Overscan F18A::GetOverscan()
{
    return m_Overscan;
}

void F18A::SaveState(std::ostream& stream)
{
    stream.write(reinterpret_cast<const char*>(m_pInfoBuffer), GC_VIDEO_MAX_WIDTH * GC_LINES_PER_FRAME_PAL);
    stream.write(reinterpret_cast<const char*>(m_pVdpVRAM), 0x4000);
    stream.write(reinterpret_cast<const char*>(&m_bFirstByteInSequence), sizeof(m_bFirstByteInSequence));
    stream.write(reinterpret_cast<const char*>(m_VdpRegister), sizeof(m_VdpRegister));
    stream.write(reinterpret_cast<const char*>(&m_VdpBuffer), sizeof(m_VdpBuffer));
    stream.write(reinterpret_cast<const char*>(&m_VdpAddress), sizeof(m_VdpAddress));
    stream.write(reinterpret_cast<const char*>(&m_iCycleCounter), sizeof(m_iCycleCounter));
    stream.write(reinterpret_cast<const char*>(&m_VdpStatus), sizeof(m_VdpStatus));
    stream.write(reinterpret_cast<const char*>(&m_iLinesPerFrame), sizeof(m_iLinesPerFrame));
    stream.write(reinterpret_cast<const char*>(&m_LineEvents.vint), sizeof(m_LineEvents.vint));
    stream.write(reinterpret_cast<const char*>(&m_LineEvents.render), sizeof(m_LineEvents.render));
    stream.write(reinterpret_cast<const char*>(&m_LineEvents.display), sizeof(m_LineEvents.display));
    stream.write(reinterpret_cast<const char*>(&m_iRenderLine), sizeof(m_iRenderLine));
    stream.write(reinterpret_cast<const char*>(&m_bPAL), sizeof(m_bPAL));
    stream.write(reinterpret_cast<const char*>(&m_iMode), sizeof(m_iMode));
    stream.write(reinterpret_cast<const char*>(m_Timing), sizeof(m_Timing));
    stream.write(reinterpret_cast<const char*>(&m_bDisplayEnabled), sizeof(m_bDisplayEnabled));
    stream.write(reinterpret_cast<const char*>(&m_bSpriteOvrRequest), sizeof(m_bSpriteOvrRequest));
    stream.write(reinterpret_cast<const char*>(m_SpriteAttribLatch), sizeof(m_SpriteAttribLatch));
    stream.write(reinterpret_cast<const char*>(&m_f18a_unlocked), sizeof(m_f18a_unlocked));
    stream.write(reinterpret_cast<const char*>(&m_f18a_unlock_sequence), sizeof(m_f18a_unlock_sequence));
    stream.write(reinterpret_cast<const char*>(&m_f18a_data_port_mode), sizeof(m_f18a_data_port_mode));
    stream.write(reinterpret_cast<const char*>(&m_f18a_palette_index), sizeof(m_f18a_palette_index));
    stream.write(reinterpret_cast<const char*>(&m_f18a_palette_latch), sizeof(m_f18a_palette_latch));
    stream.write(reinterpret_cast<const char*>(&m_f18a_register_read), sizeof(m_f18a_register_read));
    stream.write(reinterpret_cast<const char*>(&m_f18a_palette_second_byte), sizeof(m_f18a_palette_second_byte));
    stream.write(reinterpret_cast<const char*>(&m_f18a_line_interrupt_pending), sizeof(m_f18a_line_interrupt_pending));
    stream.write(reinterpret_cast<const char*>(&m_f18a_irq_line), sizeof(m_f18a_irq_line));
    stream.write(reinterpret_cast<const char*>(&m_screen_width), sizeof(m_screen_width));
    stream.write(reinterpret_cast<const char*>(&m_screen_height), sizeof(m_screen_height));
    stream.write(reinterpret_cast<const char*>(&m_pending_screen_width), sizeof(m_pending_screen_width));
    stream.write(reinterpret_cast<const char*>(&m_pending_screen_height), sizeof(m_pending_screen_height));
    stream.write(reinterpret_cast<const char*>(m_f18a_palette), sizeof(m_f18a_palette));
    stream.write(reinterpret_cast<const char*>(m_f18a_gpu_ram), sizeof(m_f18a_gpu_ram));
    stream.write(reinterpret_cast<const char*>(&m_f18a_gpu_clock_accumulator), sizeof(m_f18a_gpu_clock_accumulator));
    stream.write(reinterpret_cast<const char*>(&m_f18a_counter_nano), sizeof(m_f18a_counter_nano));
    stream.write(reinterpret_cast<const char*>(&m_f18a_counter_micro), sizeof(m_f18a_counter_micro));
    stream.write(reinterpret_cast<const char*>(&m_f18a_counter_milli), sizeof(m_f18a_counter_milli));
    stream.write(reinterpret_cast<const char*>(&m_f18a_counter_seconds), sizeof(m_f18a_counter_seconds));
    stream.write(reinterpret_cast<const char*>(&m_f18a_counter_snapshot_nano), sizeof(m_f18a_counter_snapshot_nano));
    stream.write(reinterpret_cast<const char*>(&m_f18a_counter_snapshot_micro), sizeof(m_f18a_counter_snapshot_micro));
    stream.write(reinterpret_cast<const char*>(&m_f18a_counter_snapshot_milli), sizeof(m_f18a_counter_snapshot_milli));
    stream.write(reinterpret_cast<const char*>(&m_f18a_counter_snapshot_seconds), sizeof(m_f18a_counter_snapshot_seconds));
    m_f18a_gpu.SaveState(stream);
}

void F18A::LoadState(std::istream& stream, u32 version)
{
    UNUSED(version);
    stream.read(reinterpret_cast<char*>(m_pInfoBuffer), GC_VIDEO_MAX_WIDTH * GC_LINES_PER_FRAME_PAL);
    stream.read(reinterpret_cast<char*>(m_pVdpVRAM), 0x4000);
    stream.read(reinterpret_cast<char*>(&m_bFirstByteInSequence), sizeof(m_bFirstByteInSequence));
    stream.read(reinterpret_cast<char*>(m_VdpRegister), sizeof(m_VdpRegister));
    stream.read(reinterpret_cast<char*>(&m_VdpBuffer), sizeof(m_VdpBuffer));
    stream.read(reinterpret_cast<char*>(&m_VdpAddress), sizeof(m_VdpAddress));
    stream.read(reinterpret_cast<char*>(&m_iCycleCounter), sizeof(m_iCycleCounter));
    stream.read(reinterpret_cast<char*>(&m_VdpStatus), sizeof(m_VdpStatus));
    stream.read(reinterpret_cast<char*>(&m_iLinesPerFrame), sizeof(m_iLinesPerFrame));
    stream.read(reinterpret_cast<char*>(&m_LineEvents.vint), sizeof(m_LineEvents.vint));
    stream.read(reinterpret_cast<char*>(&m_LineEvents.render), sizeof(m_LineEvents.render));
    stream.read(reinterpret_cast<char*>(&m_LineEvents.display), sizeof(m_LineEvents.display));
    stream.read(reinterpret_cast<char*>(&m_iRenderLine), sizeof(m_iRenderLine));
    stream.read(reinterpret_cast<char*>(&m_bPAL), sizeof(m_bPAL));
    stream.read(reinterpret_cast<char*>(&m_iMode), sizeof(m_iMode));
    stream.read(reinterpret_cast<char*>(m_Timing), sizeof(m_Timing));
    stream.read(reinterpret_cast<char*>(&m_bDisplayEnabled), sizeof(m_bDisplayEnabled));
    stream.read(reinterpret_cast<char*>(&m_bSpriteOvrRequest), sizeof(m_bSpriteOvrRequest));
    stream.read(reinterpret_cast<char*>(m_SpriteAttribLatch), sizeof(m_SpriteAttribLatch));
    stream.read(reinterpret_cast<char*>(&m_f18a_unlocked), sizeof(m_f18a_unlocked));
    stream.read(reinterpret_cast<char*>(&m_f18a_unlock_sequence), sizeof(m_f18a_unlock_sequence));
    stream.read(reinterpret_cast<char*>(&m_f18a_data_port_mode), sizeof(m_f18a_data_port_mode));
    stream.read(reinterpret_cast<char*>(&m_f18a_palette_index), sizeof(m_f18a_palette_index));
    stream.read(reinterpret_cast<char*>(&m_f18a_palette_latch), sizeof(m_f18a_palette_latch));
    stream.read(reinterpret_cast<char*>(&m_f18a_register_read), sizeof(m_f18a_register_read));
    stream.read(reinterpret_cast<char*>(&m_f18a_palette_second_byte), sizeof(m_f18a_palette_second_byte));
    stream.read(reinterpret_cast<char*>(&m_f18a_line_interrupt_pending), sizeof(m_f18a_line_interrupt_pending));
    stream.read(reinterpret_cast<char*>(&m_f18a_irq_line), sizeof(m_f18a_irq_line));
    stream.read(reinterpret_cast<char*>(&m_screen_width), sizeof(m_screen_width));
    stream.read(reinterpret_cast<char*>(&m_screen_height), sizeof(m_screen_height));
    stream.read(reinterpret_cast<char*>(&m_pending_screen_width), sizeof(m_pending_screen_width));
    stream.read(reinterpret_cast<char*>(&m_pending_screen_height), sizeof(m_pending_screen_height));
    stream.read(reinterpret_cast<char*>(m_f18a_palette), sizeof(m_f18a_palette));
    stream.read(reinterpret_cast<char*>(m_f18a_gpu_ram), sizeof(m_f18a_gpu_ram));
    stream.read(reinterpret_cast<char*>(&m_f18a_gpu_clock_accumulator), sizeof(m_f18a_gpu_clock_accumulator));
    stream.read(reinterpret_cast<char*>(&m_f18a_counter_nano), sizeof(m_f18a_counter_nano));
    stream.read(reinterpret_cast<char*>(&m_f18a_counter_micro), sizeof(m_f18a_counter_micro));
    stream.read(reinterpret_cast<char*>(&m_f18a_counter_milli), sizeof(m_f18a_counter_milli));
    stream.read(reinterpret_cast<char*>(&m_f18a_counter_seconds), sizeof(m_f18a_counter_seconds));
    stream.read(reinterpret_cast<char*>(&m_f18a_counter_snapshot_nano), sizeof(m_f18a_counter_snapshot_nano));
    stream.read(reinterpret_cast<char*>(&m_f18a_counter_snapshot_micro), sizeof(m_f18a_counter_snapshot_micro));
    stream.read(reinterpret_cast<char*>(&m_f18a_counter_snapshot_milli), sizeof(m_f18a_counter_snapshot_milli));
    stream.read(reinterpret_cast<char*>(&m_f18a_counter_snapshot_seconds), sizeof(m_f18a_counter_snapshot_seconds));
    m_f18a_gpu.LoadState(stream);

    for (int i = 0; i < 64; i++)
        UpdateF18APalettePixel(i);
    m_f18a_mode_dirty = true;
    UpdateIRQLine();
}
