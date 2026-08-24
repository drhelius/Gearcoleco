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
#include "Processor.h"
#include "TraceLogger.h"

static const u16 kF18APalette[64] =
{
    0x000, 0x000, 0x2C3, 0x5D6, 0x54F, 0x76F, 0xD54, 0x4EF,
    0xF54, 0xF76, 0xDC3, 0xED6, 0x2B2, 0xC5C, 0xCCC, 0xFFF,
    0x000, 0x2C3, 0x000, 0x54F, 0x000, 0xD54, 0x000, 0x4EF,
    0x000, 0xCCC, 0x000, 0xDC3, 0x000, 0xC5C, 0x000, 0xFFF,
    0x000, 0x00A, 0x0A0, 0x0AA, 0xA00, 0xA0A, 0xA50, 0xAAA,
    0x555, 0x55F, 0x5F5, 0x5FF, 0xF55, 0xF5F, 0xFF5, 0xFFF,
    0x000, 0x555, 0x000, 0x00A, 0x000, 0x0A0, 0x000, 0x0AA,
    0x000, 0xA00, 0x000, 0xA0A, 0x000, 0xA50, 0x000, 0xFFF
};

// F18A V1.9 power-on GPU RAM image.
// Source: dnotq/f18a commit 431896319fb64fa1b42811222f7a19c9b594669b, f18a_gpu.vhd.
static const u8 kF18AGPUFirmware[470] =
{
    0x02, 0x0F, 0x47, 0xFE, 0x10, 0x0D, 0x40, 0x36, 0x40, 0x5A, 0x40, 0x94, 0x40, 0xB4, 0x40, 0xFA,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x0C, 0xA0, 0x41, 0x1C, 0x03, 0x40, 0x04, 0xC1, 0xD0, 0x60, 0x3F, 0x00, 0x09, 0x71, 0xC0, 0x21,
    0x40, 0x06, 0x06, 0x90, 0x10, 0xF7, 0xC0, 0x20, 0x3F, 0x02, 0xC0, 0x60, 0x3F, 0x04, 0xC0, 0xA0,
    0x3F, 0x06, 0xD0, 0xE0, 0x3F, 0x01, 0x13, 0x05, 0xD0, 0x10, 0xDC, 0x40, 0x06, 0x02, 0x16, 0xFD,
    0x10, 0x03, 0xDC, 0x70, 0x06, 0x02, 0x16, 0xFD, 0x04, 0x5B, 0x0D, 0x0B, 0x06, 0xA0, 0x40, 0xB4,
    0x0F, 0x0B, 0xC1, 0xC7, 0x13, 0x16, 0x04, 0xC0, 0xD0, 0x20, 0x60, 0x04, 0x0A, 0x30, 0xC0, 0xC0,
    0x04, 0xC1, 0x02, 0x02, 0x04, 0x00, 0xCC, 0x01, 0x06, 0x02, 0x16, 0xFD, 0x04, 0xC0, 0xD0, 0x20,
    0x41, 0x4F, 0x06, 0xC0, 0x0A, 0x30, 0xA0, 0x03, 0x0C, 0xA0, 0x41, 0xAC, 0xD8, 0x20, 0x41, 0x4F,
    0xB0, 0x00, 0x04, 0x5B, 0xD8, 0x20, 0x41, 0x1A, 0x3F, 0x00, 0x02, 0x00, 0x41, 0xD4, 0xC8, 0x00,
    0x3F, 0x02, 0x02, 0x00, 0x40, 0x06, 0xC8, 0x00, 0x3F, 0x04, 0x02, 0x00, 0x40, 0x10, 0xC8, 0x00,
    0x3F, 0x06, 0x04, 0x5B, 0x04, 0xC7, 0xD0, 0x20, 0x3F, 0x01, 0x13, 0x13, 0xC0, 0x20, 0x41, 0x18,
    0x06, 0x00, 0x0C, 0xA0, 0x41, 0x50, 0x02, 0x04, 0x00, 0x05, 0x02, 0x05, 0x3F, 0x02, 0x02, 0x06,
    0x41, 0x40, 0x8D, 0xB5, 0x16, 0x03, 0x06, 0x04, 0x16, 0xFC, 0x10, 0x09, 0x06, 0x00, 0x16, 0xF1,
    0x10, 0x09, 0xC0, 0x20, 0x3F, 0x02, 0x0C, 0xA0, 0x41, 0x50, 0x80, 0x40, 0x14, 0x03, 0x0C, 0xA0,
    0x41, 0x98, 0x05, 0x47, 0xD8, 0x07, 0xB0, 0x00, 0x04, 0x5B, 0x0D, 0x0B, 0x06, 0xA0, 0x40, 0xB4,
    0x0F, 0x0B, 0xC1, 0xC7, 0x13, 0x04, 0xC0, 0x20, 0x3F, 0x0C, 0x0C, 0xA0, 0x41, 0xAC, 0x04, 0x5B,
    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x41, 0x10,
    0x02, 0x01, 0x41, 0x14, 0x02, 0x02, 0x0B, 0x00, 0x03, 0xA0, 0x32, 0x02, 0x32, 0x30, 0x32, 0x30,
    0x32, 0x30, 0x02, 0x02, 0x00, 0x07, 0x36, 0x31, 0x06, 0x02, 0x16, 0xFD, 0x03, 0xC0, 0x0C, 0x00,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x88, 0x00, 0x41, 0x18, 0x1A, 0x03, 0xC0, 0x60, 0x41, 0x18, 0x0C, 0x00, 0x0D, 0x00, 0x0A, 0x40,
    0x02, 0x01, 0x0B, 0x00, 0xA0, 0x20, 0x41, 0x16, 0x17, 0x01, 0x05, 0x81, 0xA0, 0x60, 0x41, 0x14,
    0x02, 0x03, 0x41, 0x40, 0x02, 0x02, 0x00, 0x10, 0x03, 0xA0, 0x32, 0x01, 0x06, 0xC1, 0x32, 0x01,
    0x32, 0x00, 0x06, 0xC0, 0x32, 0x00, 0x36, 0x00, 0x36, 0x33, 0x06, 0x02, 0x16, 0xFD, 0x03, 0xC0,
    0x0F, 0x00, 0xC0, 0x60, 0x41, 0x18, 0x0C, 0x00, 0x02, 0x00, 0x3F, 0x00, 0x02, 0x01, 0x41, 0x40,
    0x02, 0x02, 0x00, 0x08, 0xCC, 0x31, 0x06, 0x02, 0x16, 0xFD, 0x0C, 0x00, 0x02, 0x01, 0x41, 0x4A,
    0xD0, 0xA0, 0x41, 0x4E, 0x06, 0xC2, 0xD0, 0xA0, 0x41, 0x4D, 0x02, 0x03, 0x0B, 0x00, 0x03, 0xA0,
    0x32, 0x03, 0x32, 0x31, 0x32, 0x31, 0x32, 0x31, 0x36, 0x01, 0x36, 0x30, 0x06, 0x02, 0x16, 0xFD,
    0x03, 0xC0, 0x0C, 0x00, 0x03, 0x40
};

