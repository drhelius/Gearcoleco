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

#include "KeyMapper.h"
#include "AdamKeyMapperLayout.h"
#include "HostKeys.h"

static const u8 kColorBackground = 0;
static const u8 kColorTitle = 1;
static const u8 kColorKey = 2;
static const u8 kColorBorder = 3;
static const u8 kColorText = 4;
static const u8 kColorDim = 5;
static const u8 kColorRemapped = 6;
static const u8 kColorUnreachable = 7;
static const u8 kColorSelected = 8;
static const u8 kColorPressed = 9;
static const u8 kColorHover = 10;
static const u8 kColorDark = 11;
static const u8 kColorButton = 12;
static const u8 kColorWarning = 13;
static const u8 kColorGame = 14;
static const u8 kColorBlack = 15;
static const u8 kColorGlobal = 16;

static const u8 kKeyFlagReachable = 0x01;
static const u8 kKeyFlagRemapped = 0x02;
static const u8 kKeyFlagPressed = 0x04;

static const int kMargin = 8;
static const int kTitleHeight = 20;
static const int kLineHeight = 12;
static const int kChipHeight = 12;
static const int kChipRowHeight = 14;
static const int kChipCloseWidth = 12;
static const int kButtonWidth = 88;
static const int kButtonHeight = 18;

static bool IsInside(int px, int py, int x, int y, int width, int height)
{
    return (px >= x) && (py >= y) && (px < x + width) && (py < y + height);
}

static bool Drives(const GC_KeyMapEntry& entry, int key)
{
    for (int i = 0; i < entry.count; i++)
    {
        if (entry.keys[i] == (u8)key)
            return true;
    }

    return false;
}

KeyMapper::KeyMapper()
{
    InitPointer(m_pKeyMap);
    m_bOpen = false;
    m_MapperHotkey = GC_HOST_HOTKEY_LABEL("F1");
    m_MappingHotkey = GC_HOST_HOTKEY_LABEL("F2");
    m_PointerX = kKeyMapperWidth / 2;
    m_PointerY = kKeyMapperHeight / 2;
    m_Buttons = 0;
    m_Selected = -1;
    memset(m_Held, 0, sizeof(m_Held));
    m_HeldCount = 0;
    m_TapUsage = 0;
    m_bTapValid = false;
    m_QueryUsage = 0;
    m_bResetArmed = false;
    memset(m_KeyFlags, 0, sizeof(m_KeyFlags));
    memset(m_Chips, 0, sizeof(m_Chips));
    m_ChipCount = 0;
}

void KeyMapper::Init(KeyMap* pKeyMap, GC_Color_Format format)
{
    m_pKeyMap = pKeyMap;
    m_Canvas.Init(kKeyMapperWidth, kKeyMapperHeight, format);
    m_Canvas.SetColor(kColorBackground, 22, 26, 38);
    m_Canvas.SetColor(kColorTitle, 10, 12, 20);
    m_Canvas.SetColor(kColorKey, 84, 90, 108);
    m_Canvas.SetColor(kColorBorder, 8, 8, 12);
    m_Canvas.SetColor(kColorText, 236, 236, 240);
    m_Canvas.SetColor(kColorDim, 140, 146, 162);
    m_Canvas.SetColor(kColorRemapped, 40, 92, 196);
    m_Canvas.SetColor(kColorUnreachable, 178, 44, 44);
    m_Canvas.SetColor(kColorSelected, 228, 196, 40);
    m_Canvas.SetColor(kColorPressed, 52, 168, 76);
    m_Canvas.SetColor(kColorHover, 255, 255, 255);
    m_Canvas.SetColor(kColorDark, 16, 16, 16);
    m_Canvas.SetColor(kColorButton, 62, 68, 88);
    m_Canvas.SetColor(kColorWarning, 250, 196, 72);
    m_Canvas.SetColor(kColorGame, 30, 132, 140);
    m_Canvas.SetColor(kColorBlack, 0, 0, 0);
    m_Canvas.SetColor(kColorGlobal, 116, 72, 168);
}

void KeyMapper::SetTitle(const char* title)
{
    m_Title = IsValidPointer(title) ? title : "";

    if (m_bOpen && !HasGame())
        m_pKeyMap->SetEditLayer(GC_KEY_MAP_GLOBAL);
}

