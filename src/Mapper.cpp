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
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#include "Mapper.h"
#include "Cartridge.h"

void Mapper::SetTraceLogger(TraceLogger* pTraceLogger)
{
    m_pTraceLogger = pTraceLogger;
}

void Mapper::LogMapperEvent(u8 event, u16 address, u8 value,
    u8 state, u16 auxiliary)
{
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    GC_Trace_Entry e = {};
    e.type = TRACE_MAPPER;
    e.mapper.event = event;
    e.mapper.mapper = (u8)m_pCartridge->GetType();
    e.mapper.address = address;
    e.mapper.value = value;
    e.mapper.state = state;
    if (event == TRACE_MAPPER_EEPROM &&
        m_pCartridge->GetType() == Cartridge::CartridgeOCM && address == 0xFFFE)
    {
        e.mapper.state = state == GetLastBank() ? 1 : 0;
    }
    e.mapper.banks[0] = GetRomBank();
    for (int i = 0; i < 4; i++)
    {
        u8 bank = GetBankReg(i);
        if (bank != 0 || m_pCartridge->GetType() == Cartridge::CartridgeOCM)
            e.mapper.banks[i] = bank;
    }
    e.mapper.auxiliary = auxiliary;
    m_pTraceLogger->TraceLog(e);
#else
    UNUSED(event);
    UNUSED(address);
    UNUSED(value);
    UNUSED(state);
    UNUSED(auxiliary);
#endif
}