static const u8 F18A_PIXEL_OPAQUE = 0x01;
static const u8 F18A_PIXEL_PRIORITY = 0x02;

bool F18A::IsF18AHardware() const
{
    return true;
}

bool F18A::IsF18AUnlocked() const
{
    return m_f18a_unlocked;
}

int F18A::GetScreenWidth() const
{
    return m_screen_width;
}

int F18A::GetScreenHeight() const
{
    return m_screen_height;
}

const u16* F18A::GetF18APalette() const
{
    return m_f18a_palette;
}

F18AGPU* F18A::GetF18AGPU()
{
    return &m_f18a_gpu;
}

void F18A::ResetF18ARegisters()
{
    for (int i = 0; i < 64; i++)
        m_VdpRegister[i] = 0;

    m_VdpRegister[1] = 0x40;
    m_VdpRegister[3] = 0x10;
    m_VdpRegister[4] = 0x01;
    m_VdpRegister[5] = 0x0A;
    m_VdpRegister[6] = 0x02;
    m_VdpRegister[7] = 0x1F;
    m_VdpRegister[30] = 0x1F;
    m_VdpRegister[48] = 0x01;
    m_VdpRegister[51] = 0x20;
    m_VdpRegister[54] = 0x40;
    m_f18a_unlocked = false;
    m_f18a_unlock_sequence = 0;
    m_f18a_mode_dirty = true;
}

void F18A::ResetF18APalette()
{
    for (int i = 0; i < 64; i++)
    {
        m_f18a_palette[i] = kF18APalette[i];
        UpdateF18APalettePixel(i);
    }
}

void F18A::ResetF18AGPURAM()
{
    memset(m_f18a_gpu_ram, 0, sizeof(m_f18a_gpu_ram));
    memcpy(m_f18a_gpu_ram, kF18AGPUFirmware, sizeof(kF18AGPUFirmware));
}

void F18A::ResetF18A()
{
    ResetF18ARegisters();
    ResetF18APalette();
    ResetF18AGPURAM();
    m_f18a_data_port_mode = false;
    m_f18a_palette_index = 0;
    m_f18a_palette_latch = 0;
    m_f18a_register_read = 0;
    m_f18a_palette_second_byte = false;
    m_f18a_line_interrupt_pending = false;
    m_f18a_irq_line = false;
    m_f18a_gpu_clock_accumulator = 0;
    m_f18a_counter_nano = 0;
    m_f18a_counter_micro = 0;
    m_f18a_counter_milli = 0;
    m_f18a_counter_seconds = 0;
    m_f18a_counter_snapshot_nano = 0;
    m_f18a_counter_snapshot_micro = 0;
    m_f18a_counter_snapshot_milli = 0;
    m_f18a_counter_snapshot_seconds = 0;
    m_f18a_gpu.Reset(0x4000, true);
    m_screen_width = GC_RESOLUTION_WIDTH;
    m_screen_height = GC_RESOLUTION_HEIGHT;
    m_pending_screen_width = GC_RESOLUTION_WIDTH;
    m_pending_screen_height = GC_RESOLUTION_HEIGHT;
    UpdateF18AMode();
}

void F18A::UpdateF18APalettePixel(int index)
{
    u16 color = m_f18a_palette[index & 0x3F];
    u8 red = k4bitTo8bit[(color >> 8) & 0x0F];
    u8 green = k4bitTo8bit[(color >> 4) & 0x0F];
    u8 blue = k4bitTo8bit[color & 0x0F];
    int rgb = (index & 0x3F) * 3;
    m_f18a_palette_888_rgb[rgb] = red;
    m_f18a_palette_888_rgb[rgb + 1] = green;
    m_f18a_palette_888_rgb[rgb + 2] = blue;

    u8 red5 = k4bitTo5bit[(color >> 8) & 0x0F];
    u8 green5 = k4bitTo5bit[(color >> 4) & 0x0F];
    u8 green6 = k4bitTo6bit[(color >> 4) & 0x0F];
    u8 blue5 = k4bitTo5bit[color & 0x0F];
    m_f18a_palette_565_rgb[index] = (red5 << 11) | (green6 << 5) | blue5;
    m_f18a_palette_555_rgb[index] = (red5 << 10) | (green5 << 5) | blue5;
    m_f18a_palette_565_bgr[index] = (blue5 << 11) | (green6 << 5) | red5;
    m_f18a_palette_555_bgr[index] = (blue5 << 10) | (green5 << 5) | red5;
}

void F18A::WriteF18APaletteData(u8 value)
{
    if (!m_f18a_palette_second_byte)
    {
        m_f18a_palette_latch = value & 0x0F;
        m_f18a_palette_second_byte = true;
        return;
    }

    int index = m_f18a_palette_index & 0x3F;
    m_f18a_palette[index] = (m_f18a_palette_latch << 8) | value;
    UpdateF18APalettePixel(index);
    m_f18a_palette_second_byte = false;

    if (IsSetBit(m_VdpRegister[47], 6))
    {
        m_f18a_palette_index = (index + 1) & 0x3F;
        if (index == 63)
            m_f18a_data_port_mode = false;
    }
    else
    {
        m_f18a_data_port_mode = false;
    }
}

bool F18A::HandleF18AUnlockWrite(u8 index, u8 value)
{
    if (index != 57)
        return false;

    bool valid = (value & 0xFC) == 0x1C;
    m_f18a_unlocked = valid && (m_f18a_unlock_sequence != 0);
    m_f18a_unlock_sequence = valid ? 1 : 0;
    m_f18a_mode_dirty = true;
    return true;
}

void F18A::WriteVDPRegister(u8 index, u8 value, bool from_gpu)
{
    if (HandleF18AUnlockWrite(index, value))
        return;

    if (from_gpu)
    {
        m_f18a_unlock_sequence = 0;
        WriteF18ARegister(index, value, true);
        return;
    }

    if (!m_f18a_unlocked)
    {
        if (index < 8)
        {
            m_f18a_unlock_sequence = 0;
            WriteF18ARegister(index, value, from_gpu);
        }
        else if (!IsSetBit(m_VdpRegister[0], 2))
        {
            m_f18a_unlock_sequence = 0;
            WriteF18ARegister(index & 7, value, from_gpu);
        }
        return;
    }

    m_f18a_unlock_sequence = 0;
    WriteF18ARegister(index, value, from_gpu);
}

