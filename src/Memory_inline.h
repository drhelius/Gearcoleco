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
#include "Mapper.h"

inline u8 Memory::Read(u16 address)
{
    #ifndef GEARCOLECO_DISABLE_DISASSEMBLER
    CheckBreakpoints(address, false);
    #endif

    switch (address & 0xE000)
    {
        case 0x0000:
        {
            return m_bSGMLower ? m_pSGMRam[address] : m_pBios[address];
        }
        case 0x2000:
        case 0x4000:
        {
            return m_bSGMUpper ? m_pSGMRam[address] : 0xFF;
        }
        case 0x6000:
        {
            return m_bSGMUpper ? m_pSGMRam[address] : m_pRam[address & 0x03FF];
        }
        case 0x8000:
        case 0xA000:
        case 0xC000:
        case 0xE000:
        {
            return m_pMapper->Read(address);
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
            if (m_bSGMLower)
                m_pSGMRam[address] = value;
            break;
        }
        case 0x2000:
        case 0x4000:
        {
            if (m_bSGMUpper)
                m_pSGMRam[address] = value;
            break;
        }
        case 0x6000:
        {
            if (m_bSGMUpper)
                m_pSGMRam[address] = value;
            else
                m_pRam[address & 0x03FF] = value;
            break;
        }
        case 0x8000:
        case 0xA000:
        case 0xC000:
        case 0xE000:
        {
            m_pMapper->Write(address, value);
            break;
        }
    }
}

inline Memory::stDisassembleRecord** Memory::GetDisassembledRomMemoryMap()
{
    return m_pDisassembledRomMap;
}

inline Memory::stDisassembleRecord** Memory::GetDisassembledRamMemoryMap()
{
    return m_pDisassembledRamMap;
}

inline Memory::stDisassembleRecord** Memory::GetDisassembledBiosMemoryMap()
{
    return m_pDisassembledBiosMap;
}

inline Memory::stDisassembleRecord** Memory::GetDisassembledSGMRamMemoryMap()
{
    return m_pDisassembledSGMRamMap;
}

#endif	/* MEMORY_INLINE_H */

