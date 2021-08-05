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

#include "Input.h"
#include "Memory.h"

Input::Input()
{
    Reset();
}

void Input::Init()
{
    Reset();
}

void Input::Reset()
{
    m_iInputCycles = 0;
    m_Segment = SegmentKeypadRightButtons;
    m_Gamepad[0] = m_Gamepad[1] = 0xFF;
    m_Keypad[0] = m_Keypad[1] = 0x0F;
    m_InputState[0][0] = m_InputState[0][1] = 0xFF;
    m_InputState[1][0] = m_InputState[1][1] = 0xFF;
    m_LatestKey = 0x0F;
}

void Input::Tick(unsigned int clockCycles)
{
    m_iInputCycles += clockCycles;

    // Joypad Poll Speed
    if (m_iInputCycles >= 10000)
    {
        m_iInputCycles -= 10000;
        Update();
    }
}

void Input::SetInputSegment(InputSegments segment)
{
    m_Segment = segment;
}

u8 Input::ReadInput(u8 port)
{
    u8 controller = (port & 0x02) >> 1;

    if (m_Segment == SegmentKeypadRightButtons)
    {
        return m_InputState[controller][0];
    }
    else
    {
        return m_InputState[controller][1];
    }
}

void Input::KeyPressed(GC_Controllers controller, GC_Keys key)
{
    if (key > 0x0F)
    {
        m_Gamepad[controller] = UnsetBit(m_Gamepad[controller], key & 0x0F);
    }
    else if (m_LatestKey == 0x0F)
    {
        m_LatestKey = m_Keypad[controller] = key & 0x0F;
    }
}

void Input::KeyReleased(GC_Controllers controller, GC_Keys key)
{
    if (key > 0x0F)
    {
        m_Gamepad[controller] = SetBit(m_Gamepad[controller], key & 0x0F);
    }
    else if (key == m_LatestKey)
    {
        m_LatestKey = m_Keypad[controller] = 0x0F;
    }
}

void Input::Update()
{
    for (int c = 0; c < 2; c++)
    {
        m_InputState[c][0] = (m_Keypad[c] & 0x0F) | (IsSetBit(m_Gamepad[c], 5) ? 0x70 : 0x30);
        m_InputState[c][1] = (m_Gamepad[c] & 0x0F) | (IsSetBit(m_Gamepad[c], 4) ? 0x70 : 0x30);
    }
}

void Input::SaveState(std::ostream& stream)
{
    stream.write(reinterpret_cast<const char*> (m_Gamepad), sizeof(m_Gamepad));
    stream.write(reinterpret_cast<const char*> (m_Keypad), sizeof(m_Keypad));
    stream.write(reinterpret_cast<const char*> (m_InputState), sizeof(m_InputState));
    stream.write(reinterpret_cast<const char*> (&m_LatestKey), sizeof(m_LatestKey));
    stream.write(reinterpret_cast<const char*> (&m_Segment), sizeof(m_Segment));
    stream.write(reinterpret_cast<const char*> (&m_iInputCycles), sizeof(m_iInputCycles));
}

void Input::LoadState(std::istream& stream)
{
    stream.read(reinterpret_cast<char*> (m_Gamepad), sizeof(m_Gamepad));
    stream.read(reinterpret_cast<char*> (m_Keypad), sizeof(m_Keypad));
    stream.read(reinterpret_cast<char*> (m_InputState), sizeof(m_InputState));
    stream.read(reinterpret_cast<char*> (&m_LatestKey), sizeof(m_LatestKey));
    stream.read(reinterpret_cast<char*> (&m_Segment), sizeof(m_Segment));
    stream.read(reinterpret_cast<char*> (&m_iInputCycles), sizeof(m_iInputCycles));
}
