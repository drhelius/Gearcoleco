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

#ifndef KEY_MAPPER_H
#define KEY_MAPPER_H

#include <string>
#include "definitions.h"
#include "KeyMap.h"
#include "OsdCanvas.h"

#define GC_KEY_MAPPER_CHIPS 16
#define GC_KEY_MAPPER_BUTTON_LEFT 0x01
#define GC_KEY_MAPPER_BUTTON_RIGHT 0x02

class KeyMapper
{
public:
    struct Chip
    {
        int x;
        int y;
        int width;
        u16 usage;
    };

public:
    KeyMapper();
    void Init(KeyMap* pKeyMap, GC_Color_Format format);
    void SetTitle(const char* title);
    void Open();
    void Close();
    bool IsOpen() const;
    void SetHotkeyNames(const char* mapper, const char* mapping);
    void HostKey(u16 usage, bool pressed);
    void ReleaseHostKeys();
    void SetPointer(int x, int y);
    void MovePointer(int dx, int dy);
    void SetPointerButtons(u8 buttons);
    void Render(u8* pFrameBuffer);
    int GetWidth() const;
    int GetHeight() const;

private:
    void Click(int x, int y);
    void RightClick(int x, int y);
    void ClearKey(GC_AdamKey key);
    void BindHostKey(u16 usage);
    int FindLayoutKey(int x, int y) const;
    void GetKeyRect(int index, int* x, int* y, int* width, int* height) const;
    int GetKeyboardBottom() const;
    void UpdateKeyFlags();
    void DrawTitle();
    void DrawKeyboard();
    void DrawKeyLabel(int x, int y, int width, const char* label, u8 color, bool small);
    void DrawLegend();
    void DrawInfo();
    void DrawLine(int x, int y, const std::string& text, u8 color);
    void DrawChips(int y);
    void DrawButtons();
    void DrawButton(int x, int y, int width, const char* text);
    void GetResetRect(int* x, int* y, int* width, int* height) const;
    void GetCloseRect(int* x, int* y, int* width, int* height) const;
    bool IsPointerIn(int x, int y, int width, int height) const;
    std::string DescribeHostKey(u16 usage) const;
    std::string DescribeOwnHostKeys(int key) const;
    std::string DescribeTargets(const GC_KeyMapEntry& entry) const;

private:
    KeyMap* m_pKeyMap;
    OsdCanvas m_Canvas;
    bool m_bOpen;
    std::string m_Title;
    std::string m_MapperHotkey;
    std::string m_MappingHotkey;
    int m_PointerX;
    int m_PointerY;
    u8 m_Buttons;
    int m_Selected;
    bool m_Held[GC_HOST_KEY_COUNT];
    int m_HeldCount;
    u16 m_TapUsage;
    bool m_bTapValid;
    u16 m_QueryUsage;
    bool m_bResetArmed;
    std::string m_Message;
    u8 m_KeyFlags[GC_ADAM_KEY_COUNT];
    Chip m_Chips[GC_KEY_MAPPER_CHIPS];
    int m_ChipCount;
};

#endif /* KEY_MAPPER_H */
