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

#include "ColecoVisionIOPorts.h"

ColecoVisionIOPorts::ColecoVisionIOPorts(Audio* pAudio, Video* pVideo, Input* pInput, Cartridge* pCartridge, Memory* pMemory, Processor* pProcessor)
{
    m_pAudio = pAudio;
    m_pVideo = pVideo;
    m_pInput = pInput;
    m_pCartridge = pCartridge;
    m_pMemory = pMemory;
    m_pProcessor = pProcessor;
    m_pTraceLogger = NULL;
}

ColecoVisionIOPorts::~ColecoVisionIOPorts()
{
}

void ColecoVisionIOPorts::SetVideo(Video* pVideo)
{
    m_pVideo = pVideo;
}

void ColecoVisionIOPorts::Reset()
{
}

u8 ColecoVisionIOPorts::GetIOTarget(u8 port, bool write) const
{
    switch (port & 0xE0)
    {
        case 0x80:
            return write ? TRACE_IO_TARGET_INPUT : TRACE_IO_TARGET_UNKNOWN;
        case 0xA0:
            return (port & 1) ? TRACE_IO_TARGET_VDP_STATUS : TRACE_IO_TARGET_VDP_DATA;
        case 0xC0:
            return write ? TRACE_IO_TARGET_INPUT : TRACE_IO_TARGET_UNKNOWN;
        case 0xE0:
            return write ? TRACE_IO_TARGET_PSG : TRACE_IO_TARGET_INPUT;
        default:
            if (port == 0x50)
                return TRACE_IO_TARGET_AY_SELECT;
            if (port == 0x51 || port == 0x52)
                return TRACE_IO_TARGET_AY_DATA;
            if (port == 0x53)
                return TRACE_IO_TARGET_SGM_UPPER;
            if (port == 0x7F)
                return TRACE_IO_TARGET_SGM_LOWER;
            return TRACE_IO_TARGET_UNKNOWN;
    }
}

u8 ColecoVisionIOPorts::GetAY8910WriteEvent(u8 reg) const
{
    if (reg <= 5)
        return TRACE_AY8910_TONE;
    if (reg <= 7)
        return TRACE_AY8910_NOISE_MIXER;
    if (reg <= 10)
        return TRACE_AY8910_VOLUME;
    if (reg <= 13)
        return TRACE_AY8910_ENVELOPE;
    return TRACE_AY8910_IO;
}

void ColecoVisionIOPorts::LogIOEvent(u8 event, u8 port, u8 value)
{
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    GC_Trace_Entry e = {};
    e.type = TRACE_IO;
    e.io.event = event;
    e.io.port = port;
    e.io.value = value;
    e.io.target = GetIOTarget(port, event == TRACE_IO_WRITE);
    m_pTraceLogger->TraceLog(e);
#else
    UNUSED(event);
    UNUSED(port);
    UNUSED(value);
#endif
}

void ColecoVisionIOPorts::LogPSGEvent(u8 value)
{
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    Sms_Apu_State state = m_pAudio->GetPSG()->GetState();
    int channel = (state.latch >> 5) & 3;
    u8 event = (state.latch & 0x10) ? TRACE_PSG_VOLUME : (channel == 3 ? TRACE_PSG_NOISE : TRACE_PSG_TONE);
    if (!m_pTraceLogger->IsEventEnabled(TRACE_PSG, event))
        return;
    GC_Trace_Entry e = {};
    e.type = TRACE_PSG;
    e.psg.event = event;
    e.psg.value = value;
    e.psg.channel = (u8)channel;
    e.psg.latch = (u8)state.latch;
    e.psg.attenuation = (u8)state.channels[channel].volume_reg;
    e.psg.period = channel < 3 ? (u16)(state.channels[channel].period >> 4) : 0;
    e.psg.noise_rate = (u8)state.noise_rate;
    e.psg.noise_white = state.noise_white;
    m_pTraceLogger->TraceLog(e);
#else
    UNUSED(value);
#endif
}

