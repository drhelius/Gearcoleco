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

#ifndef CORE_H
#define	CORE_H

#include "definitions.h"
#include "Cartridge.h"

class Memory;
class Processor;
class Audio;
class Video;
class TMS9918A;
class F18A;
class Input;
class ColecoVisionIOPorts;
class Random;
class TraceLogger;
class Adam;
class AdamMedia;

class GearcolecoCore
{

public:
    struct GC_Debug_Run
    {
        bool step_debugger;
        bool stop_on_breakpoint;
        bool stop_on_run_to_breakpoint;
        bool stop_on_irq;
    };

public:
    GearcolecoCore();
    ~GearcolecoCore();
    void Init(GC_Color_Format pixelFormat = GC_PIXEL_RGBA8888);
    bool RunToVBlank(u8* pFrameBuffer, s16* pSampleBuffer, int* pSampleCount, GC_Debug_Run* debug = NULL, bool render = true);
    bool LoadROM(const char* szFilePath, Cartridge::ForceConfiguration* config = NULL, bool softpatching = false);
    bool LoadROMFromBuffer(const u8* buffer, int size, Cartridge::ForceConfiguration* config = NULL,
        const char* path = NULL, bool softpatching = false);
    bool LoadAdamFirmware(const u8* os7, int os7_size, const u8* eos, int eos_size,
        const u8* smartwriter, int smartwriter_size);
    bool LoadAdamFirmware(GC_AdamFirmware firmware, const u8* data, int size);
    void UnloadAdamFirmware();
    bool StartAdam(GC_AdamBootMode boot_mode = GC_ADAM_BOOT_COMPUTER);
    bool LoadAdamMediaFromBuffer(GC_AdamMediaSlot slot, GC_AdamMediaType type,
        const u8* data, size_t size, bool write_protected = false, u32 base_crc = 0);
    void EjectAdamMedia(GC_AdamMediaSlot slot);
    void EjectAllAdamMedia();
    void UnloadContent();
    AdamMedia* GetAdamMedia(GC_AdamMediaSlot slot);
    void AdamKeyPressed(GC_AdamKey key);
    void AdamKeyReleased(GC_AdamKey key);
    void AdamReleaseAllKeys();
    bool IsReady() const;
    GC_Machine GetMachine() const;
    GC_ContentType GetContentType() const;
    GC_AdamBootMode GetAdamBootMode() const;
    void SaveDisassembledROM();
    bool GetRuntimeInfo(GC_RuntimeInfo& runtime_info);
    void KeyPressed(GC_Controllers controller, GC_Keys key);
    void KeyReleased(GC_Controllers controller, GC_Keys key);
    void Spinner1(int movement);
    void Spinner2(int movement);
    void Pause(bool paused);
    bool IsPaused();
    void ResetROM(Cartridge::ForceConfiguration* config = NULL);
    void ResetROMPreservingRAM(Cartridge::ForceConfiguration* config = NULL);
    void ResetSound();
    void SaveRam();
    void SaveRam(const char* szPath, bool fullPath = false);
    void LoadRam();
    void LoadRam(const char* szPath, bool fullPath = false);
    bool SaveState(const char* path = NULL, int index = -1, bool screenshot = false);
    bool SaveState(u8* buffer, size_t& size, bool screenshot = false);
    bool LoadState(const char* path = NULL, int index = -1);
    bool LoadState(const u8* buffer, size_t size);
    size_t GetLibretroSaveStateSize() const;
    bool GetSaveStateHeader(int index, const char* path, GC_SaveState_Header* header);
    bool GetSaveStateScreenshot(int index, const char* path, GC_SaveState_Screenshot* screenshot);
    Memory* GetMemory();
    Cartridge* GetCartridge();
    Processor* GetProcessor();
    Audio* GetAudio();
    Video* GetVideo();
    void SetVideoChip(GC_VideoChip video_chip);
    GC_VideoChip GetVideoChip() const;
    Input* GetInput();
    TraceLogger* GetTraceLogger();
    Adam* GetAdam();
    u64 GetMasterClockCycles();
    bool GetAdamDebugState(GC_AdamDebugState* state);
    void RenderFrameBuffer(u8* finalFrameBuffer);

private:
    void Reset(bool cold = true);
    void SelectVideoChip(GC_VideoChip video_chip);
    void SelectVideoChipForCartridge();
    bool SaveState(std::ostream& stream, size_t& size, bool screenshot);
    bool LoadStateTransactional(std::istream& stream);
    bool LoadStateInternal(std::istream& stream);
    std::string GetSaveStatePath(const char* path, int index);

private:
    Memory* m_pMemory;
    Processor* m_pProcessor;
    Audio* m_pAudio;
    TMS9918A* m_pTMS9918A;
    F18A* m_pF18A;
    Video* m_pVideo;
    Input* m_pInput;
    Cartridge* m_pCartridge;
    ColecoVisionIOPorts* m_pColecoVisionIOPorts;
    Random* m_pRandom;
    TraceLogger* m_pTraceLogger;
    Adam* m_pAdam;
    bool m_bPaused;
    GC_Color_Format m_pixelFormat;
    u8* m_pFrameBuffer;
    u64 m_MasterClockCycles;
    GC_VideoChip m_requested_video_chip;
    GC_VideoChip m_video_chip;
    GC_Machine m_machine;
    GC_ContentType m_content_type;
    GC_AdamBootMode m_adam_boot_mode;
};

#endif	/* CORE_H */
