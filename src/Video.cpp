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

#include "Video.h"
#include "Memory.h"
#include "Processor.h"

Video::Video(Memory* pMemory, Processor* pProcessor)
{
    m_pMemory = pMemory;
    m_pProcessor = pProcessor;
    InitPointer(m_pInfoBuffer);
    InitPointer(m_pFrameBuffer);
    InitPointer(m_pVdpVRAM);
    InitPointer(m_pVdpCRAM);
    m_bFirstByteInSequence = false;
    for (int i = 0; i < 16; i++)
        m_VdpRegister[i] = 0;
    m_VdpCode = 0;
    m_VdpBuffer = 0;
    m_VdpAddress = 0;
    m_iVCounter = 0;
    m_iHCounter = 0;
    m_iCycleCounter = 0;
    m_VdpStatus = 0;
    m_iVdpRegister10Counter = 0;
    m_ScrollX = 0;
    m_ScrollY = 0;
    m_iLinesPerFrame = 0;
    m_bPAL = false;
    m_LineEvents.hint = false;
    m_LineEvents.scrollx = false;
    m_LineEvents.vcounter = false;
    m_LineEvents.vint = false;
    m_LineEvents.vintFlag = false;
    m_iRenderLine = 0;
    m_iScreenWidth = 0;
    m_iSG1000Mode = 0;
    m_bDisplayEnabled = false;
    m_bSpriteOvrRequest = false;
}

Video::~Video()
{
    SafeDeleteArray(m_pInfoBuffer);
    SafeDeleteArray(m_pFrameBuffer);
    SafeDeleteArray(m_pVdpVRAM);
    SafeDeleteArray(m_pVdpCRAM);
}

void Video::Init()
{
    m_pInfoBuffer = new u8[GC_RESOLUTION_MAX_WIDTH * GC_LINES_PER_FRAME_PAL];
    m_pFrameBuffer = new u16[GC_RESOLUTION_MAX_WIDTH * GC_LINES_PER_FRAME_PAL];
    m_pVdpVRAM = new u8[0x4000];
    m_pVdpCRAM = new u8[0x40];
    InitPalettes();
    Reset(false);
}

void Video::Reset(bool bPAL)
{
    m_bPAL = bPAL;
    m_iLinesPerFrame = bPAL ? GC_LINES_PER_FRAME_PAL : GC_LINES_PER_FRAME_NTSC;
    m_bFirstByteInSequence = true;
    m_VdpBuffer = 0;
    m_iVCounter = m_iLinesPerFrame - 1;
    m_iHCounter = 0;
    m_VdpCode = 0;
    m_VdpBuffer = 0;
    m_VdpAddress = 0;
    m_VdpStatus = 0;
    m_ScrollX = 0;
    m_ScrollY = 0;
    for (int i = 0; i < (GC_RESOLUTION_MAX_WIDTH * GC_LINES_PER_FRAME_PAL); i++)
    {
        m_pFrameBuffer[i] = 0;
        m_pInfoBuffer[i] = 0;
    }
    for (int i = 0; i < 0x4000; i++)
        m_pVdpVRAM[i] = 0;
    for (int i = 0; i < 0x40; i++)
        m_pVdpCRAM[i] = 0;

    m_VdpRegister[0] = 0x36; // Mode
    m_VdpRegister[1] = 0x80; // Mode
    m_VdpRegister[2] = 0xFF; // Screen Map Table Base
    m_VdpRegister[3] = 0xFF; // Always $FF
    m_VdpRegister[4] = 0xFF; // Always $FF
    m_VdpRegister[5] = 0xFF; // Sprite Table Base
    m_VdpRegister[6] = 0xFB; // Sprite Pattern Table Base
    m_VdpRegister[7] = 0x00; // Border color #0
    m_VdpRegister[8] = 0x00; // Scroll-X
    m_VdpRegister[9] = 0x00; // Scroll-Y
    m_VdpRegister[10] = 0xFF; // H-line interrupt ($FF=OFF)

    m_bDisplayEnabled = false;
    m_bSpriteOvrRequest = false;

    for (int i = 11; i < 16; i++)
        m_VdpRegister[i] = 0;

    m_LineEvents.hint = false;
    m_LineEvents.scrollx = false;
    m_LineEvents.vcounter = false;
    m_LineEvents.vint = false;
    m_LineEvents.vintFlag = false;
    m_LineEvents.render = false;

    m_iCycleCounter = 0;
    m_iVdpRegister10Counter = m_VdpRegister[10];
    m_iRenderLine = 0;

    m_iScreenWidth = GC_RESOLUTION_MAX_WIDTH;

    m_iSG1000Mode = 0;

    m_Timing[TIMING_VINT] = 25;
    m_Timing[TIMING_XSCROLL] = 14;
    m_Timing[TIMING_HINT] = 27;
    m_Timing[TIMING_VCOUNT] = 25;
    m_Timing[TIMING_FLAG_VINT] = 25;
    m_Timing[TIMING_RENDER] = 195;
    m_Timing[TIMING_DISPLAY] = 37;
    m_Timing[TIMING_SPRITEOVR] = 25;

    for (int i = 0; i < 8; i++)
    {
        m_NextLineSprites[i] = -1;
    }
}

