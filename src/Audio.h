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

#ifndef AUDIO_H
#define	AUDIO_H

#include "definitions.h"
#include "audio/Multi_Buffer.h"
#include "audio/Sms_Apu.h"
#include "AY8910.h"
#include "VgmRecorder.h"

class Audio
{
public:
    Audio();
    ~Audio();
    void Init();
    void Reset(bool bPAL);
    void Mute(bool mute);
    void WriteAudioRegister(u8 value);
    void SGMWrite(u8 value);
    u8 SGMRead();
    void SGMRegister(u8 reg);
    void Tick(unsigned int clockCycles);
    void EndFrame(s16* pSampleBuffer, int* pSampleCount);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);
    bool StartVgmRecording(const char* file_path, int clock_rate, bool is_pal);
    void StopVgmRecording();
    bool IsVgmRecording() const;

private:
    Sms_Apu* m_pApu;
    Stereo_Buffer* m_pBuffer;
    AY8910* m_pAY8910;
    u64 m_ElapsedCycles;
    int m_iSampleRate;
    blip_sample_t* m_pSampleBuffer;
    bool m_bPAL;
    s16* m_pSGMBuffer;
    bool m_bMute;
    VgmRecorder m_VgmRecorder;
    bool m_bVgmRecordingEnabled;
    u8 m_AY8910Register;
};

inline void Audio::Tick(unsigned int clockCycles)
{
    m_ElapsedCycles += clockCycles;
    m_pAY8910->Tick(clockCycles);
}

inline void Audio::WriteAudioRegister(u8 value)
{
    m_pApu->write_data((blip_time_t)m_ElapsedCycles, value);
#ifndef GEARCOLECO_DISABLE_VGMRECORDER
    if (m_bVgmRecordingEnabled)
        m_VgmRecorder.WritePSG(value);
#endif
}

inline void Audio::SGMWrite(u8 value)
{
    m_pAY8910->WriteRegister(value);
#ifndef GEARCOLECO_DISABLE_VGMRECORDER
    if (m_bVgmRecordingEnabled)
        m_VgmRecorder.WriteAY8910(m_AY8910Register, value);
#endif
}

inline u8 Audio::SGMRead()
{
    return m_pAY8910->ReadRegister();
}

inline void Audio::SGMRegister(u8 reg)
{
    m_pAY8910->SelectRegister(reg);
    m_AY8910Register = reg;
}

#endif	/* AUDIO_H */
