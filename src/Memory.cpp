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
#include "Mapper.h"
#include "StandardMapper.h"
#include "MegaCartMapper.h"
#include "ActivisionMapper.h"
#include "OCMMapper.h"

#include "Memory.h"
#include "Processor.h"
#include "Cartridge.h"
#include "common.h"

Memory::Memory(Cartridge* pCartridge)
{
    m_pCartridge = pCartridge;
    InitPointer(m_pProcessor);
    InitPointer(m_pMapper);
    InitPointer(m_pDisassembledRomMap);
    InitPointer(m_pDisassembledRamMap);
    InitPointer(m_pDisassembledBiosMap);
    InitPointer(m_pDisassembledSGMRamMap);
    InitPointer(m_pBios);
    InitPointer(m_pRam);
    InitPointer(m_pSGMRam);
    m_bBiosLoaded = false;
    m_bSGMUpper = false;
    m_bSGMLower = false;
    m_iTotalCycles = 0;
}

Memory::~Memory()
{
    SafeDelete(m_pMapper);
    SafeDeleteArray(m_pBios);
    SafeDeleteArray(m_pRam);
    SafeDeleteArray(m_pSGMRam);

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

    if (IsValidPointer(m_pDisassembledSGMRamMap))
    {
        for (int i = 0; i < 0x8000; i++)
        {
            SafeDelete(m_pDisassembledSGMRamMap[i]);
        }
        SafeDeleteArray(m_pDisassembledSGMRamMap);
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
    m_pSGMRam = new u8[0x8000];

#ifndef GEARCOLECO_DISABLE_DISASSEMBLER
    m_pDisassembledRomMap = new GC_Disassembler_Record*[MAX_ROM_SIZE];
    for (int i = 0; i < MAX_ROM_SIZE; i++)
    {
        InitPointer(m_pDisassembledRomMap[i]);
    }

    m_pDisassembledRamMap = new GC_Disassembler_Record*[0x400];
    for (int i = 0; i < 0x400; i++)
    {
        InitPointer(m_pDisassembledRamMap[i]);
    }

    m_pDisassembledBiosMap = new GC_Disassembler_Record*[0x2000];
    for (int i = 0; i < 0x2000; i++)
    {
        InitPointer(m_pDisassembledBiosMap[i]);
    }

    m_pDisassembledSGMRamMap = new GC_Disassembler_Record*[0x8000];
    for (int i = 0; i < 0x8000; i++)
    {
        InitPointer(m_pDisassembledSGMRamMap[i]);
    }
#endif

    Reset();
}

void Memory::SetupMapper()
{
    SafeDelete(m_pMapper);

    switch (m_pCartridge->GetType())
    {
        case Cartridge::CartridgeMegaCart:
            m_pMapper = new MegaCartMapper(m_pCartridge);
            break;
        case Cartridge::CartridgeActivisionCart:
            m_pMapper = new ActivisionMapper(m_pCartridge);
            break;
        case Cartridge::CartridgeOCM:
            m_pMapper = new OCMMapper(m_pCartridge, this);
            break;
        default:
            m_pMapper = new StandardMapper(m_pCartridge);
            break;
    }

    m_pMapper->Reset();
}

void Memory::Reset()
{
    m_iTotalCycles = 0;
    m_bSGMUpper = (m_pCartridge->GetType() == Cartridge::CartridgeOCM);
    m_bSGMLower = false;

    for (int i = 0; i < 0x400; i++)
    {
        m_pRam[i] = rand() % 256;
    }

    for (int i = 0; i < 0x8000; i++)
    {
        m_pSGMRam[i] = rand() % 256;
    }

    if (m_pCartridge->IsPAL())
        m_pBios[0x69] = 0x32;
    else
        m_pBios[0x69] = 0x3C;
}

void Memory::SaveState(std::ostream& stream)
{
    stream.write(reinterpret_cast<const char*> (m_pRam), 0x400);
    stream.write(reinterpret_cast<const char*> (m_pSGMRam), 0x8000);
    stream.write(reinterpret_cast<const char*> (&m_bSGMUpper), sizeof(m_bSGMUpper));
    stream.write(reinterpret_cast<const char*> (&m_bSGMLower), sizeof(m_bSGMLower));
    m_pMapper->SaveState(stream);
}

void Memory::LoadState(std::istream& stream)
{
    stream.read(reinterpret_cast<char*> (m_pRam), 0x400);
    stream.read(reinterpret_cast<char*> (m_pSGMRam), 0x8000);
    stream.read(reinterpret_cast<char*> (&m_bSGMUpper), sizeof(m_bSGMUpper));
    stream.read(reinterpret_cast<char*> (&m_bSGMLower), sizeof(m_bSGMLower));
    m_pMapper->LoadState(stream);
}

void Memory::LoadBios(const char* szFilePath)
{
    using namespace std;

    m_bBiosLoaded = false;

    ifstream file;
    open_ifstream_utf8(file, szFilePath, ios::in | ios::binary | ios::ate);

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

u8* Memory::GetSGMRam()
{
    return m_pSGMRam;
}

u8* Memory::GetBios()
{
    return m_pBios;
}

u8 Memory::GetRomBank()
{
    return IsValidPointer(m_pMapper) ? m_pMapper->GetRomBank() : 0;
}

u32 Memory::GetRomBankAddress()
{
    return IsValidPointer(m_pMapper) ? m_pMapper->GetRomBankAddress() : 0;
}

bool Memory::IsBiosLoaded()
{
    return m_bBiosLoaded;
}

void Memory::EnableSGMUpper(bool enable)
{
    m_bSGMUpper = enable;
}

void Memory::EnableSGMLower(bool enable)
{
    m_bSGMLower = enable;
}

Mapper* Memory::GetMapper()
{
    return m_pMapper;
}

void Memory::ResetRomDisassembledMemory()
{
    #ifndef GEARCOLECO_DISABLE_DISASSEMBLER

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

    if (IsValidPointer(m_pDisassembledSGMRamMap))
    {
        for (int i = 0; i < 0x8000; i++)
        {
            SafeDelete(m_pDisassembledSGMRamMap[i]);
        }
    }

    #endif
}

GC_Disassembler_Record* Memory::GetOrCreateDisassemblerRecord(u16 address)
{
#ifndef GEARCOLECO_DISABLE_DISASSEMBLER

    GC_Disassembler_Record** map = NULL;
    int offset = address;
    int bank = 0;

    switch (address & 0xE000)
    {
        case 0x0000:
        {
            offset = address;
            map = m_bSGMLower ? m_pDisassembledSGMRamMap : m_pDisassembledBiosMap;
            break;
        }
        case 0x2000:
        case 0x4000:
        {
            offset = address;
            map = m_pDisassembledSGMRamMap;
            break;
        }
        case 0x6000:
        {
            offset = m_bSGMUpper ? address : address & 0x03FF;
            map = m_bSGMUpper ? m_pDisassembledSGMRamMap : m_pDisassembledRamMap;
            break;
        }
        default:
        {
            map = m_pDisassembledRomMap;

            switch (m_pCartridge->GetType())
            {
                case Cartridge::CartridgeMegaCart:
                {
                    if (address < 0xC000)
                    {
                        offset = (address & 0x3FFF) + (m_pCartridge->GetROMSize() - 0x4000);
                        bank = m_pCartridge->GetROMBankCount() - 1;
                    }
                    else
                    {
                        offset = (address & 0x3FFF) + GetRomBankAddress();
                        bank = GetRomBank();
                    }
                    break;
                }
                case Cartridge::CartridgeActivisionCart:
                {
                    if (address < 0xC000)
                    {
                        offset = address & 0x3FFF;
                        bank = 0;
                    }
                    else
                    {
                        offset = (address & 0x3FFF) + GetRomBankAddress();
                        bank = GetRomBank();
                    }
                    break;
                }
                case Cartridge::CartridgeOCM:
                {
                    if (address < 0xA000)
                    {
                        offset = (address - 0x8000) + (m_pMapper->GetBankReg(3) * 0x2000);
                        bank = m_pMapper->GetBankReg(3);
                    }
                    else if (address < 0xC000)
                    {
                        offset = (address - 0xA000) + (m_pMapper->GetBankReg(0) * 0x2000);
                        bank = m_pMapper->GetBankReg(0);
                    }
                    else if (address < 0xE000)
                    {
                        offset = (address - 0xC000) + (m_pMapper->GetBankReg(1) * 0x2000);
                        bank = m_pMapper->GetBankReg(1);
                    }
                    else
                    {
                        offset = (address - 0xE000) + (m_pMapper->GetBankReg(2) * 0x2000);
                        bank = m_pMapper->GetBankReg(2);
                    }
                    break;
                }
                default:
                {
                    offset = address - 0x8000;
                    bank = 0;
                    break;
                }
            }
        }
    }

    GC_Disassembler_Record* record = map[offset];

    if (!IsValidPointer(record))
    {
        record = new GC_Disassembler_Record();
        record->address = offset;
        record->bank = (u8)bank;
        record->segment[0] = 0;
        record->name[0] = 0;
        record->bytes[0] = 0;
        record->size = 0;
        for (int i = 0; i < 7; i++)
            record->opcodes[i] = 0;
        record->jump = false;
        record->jump_address = 0;
        record->jump_bank = 0;
        record->subroutine = false;
        record->irq = 0;
        record->has_operand_address = false;
        record->operand_address = 0;
        record->auto_symbol[0] = 0;
        map[offset] = record;
    }

    return record;

#else
    UNUSED(address);
    return NULL;
#endif
}

GC_Disassembler_Record* Memory::GetDisassemblerRecord(u16 address)
{
#ifndef GEARCOLECO_DISABLE_DISASSEMBLER

    GC_Disassembler_Record** map = NULL;
    int offset = address;

    switch (address & 0xE000)
    {
        case 0x0000:
        {
            offset = address;
            map = m_bSGMLower ? m_pDisassembledSGMRamMap : m_pDisassembledBiosMap;
            break;
        }
        case 0x2000:
        case 0x4000:
        {
            offset = address;
            map = m_pDisassembledSGMRamMap;
            break;
        }
        case 0x6000:
        {
            offset = m_bSGMUpper ? address : address & 0x03FF;
            map = m_bSGMUpper ? m_pDisassembledSGMRamMap : m_pDisassembledRamMap;
            break;
        }
        default:
        {
            map = m_pDisassembledRomMap;

            switch (m_pCartridge->GetType())
            {
                case Cartridge::CartridgeMegaCart:
                {
                    if (address < 0xC000)
                    {
                        offset = (address & 0x3FFF) + (m_pCartridge->GetROMSize() - 0x4000);
                    }
                    else
                    {
                        offset = (address & 0x3FFF) + GetRomBankAddress();
                    }
                    break;
                }
                case Cartridge::CartridgeActivisionCart:
                {
                    if (address < 0xC000)
                    {
                        offset = address & 0x3FFF;
                    }
                    else
                    {
                        offset = (address & 0x3FFF) + GetRomBankAddress();
                    }
                    break;
                }
                case Cartridge::CartridgeOCM:
                {
                    if (address < 0xA000)
                        offset = (address - 0x8000) + (m_pMapper->GetBankReg(3) * 0x2000);
                    else if (address < 0xC000)
                        offset = (address - 0xA000) + (m_pMapper->GetBankReg(0) * 0x2000);
                    else if (address < 0xE000)
                        offset = (address - 0xC000) + (m_pMapper->GetBankReg(1) * 0x2000);
                    else
                        offset = (address - 0xE000) + (m_pMapper->GetBankReg(2) * 0x2000);
                    break;
                }
                default:
                {
                    offset = address - 0x8000;
                    break;
                }
            }
        }
    }

    return map[offset];

#else
    UNUSED(address);
    return NULL;
#endif
}

u32 Memory::GetPhysicalAddress(u16 address)
{
    if (address < 0x8000)
        return (u32)address;

    switch (m_pCartridge->GetType())
    {
        case Cartridge::CartridgeMegaCart:
        {
            if (address < 0xC000)
                return (u32)((address & 0x3FFF) + (m_pCartridge->GetROMSize() - 0x4000));
            else
                return (u32)((address & 0x3FFF) + GetRomBankAddress());
        }
        case Cartridge::CartridgeActivisionCart:
        {
            if (address < 0xC000)
                return (u32)(address & 0x3FFF);
            else
                return (u32)((address & 0x3FFF) + GetRomBankAddress());
        }
        case Cartridge::CartridgeOCM:
        {
            if (address < 0xA000)
                return (u32)((address - 0x8000) + (m_pMapper->GetBankReg(3) * 0x2000));
            else if (address < 0xC000)
                return (u32)((address - 0xA000) + (m_pMapper->GetBankReg(0) * 0x2000));
            else if (address < 0xE000)
                return (u32)((address - 0xC000) + (m_pMapper->GetBankReg(1) * 0x2000));
            else
                return (u32)((address - 0xE000) + (m_pMapper->GetBankReg(2) * 0x2000));
        }
        default:
            return (u32)(address - 0x8000);
    }
}

u8 Memory::GetBank(u16 address)
{
    if (address < 0x8000)
        return 0;

    switch (m_pCartridge->GetType())
    {
        case Cartridge::CartridgeMegaCart:
        {
            if (address < 0xC000)
                return (u8)(m_pCartridge->GetROMBankCount() - 1);
            else
                return GetRomBank();
        }
        case Cartridge::CartridgeActivisionCart:
        {
            if (address < 0xC000)
                return 0;
            else
                return GetRomBank();
        }
        case Cartridge::CartridgeOCM:
        {
            if (address < 0xA000)
                return (u8)m_pMapper->GetBankReg(3);
            else if (address < 0xC000)
                return (u8)m_pMapper->GetBankReg(0);
            else if (address < 0xE000)
                return (u8)m_pMapper->GetBankReg(1);
            else
                return (u8)m_pMapper->GetBankReg(2);
        }
        default:
            return 0;
    }
}