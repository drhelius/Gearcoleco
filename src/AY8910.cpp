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

#include "AY8910.h"

AY8910::AY8910()
{
    InitPointer(m_pBuffer);
}

AY8910::~AY8910()
{
    SafeDeleteArray(m_pBuffer);
}

void AY8910::Init(int clockRate)
{
    m_pBuffer = new s16[GC_AUDIO_BUFFER_SIZE];
    Reset(clockRate);
}

void AY8910::Reset(int clockRate)
{
    m_iClockRate = clockRate;

    for (int i = 0; i < 16; i++)
    {
        m_Registers[i] = 0;
    }

    for (int i = 0; i < 3; i++)
    {
        m_TonePeriod[i] = 0;
        m_ToneCounter[i] = 0;
        m_Amplitude[i] = 0;
        m_ToneDisable[i] = false;
        m_NoiseDisable[i] = false;
        m_EnvelopeMode[i] = false;
        m_Sign[i] = false;
    }

    m_SelectedRegister = 0;
    m_NoisePeriod = 0;
    m_NoiseCounter = 0;
    m_NoiseShift = 1;
    m_EnvelopePeriod = 0;
    m_EnvelopeCounter = 0;
    m_EnvelopeSegment = false;
    m_EnvelopeStep = 0;
    m_EnvelopeVolume = 0;
    m_iCycleCounter = 0;
    m_iSampleCounter = 0;
    m_iBufferIndex = 0;

    for (int i = 0; i < GC_AUDIO_BUFFER_SIZE; i++)
    {
        m_pBuffer[i] = 0;
    }

    m_ElapsedCycles = 0;
}

void AY8910::WriteRegister(u8 value)
{
    Sync();
    
    m_Registers[m_SelectedRegister] = value & kAY8910RegisterMask[m_SelectedRegister];

    switch (m_SelectedRegister)
    {
        // Channel A tone period
        case 0:
        case 1:
        {
            m_TonePeriod[0] = (m_Registers[1] << 8) | m_Registers[0];
            if (m_TonePeriod[0] == 0)
            {
                m_TonePeriod[0] = 1;
            }
            break;
        }
        // Channel B tone period
        case 2:
        case 3:
        {
            m_TonePeriod[1] = (m_Registers[3] << 8) | m_Registers[2];
            if (m_TonePeriod[1] == 0)
            {
                m_TonePeriod[1] = 1;
            }
            break;
        }
        // Channel C tone period
        case 4:
        case 5:
        {
            m_TonePeriod[2] = (m_Registers[5] << 8) | m_Registers[4];
            if (m_TonePeriod[2] == 0)
            {
                m_TonePeriod[2] = 1;
            }
            break;
        }
        // Noise period
        case 6:
        {
            m_NoisePeriod = m_Registers[6];
            if (m_NoisePeriod == 0)
            {
                m_NoisePeriod = 1;
            }
            break;
        }
        // Mixer
        case 7:
        {
            m_ToneDisable[0] = IsSetBit(m_Registers[7], 0);
            m_ToneDisable[1] = IsSetBit(m_Registers[7], 1);
            m_ToneDisable[2] = IsSetBit(m_Registers[7], 2);
            m_NoiseDisable[0] = IsSetBit(m_Registers[7], 3);
            m_NoiseDisable[1] = IsSetBit(m_Registers[7], 4);
            m_NoiseDisable[2] = IsSetBit(m_Registers[7], 5);
            break;
        }
        // Channel A amplitude
        case 8:
        {
            m_Amplitude[0] = m_Registers[8] & 0x0F;
            m_EnvelopeMode[0] = IsSetBit(m_Registers[8], 4);
            break;
        }
        // Channel B amplitude
        case 9:
        {
            m_Amplitude[1] = m_Registers[9] & 0x0F;
            m_EnvelopeMode[1] = IsSetBit(m_Registers[9], 4);
            break;
        }
        // Channel C amplitude
        case 10:
        {
            m_Amplitude[2] = m_Registers[10] & 0x0F;
            m_EnvelopeMode[2] = IsSetBit(m_Registers[10], 4);
            break;
        }
        // Envelope period
        case 11:
        case 12:
        {
            m_EnvelopePeriod = (m_Registers[12] << 8) | m_Registers[11];
            break;
        }
        // Envelope shape
        case 13:
        {
            m_EnvelopeCounter = 0;
            m_EnvelopeSegment = false;
            EnvelopeReset();
            break;
        }
        default:
        {
            break;
        }
    }
}

u8 AY8910::ReadRegister()
{
    return m_Registers[m_SelectedRegister];
}

