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

#ifndef MEMORY_INLINE_H
#define	MEMORY_INLINE_H

#include "Cartridge.h"

inline u8 Memory::Read(u16 address)
{
    #ifndef GEARCOLECO_DISABLE_DISASSEMBLER
    CheckBreakpoints(address, false);
    #endif

    switch (address & 0xE000)
    {
        case 0x0000:
        {
            return m_pBios[address];
        }
        case 0x2000:
        case 0x4000:
        {
            Log("--> ** Attempting to read from expansion: %X", address);
            return 0xFF;
        }
        case 0x6000:
        {
            return m_pRam[address & 0x03FF];
        }
        case 0x8000:
        case 0xA000:
        case 0xC000:
        case 0xE000:
        {
            u8* pRom = m_pCartridge->GetROM();
            int romSize = m_pCartridge->GetROMSize();

            if (address >= (romSize + 0x8000))
                return 0xFF;

            return pRom[address & 0x7FFF];
        }
        default:
            return 0xFF;
    }
}

inline void Memory::Write(u16 address, u8 value)
{
    #ifndef GEARCOLECO_DISABLE_DISASSEMBLER
    CheckBreakpoints(address, true);
    #endif

    switch (address & 0xE000)
    {
        case 0x0000:
        {
            Log("--> ** Attempting to write on BIOS: %X %X", address, value);
            break;
        }
        case 0x2000:
        case 0x4000:
        {
            Log("--> ** Attempting to write on expansion: %X %X", address, value);
            break;
        }
        case 0x6000:
        {
            m_pRam[address & 0x03FF] = value;
            break;
        }
        case 0x8000:
        case 0xA000:
        case 0xC000:
        case 0xE000:
        {
            Log("--> ** Attempting to write on ROM: %X %X", address, value);
            break;
        }
    }
}

inline Memory::stDisassembleRecord** Memory::GetDisassembledROMMemoryMap()
{
    return m_pDisassembledROMMap;
}

#endif	/* MEMORY_INLINE_H */

