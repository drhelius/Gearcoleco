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

#include "Input.h"
#include "Processor.h"
#include "TraceLogger.h"

Input::Input(Processor* pProcessor)
{
    m_pProcessor = pProcessor;
    InitPointer(m_pTraceLogger);
}

void Input::Init()
{
    Reset();
}

void Input::Reset()
{
    m_Segment = SegmentKeypadRightButtons;
    m_Gamepad[0] = m_Gamepad[1] = 0xFF;
    m_Keypad[0] = m_Keypad[1] = 0xFF;
    m_KeypadState[0] = m_KeypadState[1] = 0;
    m_iSpinnerRel[0] = m_iSpinnerRel[1] = 0;
}

void Input::SetInputSegment(InputSegments segment)
{
    TraceInputWriteEvent(segment);
    m_Segment = segment;
}

u8 Input::ReadInput(u8 port)
{
    u8 c = (port & 0x02) >> 1;
    u8 ret = 0xFF;

    int rel = m_iSpinnerRel[c] / 4;
    m_iSpinnerRel[c] -= rel;
    bool int_asserted = false;

    if (m_Segment == SegmentKeypadRightButtons)
    {
        ret = (m_Keypad[c] & 0x0F) | (IsSetBit(m_Gamepad[c], 5) ? 0x70 : 0x30);
    }
    else
    {
        ret = (m_Gamepad[c] & 0x0F) | (IsSetBit(m_Gamepad[c], 4) ? 0x70 : 0x30);

        if (rel > 0)
        {
            ret &= c ? 0xEF : 0xCF;
            m_pProcessor->RequestINT(true);
            int_asserted = true;
        }
        else if (rel < 0)
        {
            ret &= c ? 0xCF : 0xEF;
            m_pProcessor->RequestINT(true);
            int_asserted = true;
        }
    }

    TraceInputReadEvent(port, ret, rel, int_asserted);

    return ret;
}

void Input::KeyPressed(GC_Controllers controller, GC_Keys key)
{
    TraceInputChangeEvent(controller, key, true);
    if (key > 0x0F)
    {
        m_Gamepad[controller] = UnsetBit(m_Gamepad[controller], key & 0x0F);
    }
    else
    {
        m_KeypadState[controller] |= (1 << (key & 0x0F));
        UpdateKeypadState(controller);
    }
}

void Input::KeyReleased(GC_Controllers controller, GC_Keys key)
{
    TraceInputChangeEvent(controller, key, false);
    if (key > 0x0F)
    {
        m_Gamepad[controller] = SetBit(m_Gamepad[controller], key & 0x0F);
    }
    else
    {
        m_KeypadState[controller] &= ~(1 << (key & 0x0F));
        UpdateKeypadState(controller);
    }
}

void Input::SetTraceLogger(TraceLogger* pTraceLogger)
{
    m_pTraceLogger = pTraceLogger;
}

void Input::LogInputChangeEvent(GC_Controllers controller, GC_Keys key, bool pressed)
{
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    u8 previous = key > 0x0F ? m_Gamepad[controller] : m_Keypad[controller];
    u8 effective;
    if (key > 0x0F)
        effective = pressed ? UnsetBit(previous, key & 0x0F) : SetBit(previous, key & 0x0F);
    else
    {
        u16 keypad_state = m_KeypadState[controller];
        if (pressed)
            keypad_state |= (1 << (key & 0x0F));
        else
            keypad_state &= ~(1 << (key & 0x0F));
        effective = 0xFF;
        for (int i = 0; i < 16; i++)
        {
            if (keypad_state & (1 << i))
                effective &= i;
        }
    }
    if (previous == effective)
        return;
    GC_Trace_Entry e = {};
    e.type = TRACE_INPUT;
    e.input.event = TRACE_INPUT_CHANGE;
    e.input.port = (u8)key;
    e.input.player = (u8)controller + 1;
    e.input.segment = (u8)m_Segment;
    e.input.previous_value = previous;
    e.input.effective_value = effective;
    m_pTraceLogger->TraceLog(e);
#else
    UNUSED(controller);
    UNUSED(key);
    UNUSED(pressed);
#endif
}

void Input::LogInputReadEvent(u8 port, u8 result, int spinner_consumed, bool int_asserted)
{
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    u8 controller = (port & 0x02) >> 1;
    GC_Trace_Entry e = {};
    e.type = TRACE_INPUT;
    e.input.event = TRACE_INPUT_READ;
    e.input.port = port;
    e.input.player = controller + 1;
    e.input.segment = (u8)m_Segment;
    e.input.gamepad = m_Gamepad[controller];
    e.input.keypad = m_Keypad[controller];
    e.input.result = result;
    e.input.spinner_before = (s16)(m_iSpinnerRel[controller] + spinner_consumed);
    e.input.spinner_consumed = (s16)spinner_consumed;
    e.input.int_asserted = int_asserted;
    m_pTraceLogger->TraceLog(e);
#else
    UNUSED(port);
    UNUSED(result);
    UNUSED(spinner_consumed);
    UNUSED(int_asserted);
#endif
}

void Input::LogInputWriteEvent(InputSegments effective)
{
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    if (m_Segment == effective)
        return;
    GC_Trace_Entry e = {};
    e.type = TRACE_INPUT;
    e.input.event = TRACE_INPUT_WRITE;
    e.input.segment = (u8)effective;
    e.input.previous_segment = (u8)m_Segment;
    m_pTraceLogger->TraceLog(e);
#else
    UNUSED(effective);
#endif
}

bool Input::IsKeyPressed(GC_Controllers controller, GC_Keys key) const
{
    int bit = key & 0x0F;
    return key > 0x0F ? !(m_Gamepad[controller] & (1 << bit)) : (m_KeypadState[controller] & (1 << bit)) != 0;
}

void Input::UpdateKeypadState(GC_Controllers controller)
{
    m_Keypad[controller] = 0xFF;
    for (int i = 0; i < 16; i++)
    {
        if (m_KeypadState[controller] & (1 << i))
            m_Keypad[controller] &= i;
    }
}

void Input::Spinner1(int movement)
{
    m_iSpinnerRel[0] = movement;
}

void Input::Spinner2(int movement)
{
    m_iSpinnerRel[1] = movement;
}

void Input::SaveState(std::ostream& stream)
{
    stream.write(reinterpret_cast<const char*> (m_Gamepad), sizeof(m_Gamepad));
    stream.write(reinterpret_cast<const char*> (m_Keypad), sizeof(m_Keypad));
    stream.write(reinterpret_cast<const char*> (m_KeypadState), sizeof(m_KeypadState));
    stream.write(reinterpret_cast<const char*> (&m_Segment), sizeof(m_Segment));
    stream.write(reinterpret_cast<const char*> (m_iSpinnerRel), sizeof(m_iSpinnerRel));
}

void Input::LoadState(std::istream& stream, u32 version)
{
    stream.read(reinterpret_cast<char*> (m_Gamepad), sizeof(m_Gamepad));
    stream.read(reinterpret_cast<char*> (m_Keypad), sizeof(m_Keypad));

    if (version >= 102)
    {
        stream.read(reinterpret_cast<char*> (m_KeypadState), sizeof(m_KeypadState));
    }
    else
    {
        m_KeypadState[0] = m_KeypadState[1] = 0;
    }

    stream.read(reinterpret_cast<char*> (&m_Segment), sizeof(m_Segment));
    stream.read(reinterpret_cast<char*> (m_iSpinnerRel), sizeof(m_iSpinnerRel));
}
