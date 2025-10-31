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

#ifndef ACTIVISIONMAPPER_H
#define ACTIVISIONMAPPER_H

#include "Mapper.h"
#include "Cartridge.h"

class ActivisionMapper : public Mapper
{
public:
    ActivisionMapper(Cartridge* pCartridge);
    virtual ~ActivisionMapper();
    
    virtual void Reset();
    virtual u8 Read(u16 address);
    virtual void Write(u16 address, u8 value);
    virtual void SaveState(std::ostream& stream);
    virtual void LoadState(std::istream& stream);
    virtual u8 GetRomBank() { return m_RomBank; }
    virtual u32 GetRomBankAddress() { return m_RomBankAddress; }

private:
    u8 m_RomBank;
    u32 m_RomBankAddress;
};

inline ActivisionMapper::ActivisionMapper(Cartridge* pCartridge) : Mapper(pCartridge)
{
    Reset();
}

inline ActivisionMapper::~ActivisionMapper()
{
}

inline void ActivisionMapper::Reset()
{
    m_RomBank = 0;
    m_RomBankAddress = 0;
}

inline u8 ActivisionMapper::Read(u16 address)
{
    u8* pRom = m_pCartridge->GetROM();
    
    if (address < 0xC000)
    {
        return pRom[address & 0x3FFF];
    }
    else
    {
#ifdef DEBUG_GEARCOLECO
        if (address >= 0xFF80)
        {
            Debug("--> ** EEPROM read: %X", address);
        }
#endif
        return pRom[(address & 0x3FFF) + m_RomBankAddress];
    }
}

inline void ActivisionMapper::Write(u16 address, u8 value)
{
    if (m_pCartridge->HasSRAM() && (address >= 0xE000) && (address < 0xE800))
    {
        u8* pRom = m_pCartridge->GetROM();
        pRom[(address + 0x800) & 0x7FFF] = value;
    }
    else if (address >= 0xFF90)
    {
        if ((address == 0xFF90) || (address == 0xFFA0) || (address == 0xFFB0))
        {
            m_RomBank = (address >> 4) & (m_pCartridge->GetROMBankCount() - 1);
            m_RomBankAddress = m_RomBank << 14;
        }
    }
    else
    {
        Debug("--> ** Attempting to write on ROM: %X %X", address, value);
    }
}

inline void ActivisionMapper::SaveState(std::ostream& stream)
{
    stream.write(reinterpret_cast<const char*> (&m_RomBank), sizeof(m_RomBank));
    stream.write(reinterpret_cast<const char*> (&m_RomBankAddress), sizeof(m_RomBankAddress));
}

inline void ActivisionMapper::LoadState(std::istream& stream)
{
    stream.read(reinterpret_cast<char*> (&m_RomBank), sizeof(m_RomBank));
    stream.read(reinterpret_cast<char*> (&m_RomBankAddress), sizeof(m_RomBankAddress));
}

#endif /* ACTIVISIONMAPPER_H */