void KeyMapper::Open()
{
    if (m_bOpen)
        return;

    m_bOpen = true;
    m_Buttons = 0xFF;
    m_Selected = -1;
    memset(m_Held, 0, sizeof(m_Held));
    m_HeldCount = 0;
    m_bTapValid = false;
    m_QueryUsage = 0;
    m_bResetArmed = false;
    m_Message.clear();
    m_ChipCount = 0;
    m_pKeyMap->SetEditLayer(HasGame() ? GC_KEY_MAP_USER : GC_KEY_MAP_GLOBAL);
    m_pKeyMap->SetBlocked(true);
}

void KeyMapper::Close()
{
    if (!m_bOpen)
        return;

    m_bOpen = false;
    m_pKeyMap->SetEditLayer(GC_KEY_MAP_USER);
    m_pKeyMap->SetBlocked(false);
}

bool KeyMapper::IsOpen() const
{
    return m_bOpen;
}

void KeyMapper::SetHotkeyNames(const char* mapper, const char* mapping)
{
    m_MapperHotkey = IsValidPointer(mapper) ? mapper : "";
    m_MappingHotkey = IsValidPointer(mapping) ? mapping : "";
}

void KeyMapper::HostKey(u16 usage, bool pressed)
{
    if (!m_bOpen || !KeyMap::IsHostKey(usage))
        return;

    if (pressed)
    {
        if (m_Held[usage])
            return;

        m_Held[usage] = true;
        m_bTapValid = m_HeldCount == 0;
        m_TapUsage = usage;
        m_HeldCount++;
        if (m_Selected < 0)
        {
            m_Message = DescribeHostKey(usage) + " \x1A " + DescribeTargets(m_pKeyMap->GetEntry(usage));
            m_QueryUsage = usage;
        }

        return;
    }

    if (!m_Held[usage])
        return;

    m_Held[usage] = false;
    m_HeldCount--;
    bool tap = m_bTapValid && (m_TapUsage == usage) && (m_HeldCount == 0);
    m_bTapValid = false;

    if (usage == m_QueryUsage)
    {
        m_Message.clear();
        m_QueryUsage = 0;
    }

    if (tap && (m_Selected >= 0))
        BindHostKey(usage);
}

void KeyMapper::ReleaseHostKeys()
{
    memset(m_Held, 0, sizeof(m_Held));
    m_HeldCount = 0;
    m_bTapValid = false;
}

void KeyMapper::SetPointer(int x, int y)
{
    m_PointerX = CLAMP(x, 0, kKeyMapperWidth - 1);
    m_PointerY = CLAMP(y, 0, kKeyMapperHeight - 1);
}

void KeyMapper::MovePointer(int dx, int dy)
{
    SetPointer(m_PointerX + dx, m_PointerY + dy);
}

void KeyMapper::SetPointerButtons(u8 buttons)
{
    u8 pressed = buttons & (u8)~m_Buttons;
    m_Buttons = buttons;

    if (!m_bOpen)
        return;

    if ((pressed & GC_KEY_MAPPER_BUTTON_LEFT) != 0)
        Click(m_PointerX, m_PointerY);
    else if ((pressed & GC_KEY_MAPPER_BUTTON_RIGHT) != 0)
        RightClick(m_PointerX, m_PointerY);
}

void KeyMapper::Render(u8* pFrameBuffer)
{
    UpdateKeyFlags();
    m_Canvas.Clear(kColorBackground);
    DrawTitle();
    DrawKeyboard();
    DrawLegend();
    DrawInfo();
    DrawButtons();
    m_Canvas.DrawCursor(m_PointerX, m_PointerY, kColorHover, kColorBlack);

    if (IsValidPointer(pFrameBuffer))
        m_Canvas.Present(pFrameBuffer);
}

int KeyMapper::GetWidth() const
{
    return kKeyMapperWidth;
}

int KeyMapper::GetHeight() const
{
    return kKeyMapperHeight;
}

