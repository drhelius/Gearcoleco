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

#include "Audio.h"

Audio::Audio()
{
    m_ElapsedCycles = 0;
    m_iSampleRate = GC_AUDIO_SAMPLE_RATE;
    InitPointer(m_pApu);
    InitPointer(m_pBuffer);
    InitPointer(m_pSampleBuffer);
    m_bPAL = false;
    InitPointer(m_pAY8910);
    InitPointer(m_pSGMBuffer);
    m_bMute = false;
}

Audio::~Audio()
{
    SafeDelete(m_pApu);
    SafeDelete(m_pBuffer);
    SafeDeleteArray(m_pSampleBuffer);
    SafeDelete(m_pAY8910);
    SafeDeleteArray(m_pSGMBuffer);
}

void Audio::Init()
{
    m_pSampleBuffer = new blip_sample_t[GC_AUDIO_BUFFER_SIZE];

    m_pApu = new Sms_Apu();
    m_pBuffer = new Stereo_Buffer();

    m_pBuffer->clock_rate(m_bPAL ? GC_MASTER_CLOCK_PAL : GC_MASTER_CLOCK_NTSC);
    m_pBuffer->set_sample_rate(m_iSampleRate);

    //m_pApu->treble_eq(-15.0);
    //m_pBuffer->bass_freq(100);

    m_pApu->output(m_pBuffer->center(), m_pBuffer->left(), m_pBuffer->right());
    m_pApu->volume(0.6);

    m_pSGMBuffer = new s16[GC_AUDIO_BUFFER_SIZE];

    m_pAY8910 = new AY8910();
    m_pAY8910->Init(m_bPAL ? GC_MASTER_CLOCK_PAL : GC_MASTER_CLOCK_NTSC);
}

void Audio::Reset(bool bPAL)
{
    m_bPAL = bPAL;
    m_pApu->reset();
    m_pApu->volume(0.6);
    m_pBuffer->clear();
    m_pBuffer->clock_rate(m_bPAL ? GC_MASTER_CLOCK_PAL : GC_MASTER_CLOCK_NTSC);
    m_ElapsedCycles = 0;
    m_pAY8910->Reset(m_bPAL ? GC_MASTER_CLOCK_PAL : GC_MASTER_CLOCK_NTSC);
}

void Audio::Mute(bool mute)
{
    m_bMute = mute;
}

void Audio::EndFrame(s16* pSampleBuffer, int* pSampleCount)
{
    m_pApu->end_frame(m_ElapsedCycles);
    m_pBuffer->end_frame(m_ElapsedCycles);

    int count = static_cast<int>(m_pBuffer->read_samples(m_pSampleBuffer, GC_AUDIO_BUFFER_SIZE));

    m_pAY8910->EndFrame(m_pSGMBuffer);

    if (IsValidPointer(pSampleBuffer) && IsValidPointer(pSampleCount))
    {
        *pSampleCount = count;

        for (int i=0; i<count; i++)
        {
            if (m_bMute)
                pSampleBuffer[i] = 0;
            else
                pSampleBuffer[i] = m_pSampleBuffer[i] + m_pSGMBuffer[i];
        }
    }

    m_ElapsedCycles = 0;
}

void Audio::SaveState(std::ostream& stream)
{
    stream.write(reinterpret_cast<const char*> (&m_ElapsedCycles), sizeof(m_ElapsedCycles));
    stream.write(reinterpret_cast<const char*> (m_pSampleBuffer), sizeof(blip_sample_t) * GC_AUDIO_BUFFER_SIZE);
    stream.write(reinterpret_cast<const char*> (m_pSGMBuffer), sizeof(s16) * GC_AUDIO_BUFFER_SIZE);
    m_pAY8910->SaveState(stream);
}

void Audio::LoadState(std::istream& stream)
{
    stream.read(reinterpret_cast<char*> (&m_ElapsedCycles), sizeof(m_ElapsedCycles));
    stream.read(reinterpret_cast<char*> (m_pSampleBuffer), sizeof(blip_sample_t) * GC_AUDIO_BUFFER_SIZE);
    stream.read(reinterpret_cast<char*> (m_pSGMBuffer), sizeof(s16) * GC_AUDIO_BUFFER_SIZE);
    m_pAY8910->LoadState(stream);

    m_pApu->reset();
    m_pApu->volume(0.6);
    m_pBuffer->clear();
}