void F18A::WriteF18ARegister(u8 index, u8 value, bool from_gpu)
{
    bool mode_change = false;

    switch (index)
    {
        case 0: m_VdpRegister[index] = value & 0x16; mode_change = true; break;
        case 1: m_VdpRegister[index] = value & 0x7B; mode_change = true; break;
        case 2: m_VdpRegister[index] = value & 0x0F; mode_change = true; break;
        case 3: m_VdpRegister[index] = value; mode_change = true; break;
        case 4: m_VdpRegister[index] = value & 0x07; mode_change = true; break;
        case 5: m_VdpRegister[index] = value & 0x7F; mode_change = true; break;
        case 6: m_VdpRegister[index] = value & 0x07; mode_change = true; break;
        case 7: m_VdpRegister[index] = value; mode_change = true; break;
        case 10: m_VdpRegister[index] = value & 0x0F; mode_change = true; break;
        case 11: m_VdpRegister[index] = value; mode_change = true; break;

        case 15:
            if (IsSetBit(value, 5))
            {
                m_f18a_counter_snapshot_nano = m_f18a_counter_nano;
                m_f18a_counter_snapshot_micro = m_f18a_counter_micro;
                m_f18a_counter_snapshot_milli = m_f18a_counter_milli;
                m_f18a_counter_snapshot_seconds = m_f18a_counter_seconds;
            }
            if (IsSetBit(value, 6))
            {
                m_f18a_counter_nano = 0;
                m_f18a_counter_micro = 0;
                m_f18a_counter_milli = 0;
                m_f18a_counter_seconds = 0;
            }
            m_VdpRegister[index] = value & 0x1F;
            break;

        case 19:
        {
            u8 old_value = m_VdpRegister[index];
            m_VdpRegister[index] = value;
            if ((value != 0) && (value == m_iRenderLine) && (old_value != value))
            {
                m_f18a_line_interrupt_pending = true;
                UpdateIRQLine();
            }
            break;
        }
        case 24: m_VdpRegister[index] = value & 0x3F; mode_change = true; break;
        case 25:
        case 26:
        case 27:
        case 28:
        case 29:
            m_VdpRegister[index] = value;
            mode_change = true;
            break;

        case 30:
            m_VdpRegister[index] = value & 0x1F;
            if (m_VdpRegister[index] == 0)
                m_VdpRegister[index] = 0x1F;
            break;

        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
            m_VdpRegister[index] = value;
            mode_change = true;
            break;

        case 47:
            m_VdpRegister[index] = value;
            if (!from_gpu)
            {
                m_f18a_palette_index = value & 0x3F;
                m_f18a_data_port_mode = IsSetBit(value, 7);
            }
            break;

        case 48: m_VdpRegister[index] = value; break;
        case 49: m_VdpRegister[index] = value & 0xFB; mode_change = true; break;

        case 50:
            if (IsSetBit(value, 7))
            {
                ResetF18ARegisters();
                m_f18a_data_port_mode = false;
                m_f18a_palette_second_byte = false;
            }
            else
            {
                m_VdpRegister[index] = value & 0x7F;
                mode_change = true;
            }
            break;

        case 51: m_VdpRegister[index] = value & 0x3F; break;
        case 54: m_VdpRegister[index] = value; break;
        case 55:
            m_VdpRegister[index] = value;
            if (!from_gpu)
                m_f18a_gpu.Load((m_VdpRegister[54] << 8) | value, true);
            break;
        case 56:
            if (!from_gpu)
            {
                if (IsSetBit(value, 0))
                    m_f18a_gpu.Trigger();
                else
                    m_f18a_gpu.Load((m_VdpRegister[54] << 8) | m_VdpRegister[55], false);
            }
            break;
        default:
            break;
    }

    m_iMode = ((m_VdpRegister[1] & 0x08) >> 1) | (m_VdpRegister[0] & 0x02) |
        ((m_VdpRegister[1] & 0x10) >> 4);

    if (mode_change)
    {
        m_f18a_mode_dirty = true;
        UpdateF18AMode();
    }

    if ((index == 0) || (index == 1))
        UpdateIRQLine();
}

u8 F18A::ReadF18ARegister(u8 index) const
{
    switch (index & 0x3F)
    {
        case 0: return m_VdpRegister[0] & 0x16;
        case 1: return m_VdpRegister[1] & 0x7B;
        case 2: return m_VdpRegister[2] & 0x0F;
        case 4: return m_VdpRegister[4] & 0x07;
        case 5: return m_VdpRegister[5] & 0x7F;
        case 6: return m_VdpRegister[6] & 0x07;
        case 10: return m_VdpRegister[10] & 0x0F;
        case 15: return m_VdpRegister[15] & 0x1F;
        case 24: return m_VdpRegister[24] & 0x3F;
        case 30: return m_VdpRegister[30] & 0x1F;
        case 49: return m_VdpRegister[49] & 0xFB;
        case 50: return m_VdpRegister[50] & 0x7F;
        case 51: return m_VdpRegister[51] & 0x3F;
        case 56:
        case 57:
            return 0;
        default:
            return m_VdpRegister[index & 0x3F];
    }
}

void F18A::AdvanceF18ADataPortAddress()
{
    m_VdpAddress = (m_VdpAddress + (s8)m_VdpRegister[48]) & 0x3FFF;
}

void F18A::UpdateIRQLine()
{
    bool irq = (IsSetBit(m_VdpStatus, 7) && IsSetBit(m_VdpRegister[1], 5)) ||
        (m_f18a_line_interrupt_pending && IsSetBit(m_VdpRegister[0], 4));

    if (irq && !m_f18a_irq_line)
    {
        if (IsValidPointer(m_pProcessor))
            m_pProcessor->RequestNMI();
        TraceVDPEvent(TRACE_VDP_NMI_REQUEST, 0);
    }

    m_f18a_irq_line = irq;
}

u8 F18A::GetF18AStatusRegister(int index) const
{
    switch (index & 0x0F)
    {
        case 0: return m_VdpStatus;
        case 1:
        {
            bool blank = (m_iRenderLine >= m_screen_height) || (m_iCycleCounter < m_Timing[TIMING_DISPLAY]);

            return 0xE0 | (blank ? 0x02 : 0) | (m_f18a_line_interrupt_pending ? 0x01 : 0);
        }
        case 2: return (m_f18a_gpu.IsRunning() ? 0x80 : 0) | m_f18a_gpu.GetUserStatus();
        case 3: return m_iRenderLine < m_screen_height ? (u8)m_iRenderLine : 0;
        case 4: return (u8)m_f18a_counter_snapshot_nano;
        case 5: return (m_f18a_counter_snapshot_nano >> 8) & 0x03;
        case 6: return (u8)m_f18a_counter_snapshot_micro;
        case 7: return (m_f18a_counter_snapshot_micro >> 8) & 0x03;
        case 8: return (u8)m_f18a_counter_snapshot_milli;
        case 9: return (m_f18a_counter_snapshot_milli >> 8) & 0x03;
        case 10: return (u8)m_f18a_counter_snapshot_seconds;
        case 11: return (u8)(m_f18a_counter_snapshot_seconds >> 8);
        case 14: return 0x19;
        case 15: return m_f18a_register_read;
        default: return 0;
    }
}

