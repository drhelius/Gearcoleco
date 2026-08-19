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
    TRACE_VDP,
    TRACE_PSG,
    TRACE_AY8910,
    TRACE_IO,
    TRACE_INPUT,
    TRACE_SGM,
    TRACE_MAPPER,
    TRACE_TYPE_COUNT,
};

static_assert(TRACE_TYPE_COUNT < 32, "Trace category count exceeds flag width");

#define TRACE_VDP_WRITE TRACE_VDP
#define TRACE_VDP_STATUS TRACE_VDP
#define TRACE_IO_PORT TRACE_IO

#define TRACE_FLAG_CPU          (1U << TRACE_CPU)
#define TRACE_FLAG_CPU_IRQ      (1U << TRACE_CPU_IRQ)
#define TRACE_FLAG_VDP          (1U << TRACE_VDP)
#define TRACE_FLAG_PSG          (1U << TRACE_PSG)
#define TRACE_FLAG_AY8910       (1U << TRACE_AY8910)
#define TRACE_FLAG_IO           (1U << TRACE_IO)
#define TRACE_FLAG_INPUT        (1U << TRACE_INPUT)
#define TRACE_FLAG_SGM          (1U << TRACE_SGM)
#define TRACE_FLAG_MAPPER       (1U << TRACE_MAPPER)
#define TRACE_FLAG_ALL          ((1U << TRACE_TYPE_COUNT) - 1U)

#define TRACE_FLAG_VDP_WRITE  TRACE_FLAG_VDP
#define TRACE_FLAG_VDP_STATUS TRACE_FLAG_VDP
#define TRACE_FLAG_IO_PORT    TRACE_FLAG_IO

enum GC_Trace_VDP_Event : u8
{
    TRACE_VDP_REG_WRITE = 0,
    TRACE_VDP_NMI_REQUEST,
    TRACE_VDP_VINT_FLAG,
    TRACE_VDP_STATUS_READ,
    TRACE_VDP_SPRITE_OVERFLOW,
    TRACE_VDP_SPRITE_COLLISION,
    TRACE_VDP_DISPLAY_CHANGE,
    TRACE_VDP_VBLANK,
    TRACE_VDP_FRAME,
    TRACE_VDP_DATA_READ,
    TRACE_VDP_DATA_WRITE,
};

#define TRACE_VDP_EVENT_REGISTERS  (1U << TRACE_VDP_REG_WRITE)
#define TRACE_VDP_EVENT_INTERRUPTS (1U << TRACE_VDP_NMI_REQUEST)
#define TRACE_VDP_EVENT_STATUS     ((1U << TRACE_VDP_VINT_FLAG) | (1U << TRACE_VDP_STATUS_READ))
#define TRACE_VDP_EVENT_SPRITES    ((1U << TRACE_VDP_SPRITE_OVERFLOW) | (1U << TRACE_VDP_SPRITE_COLLISION))
#define TRACE_VDP_EVENT_TIMING     ((1U << TRACE_VDP_DISPLAY_CHANGE) | (1U << TRACE_VDP_VBLANK) | (1U << TRACE_VDP_FRAME))
#define TRACE_VDP_EVENT_VRAM       ((1U << TRACE_VDP_DATA_READ) | (1U << TRACE_VDP_DATA_WRITE))
#define TRACE_VDP_EVENT_ALL        (TRACE_VDP_EVENT_REGISTERS | TRACE_VDP_EVENT_INTERRUPTS | TRACE_VDP_EVENT_STATUS | TRACE_VDP_EVENT_SPRITES | TRACE_VDP_EVENT_TIMING | TRACE_VDP_EVENT_VRAM)
static_assert(TRACE_VDP_DATA_WRITE < 32, "VDP trace events exceed u32 width");

enum GC_Trace_PSG_Event : u8
{
    TRACE_PSG_TONE = 0,
    TRACE_PSG_VOLUME,
    TRACE_PSG_NOISE,
};

#define TRACE_PSG_EVENT_TONE   (1U << TRACE_PSG_TONE)
#define TRACE_PSG_EVENT_VOLUME (1U << TRACE_PSG_VOLUME)
#define TRACE_PSG_EVENT_NOISE  (1U << TRACE_PSG_NOISE)
#define TRACE_PSG_EVENT_ALL    (TRACE_PSG_EVENT_TONE | TRACE_PSG_EVENT_VOLUME | TRACE_PSG_EVENT_NOISE)
static_assert(TRACE_PSG_NOISE < 32, "PSG trace events exceed u32 width");

enum GC_Trace_AY8910_Event : u8
{
    TRACE_AY8910_SELECT = 0,
    TRACE_AY8910_READ,
    TRACE_AY8910_TONE,
    TRACE_AY8910_NOISE_MIXER,
    TRACE_AY8910_VOLUME,
    TRACE_AY8910_ENVELOPE,
    TRACE_AY8910_IO,
};

