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

#ifndef OCMMAPPER_H
#define OCMMAPPER_H

#include "Mapper.h"
#include "Cartridge.h"

class OCMMapper : public Mapper
{
public:
    OCMMapper(Cartridge* pCartridge);
    virtual ~OCMMapper();
    
    virtual void Reset();
    virtual u8 Read(u16 address);
    virtual void Write(u16 address, u8 value);
    virtual void SaveState(std::ostream& stream);
    virtual void LoadState(std::istream& stream);
    
    u8 GetBankReg(int index) const { return (index >= 0 && index < 4) ? m_BankReg[index] : 0; }

private:
    u8 m_EEPROMState;
    u8 m_EEPROMCmd[8];
    u8 m_EEPROMCmdPos;
    u8 m_BankReg[4];
};

inline OCMMapper::OCMMapper(Cartridge* pCartridge) : Mapper(pCartridge)
{
    Reset();
}

inline OCMMapper::~OCMMapper()
{
}

inline void OCMMapper::Reset()
{
    m_EEPROMState = 0;
    m_EEPROMCmdPos = 0;
    for (int i = 0; i < 8; i++)
        m_EEPROMCmd[i] = 0;
    for (int i = 0; i < 4; i++)
        m_BankReg[i] = 0;
}

inline u8 OCMMapper::Read(u16 address)
{
    u8* pRom = m_pCartridge->GetROM();
    int bankOffset;
    
    if (address < 0xA000)
    {
        // 8000-9FFF: Bank 3
        bankOffset = (address - 0x8000) + (m_BankReg[3] * 0x2000);
    }
    else if (address < 0xC000)
    {
        // A000-BFFF: Bank 0
        bankOffset = (address - 0xA000) + (m_BankReg[0] * 0x2000);
    }
    else if (address < 0xE000)
    {
        // C000-DFFF: Bank 1
        bankOffset = (address - 0xC000) + (m_BankReg[1] * 0x2000);
    }
    else
    {
        // E000-FFFF: Bank 2 (ROM or EEPROM)
        if (m_EEPROMState == 2)
        {
            // EEPROM read mode
            u8* pEEPROM = m_pCartridge->GetEEPROM();
            if (IsValidPointer(pEEPROM))
                return pEEPROM[address & 0x3FF];
            else
                return 0xFF;
        }
        bankOffset = (address - 0xE000) + (m_BankReg[2] * 0x2000);
    }
    
    return pRom[bankOffset];
}

inline void OCMMapper::Write(u16 address, u8 value)
{
    if (address < 0xE000)
    {
        // No writes allowed to 8000-DFFF
        return;
    }
    
    u8* pEEPROM = m_pCartridge->GetEEPROM();
    
    // Bank switching - addresses FFFC-FFFF map to banks 0-3
    if (address >= 0xFFFC)
    {
        int reg = address & 0x03;
        m_BankReg[reg] = value & 0x0F;
#ifdef DEBUG_GEARCOLECO
        Debug("--> OCM Bank Switch: addr=%X val=%X -> Bank[%d]=%d", address, value, reg, m_BankReg[reg]);
#endif
    }
    // EEPROM control registers
    else if (address == 0xFFF9)
    {
        m_EEPROMState = 1;
        m_EEPROMCmdPos = 0;
    }
    else if (address == 0xFFFA)
    {
        m_EEPROMState = 0;
        m_EEPROMCmdPos = 0;
    }
    // EEPROM data access (E000-FFF8)
    else if (address >= 0xE000 && address <= 0xFFF8)
    {
        if (m_EEPROMState == 1)
        {
            m_EEPROMCmd[m_EEPROMCmdPos++] = value;
            
            if (m_EEPROMCmdPos == 8)
            {
                if ((m_EEPROMCmd[0] == 0xA0) && (m_EEPROMCmd[2] <= 3) && (m_EEPROMCmd[3] == 0xFF))
                {
                    m_EEPROMState = 2;
                }
                else if ((m_EEPROMCmd[0] == 0xA0) && (m_EEPROMCmd[2] <= 3))
                {
                    if (IsValidPointer(pEEPROM))
                    {
                        int offset = (m_EEPROMCmd[2] << 8) | m_EEPROMCmd[1];
                        for (int i = 0; i < 4; i++)
                        {
                            pEEPROM[(offset + i) & 0x3FF] = m_EEPROMCmd[4 + i];
                        }
                    }
                    m_EEPROMState = 0;
                }
                m_EEPROMCmdPos = 0;
            }
        }
    }
}

inline void OCMMapper::SaveState(std::ostream& stream)
{
    stream.write(reinterpret_cast<const char*> (&m_EEPROMState), sizeof(m_EEPROMState));
    stream.write(reinterpret_cast<const char*> (m_EEPROMCmd), sizeof(m_EEPROMCmd));
    stream.write(reinterpret_cast<const char*> (&m_EEPROMCmdPos), sizeof(m_EEPROMCmdPos));
    stream.write(reinterpret_cast<const char*> (m_BankReg), sizeof(m_BankReg));
    
    u8* pEEPROM = m_pCartridge->GetEEPROM();
    if (IsValidPointer(pEEPROM))
    {
        stream.write(reinterpret_cast<const char*> (pEEPROM), 0x400);
    }
}

inline void OCMMapper::LoadState(std::istream& stream)
{
    stream.read(reinterpret_cast<char*> (&m_EEPROMState), sizeof(m_EEPROMState));
    stream.read(reinterpret_cast<char*> (m_EEPROMCmd), sizeof(m_EEPROMCmd));
    stream.read(reinterpret_cast<char*> (&m_EEPROMCmdPos), sizeof(m_EEPROMCmdPos));
    stream.read(reinterpret_cast<char*> (m_BankReg), sizeof(m_BankReg));
    
    u8* pEEPROM = m_pCartridge->GetEEPROM();
    if (IsValidPointer(pEEPROM))
    {
        stream.read(reinterpret_cast<char*> (pEEPROM), 0x400);
    }
}

#endif /* OCMMAPPER_H */