void F18A::UpdateF18AMode()
{
    if (!m_f18a_mode_dirty)
        return;

    int mode = ((m_VdpRegister[0] & 0x04) << 1) | ((m_VdpRegister[0] & 0x02) << 1) |
        ((m_VdpRegister[1] & 0x08) >> 2) | ((m_VdpRegister[1] & 0x10) >> 4);

    m_f18a_mode.legacy_mode = mode;
    m_f18a_mode.row30 = IsSetBit(m_VdpRegister[49], 6);
    m_f18a_mode.height = m_f18a_mode.row30 ? 240 : 192;
    m_f18a_mode.width = mode == 1 ? 240 : (mode == 9 ? 480 : (mode > 9 ? 512 : 256));
    m_f18a_mode.text_mode = (mode == 1) || (mode == 9);
    m_f18a_mode.layer_1_enabled = !IsSetBit(m_VdpRegister[50], 4);
    m_f18a_mode.layer_2_enabled = m_f18a_unlocked && IsSetBit(m_VdpRegister[49], 7);
    m_f18a_mode.bitmap_enabled = m_f18a_unlocked && IsSetBit(m_VdpRegister[31], 7);
    m_f18a_mode.sprites_enabled = !m_f18a_mode.text_mode || m_f18a_unlocked;
    m_f18a_mode.tile_ecm = m_f18a_unlocked ? ((m_VdpRegister[49] >> 4) & 3) : 0;
    m_f18a_mode.sprite_ecm = m_f18a_unlocked ? (m_VdpRegister[49] & 3) : 0;
    m_pending_screen_width = m_f18a_mode.width;
    m_pending_screen_height = m_f18a_mode.height;
    m_f18a_mode_dirty = false;
}

bool F18A::UseF18ARenderer()
{
    UpdateF18AMode();
    return m_f18a_unlocked || (m_f18a_mode.legacy_mode == 9) || (m_f18a_mode.legacy_mode > 9);
}

u8 F18A::GetF18APatternPixel(u16 address, int x, u8 ecm, int offset) const
{
    int bit = 7 - (x & 7);
    u8 pixel = IsSetBit(ReadVRAM(address), bit) ? 1 : 0;
    if (ecm >= 2)
        pixel |= IsSetBit(ReadVRAM(address + offset), bit) ? 2 : 0;
    if (ecm >= 3)
        pixel |= IsSetBit(ReadVRAM(address + (offset << 1)), bit) ? 4 : 0;
    return pixel;
}

void F18A::RenderF18ATileLayer(int line, bool layer2)
{
    const F18AMode& mode = m_f18a_mode;
    u8 ntba = layer2 ? m_VdpRegister[10] : m_VdpRegister[2];
    u8 ctba = layer2 ? m_VdpRegister[11] : m_VdpRegister[3];
    u8 hscroll = layer2 ? m_VdpRegister[25] : m_VdpRegister[27];
    u8 vscroll = layer2 ? m_VdpRegister[26] : m_VdpRegister[28];
    bool hsize = IsSetBit(m_VdpRegister[29], layer2 ? 5 : 1);
    bool vsize = IsSetBit(m_VdpRegister[29], layer2 ? 4 : 0);
    int palette_select = layer2 ? ((m_VdpRegister[24] >> 2) & 3) : (m_VdpRegister[24] & 3);
    int rows = mode.row30 ? 30 : 24;
    int columns = mode.legacy_mode == 9 ? 80 : (mode.legacy_mode == 1 ? 40 : 32);
    int tile_width = mode.text_mode ? 6 : 8;
    int pattern_offset_table[4] = { 2048, 1024, 512, 256 };
    int pattern_offset = pattern_offset_table[(m_VdpRegister[29] >> 2) & 3];

    for (int x = 0; x < mode.width; x++)
    {
        int source_x;
        int tile_column;
        int tile_row;
        int pixel_x;
        int pixel_y;
        int name_address;
        int position_address;

        if (mode.text_mode)
        {
            int scroll = hscroll * (mode.legacy_mode == 9 ? 2 : 1);
            source_x = (x + scroll) % (columns * tile_width);
            int source_y = line + vscroll;
            tile_column = source_x / tile_width;
            tile_row = (source_y >> 3) & 63;
            if (tile_row >= rows)
                tile_row = (tile_row - rows) & 31;
            pixel_x = source_x % tile_width;
            pixel_y = source_y & 7;
            if ((mode.legacy_mode == 9) && !m_f18a_unlocked)
                ntba &= 0x0C;
            name_address = (ntba << 10) + (tile_row * columns) + tile_column;
            position_address = (ctba << 6) + (tile_row * columns) + tile_column;
        }
        else
        {
            source_x = x + hscroll;
            int source_y = line + vscroll;
            int horizontal_page = ntba & 1;
            int vertical_page = (ntba >> 1) & 1;
            int horizontal_tile = source_x >> 3;
            int vertical_tile = source_y >> 3;

            if (hsize)
            {
                horizontal_tile = ((horizontal_page * 32) + horizontal_tile) & 63;
                horizontal_page = horizontal_tile >> 5;
            }
            else
            {
                horizontal_tile &= 31;
            }

            vertical_tile &= 63;
            if (vertical_tile >= rows)
            {
                vertical_tile = (vertical_tile - rows) & 31;
                if (vsize)
                    vertical_page ^= 1;
            }

            tile_column = horizontal_tile & 31;
            tile_row = vertical_tile;
            pixel_x = source_x & 7;
            pixel_y = source_y & 7;
            name_address = ((ntba & 0x0C) << 10) | (vertical_page << 11) |
                (horizontal_page << 10) | (tile_row << 5) | tile_column;
            position_address = (ctba << 6) + ((vertical_page << 11) |
                (horizontal_page << 10) | (tile_row << 5) | tile_column);
        }

        u8 name = ReadVRAM((u16)name_address);
        int attribute_address = IsSetBit(m_VdpRegister[50], 1) ? position_address :
            ((ctba << 6) + name);
        u8 attribute = ReadVRAM((u16)attribute_address);
        bool flip_x = (mode.tile_ecm != 0) && IsSetBit(attribute, 6);
        bool flip_y = (mode.tile_ecm != 0) && IsSetBit(attribute, 5);
        int pattern_x = flip_x ? (7 - pixel_x) : pixel_x;
        int pattern_y = flip_y ? (7 - pixel_y) : pixel_y;
        int pattern_address;

        if (mode.legacy_mode == 4)
        {
            int region = (tile_row & 0x18) << 5;
            int mask = ((m_VdpRegister[4] & 3) << 8) | 0xFF;
            pattern_address = (m_VdpRegister[4] & 4) << 11;
            pattern_address += (((name + region) & mask) << 3) + pattern_y;
        }
        else if (mode.legacy_mode == 2)
        {
            pattern_address = (m_VdpRegister[4] << 11) + (name << 3) +
                (((line + vscroll) & 0x1C) >> 2);
        }
        else
        {
            pattern_address = (m_VdpRegister[4] << 11) + (name << 3) + pattern_y;
        }

        u8 pixel_value = GetF18APatternPixel((u16)pattern_address, pattern_x, mode.tile_ecm, pattern_offset);
        bool opaque = false;
        u8 color = 0;

        if (mode.tile_ecm == 0)
        {
            u8 color_byte;
            if (mode.text_mode)
            {
                color_byte = IsSetBit(m_VdpRegister[50], 1) ? attribute : m_VdpRegister[7];
            }
            else if (mode.legacy_mode == 2)
            {
                u8 pattern = ReadVRAM((u16)pattern_address);
                color_byte = pattern;
                pixel_value = pixel_x < 4 ? 1 : 0;
            }
            else if (mode.legacy_mode == 4)
            {
                int region = (tile_row & 0x18) << 5;
                int mask = ((m_VdpRegister[3] & 0x7F) << 3) | 7;
                int color_address = ((m_VdpRegister[3] & 0x80) << 6) + ((((name + region) & mask) << 3) + pattern_y);
                color_byte = ReadVRAM((u16)color_address);
            }
            else
            {
                color_byte = ReadVRAM((u16)((ctba << 6) + (name >> 3)));
            }

            int color_index = pixel_value ? (color_byte >> 4) : (color_byte & 0x0F);
            opaque = color_index != 0;
            color = (palette_select << 4) | color_index;
        }
        else if (mode.tile_ecm == 1)
        {
            opaque = !IsSetBit(attribute, 4) || (pixel_value != 0);
            color = ((palette_select >> 1) << 5) | ((attribute & 0x0F) << 1) | pixel_value;
        }
        else if (mode.tile_ecm == 2)
        {
            opaque = !IsSetBit(attribute, 4) || (pixel_value != 0);
            color = ((attribute & 0x0F) << 2) | pixel_value;
        }
        else
        {
            opaque = !IsSetBit(attribute, 4) || (pixel_value != 0);
            color = ((attribute & 0x0E) << 2) | pixel_value;
        }

        if (opaque)
        {
            bool priority = (mode.tile_ecm != 0) && IsSetBit(attribute, 7);
            if (layer2 && !IsSetBit(m_VdpRegister[50], 0))
                priority = true;
            m_f18a_tile_line[x].color = color;
            m_f18a_tile_line[x].flags = F18A_PIXEL_OPAQUE | (priority ? F18A_PIXEL_PRIORITY : 0);
        }
    }
}

