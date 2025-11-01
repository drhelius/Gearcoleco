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

#define OCM_CYCLES_PER_FRAME 59740

class OCMMapper : public Mapper
{
public:
    OCMMapper(Cartridge* pCartridge, Memory* pMemory);
    virtual ~OCMMapper();

    virtual void Reset();
    virtual u8 Read(u16 address);
    virtual void Write(u16 address, u8 value);
    virtual void SaveState(std::ostream& stream);
    virtual void LoadState(std::istream& stream);

    u8 GetBankReg(int index) const { return (index >= 0 && index < 4) ? m_BankReg[index] : 0; }

private:
    // EEPROM state machine
    enum
    {
        EEP_NONE = 0,
        EEP_INIT = 1,
        EEP_STATUS = 2,
        EEP_WRITE = 3
    };

    inline bool EepromReadWindowActive() const
    {
        if (m_EepromReadExpireCycles == 0)
            return false;
        u64 now = m_pMemory->GetTotalCycles();
        if (now < m_EepromReadExpireCycles)
            return true;
        return false;
    }

    inline void ArmEepromReadWindow(u8 value)
    {
        if ((value & 0x0F) == 0x0F)
        {
            u64 now = m_pMemory->GetTotalCycles();
            m_EepromReadExpireCycles = now + (OCM_CYCLES_PER_FRAME * 3ULL);
        }
        else
            m_EepromReadExpireCycles = 0;
    }

private:
    Memory* m_pMemory;
    u8 m_BankReg[4];
    u8 m_EepromCmdPos;
    u8 m_EepromState;
    u64 m_EepromReadExpireCycles;
};

inline OCMMapper::OCMMapper(Cartridge* pCartridge, Memory* pMemory)
: Mapper(pCartridge)
{
    m_pMemory = pMemory;
    Reset();
}

inline OCMMapper::~OCMMapper()
{
}

inline void OCMMapper::Reset()
{
    m_BankReg[0] = 3;
    m_BankReg[1] = 2;
    m_BankReg[2] = 1;
    m_BankReg[3] = 0;
    m_EepromCmdPos = 0;
    m_EepromState = EEP_NONE;
    m_EepromReadExpireCycles = 0;
}

inline u8 OCMMapper::Read(u16 address)
{
    u8* pRom = m_pCartridge->GetROM();

    if (address < 0xA000)
    {
        // 8000-9FFF: Bank 3
        int bankOffset = (address - 0x8000) + (m_BankReg[3] * 0x2000);
        return pRom[bankOffset];
    }
    else if (address < 0xC000)
    {
        // A000-BFFF: Bank 0
        int bankOffset = (address - 0xA000) + (m_BankReg[0] * 0x2000);
        return pRom[bankOffset];
    }
    else if (address < 0xE000)
    {
        // C000-DFFF: Bank 1
        int bankOffset = (address - 0xC000) + (m_BankReg[1] * 0x2000);
        return pRom[bankOffset];
    }
    else
    {
        // E000-FFFF: Bank 2 (ROM or EEPROM)
        // If STATUS mode, reading E000 returns 0xFF (status OK)
        if (((address & 0x0FFF) == 0x0000) && (m_EepromState == EEP_STATUS))
        {
            return 0xFF;
        }

        // EEPROM read window: bank2==0x0F, active timer, E000-E0FF -> EEPROM
        if (m_BankReg[2] == 0x0F && EepromReadWindowActive() && ((address & 0x0FFF) < 0x0100))
        {
            u8* pEEPROM = m_pCartridge->GetEEPROM();
            if (IsValidPointer(pEEPROM))
            {
                return pEEPROM[address & 0x03FF];
            }
            return 0xFF;
        }

        // Otherwise, ROM from bank 2
        int bankOffset = (address - 0xE000) + (m_BankReg[2] * 0x2000);
        return pRom[bankOffset];
    }
}

