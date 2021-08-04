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
    InitPointer(m_pDisassembledROMMap);
    InitPointer(m_pDisassembledRAMMap);
    InitPointer(m_pDisassembledBIOSMap);
    InitPointer(m_pRunToBreakpoint);
    InitPointer(m_pBios);
    InitPointer(m_pRam);
    m_bBiosLoaded = false;
}

Memory::~Memory()
{
    SafeDeleteArray(m_pBios);
    SafeDeleteArray(m_pRam);

    if (IsValidPointer(m_pDisassembledROMMap))
    {
        for (int i = 0; i < MAX_ROM_SIZE; i++)
        {
            SafeDelete(m_pDisassembledROMMap[i]);
        }
        SafeDeleteArray(m_pDisassembledROMMap);
    }

    if (IsValidPointer(m_pDisassembledRAMMap))
    {
        for (int i = 0; i < 0x400; i++)
        {
            SafeDelete(m_pDisassembledRAMMap[i]);
        }
        SafeDeleteArray(m_pDisassembledRAMMap);
    }

    if (IsValidPointer(m_pDisassembledBIOSMap))
    {
        for (int i = 0; i < 0x2000; i++)
        {
            SafeDelete(m_pDisassembledBIOSMap[i]);
        }
        SafeDeleteArray(m_pDisassembledBIOSMap);
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
    m_pDisassembledROMMap = new stDisassembleRecord*[MAX_ROM_SIZE];
    for (int i = 0; i < MAX_ROM_SIZE; i++)
    {
        InitPointer(m_pDisassembledROMMap[i]);
    }

    m_pDisassembledRAMMap = new stDisassembleRecord*[0x400];
    for (int i = 0; i < 0x400; i++)
    {
        InitPointer(m_pDisassembledRAMMap[i]);
    }

    m_pDisassembledBIOSMap = new stDisassembleRecord*[0x2000];
    for (int i = 0; i < 0x2000; i++)
    {
        InitPointer(m_pDisassembledBIOSMap[i]);
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
    long unsigned int size = m_BreakpointsMem.size();

    for (long unsigned int b = 0; b < size; b++)
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

    if (IsValidPointer(m_pDisassembledROMMap))
    {
        for (int i = 0; i < MAX_ROM_SIZE; i++)
        {
            SafeDelete(m_pDisassembledROMMap[i]);
        }
    }

    #endif
}