void F18A::RenderF18ABitmapLine(int line)
{
    if (!m_f18a_mode.bitmap_enabled)
        return;

    int local_y = line - m_VdpRegister[34];
    int height = m_VdpRegister[36];
    if ((local_y < 0) || (local_y >= height))
        return;

    int width = m_VdpRegister[35] == 0 ? 256 : m_VdpRegister[35];
    int stride = (width + 3) >> 2;
    int base = m_VdpRegister[32] << 6;
    bool fat = IsSetBit(m_VdpRegister[31], 4);
    bool transparent = IsSetBit(m_VdpRegister[31], 5);
    bool priority = IsSetBit(m_VdpRegister[31], 6);

    for (int x = 0; x < m_f18a_mode.width; x++)
    {
        int grid_x = m_f18a_mode.width > 256 ? (x >> 1) : x;
        int local_x = grid_x - m_VdpRegister[33];
        if ((local_x < 0) || (local_x >= width))
            continue;

        u8 data = ReadVRAM((u16)(base + (local_y * stride) + (local_x >> 2)));
        int pixel;
        int palette;
        if (fat)
        {
            pixel = (data >> (((local_x >> 1) & 1) ? 0 : 4)) & 0x0F;
            palette = ((m_VdpRegister[31] & 0x0C) << 2) | pixel;
        }
        else
        {
            pixel = (data >> (6 - ((local_x & 3) << 1))) & 3;
            palette = ((m_VdpRegister[31] & 0x0F) << 2) | pixel;
        }

        if (transparent && (pixel == 0))
            continue;
        if (priority || ((m_f18a_tile_line[x].flags & F18A_PIXEL_OPAQUE) == 0))
        {
            m_f18a_tile_line[x].color = palette & 0x3F;
            m_f18a_tile_line[x].flags = F18A_PIXEL_OPAQUE;
        }
    }
}