void KeyMapper::Click(int x, int y)
{
    int bx, by, bw, bh;
    GetResetRect(&bx, &by, &bw, &bh);
    m_QueryUsage = 0;
    m_bTapValid = false;

    if (IsInside(x, y, bx, by, bw, bh))
    {
        if (m_bResetArmed)
        {
            GC_KeyMapLayer layer = m_pKeyMap->GetEditLayer();
            m_pKeyMap->ClearLayer(layer);
            m_pKeyMap->SetChanged(layer, true);
            m_Message = layer == GC_KEY_MAP_GLOBAL ? "Reset the keys of all games" : "Reset to defaults";
        }

        m_bResetArmed = !m_bResetArmed;
        return;
    }

    m_bResetArmed = false;

    for (int i = 0; i < 2; i++)
    {
        GetLayerRect(i, &bx, &by, &bw, &bh);

        if (IsInside(x, y, bx, by, bw, bh))
        {
            SetEditLayer(i == 0 ? GC_KEY_MAP_USER : GC_KEY_MAP_GLOBAL);
            return;
        }
    }

    GetCloseRect(&bx, &by, &bw, &bh);

    if (IsInside(x, y, bx, by, bw, bh))
    {
        Close();
        return;
    }

    for (int i = 0; i < m_ChipCount; i++)
    {
        const Chip& chip = m_Chips[i];

        if (!IsInside(x, y, chip.x + chip.width - kChipCloseWidth, chip.y, kChipCloseWidth, kChipHeight))
            continue;

        if (m_pKeyMap->Unbind(chip.usage, (GC_AdamKey)m_Selected))
            m_Message = "Removed " + DescribeHostKey(chip.usage);

        return;
    }

    int index = FindLayoutKey(x, y);

    if (index >= 0)
    {
        int key = kKeyMapperLayout[index].key;
        m_Selected = key == m_Selected ? -1 : key;
        m_Message.clear();
    }
}

void KeyMapper::RightClick(int x, int y)
{
    m_bResetArmed = false;
    m_QueryUsage = 0;
    int index = FindLayoutKey(x, y);

    if (index < 0)
        return;

    GC_AdamKey key = kKeyMapperLayout[index].key;
    ClearKey(key);

    if (key == m_Selected)
        m_Selected = -1;
}

void KeyMapper::ClearKey(GC_AdamKey key)
{
    bool cleared = false;

    for (int usage = 0; usage < GC_HOST_KEY_COUNT; usage++)
    {
        GC_KeyMapLayer layer = GC_KEY_MAP_BUILT_IN;
        const GC_KeyMapEntry& entry = m_pKeyMap->GetEntry((u16)usage, &layer);

        if ((layer != GC_KEY_MAP_BUILT_IN) && Drives(entry, key))
            cleared = m_pKeyMap->RestoreBuiltIn((u16)usage) || cleared;
    }

    m_Message = cleared ? std::string("Cleared ") + KeyMap::GetKeyLabel(key) : "Nothing to clear";
}

void KeyMapper::BindHostKey(u16 usage)
{
    GC_AdamKey key = (GC_AdamKey)m_Selected;

    if (!m_pKeyMap->Bind(usage, &key, 1))
    {
        m_Message = DescribeHostKey(usage) + " can't be bound";
        return;
    }

    m_Message = "Bound " + DescribeHostKey(usage) + " \x1A " + KeyMap::GetKeyLabel(key);
}

int KeyMapper::FindLayoutKey(int x, int y) const
{
    for (int i = 0; i < kKeyMapperKeyCount; i++)
    {
        int kx, ky, kw, kh;
        GetKeyRect(i, &kx, &ky, &kw, &kh);

        if (IsInside(x, y, kx, ky, kw, kh))
            return i;
    }

    return -1;
}

void KeyMapper::GetKeyRect(int index, int* x, int* y, int* width, int* height) const
{
    const GC_KeyMapperKey& key = kKeyMapperLayout[index];
    int x0 = kKeyMapperOriginX + ((key.x * kKeyMapperUnitX) / kKeyMapperGrid);
    int x1 = kKeyMapperOriginX + (((key.x + key.width) * kKeyMapperUnitX) / kKeyMapperGrid);
    int y0 = kKeyMapperOriginY + ((key.y * kKeyMapperUnitY) / kKeyMapperGrid);
    int y1 = kKeyMapperOriginY + (((key.y + key.height) * kKeyMapperUnitY) / kKeyMapperGrid);
    *x = x0;
    *y = y0;
    *width = x1 - x0 - 2;
    *height = y1 - y0 - 2;
}

int KeyMapper::GetKeyboardBottom() const
{
    int bottom = kKeyMapperOriginY;

    for (int i = 0; i < kKeyMapperKeyCount; i++)
    {
        int x, y, width, height;
        GetKeyRect(i, &x, &y, &width, &height);
        bottom = MAX(bottom, y + height);
    }

    return bottom;
}