bool Video::Tick(unsigned int clockCycles)
{
    int max_height = GC_RESOLUTION_MAX_HEIGHT;
    bool return_vblank = false;

    m_iCycleCounter += clockCycles;

    ///// VINT /////
    if (!m_LineEvents.vint && (m_iCycleCounter >= m_Timing[TIMING_VINT]))
    {
        m_LineEvents.vint = true;
        if ((m_iRenderLine == (max_height + 1)) && (IsSetBit(m_VdpRegister[1], 5)))
            m_pProcessor->RequestINT(true);
    }

    ///// DISPLAY ON/OFF /////
    if (!m_LineEvents.display && (m_iCycleCounter >= m_Timing[TIMING_DISPLAY]))
    {
        m_LineEvents.display = true;
        m_bDisplayEnabled = IsSetBit(m_VdpRegister[1], 6);
    }

    ///// SCROLLX /////
    if (!m_LineEvents.scrollx && (m_iCycleCounter >= m_Timing[TIMING_XSCROLL]))
    {
        m_LineEvents.scrollx = true;
        m_ScrollX = m_VdpRegister[8];   // latch scroll X
    }

    ///// HINT /////
    if (!m_LineEvents.hint && (m_iCycleCounter >= m_Timing[TIMING_HINT]))
    {
        m_LineEvents.hint = true;
        if (m_iRenderLine <= max_height)
        {
            if (m_iVdpRegister10Counter == 0)
            {
                m_iVdpRegister10Counter = m_VdpRegister[10];
            }
            else
            {
                m_iVdpRegister10Counter--;
            }
        }
        else
            m_iVdpRegister10Counter = m_VdpRegister[10];
    }

    ///// VCOUNT /////
    if (!m_LineEvents.vcounter && (m_iCycleCounter >= m_Timing[TIMING_VCOUNT]))
    {
        m_LineEvents.vcounter = true;
        m_iVCounter++;
        if (m_iVCounter >= m_iLinesPerFrame)
        {
            m_ScrollY = m_VdpRegister[9];   // latch scroll Y
            m_iVCounter = 0;
        }
    }

    ///// FLAG VINT /////
    if (!m_LineEvents.vintFlag && (m_iCycleCounter >= m_Timing[TIMING_FLAG_VINT]))
    {
        m_LineEvents.vintFlag = true;
        if (m_iRenderLine == (max_height + 1))
        {
            bool oldFlag = IsSetBit(m_VdpStatus, 7);

            m_VdpStatus = SetBit(m_VdpStatus, 7);

            if (IsSetBit(m_VdpRegister[1], 5) && !oldFlag)
                m_pProcessor->RequestNMI();
        }
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
        if (m_iRenderLine == (max_height - 1))
        {
            return_vblank = true;
        }
        m_iRenderLine++;
        m_iRenderLine %= m_iLinesPerFrame;
        m_iCycleCounter -= GC_CYCLES_PER_LINE;
        m_LineEvents.hint = false;
        m_LineEvents.scrollx = false;
        m_LineEvents.vcounter = false;
        m_LineEvents.vint = false;
        m_LineEvents.vintFlag = false;
        m_LineEvents.render = false;
        m_LineEvents.display = false;
        m_LineEvents.spriteovr = false;
    }

    return return_vblank;
}

void Video::LatchHCounter()
{
    m_iHCounter = kVdpHCounter[m_iCycleCounter % 228];
}

u8 Video::GetVCounter()
{
    if (m_bPAL)
    {
        if (m_iVCounter > 0xF2)
            return m_iVCounter - 0x39;
        else
            return m_iVCounter;
    }
    else
    {
        if (m_iVCounter > 0xDA)
            return m_iVCounter - 0x06;
        else
            return m_iVCounter;
    }
}