void F18A::RenderF18ASprites(int line)
{
    for (int x = 0; x < m_f18a_mode.width; x++)
    {
        m_f18a_sprite_line[x].color = 0;
        m_f18a_sprite_line[x].flags = 0;
    }
    for (int x = 0; x < 256; x++)
        m_f18a_sprite_occupancy[x] = 0;

    if (!m_f18a_mode.sprites_enabled)
        return;

    int sat = (m_VdpRegister[5] & 0x7F) << 7;
    int pattern_base = (m_VdpRegister[6] & 7) << 11;
    int pattern_offset_table[4] = { 2048, 1024, 512, 256 };
    int pattern_offset = pattern_offset_table[(m_VdpRegister[29] >> 6) & 3];
    int active_count = 0;
    int sprite_max = m_VdpRegister[30] & 0x1F;
    int stop_sprite = m_VdpRegister[51] & 0x3F;

    for (int sprite = 0; sprite < 32; sprite++)
    {
        if ((stop_sprite < 32) && (sprite == stop_sprite))
            break;

        int offset = sat + (sprite << 2);
        u8 raw_y = ReadVRAM((u16)offset);
        if (!m_f18a_mode.row30 && (raw_y == 0xD0))
            break;

        u8 raw_x = ReadVRAM((u16)(offset + 1));
        u8 name = ReadVRAM((u16)(offset + 2));
        u8 tag = ReadVRAM((u16)(offset + 3));
        bool sprite_size_16 = IsSetBit(m_VdpRegister[1], 1);
        if (m_f18a_unlocked && IsSetBit(tag, 4))
            sprite_size_16 = true;
        int source_size = sprite_size_16 ? 16 : 8;
        int magnification = IsSetBit(m_VdpRegister[1], 0) ? 2 : 1;
        int display_size = source_size * magnification;
        int top = IsSetBit(m_VdpRegister[49], 3) ? raw_y : ((raw_y + 1) & 0xFF);
        int delta_y = ((u8)line - (u8)top) & 0xFF;
        if (delta_y >= display_size)
            continue;

        if ((sprite_max != 31) && (active_count == sprite_max))
        {
            if (!IsSetBit(m_VdpStatus, 7) && !IsSetBit(m_VdpStatus, 6))
            {
                m_VdpStatus = (m_VdpStatus & 0xA0) | 0x40 | sprite;
                TraceVDPEvent(TRACE_VDP_SPRITE_OVERFLOW, 0xFF, 0, sprite);
            }
            break;
        }

        active_count++;
        if ((active_count == 5) && !IsSetBit(m_VdpRegister[50], 3) &&
            !IsSetBit(m_VdpStatus, 7) && !IsSetBit(m_VdpStatus, 6))
        {
            m_VdpStatus = (m_VdpStatus & 0xA0) | 0x40 | sprite;
            TraceVDPEvent(TRACE_VDP_SPRITE_OVERFLOW, 0xFF, 0, sprite);
        }

        int pattern_y = delta_y / magnification;
        if (m_f18a_unlocked && IsSetBit(tag, 5))
            pattern_y = source_size - 1 - pattern_y;
        int sprite_x = raw_x - (IsSetBit(tag, 7) ? 32 : 0);

        for (int display_x = 0; display_x < display_size; display_x++)
        {
            int grid_x = sprite_x + display_x;
            if ((grid_x < 0) || (grid_x >= 256))
                continue;

            int pattern_x = display_x / magnification;
            if (m_f18a_unlocked && IsSetBit(tag, 6))
                pattern_x = source_size - 1 - pattern_x;

            u8 pattern = name;
            int address;
            if (source_size == 16)
            {
                pattern &= 0xFC;
                address = pattern_base + (pattern << 3) + pattern_y;
                if (pattern_x >= 8)
                    address += 16;
            }
            else
            {
                address = pattern_base + (pattern << 3) + pattern_y;
            }

            u8 pixel = GetF18APatternPixel((u16)address, pattern_x & 7,
                m_f18a_mode.sprite_ecm, pattern_offset);
            if (pixel == 0)
                continue;

            if (m_f18a_sprite_occupancy[grid_x] != 0)
            {
                if (!IsSetBit(m_VdpStatus, 5))
                {
                    m_VdpStatus = SetBit(m_VdpStatus, 5);
                    TraceVDPEvent(TRACE_VDP_SPRITE_COLLISION, 0xFF, 0, sprite, grid_x);
                }
            }
            else
            {
                m_f18a_sprite_occupancy[grid_x] = 1;
            }

            bool opaque = (m_f18a_mode.sprite_ecm != 0) || ((tag & 0x0F) != 0);
            if (!opaque)
                continue;

            int color;
            if (m_f18a_mode.sprite_ecm == 0)
                color = (((m_VdpRegister[24] >> 4) & 3) << 4) | (tag & 0x0F);
            else if (m_f18a_mode.sprite_ecm == 1)
                color = (((m_VdpRegister[24] >> 5) & 1) << 5) | ((tag & 0x0F) << 1) | pixel;
            else if (m_f18a_mode.sprite_ecm == 2)
                color = ((tag & 0x0F) << 2) | pixel;
            else
                color = ((tag & 0x0E) << 2) | pixel;

            int scale = m_f18a_mode.width > 256 ? 2 : 1;
            int origin = m_f18a_mode.text_mode ? 8 : 0;
            int output_x = (grid_x - origin) * scale;
            for (int duplicate = 0; duplicate < scale; duplicate++)
            {
                int final_x = output_x + duplicate;
                if ((final_x >= 0) && (final_x < m_f18a_mode.width) &&
                    ((m_f18a_sprite_line[final_x].flags & F18A_PIXEL_OPAQUE) == 0))
                {
                    m_f18a_sprite_line[final_x].color = color & 0x3F;
                    m_f18a_sprite_line[final_x].flags = F18A_PIXEL_OPAQUE;
                }
            }
        }
    }
}

void F18A::ComposeF18AScanline(int line)
{
    int line_offset = line * m_screen_width;

    for (int x = 0; x < m_screen_width; x++)
    {
        F18ALinePixel pixel = m_f18a_tile_line[x];
        bool tile_priority = (pixel.flags & (F18A_PIXEL_OPAQUE | F18A_PIXEL_PRIORITY)) ==
            (F18A_PIXEL_OPAQUE | F18A_PIXEL_PRIORITY);
        if (((m_f18a_sprite_line[x].flags & F18A_PIXEL_OPAQUE) != 0) && !tile_priority)
            pixel = m_f18a_sprite_line[x];
        m_pFrameBuffer[line_offset + x] = pixel.color;
        m_pInfoBuffer[line_offset + x] = pixel.flags;
    }
}

void F18A::RenderF18AScanline(int line)
{
    UpdateF18AMode();

    int backdrop = ((m_VdpRegister[24] & 3) << 4) | (m_VdpRegister[7] & 0x0F);

    for (int x = 0; x < m_screen_width; x++)
    {
        m_f18a_tile_line[x].color = backdrop;
        m_f18a_tile_line[x].flags = 0;
    }

    if (m_bDisplayEnabled)
    {
        if (m_f18a_mode.layer_1_enabled)
            RenderF18ATileLayer(line, false);

        RenderF18ABitmapLine(line);

        if (m_f18a_mode.layer_2_enabled)
            RenderF18ATileLayer(line, true);

        RenderF18ASprites(line);
    }
    else
    {
        for (int x = 0; x < m_screen_width; x++)
        {
            m_f18a_sprite_line[x].color = 0;
            m_f18a_sprite_line[x].flags = 0;
        }
    }

    ComposeF18AScanline(line);
}