void KeyMapper::UpdateKeyFlags()
{
    memset(m_KeyFlags, 0, sizeof(m_KeyFlags));

    for (int usage = 0; usage < GC_HOST_KEY_COUNT; usage++)
    {
        GC_KeyMapLayer layer = GC_KEY_MAP_BUILT_IN;
        const GC_KeyMapEntry& entry = m_pKeyMap->GetEntry((u16)usage, &layer);
        u8 flags = kKeyFlagReachable;

        if (layer != GC_KEY_MAP_BUILT_IN)
            flags |= kKeyFlagRemapped;

        if (m_Held[usage] && (m_Selected < 0))
            flags |= kKeyFlagPressed;

        for (int i = 0; i < entry.count; i++)
            m_KeyFlags[entry.keys[i]] |= flags;
    }
}

void KeyMapper::DrawTitle()
{
    std::string hint = m_MapperHotkey.empty() ? "" : m_MapperHotkey + " closes";
    int hint_width = OsdCanvas::GetTextWidth(hint.c_str());
    std::string title = m_pKeyMap->GetEditLayer() == GC_KEY_MAP_GLOBAL ? std::string("All games") : m_Title;
    size_t fit = (size_t)MAX(0, (kKeyMapperWidth - (kMargin * 2) - hint_width - 16) / 8);

    if (title.size() > fit)
        title = title.substr(0, fit);

    m_Canvas.FillRect(0, 0, kKeyMapperWidth, kTitleHeight, kColorTitle);
    m_Canvas.DrawText(kMargin, 2, title.c_str(), kColorText, true);
    m_Canvas.DrawText(kKeyMapperWidth - kMargin - hint_width, 2, hint.c_str(), kColorDim, true);
}

void KeyMapper::DrawKeyboard()
{
    for (int i = 0; i < kKeyMapperKeyCount; i++)
    {
        const GC_KeyMapperKey& key = kKeyMapperLayout[i];
        int x, y, width, height;
        GetKeyRect(i, &x, &y, &width, &height);

        bool selected = key.key == m_Selected;
        u8 flags = m_KeyFlags[key.key];
        u8 fill = kColorKey;
        u8 text = kColorText;
        u8 frame = kColorBorder;

        if ((flags & kKeyFlagPressed) != 0)
            fill = kColorPressed;
        else if (selected)
        {
            fill = kColorSelected;
            text = kColorDark;
        }
        else if ((flags & kKeyFlagReachable) == 0)
            fill = kColorUnreachable;
        else if ((flags & kKeyFlagRemapped) != 0)
            fill = kColorRemapped;

        if (IsPointerIn(x, y, width, height))
            frame = kColorHover;

        m_Canvas.FillRect(x, y, width, height, fill);
        m_Canvas.FrameRect(x, y, width, height, frame);

        std::string label = key.label;
        size_t split = label.find('\n');
        std::string first = label.substr(0, split);
        std::string second = split == std::string::npos ? "" : label.substr(split + 1);
        int lines = second.empty() ? 1 : 2;
        bool stacked = (first.length() == 1) && (second.length() == 1);
        bool small = (OsdCanvas::GetTextWidth(first.c_str()) > width - 4) ||
            (OsdCanvas::GetTextWidth(second.c_str()) > width - 4) ||
            (!stacked && (((lines * 8) + ((lines - 1) * 2)) > height - 4));
        int line_height = small ? 6 : 8;
        int top = y + ((height - ((lines * line_height) + ((lines - 1) * 2))) / 2);

        DrawKeyLabel(x, top, width, first.c_str(), text, small);

        if (lines == 2)
            DrawKeyLabel(x, top + line_height + 2, width, second.c_str(), text, small);
    }
}

void KeyMapper::DrawKeyLabel(int x, int y, int width, const char* label, u8 color, bool small)
{
    if (small)
        m_Canvas.DrawSmallText(x + ((width - OsdCanvas::GetSmallTextWidth(label)) / 2), y, label, color);
    else
        m_Canvas.DrawText(x + ((width - OsdCanvas::GetTextWidth(label) + 1) / 2), y, label, color, false);
}

void KeyMapper::DrawLegend()
{
    static const u8 kColors[5] = { kColorKey, kColorRemapped, kColorUnreachable, kColorSelected, kColorPressed };
    static const char* const kNames[5] = { "Built-in", "Remapped", "No host key", "Selected", "Pressed" };
    int x = kKeyMapperOriginX;
    int y = GetKeyboardBottom() + 6;

    for (int i = 0; i < 5; i++)
    {
        m_Canvas.FillRect(x, y, 8, 8, kColors[i]);
        m_Canvas.FrameRect(x, y, 8, 8, kColorBorder);
        m_Canvas.DrawText(x + 12, y, kNames[i], kColorDim, false);
        x += 12 + OsdCanvas::GetTextWidth(kNames[i]) + 20;
    }
}