#define TRACE_AY8910_EVENT_REGISTERS   ((1U << TRACE_AY8910_SELECT) | (1U << TRACE_AY8910_READ))
#define TRACE_AY8910_EVENT_TONE        (1U << TRACE_AY8910_TONE)
#define TRACE_AY8910_EVENT_NOISE_MIXER (1U << TRACE_AY8910_NOISE_MIXER)
#define TRACE_AY8910_EVENT_VOLUME      (1U << TRACE_AY8910_VOLUME)
#define TRACE_AY8910_EVENT_ENVELOPE    (1U << TRACE_AY8910_ENVELOPE)
#define TRACE_AY8910_EVENT_IO          (1U << TRACE_AY8910_IO)
#define TRACE_AY8910_EVENT_ALL         (TRACE_AY8910_EVENT_REGISTERS | TRACE_AY8910_EVENT_TONE | TRACE_AY8910_EVENT_NOISE_MIXER | TRACE_AY8910_EVENT_VOLUME | TRACE_AY8910_EVENT_ENVELOPE | TRACE_AY8910_EVENT_IO)
static_assert(TRACE_AY8910_IO < 32, "AY-3-8910 trace events exceed u32 width");

enum GC_Trace_IO_Event : u8
{
    TRACE_IO_READ = 0,
    TRACE_IO_WRITE,
};

#define TRACE_IO_EVENT_READS  (1U << TRACE_IO_READ)
#define TRACE_IO_EVENT_WRITES (1U << TRACE_IO_WRITE)
#define TRACE_IO_EVENT_ALL    (TRACE_IO_EVENT_READS | TRACE_IO_EVENT_WRITES)
static_assert(TRACE_IO_WRITE < 32, "I/O trace events exceed u32 width");

enum GC_Trace_Input_Event : u8
{
    TRACE_INPUT_READ = 0,
    TRACE_INPUT_WRITE,
    TRACE_INPUT_CHANGE,
};

#define TRACE_INPUT_EVENT_READS  (1U << TRACE_INPUT_READ)
#define TRACE_INPUT_EVENT_WRITES ((1U << TRACE_INPUT_WRITE) | (1U << TRACE_INPUT_CHANGE))
#define TRACE_INPUT_EVENT_ALL    (TRACE_INPUT_EVENT_READS | TRACE_INPUT_EVENT_WRITES)
static_assert(TRACE_INPUT_CHANGE < 32, "Input trace events exceed u32 width");

enum GC_Trace_IO_Target : u8
{
    TRACE_IO_TARGET_UNKNOWN = 0,
    TRACE_IO_TARGET_VDP_DATA,
    TRACE_IO_TARGET_VDP_STATUS,
    TRACE_IO_TARGET_INPUT,
    TRACE_IO_TARGET_PSG,
    TRACE_IO_TARGET_AY_SELECT,
    TRACE_IO_TARGET_AY_DATA,
    TRACE_IO_TARGET_SGM_UPPER,
    TRACE_IO_TARGET_SGM_LOWER,
};

enum GC_Trace_SGM_Event : u8
{
    TRACE_SGM_CONTROL = 0,
};

#define TRACE_SGM_EVENT_CONTROL (1U << TRACE_SGM_CONTROL)
#define TRACE_SGM_EVENT_ALL     TRACE_SGM_EVENT_CONTROL
static_assert(TRACE_SGM_CONTROL < 32, "SGM trace events exceed u32 width");

enum GC_Trace_Mapper_Event : u8
{
    TRACE_MAPPER_BANK = 0,
    TRACE_MAPPER_EEPROM,
    TRACE_MAPPER_SRAM,
};

#define TRACE_MAPPER_EVENT_BANKS  (1U << TRACE_MAPPER_BANK)
#define TRACE_MAPPER_EVENT_EEPROM (1U << TRACE_MAPPER_EEPROM)
#define TRACE_MAPPER_EVENT_SRAM   (1U << TRACE_MAPPER_SRAM)
#define TRACE_MAPPER_EVENT_ALL    (TRACE_MAPPER_EVENT_BANKS | TRACE_MAPPER_EVENT_EEPROM | TRACE_MAPPER_EVENT_SRAM)
static_assert(TRACE_MAPPER_SRAM < 32, "Mapper trace events exceed u32 width");

#define GC_VDP_EVENT_VINT        TRACE_VDP_VINT_FLAG
#define GC_VDP_EVENT_VINT_FLAG   TRACE_VDP_VINT_FLAG
#define GC_VDP_EVENT_DISPLAY     TRACE_VDP_DISPLAY_CHANGE
#define GC_VDP_EVENT_SPRITE_OVR  TRACE_VDP_SPRITE_OVERFLOW
#define GC_VDP_EVENT_SPRITE_COL  TRACE_VDP_SPRITE_COLLISION

