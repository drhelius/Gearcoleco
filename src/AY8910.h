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

#ifndef AY8910_H
#define	AY8910_H

#include "definitions.h"

class AY8910
{
public:
    AY8910();
    ~AY8910();
    void Init(int clockRate);
    void Reset(int clockRate);
    void WriteRegister(u8 value);
    u8 ReadRegister();
    void SelectRegister(u8 reg);
    void Tick(unsigned int clockCycles);
    int EndFrame(s16* pSampleBuffer);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

private:
    void EnvelopeReset();
    void Sync();

private:
    u8 m_Registers[16];
    u8 m_SelectedRegister;
    u16 m_TonePeriod[3];
    u16 m_ToneCounter[3];
    u8 m_Amplitude[3];
    u8 m_NoisePeriod;
    u16 m_NoiseCounter;
    u32 m_NoiseShift;
    u16 m_EnvelopePeriod;
    u16 m_EnvelopeCounter;
    bool m_EnvelopeSegment;
    u8 m_EnvelopeStep;
    u8 m_EnvelopeVolume;
    bool m_ToneDisable[3];
    bool m_NoiseDisable[3];
    bool m_EnvelopeMode[3];
    bool m_Sign[3];
    int m_iCycleCounter;
    int m_iSampleCounter;
    int m_iCyclesPerSample;
    s16* m_pBuffer;
    int m_iBufferIndex;
    int m_ElapsedCycles;
    int m_iClockRate;
    s16 m_CurrentSample;
};

const u8 kAY8910RegisterMask[16] = {0xFF, 0x0F, 0xFF, 0x0F, 0xFF, 0x0F, 0x1F, 0xFF, 0x1F, 0x1F, 0x1F, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF};
const s16 kAY8910VolumeTable[16] = {0, 40, 60, 86, 124, 186, 264, 440, 518, 840, 1196, 1526, 2016, 2602, 3300, 4096};

#endif	/* AY8910_H */