void KeyMapper::DrawInfo()
{
    int y = GetKeyboardBottom() + 22;
    m_ChipCount = 0;

    if (m_Selected < 0)
        DrawLine(kMargin, y, "Select a key to show its mapping", kColorText);
    else
    {
        const char* label = KeyMap::GetKeyLabel((GC_AdamKey)m_Selected);
        DrawLine(kMargin, y, label, kColorText);
        DrawLine(kMargin + OsdCanvas::GetTextWidth(label) + 24, y, DescribeOwnHostKeys(m_Selected), kColorText);
        DrawChips(y + kLineHeight + 2);
    }

    y += kLineHeight + 2 + (kChipRowHeight * 2) + 8;
    DrawLine(kMargin, y, "Click a key, press a host key to bind it. Right click clears.", kColorDim);
    y += kLineHeight;
    DrawLine(kMargin, y, m_Message, kColorText);

    std::string warning;

    if (m_bResetArmed)
        warning = "Click Reset All again to confirm";
    else if (!m_pKeyMap->IsEnabled())
        warning = "Mapping off (" + m_MappingHotkey + "): using built-in and all games keys";

    int bx, by, bw, bh;
    GetResetRect(&bx, &by, &bw, &bh);
    DrawLine(kMargin, by - kLineHeight, warning, kColorWarning);
}

void KeyMapper::DrawLine(int x, int y, const std::string& text, u8 color)
{
    size_t fit = (size_t)MAX(0, (kKeyMapperWidth - x - kMargin) / 8);
    m_Canvas.DrawText(x, y, text.size() > fit ? text.substr(0, fit).c_str() : text.c_str(), color, false);
}

void KeyMapper::DrawChips(int y)
{
    static const char* const kHead = "Host keys:";
    static const char* const kTags[GC_KEY_MAP_LAYER_COUNT] = { " (built in)", " (all games)", " (game)", " (user)" };
    static const u8 kFills[GC_KEY_MAP_LAYER_COUNT] = { kColorKey, kColorGlobal, kColorGame, kColorRemapped };
    int start = kMargin + OsdCanvas::GetTextWidth(kHead) + 8;
    int x = start;
    int row = 0;
    bool any = false;

    m_Canvas.DrawText(kMargin, y, kHead, kColorDim, false);

    for (int l = GC_KEY_MAP_BUILT_IN; (l < GC_KEY_MAP_LAYER_COUNT) && (row < 2); l++)
    {
        for (int usage = 0; (usage < GC_HOST_KEY_COUNT) && (row < 2); usage++)
        {
            GC_KeyMapLayer layer = GC_KEY_MAP_BUILT_IN;
            const GC_KeyMapEntry& entry = m_pKeyMap->GetEntry((u16)usage, &layer);

            if ((layer != l) || !Drives(entry, m_Selected))
                continue;

            any = true;
            std::string text = DescribeHostKey((u16)usage) + kTags[l];
            int width = OsdCanvas::GetTextWidth(text.c_str()) + 8 + kChipCloseWidth;

            if ((x + width > kKeyMapperWidth - kMargin) && (x > start))
            {
                if (++row == 2)
                    break;

                x = start;
                y += kChipRowHeight;
            }

            int close_x = x + width - kChipCloseWidth;
            bool hover = IsPointerIn(close_x, y - 2, kChipCloseWidth, kChipHeight);

            m_Canvas.FillRect(x, y - 2, width, kChipHeight, kFills[l]);
            m_Canvas.FrameRect(x, y - 2, width, kChipHeight, kColorBorder);
            m_Canvas.DrawText(x + 4, y, text.c_str(), kColorText, false);
            m_Canvas.FrameRect(close_x, y - 2, kChipCloseWidth, kChipHeight, hover ? kColorHover : kColorBorder);
            m_Canvas.DrawText(close_x + 2, y, "x", hover ? kColorHover : kColorText, false);

            if (m_ChipCount < GC_KEY_MAPPER_CHIPS)
            {
                Chip& chip = m_Chips[m_ChipCount++];
                chip.x = x;
                chip.y = y - 2;
                chip.width = width;
                chip.usage = (u16)usage;
            }

            x += width + 6;
        }
    }

    if (!any)
        m_Canvas.DrawText(start, y, "none", kColorDim, false);
}