u8 Video::GetHCounter()
{
    return m_iHCounter;
}

u8 Video::GetDataPort()
{
    m_bFirstByteInSequence = true;
    u8 ret = m_VdpBuffer;
    m_VdpBuffer = m_pVdpVRAM[m_VdpAddress];
    m_VdpAddress++;
    m_VdpAddress &= 0x3FFF;
    return ret;
}

u8 Video::GetStatusFlags()
{
    u8 ret = m_VdpStatus;
    m_bFirstByteInSequence = true;
    m_VdpStatus = 0x00;
    m_pProcessor->RequestINT(false);
    return ret;
}

void Video::WriteData(u8 data)
{
    m_bFirstByteInSequence = true;
    m_VdpBuffer = data;
    switch (m_VdpCode)
    {
        case VDP_READ_VRAM_OPERATION:
        case VDP_WRITE_VRAM_OPERATION:
        case VDP_WRITE_REG_OPERATION:
        {
            m_pVdpVRAM[m_VdpAddress] = data;
            break;
        }
        case VDP_WRITE_CRAM_OPERATION:
        {
            m_pVdpCRAM[m_VdpAddress & 0x1F] = data;
            break;
        }
    }
    m_VdpAddress++;
    m_VdpAddress &= 0x3FFF;
}

void Video::WriteControl(u8 control)
{
    if (m_bFirstByteInSequence)
    {
        m_bFirstByteInSequence = false;
        m_VdpAddress = (m_VdpAddress & 0xFF00) | control;
    }
    else
    {
        m_bFirstByteInSequence = true;
        m_VdpCode = (control >> 6) & 0x03;
        m_VdpAddress = (m_VdpAddress & 0x00FF) | ((control & 0x3F) << 8);

        if (m_VdpCode == VDP_WRITE_CRAM_OPERATION)
        {
            Log("--> ** Attempting to write on CRAM");
        }

        switch (m_VdpCode)
        {
            case VDP_READ_VRAM_OPERATION:
            {
                m_VdpBuffer = m_pVdpVRAM[m_VdpAddress];
                m_VdpAddress++;
                m_VdpAddress &= 0x3FFF;
                break;
            }
            case VDP_WRITE_REG_OPERATION:
            {
                u8 reg = control & 0x07;
                m_VdpRegister[reg] = (m_VdpAddress & 0x00FF);

                if (reg < 2)
                {
                    m_iSG1000Mode = ((m_VdpRegister[0] & 0x06) << 8) | (m_VdpRegister[1] & 0x18);
                }
                else if (reg > 10)
                {
                    Log("--> ** Attempting to write on VDP REG %d: %X", reg, control);
                }
                break;
            }
        }
    }
}

void Video::ScanLine(int line)
{
    int max_height = GC_RESOLUTION_MAX_HEIGHT;

    if (m_bDisplayEnabled)
    {
        if (line < max_height)
        {
            RenderBackground(line);
            RenderSprites(line);
        }
    }
    else
    {
        if (line < max_height)
        {
            int line_width = line * m_iScreenWidth;

            for (int scx = 0; scx < m_iScreenWidth; scx++)
            {
                int pixel = line_width + scx;
                m_pFrameBuffer[pixel] = 0;
                m_pInfoBuffer[pixel] = 0;
            }
        }
    }
}