void AY8910::SelectRegister(u8 reg)
{
    m_SelectedRegister = reg & 0x0F;
}

void AY8910::EnvelopeReset()
{
    m_EnvelopeStep = 0;

    if (m_EnvelopeSegment)
    {
        switch (m_Registers[13])
        {
            case 8:
            case 11:
            case 13:
            case 14:
            {
                m_EnvelopeVolume = 0x0F;
                break;
            }
            default:
            {
                m_EnvelopeVolume = 0x00;
                break;
            }
        }
    }
    else
    {
        m_EnvelopeVolume = IsSetBit(m_Registers[13], 2) ? 0x00 : 0x0F;
    }
}

void AY8910::Tick(unsigned int clockCycles)
{
    m_ElapsedCycles += clockCycles;
}

void AY8910::Sync()
{
    for (int i = 0; i < m_ElapsedCycles; i++)
    {
        m_iCycleCounter ++;
        if (m_iCycleCounter >= 16)
        {
            m_iCycleCounter -= 16;

            for (int i = 0; i < 3; i++)
            {
                m_ToneCounter[i]++;
                if (m_ToneCounter[i] >= m_TonePeriod[i])
                {
                    m_ToneCounter[i] = 0;
                    m_Sign[i] = !m_Sign[i];
                }
            }

            m_NoiseCounter++;
            if (m_NoiseCounter >= (m_NoisePeriod << 1))
            {
                m_NoiseCounter = 0;
                m_NoiseShift = (m_NoiseShift >> 1) | (((m_NoiseShift ^ (m_NoiseShift >> 3)) & 0x01) << 16);
            }

            m_EnvelopeCounter++;
            if (m_EnvelopeCounter >= (m_EnvelopePeriod << 1))
            {
                m_EnvelopeCounter = 0;

                if (m_EnvelopeStep)
                {
                    if (m_EnvelopeSegment)
                    {
                        if ((m_Registers[13] == 10) || (m_Registers[13] == 12))
                        {
                            m_EnvelopeVolume++;
                        }
                        else if ((m_Registers[13] == 8) || (m_Registers[13] == 14))
                        {
                            m_EnvelopeVolume--;
                        }
                    }
                    else
                    {
                        if (IsSetBit(m_Registers[13], 2))
                        {
                            m_EnvelopeVolume++;
                        }
                        else
                        {
                            m_EnvelopeVolume--;
                        }
                    }
                }

                m_EnvelopeStep++;
                if (m_EnvelopeStep >= 16)
                {
                    if ((m_Registers[13] & 0x09) == 0x08)
                    {
                        m_EnvelopeSegment = !m_EnvelopeSegment;
                    }
                    else
                    {
                        m_EnvelopeSegment = true;
                    }
                    EnvelopeReset();
                }
            }
        }

        m_iSampleCounter++;
        int cyclesPerSample = m_iClockRate / GC_AUDIO_SAMPLE_RATE;
        if (m_iSampleCounter >= cyclesPerSample)
        {
            m_iSampleCounter -= cyclesPerSample;
            s16 sample = 0;

            for (int i = 0; i < 3; i++)
            {
                if ((m_ToneDisable[i] || m_Sign[i]) && (m_NoiseDisable[i] || ((m_NoiseShift & 0x01) == 0x01)))
                {
                    sample += m_EnvelopeMode[i] ? kAY8910VolumeTable[m_EnvelopeVolume] : kAY8910VolumeTable[m_Amplitude[i]];
                }
            }

            m_pBuffer[m_iBufferIndex] = sample;
            m_pBuffer[m_iBufferIndex + 1] = sample;
            m_iBufferIndex += 2;

            if (m_iBufferIndex >= GC_AUDIO_BUFFER_SIZE)
            {
                Log("SGM Audio buffer overflow");
                m_iBufferIndex = 0;
            }
        }
    }

    m_ElapsedCycles = 0;
}

int AY8910::EndFrame(s16* pSampleBuffer)
{
    Sync();

    int ret = 0;

    if (IsValidPointer(pSampleBuffer))
    {
        ret = m_iBufferIndex;

        for (int i = 0; i < m_iBufferIndex; i++)
        {
            pSampleBuffer[i] = m_pBuffer[i];
        }
    }

    m_iBufferIndex = 0;

    return ret;
}