#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER) || defined(GEARCOLECO_ENABLE_VIDEO_DEBUG_TESTS)
bool F18A::GetDebugTileInfo(int x, int y, bool layer2, F18ADebugTileInfo& info)
{
    UpdateF18AMode();

    if ((x < 0) || (x >= m_f18a_mode.width) || (y < 0) || (y >= m_f18a_mode.height))
        return false;

    const F18AMode& mode = m_f18a_mode;
    u8 ntba = layer2 ? m_VdpRegister[10] : m_VdpRegister[2];
    u8 ctba = layer2 ? m_VdpRegister[11] : m_VdpRegister[3];
    u8 hscroll = layer2 ? m_VdpRegister[25] : m_VdpRegister[27];
    u8 vscroll = layer2 ? m_VdpRegister[26] : m_VdpRegister[28];
    bool hsize = IsSetBit(m_VdpRegister[29], layer2 ? 5 : 1);
    bool vsize = IsSetBit(m_VdpRegister[29], layer2 ? 4 : 0);
    int palette_select = layer2 ? ((m_VdpRegister[24] >> 2) & 3) :
        (m_VdpRegister[24] & 3);
    int rows = mode.row30 ? 30 : 24;
    int columns = mode.legacy_mode == 9 ? 80 : (mode.legacy_mode == 1 ? 40 : 32);
    int tile_width = mode.text_mode ? 6 : 8;
    int source_x;
    int tile_column;
    int tile_row;
    int pixel_y;
    int name_address;
    int position_address;

    if (mode.text_mode)
    {
        int scroll = hscroll * (mode.legacy_mode == 9 ? 2 : 1);
        source_x = (x + scroll) % (columns * tile_width);
        int source_y = y + vscroll;
        tile_column = source_x / tile_width;
        tile_row = (source_y >> 3) & 63;
        if (tile_row >= rows)
            tile_row = (tile_row - rows) & 31;
        pixel_y = source_y & 7;
        if ((mode.legacy_mode == 9) && !m_f18a_unlocked)
            ntba &= 0x0C;
        name_address = (ntba << 10) + (tile_row * columns) + tile_column;
        position_address = (ctba << 6) + (tile_row * columns) + tile_column;
    }
    else
    {
        source_x = x + hscroll;
        int source_y = y + vscroll;
        int horizontal_page = ntba & 1;
        int vertical_page = (ntba >> 1) & 1;
        int horizontal_tile = source_x >> 3;
        int vertical_tile = source_y >> 3;

        if (hsize)
        {
            horizontal_tile = ((horizontal_page * 32) + horizontal_tile) & 63;
            horizontal_page = horizontal_tile >> 5;
        }
        else
        {
            horizontal_tile &= 31;
        }

        vertical_tile &= 63;
        if (vertical_tile >= rows)
        {
            vertical_tile = (vertical_tile - rows) & 31;
            if (vsize)
                vertical_page ^= 1;
        }

        tile_column = horizontal_tile & 31;
        tile_row = vertical_tile;
        pixel_y = source_y & 7;
        name_address = ((ntba & 0x0C) << 10) | (vertical_page << 11) |
            (horizontal_page << 10) | (tile_row << 5) | tile_column;
        position_address = (ctba << 6) + ((vertical_page << 11) |
            (horizontal_page << 10) | (tile_row << 5) | tile_column);
    }

    u8 name = ReadVRAM((u16)name_address);
    int attribute_address = IsSetBit(m_VdpRegister[50], 1) ? position_address :
        ((ctba << 6) + name);
    u8 attribute = ReadVRAM((u16)attribute_address);
    bool flip_x = (mode.tile_ecm != 0) && IsSetBit(attribute, 6);
    bool flip_y = (mode.tile_ecm != 0) && IsSetBit(attribute, 5);
    int pattern_y = flip_y ? (7 - pixel_y) : pixel_y;
    int pattern_address;

    if (mode.legacy_mode == 4)
    {
        int region = (tile_row & 0x18) << 5;
        int mask = ((m_VdpRegister[4] & 3) << 8) | 0xFF;
        pattern_address = (m_VdpRegister[4] & 4) << 11;
        pattern_address += (((name + region) & mask) << 3) + pattern_y;
    }
    else if (mode.legacy_mode == 2)
    {
        pattern_address = (m_VdpRegister[4] << 11) + (name << 3) +
            (((y + vscroll) & 0x1C) >> 2);
    }
    else
    {
        pattern_address = (m_VdpRegister[4] << 11) + (name << 3) + pattern_y;
    }

    bool priority = (mode.tile_ecm != 0) && IsSetBit(attribute, 7);
    if (layer2 && !IsSetBit(m_VdpRegister[50], 0))
        priority = true;

    info.column = tile_column;
    info.row = tile_row;
    info.name_address = name_address & 0x3FFF;
    info.attribute_address = attribute_address & 0x3FFF;
    info.pattern_address = pattern_address & 0x3FFF;
    info.name = name;
    info.attribute = attribute;
    info.palette_select = palette_select;
    info.flip_x = flip_x;
    info.flip_y = flip_y;
    info.priority = priority;
    return true;
}

void F18A::RenderDebugNameTable(u16* buffer, bool layer2)
{
    UpdateF18AMode();

    u16 backdrop = m_VdpRegister[7] & 0x3F;
    for (int i = 0; i < (GC_VIDEO_MAX_WIDTH * GC_VIDEO_MAX_HEIGHT); i++)
        buffer[i] = backdrop;

    for (int line = 0; line < m_f18a_mode.height; line++)
    {
        for (int x = 0; x < m_f18a_mode.width; x++)
        {
            m_f18a_tile_line[x].color = 0;
            m_f18a_tile_line[x].flags = 0;
        }

        RenderF18ATileLayer(line, layer2);

        int line_offset = line * GC_VIDEO_MAX_WIDTH;
        for (int x = 0; x < m_f18a_mode.width; x++)
        {
            if ((m_f18a_tile_line[x].flags & F18A_PIXEL_OPAQUE) != 0)
                buffer[line_offset + x] = m_f18a_tile_line[x].color;
        }
    }

    for (int x = 0; x < m_f18a_mode.width; x++)
    {
        m_f18a_tile_line[x].color = 0;
        m_f18a_tile_line[x].flags = 0;
    }
}

void F18A::RenderDebugPatternTable(u16* buffer, int palette)
{
    UpdateF18AMode();

    for (int i = 0; i < (256 * 256); i++)
        buffer[i] = 0;

    int pattern_offset_table[4] = { 2048, 1024, 512, 256 };
    int pattern_offset = pattern_offset_table[(m_VdpRegister[29] >> 2) & 3];
    int pattern_base = m_f18a_mode.legacy_mode == 4 ?
        ((m_VdpRegister[4] & 4) << 11) : (m_VdpRegister[4] << 11);
    int pattern_count = m_f18a_mode.legacy_mode == 4 ? 768 : 256;
    int ecm = m_f18a_mode.tile_ecm;

    int max_palette = ecm == 1 ? 31 : (ecm == 2 ? 15 : 7);
    if (palette < 0)
        palette = 0;
    if (palette > max_palette)
        palette = max_palette;

    for (int pattern = 0; pattern < pattern_count; pattern++)
    {
        int tile_x = pattern & 31;
        int tile_y = pattern >> 5;

        for (int y = 0; y < 8; y++)
        {
            u16 address = (u16)(pattern_base + (pattern << 3) + y);
            u8 legacy_color = 0xF0;

            if (ecm == 0)
            {
                if (m_f18a_mode.text_mode)
                    legacy_color = m_VdpRegister[7];
                else if (m_f18a_mode.legacy_mode == 4)
                    legacy_color = ReadVRAM((u16)(((m_VdpRegister[3] & 0x80) << 6) +
                        (pattern << 3) + y));
                else
                    legacy_color = ReadVRAM((u16)((m_VdpRegister[3] << 6) + (pattern >> 3)));
            }

            for (int x = 0; x < 8; x++)
            {
                u8 pixel = GetF18APatternPixel(address, x, (u8)ecm, pattern_offset);
                int color = ecm == 0 ? (pixel ? (legacy_color >> 4) : (legacy_color & 0x0F)) :
                    ((palette << ecm) | pixel);
                int output = ((tile_y * 8 + y) * 256) + (tile_x * 8) + x;
                buffer[output] = color & 0x3F;
            }
        }
    }
}