inline void OCMMapper::Write(u16 address, u8 value)
{
    // OCM EEPROM & banking are only active on E000-FFFF range
    if (address < 0xE000)
    {
        return;
    }

    // EEPROM command/byte write on E000..FFFB
    if (address <= 0xFFFB)
    {
        // Handshake: AA -> 55 -> CMD
        if (value == 0xAA && m_EepromCmdPos == 0)
        {
            m_EepromCmdPos = 1;
            return;
        }
        else if (value == 0x55 && m_EepromCmdPos == 1)
        {
            m_EepromCmdPos = 2;
            return;
        }
        else if (m_EepromCmdPos == 2)
        {
            // Interpret command byte
            if (value == 0x80)
            {
                m_EepromState = EEP_INIT;
            }
            else if (value == 0x30)
            {
                if (m_EepromState == EEP_INIT)
                {
                    m_EepromState = EEP_STATUS;
                }
            }
            else if (value == 0xA0)
            {
                m_EepromState = EEP_WRITE;
            }
            else
            {
                Debug("--> OCM EEP Unknown command: %02X @ %04X", value, address);
            }

            m_EepromCmdPos = 0;
            return;
        }
        else if (m_EepromState == EEP_WRITE)
        {
            // Next write persists a single byte to 1KB EEPROM space
            u8* pEEPROM = m_pCartridge->GetEEPROM();
            if (IsValidPointer(pEEPROM))
            {
                pEEPROM[address & 0x03FF] = value;
            }
            m_EepromState = EEP_NONE;
            return;
        }

        // Any other writes in this range that don't match the handshake are ignored
        return;
    }

    // Bank registers and EEPROM read-window trigger
    // FFFC -> bank 0, FFFD -> bank 1, FFFE -> bank 2 (also arms read-window), FFFF -> bank 3
    if (address >= 0xFFFC)
    {
        if (address == 0xFFFE)
        {
            // Arm or clear the 3-frame EEPROM read window
            ArmEepromReadWindow(value);
        }

        m_BankReg[address & 0x0003] = (u8)(value & 0x0F);

        // Debug("--> OCM Bank[%d] = %d (addr=%04X, val=%02X)", (address & 3), m_BankReg[address & 3], address, value);
        return;
    }
}

inline void OCMMapper::SaveState(std::ostream& stream)
{

    stream.write(reinterpret_cast<const char*>(m_BankReg), sizeof(m_BankReg));
    stream.write(reinterpret_cast<const char*>(&m_EepromCmdPos), sizeof(m_EepromCmdPos));
    stream.write(reinterpret_cast<const char*>(&m_EepromState),   sizeof(m_EepromState));

    // Persist “frames remaining” instead of raw cycle deadline
    u64 now = m_pMemory->GetTotalCycles();
    u8 framesLeft = 0;
    if (m_EepromReadExpireCycles > now)
    {
        u64 delta = m_EepromReadExpireCycles - now;
        // Ceil to frames so the window isn't lost by rounding
        framesLeft = (u8)((delta + (OCM_CYCLES_PER_FRAME - 1)) / OCM_CYCLES_PER_FRAME);
        if (framesLeft > 3)
            framesLeft = 3;
    }
    stream.write(reinterpret_cast<const char*>(&framesLeft), sizeof(framesLeft));

    u8* pEEPROM = m_pCartridge->GetEEPROM();
    stream.write(reinterpret_cast<const char*>(pEEPROM), 0x400);
}

inline void OCMMapper::LoadState(std::istream& stream)
{
    stream.read(reinterpret_cast<char*>(m_BankReg), sizeof(m_BankReg));
    stream.read(reinterpret_cast<char*>(&m_EepromCmdPos), sizeof(m_EepromCmdPos));
    stream.read(reinterpret_cast<char*>(&m_EepromState),   sizeof(m_EepromState));

    u8 framesLeft = 0;
    stream.read(reinterpret_cast<char*>(&framesLeft), sizeof(framesLeft));
    if (framesLeft > 0)
    {
        u64 now = m_pMemory->GetTotalCycles();
        m_EepromReadExpireCycles = now + (u64)framesLeft * OCM_CYCLES_PER_FRAME;
    }
    else
    {
        m_EepromReadExpireCycles = 0;
    }

    u8* pEEPROM = m_pCartridge->GetEEPROM();
    stream.read(reinterpret_cast<char*>(pEEPROM), 0x400);
}

#endif /* OCMMAPPER_H */
