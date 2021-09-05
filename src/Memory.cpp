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

#include <iostream>
#include <iomanip>
#include <fstream>
#include "Memory.h"
#include "Processor.h"
#include "Cartridge.h"

Memory::Memory(Cartridge* pCartridge)
{
    m_pCartridge = pCartridge;
    InitPointer(m_pProcessor);
    InitPointer(m_pDisassembledRomMap);
    InitPointer(m_pDisassembledRamMap);
    InitPointer(m_pDisassembledBiosMap);
    InitPointer(m_pDisassembledExpansionMap);
    InitPointer(m_pRunToBreakpoint);
    InitPointer(m_pBios);
    InitPointer(m_pRam);
    m_bBiosLoaded = false;
}

Memory::~Memory()
{
    SafeDeleteArray(m_pBios);
    SafeDeleteArray(m_pRam);

    if (IsValidPointer(m_pDisassembledRomMap))
    {
        for (int i = 0; i < MAX_ROM_SIZE; i++)
        {
            SafeDelete(m_pDisassembledRomMap[i]);
        }
        SafeDeleteArray(m_pDisassembledRomMap);
    }

    if (IsValidPointer(m_pDisassembledRamMap))
    {
        for (int i = 0; i < 0x400; i++)
        {
            SafeDelete(m_pDisassembledRamMap[i]);
        }
        SafeDeleteArray(m_pDisassembledRamMap);
    }

    if (IsValidPointer(m_pDisassembledBiosMap))
    {
        for (int i = 0; i < 0x2000; i++)
        {
            SafeDelete(m_pDisassembledBiosMap[i]);
        }
        SafeDeleteArray(m_pDisassembledBiosMap);
    }

    if (IsValidPointer(m_pDisassembledExpansionMap))
    {
        for (int i = 0; i < 0x4000; i++)
        {
            SafeDelete(m_pDisassembledExpansionMap[i]);
        }
        SafeDeleteArray(m_pDisassembledExpansionMap);
    }
}

void Memory::SetProcessor(Processor* pProcessor)
{
    m_pProcessor = pProcessor;
}

void Memory::Init()
{
    m_pRam = new u8[0x0400];
    m_pBios = new u8[0x2000];

#ifndef GEARCOLECO_DISABLE_DISASSEMBLER
    m_pDisassembledRomMap = new stDisassembleRecord*[MAX_ROM_SIZE];
    for (int i = 0; i < MAX_ROM_SIZE; i++)
    {
        InitPointer(m_pDisassembledRomMap[i]);
    }

    m_pDisassembledRamMap = new stDisassembleRecord*[0x400];
    for (int i = 0; i < 0x400; i++)
    {
        InitPointer(m_pDisassembledRamMap[i]);
    }

    m_pDisassembledBiosMap = new stDisassembleRecord*[0x2000];
    for (int i = 0; i < 0x2000; i++)
    {
        InitPointer(m_pDisassembledBiosMap[i]);
    }

    m_pDisassembledExpansionMap = new stDisassembleRecord*[0x4000];
    for (int i = 0; i < 0x4000; i++)
    {
        InitPointer(m_pDisassembledExpansionMap[i]);
    }
#endif

    m_BreakpointsCPU.clear();
    m_BreakpointsMem.clear();

    InitPointer(m_pRunToBreakpoint);

    Reset();
}

void Memory::Reset()
{
    for (int i = 0; i < 0x400; i++)
    {
        m_pRam[i] = rand() % 256;
    }
}

void Memory::SaveState(std::ostream& stream)
{
    stream.write(reinterpret_cast<const char*> (m_pRam), 0x400);
}

void Memory::LoadState(std::istream& stream)
{
    stream.read(reinterpret_cast<char*> (m_pRam), 0x400);
}

std::vector<Memory::stDisassembleRecord*>* Memory::GetBreakpointsCPU()
{
    return &m_BreakpointsCPU;
}

std::vector<Memory::stMemoryBreakpoint>* Memory::GetBreakpointsMem()
{
    return &m_BreakpointsMem;
}

Memory::stDisassembleRecord* Memory::GetRunToBreakpoint()
{
    return m_pRunToBreakpoint;
}

void Memory::SetRunToBreakpoint(Memory::stDisassembleRecord* pBreakpoint)
{
    m_pRunToBreakpoint = pBreakpoint;
}

