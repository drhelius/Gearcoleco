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

#ifndef INPUT_H
#define	INPUT_H

#include "definitions.h"

class Processor;
class TraceLogger;

class Input
{
public:
    enum InputSegments
    {
        SegmentKeypadRightButtons,
        SegmentJoystickLeftButtons
    };

public:
    Input(Processor* pProcessor);
    void Init();
    void Reset();
    void KeyPressed(GC_Controllers controller, GC_Keys key);
    void KeyReleased(GC_Controllers controller, GC_Keys key);
    bool IsKeyPressed(GC_Controllers controller, GC_Keys key) const;
    void Spinner1(int movement);
    void Spinner2(int movement);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream, u32 version);
    void SetTraceLogger(TraceLogger* pTraceLogger);
    void SetInputSegment(InputSegments segment);
    u8 ReadInput(u8 port);

private:
    void UpdateKeypadState(GC_Controllers controller);
    INLINE void TraceInputChangeEvent(GC_Controllers controller, GC_Keys key, bool pressed);
    INLINE void TraceInputReadEvent(u8 port, u8 result, int spinner_consumed, bool int_asserted);
    INLINE void TraceInputWriteEvent(InputSegments effective);
    void LogInputChangeEvent(GC_Controllers controller, GC_Keys key, bool pressed);
    void LogInputReadEvent(u8 port, u8 result, int spinner_consumed, bool int_asserted);
    void LogInputWriteEvent(InputSegments effective);

private:
    Processor* m_pProcessor;
    TraceLogger* m_pTraceLogger;
    u8 m_Gamepad[2];
    u8 m_Keypad[2];
    u16 m_KeypadState[2];
    InputSegments m_Segment;
    int m_iSpinnerRel[2];
};

#include "TraceLogger.h"

INLINE void Input::TraceInputChangeEvent(GC_Controllers controller, GC_Keys key, bool pressed)
{
    if (IsValidPointer(m_pTraceLogger) && m_pTraceLogger->IsEventEnabled(TRACE_INPUT, TRACE_INPUT_CHANGE))
        LogInputChangeEvent(controller, key, pressed);
}

INLINE void Input::TraceInputReadEvent(u8 port, u8 result, int spinner_consumed, bool int_asserted)
{
    if (IsValidPointer(m_pTraceLogger) && m_pTraceLogger->IsEventEnabled(TRACE_INPUT, TRACE_INPUT_READ))
        LogInputReadEvent(port, result, spinner_consumed, int_asserted);
}

INLINE void Input::TraceInputWriteEvent(InputSegments effective)
{
    if (IsValidPointer(m_pTraceLogger) && m_pTraceLogger->IsEventEnabled(TRACE_INPUT, TRACE_INPUT_WRITE))
        LogInputWriteEvent(effective);
}

#endif	/* INPUT_H */
