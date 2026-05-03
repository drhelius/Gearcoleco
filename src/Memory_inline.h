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

#include "Processor.h"
#include "Cartridge.h"
#include "Mapper.h"

inline u8 Memory::Read(u16 address)
{
    #ifndef GEARCOLECO_DISABLE_DISASSEMBLER
    m_pProcessor->CheckMemoryBreakpoints(Processor::GC_BREAKPOINT_TYPE_ROMRAM, address, true);
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

inline u8 Memory::DebugRetrieve(u16 address)
{
    switch (address & 0xE000)
    {
        case 0x0000:
            return m_bSGMLower ? m_pSGMRam[address] : m_pBios[address];
        case 0x2000:
        case 0x4000:
            return m_bSGMUpper ? m_pSGMRam[address] : 0xFF;
        case 0x6000:
            return m_bSGMUpper ? m_pSGMRam[address] : m_pRam[address & 0x03FF];
        case 0x8000:
        case 0xA000:
        case 0xC000:
        case 0xE000:
            return m_pMapper->Peek(address);
        default:
            return 0xFF;
    }
}

inline void Memory::Write(u16 address, u8 value)
{
    #ifndef GEARCOLECO_DISABLE_DISASSEMBLER
    m_pProcessor->CheckMemoryBreakpoints(Processor::GC_BREAKPOINT_TYPE_ROMRAM, address, false);
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

inline GC_Disassembler_Record** Memory::GetDisassemblerRomMap()
{
    return m_pDisassembledRomMap;
}

inline GC_Disassembler_Record** Memory::GetDisassemblerRamMap()
{
    return m_pDisassembledRamMap;
}

inline GC_Disassembler_Record** Memory::GetDisassemblerBiosMap()
{
    return m_pDisassembledBiosMap;
}

inline GC_Disassembler_Record** Memory::GetDisassemblerSGMRamMap()
{
    return m_pDisassembledSGMRamMap;
}

inline GC_Disassembler_Record* Memory::GetDisassemblerRecord(u16 address, u8 bank)
{
#ifndef GEARCOLECO_DISABLE_DISASSEMBLER

    if (address < 0x8000)
        return GetDisassemblerRecord(address);

    u32 physical_address = 0;

    switch (m_pCartridge->GetType())
    {
        case Cartridge::CartridgeMegaCart:
        case Cartridge::CartridgeActivisionCart:
        {
            physical_address = (u32)(0x4000 * bank) + (address & 0x3FFF);
            break;
        }
        case Cartridge::CartridgeOCM:
        {
            physical_address = (u32)(0x2000 * bank) + (address & 0x1FFF);
            break;
        }
        default:
        {
            if (bank != 0)
                return NULL;

            physical_address = (u32)(address - 0x8000);
            break;
        }
    }

    if (physical_address >= MAX_ROM_SIZE)
        return NULL;

    return m_pDisassembledRomMap[physical_address];

#else
    UNUSED(address);
    UNUSED(bank);
    return NULL;
#endif
}

#endif	/* MEMORY_INLINE_H */