struct GC_Trace_Entry
{
    GC_Trace_Type type;
    u64 cycle;
    union
    {
        struct
        {
            u16 pc;
            u16 bank;
            u16 af;
            u16 bc;
            u16 de;
            u16 hl;
            u16 ix;
            u16 iy;
            u16 sp;
            u8 i;
            u8 r;
            u8 im;
            u8 size;
            u8 opcodes[7];
            char name[64];
            bool iff1;
            bool iff2;
            bool halt;
        } cpu;

        struct
        {
            u16 pc;
            u16 vector;
            u8 type;
        } irq;

        struct
        {
            u8 event;
            u8 reg;
            u8 raw;
            u8 effective;
            u8 status_before;
            u8 status_after;
            u8 mode;
            u8 sprite;
            u16 address;
            u16 line;
            u16 hpos;
            u16 auxiliary;
        } vdp;

        struct
        {
            u8 value;
            u8 event;
            u8 channel;
            u8 latch;
            u8 attenuation;
            u8 noise_rate;
            bool noise_white;
            u16 period;
        } psg;

        struct
        {
            u8 event;
            u8 port;
            u8 value;
            u8 target;
        } io;

        struct
        {
            u8 event;
            u8 reg;
            u8 raw;
            u8 effective;
            u8 channel;
            u8 mixer;
            u8 amplitude;
            bool envelope_enabled;
            u16 period;
        } ay8910;

        struct
        {
            u8 event;
            u8 port;
            u8 player;
            u8 segment;
            u8 previous_segment;
            u8 previous_value;
            u8 effective_value;
            u8 gamepad;
            u8 keypad;
            u8 result;
            s16 spinner_before;
            s16 spinner_consumed;
            bool int_asserted;
        } input;

        struct
        {
            u8 event;
            u8 port;
            u8 raw;
            bool old_upper;
            bool new_upper;
            bool old_lower;
            bool new_lower;
        } sgm;

        struct
        {
            u8 event;
            u8 mapper;
            u8 value;
            u8 state;
            u16 address;
            u16 banks[4];
            u16 auxiliary;
        } mapper;
    };
};

static_assert(sizeof(GC_Trace_Entry) <= 112, "Trace entry exceeds memory budget");

class TraceLogger
{
public:
    TraceLogger(const u64* master_clock_cycles = NULL);
    ~TraceLogger();
    void Reset();
    bool SetCapacity(u32 capacity);
    INLINE bool IsEnabled(GC_Trace_Type type) const;
    INLINE bool IsEventEnabled(GC_Trace_Type type, u8 event) const;
    INLINE void TraceLog(const GC_Trace_Entry& entry);
    void SetEnabledFlags(u32 flags);
    void SetEventFilter(GC_Trace_Type type, u32 filter);
    u32 GetEnabledFlags() const;
    u32 GetEventFilter(GC_Trace_Type type) const;
    const GC_Trace_Entry* GetBuffer() const;
    u32 GetCount() const;
    u32 GetCapacity() const;
    u32 GetPosition() const;
    u64 GetTotalLogged() const;
    u64 GetSequence() const;
    const GC_Trace_Entry& GetEntry(u32 index) const;

private:
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    void UpdateEnabled();
#endif
    GC_Trace_Entry* m_buffer;
    u32 m_capacity;
    u32 m_position;
    u32 m_count;
    u32 m_enabled_flags;
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    bool m_enabled;
#endif
    u32 m_event_filters[TRACE_TYPE_COUNT];
    u64 m_total_logged;
    u64 m_sequence;
    const u64* m_master_clock_cycles;
};

INLINE bool TraceLogger::IsEnabled(GC_Trace_Type type) const
{
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    if (likely(!m_enabled))
        return false;

    return type < TRACE_TYPE_COUNT && (m_enabled_flags & (1U << type)) != 0;
#else
    UNUSED(type);
    return false;
#endif
}

INLINE bool TraceLogger::IsEventEnabled(GC_Trace_Type type, u8 event) const
{
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    if (likely(!m_enabled))
        return false;

    return type < TRACE_TYPE_COUNT && (m_enabled_flags & (1U << type)) != 0 &&
        event < 32 && (m_event_filters[type] & (1U << event)) != 0;
#else
    UNUSED(type);
    UNUSED(event);
    return false;
#endif
}

INLINE void TraceLogger::TraceLog(const GC_Trace_Entry& entry)
{
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    m_buffer[m_position] = entry;
    if (IsValidPointer(m_master_clock_cycles))
        m_buffer[m_position].cycle = *m_master_clock_cycles;
    m_position++;
    if (m_position == m_capacity)
        m_position = 0;
    if (m_count < m_capacity)
        m_count++;
    m_total_logged++;
    m_sequence++;
#else
    UNUSED(entry);
#endif
}

#endif /* TRACE_LOGGER_H */
