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
    void SetVideo(Video* pVideo);
    void SetTraceLogger(TraceLogger* pTraceLogger);
    u8 In(u8 port);
    void Out(u8 port, u8 value);
private:
    INLINE void TraceIOEvent(u8 event, u8 port, u8 value);
    INLINE void TracePSGEvent(u8 value);
    INLINE void TraceAY8910SelectEvent(u8 raw);
    INLINE void TraceAY8910ReadEvent(u8 raw);
    INLINE void TraceAY8910WriteEvent(u8 raw);
    INLINE void TraceSGMEvent(u8 port, u8 raw);
    void LogIOEvent(u8 event, u8 port, u8 value);
    void LogPSGEvent(u8 value);
    void LogAY8910SelectEvent(u8 raw);
    void LogAY8910ReadEvent(u8 raw);
    void LogAY8910WriteEvent(u8 raw);
    void LogSGMEvent(u8 port, u8 raw);
    u8 GetIOTarget(u8 port, bool write) const;
    u8 GetAY8910WriteEvent(u8 reg) const;
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
#include "TraceLogger.h"

inline void ColecoVisionIOPorts::SetTraceLogger(TraceLogger* pTraceLogger)
{
    m_pTraceLogger = pTraceLogger;
}

INLINE void ColecoVisionIOPorts::TraceIOEvent(u8 event, u8 port, u8 value)
{
    if (IsValidPointer(m_pTraceLogger) && m_pTraceLogger->IsEventEnabled(TRACE_IO, event))
        LogIOEvent(event, port, value);
}

INLINE void ColecoVisionIOPorts::TracePSGEvent(u8 value)
{
    if (IsValidPointer(m_pTraceLogger) && m_pTraceLogger->IsEnabled(TRACE_PSG))
        LogPSGEvent(value);
}

INLINE void ColecoVisionIOPorts::TraceAY8910SelectEvent(u8 raw)
{
    if (IsValidPointer(m_pTraceLogger) && m_pTraceLogger->IsEventEnabled(TRACE_AY8910, TRACE_AY8910_SELECT))
        LogAY8910SelectEvent(raw);
}

INLINE void ColecoVisionIOPorts::TraceAY8910ReadEvent(u8 raw)
{
    if (IsValidPointer(m_pTraceLogger) && m_pTraceLogger->IsEnabled(TRACE_AY8910))
        LogAY8910ReadEvent(raw);
}

INLINE void ColecoVisionIOPorts::TraceAY8910WriteEvent(u8 raw)
{
    if (IsValidPointer(m_pTraceLogger) && m_pTraceLogger->IsEnabled(TRACE_AY8910))
        LogAY8910WriteEvent(raw);
}

INLINE void ColecoVisionIOPorts::TraceSGMEvent(u8 port, u8 raw)
{
    if (IsValidPointer(m_pTraceLogger) && m_pTraceLogger->IsEventEnabled(TRACE_SGM, TRACE_SGM_CONTROL))
        LogSGMEvent(port, raw);
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
            {
                ret = m_pAudio->SGMRead();
                TraceAY8910ReadEvent(ret);
            }
            break;
        }
    }

    TraceIOEvent(TRACE_IO_READ, port, ret);

    return ret;
}

inline void ColecoVisionIOPorts::Out(u8 port, u8 value)
{
    TraceIOEvent(TRACE_IO_WRITE, port, value);

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
            m_pAudio->WriteAudioRegister(value);
            TracePSGEvent(value);
            m_pProcessor->InjectTStates(32);
            break;
        }
        default:
        {
            if (port == 0x50)
            {
                m_pAudio->SGMRegister(value);
                TraceAY8910SelectEvent(value);
                break;
            }
            else if (port == 0x51)
            {
                m_pAudio->SGMWrite(value);
                TraceAY8910WriteEvent(value);
                break;
            }
            else if (port == 0x53)
            {
                TraceSGMEvent(port, value);
                m_pMemory->EnableSGMUpper((value & 0x01) != 0);
            }
            else if (port == 0x7F)
            {
                TraceSGMEvent(port, value);
                m_pMemory->EnableSGMLower((~value & 0x02) != 0);
            }
            else
            {
                Debug("--> ** Output to port $%X: %X", port, value);
            }
        }
    }
}

#endif	/* COLECOVISIONIOPORTS_H */