void Video::RenderBackground(int line)
{
    int line_width = line * m_iScreenWidth;

    int name_table_addr = (m_VdpRegister[2] & 0x0F) << 10;
    int pattern_table_addr = 0;
    int color_table_addr = 0;

    if (m_iSG1000Mode == 0x200)
    {
        pattern_table_addr = (m_VdpRegister[4] & 0x04) << 11;
        color_table_addr = (m_VdpRegister[3] & 0x80) << 6;
    }
    else
    {
        pattern_table_addr = (m_VdpRegister[4] & 0x07) << 11;
        color_table_addr = m_VdpRegister[3] << 6;
    }

    int region = (m_VdpRegister[4] & 0x03) << 8;
    int backdrop_color = m_VdpRegister[7] & 0x0F;

    int tile_y = line >> 3;
    int tile_y_offset = line & 7;

    for (int scx = 0; scx < m_iScreenWidth; scx++)
    {
        int tile_x = scx >> 3;
        int tile_x_offset = scx & 7;

        int tile_number = ((tile_y << 5) + tile_x);

        int name_tile_addr = name_table_addr + tile_number;

        int name_tile = 0;

        if (m_iSG1000Mode == 0x200)
            name_tile = m_pVdpVRAM[name_tile_addr] | (region & 0x300 & tile_number);
        else
            name_tile = m_pVdpVRAM[name_tile_addr];

        u8 pattern_line = m_pVdpVRAM[pattern_table_addr + (name_tile << 3) + tile_y_offset];

        u8 color_line = 0;

        if (m_iSG1000Mode == 0x200)
            color_line = m_pVdpVRAM[color_table_addr + (name_tile << 3) + tile_y_offset];
        else
            color_line = m_pVdpVRAM[color_table_addr + (name_tile >> 3)];

        int bg_color = color_line & 0x0F;
        int fg_color = color_line >> 4;

        int pixel = line_width + scx;

        int final_color = IsSetBit(pattern_line, 7 - tile_x_offset) ? fg_color : bg_color;

        m_pFrameBuffer[pixel] = (final_color > 0) ? final_color : backdrop_color;
        m_pInfoBuffer[pixel] = 0x00;
    }
}

void Video::RenderSprites(int line)
{
    int sprite_collision = false;
    int sprite_count = 0;
    int line_width = line * m_iScreenWidth;
    int sprite_size = IsSetBit(m_VdpRegister[1], 1) ? 16 : 8;
    bool sprite_zoom = IsSetBit(m_VdpRegister[1], 0);
    if (sprite_zoom)
        sprite_size *= 2;
    u16 sprite_attribute_addr = (m_VdpRegister[5] & 0x7F) << 7;
    u16 sprite_pattern_addr = (m_VdpRegister[6] & 0x07) << 11;

    int max_sprite = 31;

    for (int sprite = 0; sprite <= max_sprite; sprite++)
    {
        if (m_pVdpVRAM[sprite_attribute_addr + (sprite << 2)] == 0xD0)
        {
            max_sprite = sprite - 1;
            break;
        }
    }

    for (int sprite = 0; sprite <= max_sprite; sprite++)
    {
        int sprite_attribute_offset = sprite_attribute_addr + (sprite << 2);
        int sprite_y = (m_pVdpVRAM[sprite_attribute_offset] + 1) & 0xFF;

        if (sprite_y >= 0xE0)
            sprite_y = -(0x100 - sprite_y);

        if ((sprite_y > line) || ((sprite_y + sprite_size) <= line))
            continue;

        sprite_count++;
        if (!SetBit(m_VdpStatus, 6) && (sprite_count > 4))
        {
            m_VdpStatus = SetBit(m_VdpStatus, 6);
            m_VdpStatus = (m_VdpStatus & 0xE0) | sprite;
        }

        int sprite_color = m_pVdpVRAM[sprite_attribute_offset + 3] & 0x0F;

        if (sprite_color == 0)
            continue;

        int sprite_shift = (m_pVdpVRAM[sprite_attribute_offset + 3] & 0x80) ? 32 : 0;
        int sprite_x = m_pVdpVRAM[sprite_attribute_offset + 1] - sprite_shift;

        if (sprite_x >= GC_RESOLUTION_MAX_WIDTH)
            continue;

        int sprite_tile = m_pVdpVRAM[sprite_attribute_offset + 2];
        sprite_tile &= IsSetBit(m_VdpRegister[1], 1) ? 0xFC : 0xFF;

        int sprite_line_addr = sprite_pattern_addr + (sprite_tile << 3) + ((line - sprite_y ) >> (sprite_zoom ? 1 : 0));

        for (int tile_x = 0; tile_x < sprite_size; tile_x++)
        {
            int sprite_pixel_x = sprite_x + tile_x;
            if (sprite_pixel_x >= m_iScreenWidth)
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

            if (sprite_pixel && (sprite_count < 5) && ((m_pInfoBuffer[pixel] & 0x08) == 0))
            {
                m_pFrameBuffer[pixel] = sprite_color;
                m_pInfoBuffer[pixel] |= 0x08;
            }

            if ((m_pInfoBuffer[pixel] & 0x04) != 0)
                sprite_collision = true;
            else
                m_pInfoBuffer[pixel] |= 0x04;
        }
    }

    if (sprite_collision)
        m_VdpStatus = SetBit(m_VdpStatus, 5);
}