void AY8910::SaveState(std::ostream& stream)
{
    stream.write(reinterpret_cast<const char*>(m_Registers), sizeof(m_Registers));
    stream.write(reinterpret_cast<const char*>(&m_SelectedRegister), sizeof(m_SelectedRegister));
    stream.write(reinterpret_cast<const char*>(m_TonePeriod), sizeof(m_TonePeriod));
    stream.write(reinterpret_cast<const char*>(m_ToneCounter), sizeof(m_ToneCounter));
    stream.write(reinterpret_cast<const char*>(m_Amplitude), sizeof(m_Amplitude));
    stream.write(reinterpret_cast<const char*>(&m_NoisePeriod), sizeof(m_NoisePeriod));
    stream.write(reinterpret_cast<const char*>(&m_NoiseCounter), sizeof(m_NoiseCounter));
    stream.write(reinterpret_cast<const char*>(&m_NoiseShift), sizeof(m_NoiseShift));
    stream.write(reinterpret_cast<const char*>(&m_EnvelopePeriod), sizeof(m_EnvelopePeriod));
    stream.write(reinterpret_cast<const char*>(&m_EnvelopeCounter), sizeof(m_EnvelopeCounter));
    stream.write(reinterpret_cast<const char*>(&m_EnvelopeSegment), sizeof(m_EnvelopeSegment));
    stream.write(reinterpret_cast<const char*>(&m_EnvelopeStep), sizeof(m_EnvelopeStep));
    stream.write(reinterpret_cast<const char*>(&m_EnvelopeVolume), sizeof(m_EnvelopeVolume));
    stream.write(reinterpret_cast<const char*>(m_ToneDisable), sizeof(m_ToneDisable));
    stream.write(reinterpret_cast<const char*>(m_NoiseDisable), sizeof(m_NoiseDisable));
    stream.write(reinterpret_cast<const char*>(m_EnvelopeMode), sizeof(m_EnvelopeMode));
    stream.write(reinterpret_cast<const char*>(m_Sign), sizeof(m_Sign));
    stream.write(reinterpret_cast<const char*>(&m_iCycleCounter), sizeof(m_iCycleCounter));
    stream.write(reinterpret_cast<const char*>(&m_iSampleCounter), sizeof(m_iSampleCounter));
    stream.write(reinterpret_cast<const char*>(m_pBuffer), GC_AUDIO_BUFFER_SIZE * sizeof(s16));
    stream.write(reinterpret_cast<const char*>(&m_iBufferIndex), sizeof(m_iBufferIndex));
}

void AY8910::LoadState(std::istream& stream)
{
    stream.read(reinterpret_cast<char*>(m_Registers), sizeof(m_Registers));
    stream.read(reinterpret_cast<char*>(&m_SelectedRegister), sizeof(m_SelectedRegister));
    stream.read(reinterpret_cast<char*>(m_TonePeriod), sizeof(m_TonePeriod));
    stream.read(reinterpret_cast<char*>(m_ToneCounter), sizeof(m_ToneCounter));
    stream.read(reinterpret_cast<char*>(m_Amplitude), sizeof(m_Amplitude));
    stream.read(reinterpret_cast<char*>(&m_NoisePeriod), sizeof(m_NoisePeriod));
    stream.read(reinterpret_cast<char*>(&m_NoiseCounter), sizeof(m_NoiseCounter));
    stream.read(reinterpret_cast<char*>(&m_NoiseShift), sizeof(m_NoiseShift));
    stream.read(reinterpret_cast<char*>(&m_EnvelopePeriod), sizeof(m_EnvelopePeriod));
    stream.read(reinterpret_cast<char*>(&m_EnvelopeCounter), sizeof(m_EnvelopeCounter));
    stream.read(reinterpret_cast<char*>(&m_EnvelopeSegment), sizeof(m_EnvelopeSegment));
    stream.read(reinterpret_cast<char*>(&m_EnvelopeStep), sizeof(m_EnvelopeStep));
    stream.read(reinterpret_cast<char*>(&m_EnvelopeVolume), sizeof(m_EnvelopeVolume));
    stream.read(reinterpret_cast<char*>(m_ToneDisable), sizeof(m_ToneDisable));
    stream.read(reinterpret_cast<char*>(m_NoiseDisable), sizeof(m_NoiseDisable));
    stream.read(reinterpret_cast<char*>(m_EnvelopeMode), sizeof(m_EnvelopeMode));
    stream.read(reinterpret_cast<char*>(m_Sign), sizeof(m_Sign));
    stream.read(reinterpret_cast<char*>(&m_iCycleCounter), sizeof(m_iCycleCounter));
    stream.read(reinterpret_cast<char*>(&m_iSampleCounter), sizeof(m_iSampleCounter));
    stream.read(reinterpret_cast<char*>(m_pBuffer), GC_AUDIO_BUFFER_SIZE * sizeof(s16));
    stream.read(reinterpret_cast<char*>(&m_iBufferIndex), sizeof(m_iBufferIndex));
}