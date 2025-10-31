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

#ifndef MEGACARTMAPPER_H
#define MEGACARTMAPPER_H

#include "Mapper.h"
#include "Cartridge.h"

class MegaCartMapper : public Mapper
{
public:
    MegaCartMapper(Cartridge* pCartridge);
    virtual ~MegaCartMapper();
    
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

inline MegaCartMapper::MegaCartMapper(Cartridge* pCartridge) : Mapper(pCartridge)
{
    Reset();
}

inline MegaCartMapper::~MegaCartMapper()
{
}

inline void MegaCartMapper::Reset()
{
    m_RomBank = 0;
    m_RomBankAddress = 0;
}

inline u8 MegaCartMapper::Read(u16 address)
{
    u8* pRom = m_pCartridge->GetROM();
    int romSize = m_pCartridge->GetROMSize();

    if (address < 0xC000)
    {
        return pRom[(address & 0x3FFF) + (romSize - 0x4000)];
    }
    else
    {
        if (address >= 0xFFC0)
        {
            m_RomBank = address & (m_pCartridge->GetROMBankCount() - 1);
            m_RomBankAddress = m_RomBank << 14;
        }
        return pRom[(address & 0x3FFF) + m_RomBankAddress];
    }
}

inline void MegaCartMapper::Write(u16 address, u8 value)
{
    if (m_pCartridge->HasSRAM() && (address >= 0xE000) && (address < 0xE800))
    {
        u8* pRom = m_pCartridge->GetROM();
        pRom[(address + 0x800) & 0x7FFF] = value;
    }
    else if (address >= 0xFFC0)
    {
        m_RomBank = address & (m_pCartridge->GetROMBankCount() - 1);
        m_RomBankAddress = m_RomBank << 14;
    }
    else
    {
        Debug("--> ** Attempting to write on ROM: %X %X", address, value);
    }
}

inline void MegaCartMapper::SaveState(std::ostream& stream)
{
    stream.write(reinterpret_cast<const char*> (&m_RomBank), sizeof(m_RomBank));
    stream.write(reinterpret_cast<const char*> (&m_RomBankAddress), sizeof(m_RomBankAddress));
}

inline void MegaCartMapper::LoadState(std::istream& stream)
{
    stream.read(reinterpret_cast<char*> (&m_RomBank), sizeof(m_RomBank));
    stream.read(reinterpret_cast<char*> (&m_RomBankAddress), sizeof(m_RomBankAddress));
}

#endif /* MEGACARTMAPPER_H */