void Video::Render24bit(u16* srcFrameBuffer, u8* dstFrameBuffer, GC_Color_Format pixelFormat, int size)
{
    bool bgr = (pixelFormat == GC_PIXEL_BGR888);

    for (int i = 0, j = 0; i < size; i ++, j += 3)
    {
        u16 src_color = srcFrameBuffer[i] * 3;

        if (bgr)
        {
            dstFrameBuffer[j + 2] = kSG1000_palette_888[src_color];
            dstFrameBuffer[j] = kSG1000_palette_888[src_color + 2];
        }
        else
        {
            dstFrameBuffer[j] = kSG1000_palette_888[src_color];
            dstFrameBuffer[j + 2] = kSG1000_palette_888[src_color + 2];
        }
        dstFrameBuffer[j + 1] = kSG1000_palette_888[src_color + 1];
    }
}

void Video::Render16bit(u16* srcFrameBuffer, u8* dstFrameBuffer, GC_Color_Format pixelFormat, int size)
{
    bool green_6bit = (pixelFormat == GC_PIXEL_RGB565) || (pixelFormat == GC_PIXEL_BGR565);
    bool bgr = ((pixelFormat == GC_PIXEL_BGR555) || (pixelFormat == GC_PIXEL_BGR565));

    const u16* pal;

    if (bgr)
        pal = green_6bit ? m_SG1000_palette_565_bgr : m_SG1000_palette_555_bgr;
    else
        pal = green_6bit ? m_SG1000_palette_565_rgb : m_SG1000_palette_555_rgb;

    for (int i = 0, j = 0; i < size; i ++, j += 2)
    {
        u16 src_color = srcFrameBuffer[i];

        *(u16*)(&dstFrameBuffer[j]) = pal[src_color];
    }
}

void Video::InitPalettes()
{
    for (int i=0,j=0; i<16; i++,j+=3)
    {
        u8 red = kSG1000_palette_888[j];
        u8 green = kSG1000_palette_888[j+1];
        u8 blue = kSG1000_palette_888[j+2];

        u8 red_5 = red * 31 / 255;
        u8 green_5 = green * 31 / 255;
        u8 green_6 = green * 63 / 255;
        u8 blue_5 = blue * 31 / 255;

        m_SG1000_palette_565_rgb[i] = red_5 << 11 | green_6 << 5 | blue_5;
        m_SG1000_palette_555_rgb[i] = red_5 << 10 | green_5 << 5 | blue_5;
        m_SG1000_palette_565_bgr[i] = blue_5 << 11 | green_6 << 5 | red_5;
        m_SG1000_palette_555_bgr[i] = blue_5 << 10 | green_5 << 5 | red_5;
    }
}

void Video::SaveState(std::ostream& stream)
{
    stream.write(reinterpret_cast<const char*> (m_pInfoBuffer), GC_RESOLUTION_MAX_WIDTH * GC_LINES_PER_FRAME_PAL);
    stream.write(reinterpret_cast<const char*> (m_pVdpVRAM), 0x4000);
    stream.write(reinterpret_cast<const char*> (m_pVdpCRAM), 0x40);
    stream.write(reinterpret_cast<const char*> (&m_bFirstByteInSequence), sizeof(m_bFirstByteInSequence));
    stream.write(reinterpret_cast<const char*> (m_VdpRegister), sizeof(m_VdpRegister));
    stream.write(reinterpret_cast<const char*> (&m_VdpCode), sizeof(m_VdpCode));
    stream.write(reinterpret_cast<const char*> (&m_VdpBuffer), sizeof(m_VdpBuffer));
    stream.write(reinterpret_cast<const char*> (&m_VdpAddress), sizeof(m_VdpAddress));
    stream.write(reinterpret_cast<const char*> (&m_iVCounter), sizeof(m_iVCounter));
    stream.write(reinterpret_cast<const char*> (&m_iHCounter), sizeof(m_iHCounter));
    stream.write(reinterpret_cast<const char*> (&m_iCycleCounter), sizeof(m_iCycleCounter));
    stream.write(reinterpret_cast<const char*> (&m_VdpStatus), sizeof(m_VdpStatus));
    stream.write(reinterpret_cast<const char*> (&m_iVdpRegister10Counter), sizeof(m_iVdpRegister10Counter));
    stream.write(reinterpret_cast<const char*> (&m_ScrollX), sizeof(m_ScrollX));
    stream.write(reinterpret_cast<const char*> (&m_ScrollY), sizeof(m_ScrollY));
    stream.write(reinterpret_cast<const char*> (&m_iLinesPerFrame), sizeof(m_iLinesPerFrame));
    bool bogus = false;
    stream.write(reinterpret_cast<const char*> (&bogus), sizeof(bogus));
    stream.write(reinterpret_cast<const char*> (&m_LineEvents), sizeof(m_LineEvents));
    stream.write(reinterpret_cast<const char*> (&m_iRenderLine), sizeof(m_iRenderLine));
    stream.write(reinterpret_cast<const char*> (&m_bPAL), sizeof(m_bPAL));
    stream.write(reinterpret_cast<const char*> (&m_iScreenWidth), sizeof(m_iScreenWidth));
    stream.write(reinterpret_cast<const char*> (&m_iSG1000Mode), sizeof(m_iSG1000Mode));
    stream.write(reinterpret_cast<const char*> (&m_Timing), sizeof(m_Timing));
    stream.write(reinterpret_cast<const char*> (&m_NextLineSprites), sizeof(m_NextLineSprites));
    stream.write(reinterpret_cast<const char*> (&m_bDisplayEnabled), sizeof(m_bDisplayEnabled));
    stream.write(reinterpret_cast<const char*> (&m_bSpriteOvrRequest), sizeof(m_bSpriteOvrRequest));
}

