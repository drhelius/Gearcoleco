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
#include <vector>

class Processor;
class Cartridge;

class Memory
{
public:
    struct stDisassembleRecord
    {
        u16 address;
        char name[32];
        char bytes[16];
        int size;
        int bank;
        u8 opcodes[4];
        bool jump;
        u16 jump_address;
    };

    struct stMemoryBreakpoint
    {
        u16 address1;
        u16 address2;
        bool read;
        bool write;
        bool range;
    };

public:
    Memory(Cartridge* pCartridge);
    ~Memory();
    void SetProcessor(Processor* pProcessor);
    void Init();
    void Reset();
    u8 Read(u16 address);
    void Write(u16 address, u8 value);
    stDisassembleRecord** GetDisassembledROMMemoryMap();
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);
    std::vector<stDisassembleRecord*>* GetBreakpointsCPU();
    std::vector<stMemoryBreakpoint>* GetBreakpointsMem();
    stDisassembleRecord* GetRunToBreakpoint();
    void SetRunToBreakpoint(stDisassembleRecord* pBreakpoint);
    void LoadBios(const char* szFilePath);
    u8* GetBios();
    bool IsBiosLoaded();
    void ResetRomDisassembledMemory();

private:
    void CheckBreakpoints(u16 address, bool write);

private:
    Processor* m_pProcessor;
    Cartridge* m_pCartridge;
    stDisassembleRecord** m_pDisassembledROMMap;
    stDisassembleRecord** m_pDisassembledRAMMap;
    stDisassembleRecord** m_pDisassembledBIOSMap;
    std::vector<stDisassembleRecord*> m_BreakpointsCPU;
    std::vector<stMemoryBreakpoint> m_BreakpointsMem;
    stDisassembleRecord* m_pRunToBreakpoint;
    bool m_bBiosLoaded;
    u8* m_pBios;
    u8* m_pRam;
};

#include "Memory_inline.h"

#endif	/* MEMORY_H */
