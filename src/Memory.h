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

#ifndef MEMORY_H
#define	MEMORY_H

#include "definitions.h"
#include "log.h"
#include <vector>

class Processor;
class Cartridge;
class Mapper;

class Memory
{
public:
    Memory(Cartridge* pCartridge);
    ~Memory();
    void SetProcessor(Processor* pProcessor);
    void Init();
    void Reset();
    void SetupMapper();
    u8 Read(u16 address);
    void Write(u16 address, u8 value);
    u8* GetRam();
    u8* GetSGMRam();
    u8* GetBios();
    u8 GetRomBank();
    u32 GetRomBankAddress();
    void LoadBios(const char* szFilePath);
    bool IsBiosLoaded();
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);
    void ResetRomDisassembledMemory();
    u8 DebugRetrieve(u16 address);
    GC_Disassembler_Record* GetOrCreateDisassemblerRecord(u16 address);
    GC_Disassembler_Record* GetDisassemblerRecord(u16 address);
    GC_Disassembler_Record* GetDisassemblerRecord(u16 address, u8 bank);
    GC_Disassembler_Record** GetDisassemblerRomMap();
    GC_Disassembler_Record** GetDisassemblerRamMap();
    GC_Disassembler_Record** GetDisassemblerBiosMap();
    GC_Disassembler_Record** GetDisassemblerSGMRamMap();
    u32 GetPhysicalAddress(u16 address);
    u8 GetBank(u16 address);
    bool IsSGMUpperEnabled() { return m_bSGMUpper; }
    bool IsSGMLowerEnabled() { return m_bSGMLower; }
    void EnableSGMUpper(bool enable);
    void EnableSGMLower(bool enable);
    Mapper* GetMapper();
    void Tick(unsigned int cycles) { m_iTotalCycles += cycles; }
    u64 GetTotalCycles() const { return m_iTotalCycles; }

private:
    Processor* m_pProcessor;
    Cartridge* m_pCartridge;
    Mapper* m_pMapper;
    GC_Disassembler_Record** m_pDisassembledRomMap;
    GC_Disassembler_Record** m_pDisassembledRamMap;
    GC_Disassembler_Record** m_pDisassembledBiosMap;
    GC_Disassembler_Record** m_pDisassembledSGMRamMap;
    bool m_bBiosLoaded;
    bool m_bSGMUpper;
    bool m_bSGMLower;
    u8* m_pBios;
    u8* m_pRam;
    u8* m_pSGMRam;
    u64 m_iTotalCycles;
};

#include "Memory_inline.h"

#endif	/* MEMORY_H */
