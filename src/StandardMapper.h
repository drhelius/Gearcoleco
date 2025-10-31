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

#ifndef STANDARDMAPPER_H
#define STANDARDMAPPER_H

#include "Mapper.h"
#include "Cartridge.h"

class StandardMapper : public Mapper
{
public:
    StandardMapper(Cartridge* pCartridge);
    virtual ~StandardMapper();
    
    virtual void Reset();
    virtual u8 Read(u16 address);
    virtual void Write(u16 address, u8 value);
    virtual void SaveState(std::ostream& stream);
    virtual void LoadState(std::istream& stream);
};

inline StandardMapper::StandardMapper(Cartridge* pCartridge) : Mapper(pCartridge)
{
}

inline StandardMapper::~StandardMapper()
{
}

inline void StandardMapper::Reset()
{
}

inline u8 StandardMapper::Read(u16 address)
{
    u8* pRom = m_pCartridge->GetROM();
    int romSize = m_pCartridge->GetROMSize();

    if (address >= (romSize + 0x8000))
    {
        Debug("--> ** Attempting to read from outer ROM: %X. ROM Size: %X", address, romSize);
        return 0xFF;
    }

    return pRom[address & 0x7FFF];
}

inline void StandardMapper::Write(u16 address, u8 value)
{
    if (m_pCartridge->HasSRAM() && (address >= 0xE000) && (address < 0xE800))
    {
        u8* pRom = m_pCartridge->GetROM();
        pRom[(address + 0x800) & 0x7FFF] = value;
    }
    else
    {
        Debug("--> ** Attempting to write on ROM: %X %X", address, value);
    }
}

inline void StandardMapper::SaveState(std::ostream& stream)
{
    (void)stream;
}

inline void StandardMapper::LoadState(std::istream& stream)
{
    (void)stream;
}

#endif /* STANDARDMAPPER_H */
