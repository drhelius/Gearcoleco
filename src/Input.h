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

#ifndef INPUT_H
#define	INPUT_H

#include "definitions.h"

class Memory;

class Input
{
public:
    Input();
    void Init();
    void Reset();
    void Tick(unsigned int clockCycles);
    void KeyPressed(GC_Joypads joypad, GC_Keys key);
    void KeyReleased(GC_Joypads joypad, GC_Keys key);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    void Update();

private:
    u8 m_Joypad1;
    u8 m_Joypad2;
    int m_iInputCycles;
};

#endif	/* INPUT_H */
