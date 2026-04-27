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

#ifndef TRACE_LOGGER_H
#define TRACE_LOGGER_H

#include "definitions.h"

#define TRACE_BUFFER_SIZE 100000

enum GC_Trace_Type : u8
{
    TRACE_CPU = 0,
    TRACE_CPU_IRQ,
    TRACE_VDP_WRITE,
    TRACE_VDP_STATUS,
    TRACE_PSG,
    TRACE_AY8910,
    TRACE_IO_PORT,
    TRACE_SGM,
    TRACE_TYPE_COUNT,
};

#define TRACE_FLAG_CPU          (1 << TRACE_CPU)
#define TRACE_FLAG_CPU_IRQ      (1 << TRACE_CPU_IRQ)
#define TRACE_FLAG_VDP_WRITE    (1 << TRACE_VDP_WRITE)
#define TRACE_FLAG_VDP_STATUS   (1 << TRACE_VDP_STATUS)
#define TRACE_FLAG_PSG          (1 << TRACE_PSG)
#define TRACE_FLAG_AY8910       (1 << TRACE_AY8910)
#define TRACE_FLAG_IO_PORT      (1 << TRACE_IO_PORT)
#define TRACE_FLAG_SGM          (1 << TRACE_SGM)
#define TRACE_FLAG_ALL          0xFF

#define GC_VDP_EVENT_VINT        0
#define GC_VDP_EVENT_VINT_FLAG   1
#define GC_VDP_EVENT_DISPLAY     2
#define GC_VDP_EVENT_SPRITE_OVR  3
#define GC_VDP_EVENT_SPRITE_COL  4

struct GC_Trace_Entry
{
    GC_Trace_Type type;
    u64 cycle;
    union
    {
        struct
        {
            u16 pc;
            u8 bank;
            u16 af;
            u16 bc;
            u16 de;
            u16 hl;
            u16 sp;
        } cpu;

        struct
        {
            u16 pc;
            u16 vector;
            u8 type;
        } irq;

        struct
        {
            u8 reg;
            u8 value;
        } vdp_write;

        struct
        {
            u8 event;
            u8 value;
            u16 line;
        } vdp_status;

        struct
        {
            u8 value;
        } psg;

        struct
        {
            u8 port;
            u8 value;
            bool is_write;
        } io_port;

        struct
        {
            u8 reg;
            u8 value;
            bool is_write;
        } ay8910;

        struct
        {
            u8 port;
            u8 value;
        } sgm;
    };
};

class TraceLogger
{
public:
    TraceLogger();
    ~TraceLogger();
    void Reset();
    inline bool IsEnabled(GC_Trace_Type type) const;
    inline void TraceLog(const GC_Trace_Entry& entry);
    void SetEnabledFlags(u32 flags);
    u32 GetEnabledFlags() const;
    const GC_Trace_Entry* GetBuffer() const;
    u32 GetCount() const;
    u32 GetPosition() const;
    u64 GetTotalLogged() const;
    const GC_Trace_Entry& GetEntry(u32 index) const;

private:
    GC_Trace_Entry* m_buffer;
    u32 m_position;
    u32 m_count;
    u32 m_enabled_flags;
    u64 m_total_logged;
};

inline bool TraceLogger::IsEnabled(GC_Trace_Type type) const
{
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    return (m_enabled_flags & (1 << type)) != 0;
#else
    UNUSED(type);
    return false;
#endif
}

inline void TraceLogger::TraceLog(const GC_Trace_Entry& entry)
{
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    m_buffer[m_position] = entry;
    m_position = (m_position + 1) % TRACE_BUFFER_SIZE;
    if (m_count < TRACE_BUFFER_SIZE)
        m_count++;
    m_total_logged++;
#else
    UNUSED(entry);
#endif
}

#endif /* TRACE_LOGGER_H */
