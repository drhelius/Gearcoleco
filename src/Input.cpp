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
    m_Joypad1 = 0;
    m_Joypad2 = 0;
    m_iInputCycles = 0;
}

void Input::Init()
{
    Reset();
}

void Input::Reset()
{
    m_Joypad1 = 0xFF;
    m_Joypad2 = 0xFF;
    m_iInputCycles = 0;
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

void Input::KeyPressed(GC_Joypads joypad, GC_Keys key)
{
    if (joypad == Joypad_1)
    {
        m_Joypad1 = UnsetBit(m_Joypad1, key);
    }
    else
        m_Joypad2 = UnsetBit(m_Joypad2, key);
}

void Input::KeyReleased(GC_Joypads joypad, GC_Keys key)
{
    if (joypad == Joypad_1)
        m_Joypad1 = SetBit(m_Joypad1, key);
    else
        m_Joypad2 = SetBit(m_Joypad2, key);
}

void Input::Update()
{
    // TODO
}

void Input::SaveState(std::ostream& stream)
{
    stream.write(reinterpret_cast<const char*> (&m_Joypad1), sizeof(m_Joypad1));
    stream.write(reinterpret_cast<const char*> (&m_Joypad2), sizeof(m_Joypad2));
    stream.write(reinterpret_cast<const char*> (&m_iInputCycles), sizeof(m_iInputCycles));
}

void Input::LoadState(std::istream& stream)
{
    stream.read(reinterpret_cast<char*> (&m_Joypad1), sizeof(m_Joypad1));
    stream.read(reinterpret_cast<char*> (&m_Joypad2), sizeof(m_Joypad2));
    stream.read(reinterpret_cast<char*> (&m_iInputCycles), sizeof(m_iInputCycles));
}
