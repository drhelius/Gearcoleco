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

#ifndef COLECOVISIONIOPORTS_H
#define	COLECOVISIONIOPORTS_H

#include "IOPorts.h"

class Audio;
class Video;
class Input;
class Cartridge;
class Memory;
class Processor;
class TraceLogger;

class ColecoVisionIOPorts : public IOPorts
{
public:
    ColecoVisionIOPorts(Audio* pAudio, Video* pVideo, Input* pInput, Cartridge* pCartridge, Memory* pMemory, Processor* pProcessor);
    ~ColecoVisionIOPorts();
    void Reset();
    void SetTraceLogger(TraceLogger* pTraceLogger);
    u8 In(u8 port);
    void Out(u8 port, u8 value);
private:
    Audio* m_pAudio;
    Video* m_pVideo;
    Input* m_pInput;
    Cartridge* m_pCartridge;
    Memory* m_pMemory;
    Processor* m_pProcessor;
    TraceLogger* m_pTraceLogger;
};

#include "Video.h"
#include "Audio.h"
#include "Input.h"
#include "Cartridge.h"
#include "Memory.h"
#include "Processor.h"
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
#include "TraceLogger.h"
#endif

inline void ColecoVisionIOPorts::SetTraceLogger(TraceLogger* pTraceLogger)
{
    m_pTraceLogger = pTraceLogger;
}

inline u8 ColecoVisionIOPorts::In(u8 port)
{
    u8 ret = 0xFF;

    switch(port & 0xE0) {
        case 0xA0:
        {
            if (port & 0x01)
                ret = m_pVideo->GetStatusFlags();
            else
                ret = m_pVideo->GetDataPort();
            break;
        }
        case 0xE0:
        {
            ret = m_pInput->ReadInput(port);
            break;
        }
        default:
        {
            if (port == 0x52)
                ret = m_pAudio->SGMRead();
            break;
        }
    }

#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    if (m_pTraceLogger && m_pTraceLogger->IsEnabled(TRACE_IO_PORT))
    {
        GC_Trace_Entry e = {};
        e.type = TRACE_IO_PORT;
        e.io_port.port = port;
        e.io_port.value = ret;
        e.io_port.is_write = false;
        m_pTraceLogger->TraceLog(e);
    }
#endif

    return ret;
}

inline void ColecoVisionIOPorts::Out(u8 port, u8 value)
{
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    if (m_pTraceLogger && m_pTraceLogger->IsEnabled(TRACE_IO_PORT))
    {
        GC_Trace_Entry e = {};
        e.type = TRACE_IO_PORT;
        e.io_port.port = port;
        e.io_port.value = value;
        e.io_port.is_write = true;
        m_pTraceLogger->TraceLog(e);
    }
#endif

    switch(port & 0xE0) {
        case 0x80:
        {
            m_pInput->SetInputSegment(Input::SegmentKeypadRightButtons);
            break;
        }
        case 0xA0:
        {
            if (port & 0x01)
            {
                m_pVideo->WriteControl(value);
            }
            else
            {
                m_pVideo->WriteData(value);
            }
            break;
        }
        case 0xC0:
        {
            m_pInput->SetInputSegment(Input::SegmentJoystickLeftButtons);
            break;
        }
        case 0xE0:
        {
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
            if (m_pTraceLogger && m_pTraceLogger->IsEnabled(TRACE_PSG))
            {
                GC_Trace_Entry e = {};
                e.type = TRACE_PSG;
                e.psg.value = value;
                m_pTraceLogger->TraceLog(e);
            }
#endif
            m_pAudio->WriteAudioRegister(value);
            m_pProcessor->InjectTStates(32);
            break;
        }
        default:
        {
            if (port == 0x50)
            {
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
                if (m_pTraceLogger && m_pTraceLogger->IsEnabled(TRACE_AY8910))
                {
                    GC_Trace_Entry e = {};
                    e.type = TRACE_AY8910;
                    e.ay8910.reg = value;
                    e.ay8910.value = 0;
                    e.ay8910.is_write = false;
                    m_pTraceLogger->TraceLog(e);
                }
#endif
                m_pAudio->SGMRegister(value);
                break;
            }
            else if (port == 0x51)
            {
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
                if (m_pTraceLogger && m_pTraceLogger->IsEnabled(TRACE_AY8910))
                {
                    GC_Trace_Entry e = {};
                    e.type = TRACE_AY8910;
                    e.ay8910.reg = 0xFF;
                    e.ay8910.value = value;
                    e.ay8910.is_write = true;
                    m_pTraceLogger->TraceLog(e);
                }
#endif
                m_pAudio->SGMWrite(value);
                break;
            }
            else if (port == 0x53)
            {
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
                if (m_pTraceLogger && m_pTraceLogger->IsEnabled(TRACE_SGM))
                {
                    GC_Trace_Entry e = {};
                    e.type = TRACE_SGM;
                    e.sgm.port = port;
                    e.sgm.value = value;
                    m_pTraceLogger->TraceLog(e);
                }
#endif
                m_pMemory->EnableSGMUpper((value & 0x01) != 0);
            }
            else if (port == 0x7F)
            {
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
                if (m_pTraceLogger && m_pTraceLogger->IsEnabled(TRACE_SGM))
                {
                    GC_Trace_Entry e = {};
                    e.type = TRACE_SGM;
                    e.sgm.port = port;
                    e.sgm.value = value;
                    m_pTraceLogger->TraceLog(e);
                }
#endif
                m_pMemory->EnableSGMLower((~value & 0x02) != 0);
                m_pMemory->EnableSGMUpper((value & 0x01) != 0);
            }
            else
            {
                Debug("--> ** Output to port $%X: %X", port, value);
            }
        }
    }
}

#endif	/* COLECOVISIONIOPORTS_H */