void KeyMapper::DrawButtons()
{
    int x, y, width, height;
    GetResetRect(&x, &y, &width, &height);
    DrawButton(x, y, width, "Reset All");
    GetCloseRect(&x, &y, &width, &height);
    DrawButton(x, y, width, "Close");

    bool global = m_pKeyMap->GetEditLayer() == GC_KEY_MAP_GLOBAL;
    GetLayerRect(0, &x, &y, &width, &height);
    DrawButton(x, y, width, "This Game", !global, HasGame());
    GetLayerRect(1, &x, &y, &width, &height);
    DrawButton(x, y, width, "All Games", global);
}

void KeyMapper::DrawButton(int x, int y, int width, const char* text, bool active, bool enabled)
{
    bool hover = enabled && IsPointerIn(x, y, width, kButtonHeight);
    m_Canvas.FillRect(x, y, width, kButtonHeight, active ? kColorSelected : kColorButton);
    m_Canvas.FrameRect(x, y, width, kButtonHeight, hover ? kColorHover : kColorBorder);
    m_Canvas.DrawText(x + ((width - OsdCanvas::GetTextWidth(text)) / 2), y + 5, text,
        !enabled ? kColorDim : active ? kColorDark : kColorText, false);
}

void KeyMapper::SetEditLayer(GC_KeyMapLayer layer)
{
    if ((layer == GC_KEY_MAP_USER) && !HasGame())
        return;

    m_pKeyMap->SetEditLayer(layer);
    m_Message = layer == GC_KEY_MAP_GLOBAL ? "Changes now apply to all games" : "Changes now apply to this game";
}

bool KeyMapper::HasGame() const
{
    return !m_Title.empty();
}

void KeyMapper::GetResetRect(int* x, int* y, int* width, int* height) const
{
    *x = kMargin;
    *y = kKeyMapperHeight - kButtonHeight - 6;
    *width = kButtonWidth;
    *height = kButtonHeight;
}

void KeyMapper::GetLayerRect(int index, int* x, int* y, int* width, int* height) const
{
    *x = (kKeyMapperWidth / 2) - kButtonWidth - 2 + (index * (kButtonWidth + 4));
    *y = kKeyMapperHeight - kButtonHeight - 6;
    *width = kButtonWidth;
    *height = kButtonHeight;
}

void KeyMapper::GetCloseRect(int* x, int* y, int* width, int* height) const
{
    *x = kKeyMapperWidth - kMargin - kButtonWidth;
    *y = kKeyMapperHeight - kButtonHeight - 6;
    *width = kButtonWidth;
    *height = kButtonHeight;
}

bool KeyMapper::IsPointerIn(int x, int y, int width, int height) const
{
    return IsInside(m_PointerX, m_PointerY, x, y, width, height);
}

std::string KeyMapper::DescribeHostKey(u16 usage) const
{
    const char* label = KeyMap::GetHostKeyLabel(usage);

    if (IsValidPointer(label))
        return label;

    char text[16];
    snprintf(text, sizeof(text), "Key %02Xh", usage);
    return text;
}

std::string KeyMapper::DescribeOwnHostKeys(int key) const
{
    std::string text;

    for (int usage = 0; usage < GC_HOST_KEY_COUNT; usage++)
    {
        GC_KeyMapLayer layer = GC_KEY_MAP_BUILT_IN;
        const GC_KeyMapEntry& entry = m_pKeyMap->GetEntry((u16)usage, &layer);

        if ((layer == GC_KEY_MAP_BUILT_IN) || !Drives(m_pKeyMap->GetLayerEntry(GC_KEY_MAP_BUILT_IN, (u16)usage), key))
            continue;

        if ((entry.count == 1) && (entry.keys[0] == (u8)key))
            continue;

        text += (text.empty() ? "Host " : ", ") + DescribeHostKey((u16)usage) + " \x1A " + DescribeTargets(entry);
    }

    return text;
}

std::string KeyMapper::DescribeTargets(const GC_KeyMapEntry& entry) const
{
    if (entry.count == 0)
        return "nothing";

    std::string text;

    for (int i = 0; i < entry.count; i++)
    {
        const char* label = KeyMap::GetKeyLabel((GC_AdamKey)entry.keys[i]);
        text += std::string(i > 0 ? " + " : "") + (IsValidPointer(label) ? label : "?");
    }

    return text;
}