void Memory::LoadBios(const char* szFilePath)
{
    using namespace std;

    m_bBiosLoaded = false;

    ifstream file(szFilePath, ios::in | ios::binary | ios::ate);

    if (file.is_open())
    {
        int size = static_cast<int> (file.tellg());

        if (size != 0x2000)
        {
            Log("Incorrect BIOS size %d: %s", size, szFilePath);
            return;
        }

        file.seekg(0, ios::beg);
        file.read(reinterpret_cast<char*>(m_pBios), size);
        file.close();

        m_bBiosLoaded = true;

        Log("BIOS %s loaded (%d bytes)", szFilePath, size);
    }
    else
    {
        Log("There was a problem opening the file %s", szFilePath);
    }
}

u8* Memory::GetRam()
{
    return m_pRam;
}

u8* Memory::GetBios()
{
    return m_pBios;
}

bool Memory::IsBiosLoaded()
{
    return m_bBiosLoaded;
}

void Memory::CheckBreakpoints(u16 address, bool write)
{
    size_t size = m_BreakpointsMem.size();

    for (size_t b = 0; b < size; b++)
    {
        if (write && !m_BreakpointsMem[b].write)
            continue;

        if (!write && !m_BreakpointsMem[b].read)
            continue;

        bool proceed = false;

        if (m_BreakpointsMem[b].range)
        {
            if ((address >= m_BreakpointsMem[b].address1) && (address <= m_BreakpointsMem[b].address2))
            {
                proceed = true;
            }
        }
        else
        {
            if (m_BreakpointsMem[b].address1 == address)
            {
                proceed = true;
            }
        }

        if (proceed)
        {
            m_pProcessor->RequestMemoryBreakpoint();
            break;
        }
    }
}

void Memory::ResetRomDisassembledMemory()
{
    #ifndef GEARCOLECO_DISABLE_DISSSEMBLER

    m_BreakpointsCPU.clear();

    if (IsValidPointer(m_pDisassembledRomMap))
    {
        for (int i = 0; i < MAX_ROM_SIZE; i++)
        {
            SafeDelete(m_pDisassembledRomMap[i]);
        }
    }

    if (IsValidPointer(m_pDisassembledRamMap))
    {
        for (int i = 0; i < 0x400; i++)
        {
            SafeDelete(m_pDisassembledRamMap[i]);
        }
    }

    if (IsValidPointer(m_pDisassembledBiosMap))
    {
        for (int i = 0; i < 0x2000; i++)
        {
            SafeDelete(m_pDisassembledBiosMap[i]);
        }
    }

    if (IsValidPointer(m_pDisassembledExpansionMap))
    {
        for (int i = 0; i < 0x4000; i++)
        {
            SafeDelete(m_pDisassembledExpansionMap[i]);
        }
    }

    #endif
}

Memory::stDisassembleRecord* Memory::GetDisassembleRecord(u16 address, bool createIfNotFound)
{
    #ifndef GEARCOLECO_DISABLE_DISSSEMBLER

    stDisassembleRecord** map = NULL;
    int offset = address;
    int segment = 0;

    switch (address & 0xE000)
    {
        case 0x0000:
        {
            offset = address;
            map = m_pDisassembledBiosMap;
            segment = 0;
            break;
        }
        case 0x2000:
        case 0x4000:
        {
            offset = address - 0x2000;
            map = m_pDisassembledExpansionMap;
            segment = 1;
            break;
        }
        case 0x6000:
        {
            offset = address & 0x03FF;
            map = m_pDisassembledRamMap;
            segment = 2;
            break;
        }
        default:
        {
            offset = address - 0x8000;
            map = m_pDisassembledRomMap;
            segment = 3;
        }
    }

    if (!IsValidPointer(map[offset]) && createIfNotFound)
    {
        map[offset] = new Memory::stDisassembleRecord;

        map[offset]->address = address;
        map[offset]->bank = 0;
        map[offset]->name[0] = 0;
        map[offset]->bytes[0] = 0;
        map[offset]->size = 0;
        for (int i = 0; i < 4; i++)
            map[offset]->opcodes[i] = 0;
        map[offset]->jump = false;
        map[offset]->jump_address = 0;

        switch (segment)
        {
            case 0:
            {
                strcpy(map[offset]->segment, "BIOS");
                break;
            }
            case 1:
            {
                strcpy(map[offset]->segment, "EXP ");
                break;
            }
            case 2:
            {
                strcpy(map[offset]->segment, "RAM ");
                break;
            }
            case 3:
            {
                strcpy(map[offset]->segment, "ROM ");
                break;
            }
        }
    }

    return map[offset];

    #else

    return NULL;

    #endif
}