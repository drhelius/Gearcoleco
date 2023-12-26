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
    void Spinner1(int movement);
    void Spinner2(int movement);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);
    void SetInputSegment(InputSegments segment);
    u8 ReadInput(u8 port);

private:
    Processor* m_pProcessor;
    u8 m_Gamepad[2];
    u8 m_Keypad[2];
    InputSegments m_Segment;
    int m_iSpinnerRel[2];
};

#endif	/* INPUT_H */
