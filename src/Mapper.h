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

#ifndef MAPPER_H
#define MAPPER_H

#include "definitions.h"
#include <iostream>

class Cartridge;
class TraceLogger;

class Mapper
{
public:
    Mapper(Cartridge* pCartridge) : m_pCartridge(pCartridge), m_pTraceLogger(NULL) { }
    virtual ~Mapper() { }

    virtual void Reset() = 0;
    virtual u8 Read(u16 address) = 0;
    virtual u8 Peek(u16 address) { return Read(address); }
    virtual void Write(u16 address, u8 value) = 0;
    virtual void SaveState(std::ostream& stream) = 0;
    virtual void LoadState(std::istream& stream) = 0;
    virtual u8 GetRomBank() { return 0; }
    virtual u32 GetRomBankAddress() { return 0; }
    virtual u8 GetBankReg(int) { return 0; }
    virtual u8 GetLastBank() { return 0; }
    virtual u8* GetSaveData() { return NULL; }
    virtual int GetSaveDataSize() { return 0; }
    void SetTraceLogger(TraceLogger* pTraceLogger);

protected:
    INLINE void TraceMapperEvent(u8 event, u16 address, u8 value,
        u8 state = 0, u16 auxiliary = 0);
    void LogMapperEvent(u8 event, u16 address, u8 value,
        u8 state, u16 auxiliary);
    Cartridge* m_pCartridge;
    TraceLogger* m_pTraceLogger;
};

#include "TraceLogger.h"

INLINE void Mapper::TraceMapperEvent(u8 event, u16 address, u8 value,
    u8 state, u16 auxiliary)
{
    if (IsValidPointer(m_pTraceLogger) && m_pTraceLogger->IsEventEnabled(TRACE_MAPPER, event))
        LogMapperEvent(event, address, value, state, auxiliary);
}

#endif /* MAPPER_H */
