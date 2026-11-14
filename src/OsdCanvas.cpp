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

#include "OsdCanvas.h"
#include "OsdFont.h"

static const char* const kOsdCursor[] =
{
    "X",
    "XX",
    "X.X",
    "X..X",
    "X...X",
    "X....X",
    "X.....X",
    "X......X",
    "X.......X",
    "X........X",
    "X.....XXXXX",
    "X..X..X",
    "X.X X..X",
    "XX  X..X",
    "X    X..X",
    "     X..X",
    "      XX"
};

static const int kOsdCursorRows = (int)(sizeof(kOsdCursor) / sizeof(kOsdCursor[0]));

OsdCanvas::OsdCanvas()
{
    InitPointer(m_pPixels);
    m_Width = 0;
    m_Height = 0;
    m_Format = GC_PIXEL_RGBA8888;
    memset(m_Palette, 0, sizeof(m_Palette));
}

OsdCanvas::~OsdCanvas()
{
    SafeDeleteArray(m_pPixels);
}

void OsdCanvas::Init(int width, int height, GC_Color_Format format)
{
    SafeDeleteArray(m_pPixels);
    m_Width = width;
    m_Height = height;
    m_Format = format;
    m_pPixels = new u8[width * height];
    memset(m_pPixels, 0, width * height);
}

int OsdCanvas::GetWidth() const
{
    return m_Width;
}

int OsdCanvas::GetHeight() const
{
    return m_Height;
}

void OsdCanvas::SetColor(u8 index, u8 red, u8 green, u8 blue)
{
    if (index >= OSD_CANVAS_COLORS)
        return;

    u8 red_5 = red * 31 / 255;
    u8 green_5 = green * 31 / 255;
    u8 green_6 = green * 63 / 255;
    u8 blue_5 = blue * 31 / 255;

    switch (m_Format)
    {
        case GC_PIXEL_RGB565:
            m_Palette[index] = (red_5 << 11) | (green_6 << 5) | blue_5;
            break;
        case GC_PIXEL_RGB555:
            m_Palette[index] = (red_5 << 10) | (green_5 << 5) | blue_5;
            break;
        case GC_PIXEL_BGR565:
            m_Palette[index] = (blue_5 << 11) | (green_6 << 5) | red_5;
            break;
        case GC_PIXEL_BGR555:
            m_Palette[index] = (blue_5 << 10) | (green_5 << 5) | red_5;
            break;
        case GC_PIXEL_BGRA8888:
        {
            u8 bytes[4] = { blue, green, red, 0xFF };
            memcpy(&m_Palette[index], bytes, sizeof(bytes));
            break;
        }
        default:
        {
            u8 bytes[4] = { red, green, blue, 0xFF };
            memcpy(&m_Palette[index], bytes, sizeof(bytes));
            break;
        }
    }
}

void OsdCanvas::Clear(u8 color)
{
    memset(m_pPixels, color, m_Width * m_Height);
}

void OsdCanvas::FillRect(int x, int y, int width, int height, u8 color)
{
    int x0 = MAX(x, 0);
    int y0 = MAX(y, 0);
    int x1 = MIN(x + width, m_Width);
    int y1 = MIN(y + height, m_Height);

    for (int row = y0; row < y1; row++)
    {
        if (x1 > x0)
            memset(m_pPixels + (row * m_Width) + x0, color, x1 - x0);
    }
}

void OsdCanvas::FrameRect(int x, int y, int width, int height, u8 color)
{
    FillRect(x, y, width, 1, color);
    FillRect(x, y + height - 1, width, 1, color);
    FillRect(x, y, 1, height, color);
    FillRect(x + width - 1, y, 1, height, color);
}

void OsdCanvas::DrawText(int x, int y, const char* text, u8 color, bool tall)
{
    for (int i = 0; IsValidPointer(text) && (text[i] != 0); i++)
    {
        u8 character = (u8)text[i];
        const u8* glyph = tall ? &kOsdFont16[character * 16] : &kOsdFont8[character * 8];
        DrawGlyph(x + (i * 8), y, glyph, tall ? 16 : 8, color);
    }
}

void OsdCanvas::DrawSmallText(int x, int y, const char* text, u8 color)
{
    for (int i = 0; IsValidPointer(text) && (text[i] != 0); i++)
    {
        u8 character = (u8)text[i];

        if (character < 128)
            DrawGlyph(x + (i * 5), y, &kOsdFontSmall[character * 7], 7, color);
    }
}

void OsdCanvas::DrawCursor(int x, int y, u8 fill, u8 outline)
{
    for (int row = 0; row < kOsdCursorRows; row++)
    {
        for (int column = 0; kOsdCursor[row][column] != 0; column++)
        {
            char c = kOsdCursor[row][column];

            if (c == 'X')
                Plot(x + column, y + row, outline);
            else if (c == '.')
                Plot(x + column, y + row, fill);
        }
    }
}

void OsdCanvas::Present(u8* frame_buffer) const
{
    int count = m_Width * m_Height;

    if (Is16Bit())
    {
        u16* destination = (u16*)frame_buffer;

        for (int i = 0; i < count; i++)
            destination[i] = (u16)m_Palette[m_pPixels[i] & (OSD_CANVAS_COLORS - 1)];
    }
    else
    {
        u32* destination = (u32*)frame_buffer;

        for (int i = 0; i < count; i++)
            destination[i] = m_Palette[m_pPixels[i] & (OSD_CANVAS_COLORS - 1)];
    }
}

int OsdCanvas::GetTextWidth(const char* text)
{
    return IsValidPointer(text) ? (int)strlen(text) * 8 : 0;
}

int OsdCanvas::GetTextHeight(bool tall)
{
    return tall ? 16 : 8;
}

int OsdCanvas::GetSmallTextWidth(const char* text)
{
    int length = IsValidPointer(text) ? (int)strlen(text) : 0;
    return length > 0 ? (length * 5) - 1 : 0;
}

void OsdCanvas::DrawGlyph(int x, int y, const u8* glyph, int rows, u8 color)
{
    for (int row = 0; row < rows; row++)
    {
        u8 bits = glyph[row];

        for (int column = 0; (column < 8) && (bits != 0); column++)
        {
            if ((bits & (0x80 >> column)) != 0)
                Plot(x + column, y + row, color);
        }
    }
}

void OsdCanvas::Plot(int x, int y, u8 color)
{
    if ((x >= 0) && (y >= 0) && (x < m_Width) && (y < m_Height))
        m_pPixels[(y * m_Width) + x] = color;
}

bool OsdCanvas::Is16Bit() const
{
    return (m_Format == GC_PIXEL_RGB565) || (m_Format == GC_PIXEL_RGB555) || (m_Format == GC_PIXEL_BGR565) ||
        (m_Format == GC_PIXEL_BGR555);
}