void Video::LoadState(std::istream& stream)
{
    stream.read(reinterpret_cast<char*> (m_pInfoBuffer), GC_RESOLUTION_MAX_WIDTH * GC_LINES_PER_FRAME_PAL);
    stream.read(reinterpret_cast<char*> (m_pVdpVRAM), 0x4000);
    stream.read(reinterpret_cast<char*> (m_pVdpCRAM), 0x40);
    stream.read(reinterpret_cast<char*> (&m_bFirstByteInSequence), sizeof(m_bFirstByteInSequence));
    stream.read(reinterpret_cast<char*> (m_VdpRegister), sizeof(m_VdpRegister));
    stream.read(reinterpret_cast<char*> (&m_VdpCode), sizeof(m_VdpCode));
    stream.read(reinterpret_cast<char*> (&m_VdpBuffer), sizeof(m_VdpBuffer));
    stream.read(reinterpret_cast<char*> (&m_VdpAddress), sizeof(m_VdpAddress));
    stream.read(reinterpret_cast<char*> (&m_iVCounter), sizeof(m_iVCounter));
    stream.read(reinterpret_cast<char*> (&m_iHCounter), sizeof(m_iHCounter));
    stream.read(reinterpret_cast<char*> (&m_iCycleCounter), sizeof(m_iCycleCounter));
    stream.read(reinterpret_cast<char*> (&m_VdpStatus), sizeof(m_VdpStatus));
    stream.read(reinterpret_cast<char*> (&m_iVdpRegister10Counter), sizeof(m_iVdpRegister10Counter));
    stream.read(reinterpret_cast<char*> (&m_ScrollX), sizeof(m_ScrollX));
    stream.read(reinterpret_cast<char*> (&m_ScrollY), sizeof(m_ScrollY));
    stream.read(reinterpret_cast<char*> (&m_iLinesPerFrame), sizeof(m_iLinesPerFrame));
    bool bogus;
    stream.read(reinterpret_cast<char*> (&bogus), sizeof(bogus));
    stream.read(reinterpret_cast<char*> (&m_LineEvents), sizeof(m_LineEvents));
    stream.read(reinterpret_cast<char*> (&m_iRenderLine), sizeof(m_iRenderLine));
    stream.read(reinterpret_cast<char*> (&m_bPAL), sizeof(m_bPAL));
    stream.read(reinterpret_cast<char*> (&m_iScreenWidth), sizeof(m_iScreenWidth));
    stream.read(reinterpret_cast<char*> (&m_iSG1000Mode), sizeof(m_iSG1000Mode));
    stream.read(reinterpret_cast<char*> (&m_Timing), sizeof(m_Timing));
    stream.read(reinterpret_cast<char*> (&m_NextLineSprites), sizeof(m_NextLineSprites));
    stream.read(reinterpret_cast<char*> (&m_bDisplayEnabled), sizeof(m_bDisplayEnabled));
    stream.read(reinterpret_cast<char*> (&m_bSpriteOvrRequest), sizeof(m_bSpriteOvrRequest));
}