int F18A::RenderDebugSprite(u16* buffer, int index)
{
    for (int i = 0; i < (16 * 16); i++)
        buffer[i] = 0;

    if ((index < 0) || (index >= 32))
        return 0;

    UpdateF18AMode();

    int sat = (m_VdpRegister[5] & 0x7F) << 7;
    int offset = sat + (index << 2);
    u8 name = ReadVRAM((u16)(offset + 2));
    u8 tag = ReadVRAM((u16)(offset + 3));
    bool size16 = IsSetBit(m_VdpRegister[1], 1) ||
        (m_f18a_unlocked && IsSetBit(tag, 4));
    int size = size16 ? 16 : 8;
    int pattern_base = (m_VdpRegister[6] & 7) << 11;
    int pattern_offset_table[4] = { 2048, 1024, 512, 256 };
    int pattern_offset = pattern_offset_table[(m_VdpRegister[29] >> 6) & 3];

    for (int y = 0; y < size; y++)
    {
        int source_y = m_f18a_unlocked && IsSetBit(tag, 5) ? (size - 1 - y) : y;

        for (int x = 0; x < size; x++)
        {
            int source_x = m_f18a_unlocked && IsSetBit(tag, 6) ? (size - 1 - x) : x;
            u8 pattern = size16 ? (name & 0xFC) : name;
            int address = pattern_base + (pattern << 3) + source_y;
            if (size16 && (source_x >= 8))
                address += 16;

            u8 pixel = GetF18APatternPixel((u16)address, source_x & 7,
                m_f18a_mode.sprite_ecm, pattern_offset);
            if (pixel == 0)
                continue;

            int color;
            if (m_f18a_mode.sprite_ecm == 0)
                color = (((m_VdpRegister[24] >> 4) & 3) << 4) | (tag & 0x0F);
            else if (m_f18a_mode.sprite_ecm == 1)
                color = (((m_VdpRegister[24] >> 5) & 1) << 5) | ((tag & 0x0F) << 1) | pixel;
            else if (m_f18a_mode.sprite_ecm == 2)
                color = ((tag & 0x0F) << 2) | pixel;
            else
                color = ((tag & 0x0E) << 2) | pixel;

            buffer[(y << 4) + x] = color & 0x3F;
        }
    }

    return size;
}
#endif

void F18A::RunF18ACounter(unsigned int gpu_cycles)
{
    if (!IsSetBit(m_VdpRegister[15], 4))
        return;

    u64 nanoseconds = m_f18a_counter_nano + ((u64)gpu_cycles * 10);
    u64 microseconds = m_f18a_counter_micro + (nanoseconds / 1000);
    u64 milliseconds = m_f18a_counter_milli + (microseconds / 1000);
    u64 seconds = m_f18a_counter_seconds + (milliseconds / 1000);
    m_f18a_counter_nano = nanoseconds % 1000;
    m_f18a_counter_micro = microseconds % 1000;
    m_f18a_counter_milli = milliseconds % 1000;
    m_f18a_counter_seconds = seconds & 0xFFFF;
}

void F18A::RunF18AGPU(unsigned int clockCycles)
{
    u64 master_clock = m_bPAL ? GC_MASTER_CLOCK_PAL : GC_MASTER_CLOCK_NTSC;
    m_f18a_gpu_clock_accumulator += (u64)clockCycles * 100000000ULL;
    unsigned int gpu_cycles = (unsigned int)(m_f18a_gpu_clock_accumulator / master_clock);
    m_f18a_gpu_clock_accumulator %= master_clock;
    RunF18ACounter(gpu_cycles);
    m_f18a_gpu.RunCycles((int)gpu_cycles, this);
}

u8 F18A::ReadF18AGPUByte(u16 address)
{
    switch (address >> 12)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            return ReadVRAM(address);
        case 4:
            return m_f18a_gpu_ram[address & 0x07FF];
        case 5:
        {
            u16 color = m_f18a_palette[(address >> 1) & 0x3F];
            return (address & 1) ? (u8)color : (u8)(color >> 8);
        }
        case 6:
            return ReadF18ARegister(address & 0x3F);
        case 7:
            return (address & 1) ? (GetF18AStatusRegister(1) & 0x02 ? 1 : 0) :
                (m_iRenderLine < m_screen_height ? (u8)m_iRenderLine : 0);
        case 8:
            return m_f18a_gpu.ReadDMARegister(address & 0x0F);
        case 10:
            return 0x19;
        default:
            return 0;
    }
}

void F18A::WriteF18AGPUByte(u16 address, u8 value)
{
    switch (address >> 12)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            m_pVdpVRAM[address & 0x3FFF] = value;
            break;
        case 4:
            m_f18a_gpu_ram[address & 0x07FF] = value;
            break;
        case 6:
            WriteVDPRegister(address & 0x3F, value, true);
            break;
        case 8:
            m_f18a_gpu.WriteDMARegister(address & 0x0F, value, this);
            break;
        case 11:
            m_f18a_gpu.SetUserStatus(value);
            break;
    }
}

u16 F18A::ReadF18AGPUWord(u16 address)
{
    address &= 0xFFFE;
    return ((u16)ReadF18AGPUByte(address) << 8) | ReadF18AGPUByte(address + 1);
}

void F18A::WriteF18AGPUWord(u16 address, u16 value)
{
    address &= 0xFFFE;
    if ((address >> 12) == 5)
    {
        int index = (address >> 1) & 0x3F;
        m_f18a_palette[index] = value & 0x0FFF;
        UpdateF18APalettePixel(index);
        return;
    }

    WriteF18AGPUByte(address, (u8)(value >> 8));
    WriteF18AGPUByte(address + 1, (u8)value);
}
