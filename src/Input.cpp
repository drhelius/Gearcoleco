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
#include "Processor.h"

Input::Input(Processor* pProcessor)
{
    m_pProcessor = pProcessor;
}

void Input::Init()
{
    Reset();
}

void Input::Reset()
{
    m_Segment = SegmentKeypadRightButtons;
    m_Gamepad[0] = m_Gamepad[1] = 0xFF;
    m_Keypad[0] = m_Keypad[1] = 0xFF;
    m_iSpinnerRel[0] = m_iSpinnerRel[1] = 0;
}

void Input::SetInputSegment(InputSegments segment)
{
    m_Segment = segment;
}

u8 Input::ReadInput(u8 port)
{
    u8 c = (port & 0x02) >> 1;
    u8 ret = 0xFF;

    int rel = m_iSpinnerRel[c] / 4;
    m_iSpinnerRel[c] -= rel;

    if (m_Segment == SegmentKeypadRightButtons)
    {
        ret = (m_Keypad[c] & 0x0F) | (IsSetBit(m_Gamepad[c], 5) ? 0x70 : 0x30);
    }
    else
    {
        ret = (m_Gamepad[c] & 0x0F) | (IsSetBit(m_Gamepad[c], 4) ? 0x70 : 0x30);

        if (rel > 0)
        {
            ret &= c ? 0xEF : 0xCF;
            m_pProcessor->RequestINT(true);
        }
        else if (rel < 0)
        {
            ret &= c ? 0xCF : 0xEF;
            m_pProcessor->RequestINT(true);
        }
    }

    return ret;
}

void Input::KeyPressed(GC_Controllers controller, GC_Keys key)
{
    if (key > 0x0F)
    {
        m_Gamepad[controller] = UnsetBit(m_Gamepad[controller], key & 0x0F);
    }
    else
    {
        m_Keypad[controller] &= (key & 0x0F);
    }
}

void Input::KeyReleased(GC_Controllers controller, GC_Keys key)
{
    if (key > 0x0F)
    {
        m_Gamepad[controller] = SetBit(m_Gamepad[controller], key & 0x0F);
    }
    else
    {
        m_Keypad[controller] |= ~(key & 0x0F);
    }
}

void Input::Spinner1(int movement)
{
    m_iSpinnerRel[0] = movement;
}

void Input::Spinner2(int movement)
{
    m_iSpinnerRel[1] = movement;
}

void Input::SaveState(std::ostream& stream)
{
    stream.write(reinterpret_cast<const char*> (m_Gamepad), sizeof(m_Gamepad));
    stream.write(reinterpret_cast<const char*> (m_Keypad), sizeof(m_Keypad));
    stream.write(reinterpret_cast<const char*> (&m_Segment), sizeof(m_Segment));
    stream.write(reinterpret_cast<const char*> (&m_iSpinnerRel), sizeof(m_iSpinnerRel));
}

void Input::LoadState(std::istream& stream)
{
    stream.read(reinterpret_cast<char*> (m_Gamepad), sizeof(m_Gamepad));
    stream.read(reinterpret_cast<char*> (m_Keypad), sizeof(m_Keypad));
    stream.read(reinterpret_cast<char*> (&m_Segment), sizeof(m_Segment));
    stream.read(reinterpret_cast<char*> (&m_iSpinnerRel), sizeof(m_iSpinnerRel));
}
