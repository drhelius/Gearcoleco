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
    void LoadStateV1(std::istream& stream);
    bool StartVgmRecording(const char* file_path, int clock_rate, bool is_pal);
    void StopVgmRecording();
    bool IsVgmRecording() const;
    void EnablePSGDebug(bool enable);
    bool IsPSGDebugEnabled();
    blip_sample_t* GetDebugChannelBuffer(int channel);
    int GetDebugChannelSamples(int channel);
    void EnableAY8910Debug(bool enable);
    bool IsAY8910DebugEnabled();
    s16* GetAY8910DebugChannelBuffer(int channel);
    int GetAY8910DebugChannelSamples(int channel);
    Sms_Apu* GetPSG() { return m_pApu; }
    AY8910* GetAY8910() { return m_pAY8910; }

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
    blip_sample_t* m_pDebugChannelBuffer[4];
    long m_iDebugChannelSamples[4];
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

inline void Audio::EnablePSGDebug(bool enable)
{
    if (enable && !m_pApu->is_debug_enabled())
    {
        long clock = m_bPAL ? GC_MASTER_CLOCK_PAL : GC_MASTER_CLOCK_NTSC;
        m_pApu->init_debug_buffers(m_iSampleRate, clock);
    }
    else if (!enable && m_pApu->is_debug_enabled())
    {
        m_pApu->disable_debug_buffers();
    }
}

inline bool Audio::IsPSGDebugEnabled()
{
    return m_pApu->is_debug_enabled();
}

inline blip_sample_t* Audio::GetDebugChannelBuffer(int channel)
{
    if (channel < 0 || channel >= 4)
        return NULL;
    return m_pDebugChannelBuffer[channel];
}

inline int Audio::GetDebugChannelSamples(int channel)
{
    if (channel < 0 || channel >= 4)
        return 0;
    return (int)m_iDebugChannelSamples[channel];
}

inline void Audio::EnableAY8910Debug(bool enable)
{
    m_pAY8910->EnableDebug(enable);
}

inline bool Audio::IsAY8910DebugEnabled()
{
    return m_pAY8910->IsDebugEnabled();
}

inline s16* Audio::GetAY8910DebugChannelBuffer(int channel)
{
    return m_pAY8910->GetDebugChannelBuffer(channel);
}

inline int Audio::GetAY8910DebugChannelSamples(int channel)
{
    return m_pAY8910->GetDebugChannelSamples(channel);
}

#endif	/* AUDIO_H */