void ColecoVisionIOPorts::LogAY8910SelectEvent(u8 raw)
{
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    AY8910* ay = m_pAudio->GetAY8910();
    u8 reg = ay->GetSelectedRegister();
    const u8* registers = ay->GetRegisters();
    GC_Trace_Entry e = {};
    e.type = TRACE_AY8910;
    e.ay8910.event = TRACE_AY8910_SELECT;
    e.ay8910.reg = reg;
    e.ay8910.raw = raw;
    e.ay8910.effective = registers[reg & 0x0F];

    m_pTraceLogger->TraceLog(e);
#else
    UNUSED(raw);
#endif
}

void ColecoVisionIOPorts::LogAY8910ReadEvent(u8 raw)
{
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    AY8910* ay = m_pAudio->GetAY8910();
    u8 reg = ay->GetSelectedRegister();
    u8 event = reg >= 14 ? TRACE_AY8910_IO : TRACE_AY8910_READ;
    if (!m_pTraceLogger->IsEventEnabled(TRACE_AY8910, event))
        return;

    const u8* registers = ay->GetRegisters();
    GC_Trace_Entry e = {};
    e.type = TRACE_AY8910;
    e.ay8910.event = event;
    e.ay8910.reg = reg;
    e.ay8910.raw = raw;
    e.ay8910.effective = registers[reg & 0x0F];
    m_pTraceLogger->TraceLog(e);
#else
    UNUSED(raw);
#endif
}

void ColecoVisionIOPorts::LogAY8910WriteEvent(u8 raw)
{
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    AY8910* ay = m_pAudio->GetAY8910();
    u8 reg = ay->GetSelectedRegister();
    u8 event = GetAY8910WriteEvent(reg);
    if (!m_pTraceLogger->IsEventEnabled(TRACE_AY8910, event))
        return;

    const u8* registers = ay->GetRegisters();
    GC_Trace_Entry e = {};
    e.type = TRACE_AY8910;
    e.ay8910.event = event;
    e.ay8910.reg = reg;
    e.ay8910.raw = raw;
    e.ay8910.effective = registers[reg & 0x0F];

    if (reg <= 5)
    {
        e.ay8910.channel = reg >> 1;
        e.ay8910.period = ay->GetTonePeriods()[e.ay8910.channel];
    }
    else if (reg <= 7)
    {
        e.ay8910.period = ay->GetNoisePeriod();
        e.ay8910.mixer = registers[7];
    }
    else if (reg <= 10)
    {
        e.ay8910.channel = reg - 8;
        e.ay8910.amplitude = ay->GetAmplitudes()[e.ay8910.channel];
        e.ay8910.envelope_enabled = ay->GetEnvelopeMode()[e.ay8910.channel];
    }
    else if (reg <= 13)
    {
        e.ay8910.period = ay->GetEnvelopePeriod();
    }

    m_pTraceLogger->TraceLog(e);
#else
    UNUSED(raw);
#endif
}

void ColecoVisionIOPorts::LogSGMEvent(u8 port, u8 raw)
{
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    GC_Trace_Entry e = {};
    e.type = TRACE_SGM;
    e.sgm.event = TRACE_SGM_CONTROL;
    e.sgm.port = port;
    e.sgm.raw = raw;
    e.sgm.old_upper = m_pMemory->IsSGMUpperEnabled();
    e.sgm.new_upper = port == 0x53 ? (raw & 0x01) != 0 : e.sgm.old_upper;
    e.sgm.old_lower = m_pMemory->IsSGMLowerEnabled();
    e.sgm.new_lower = port == 0x7F ? (~raw & 0x02) != 0 : e.sgm.old_lower;
    m_pTraceLogger->TraceLog(e);
#else
    UNUSED(port);
    UNUSED(raw);
#endif
}
