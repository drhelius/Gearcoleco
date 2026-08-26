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
    bool LoadROMFromBuffer(const u8* buffer, int size, Cartridge::ForceConfiguration* config = NULL);
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
    u64 GetMasterClockCycles();
    void RenderFrameBuffer(u8* finalFrameBuffer);

private:
    void Reset();
    void SelectVideoChip(GC_VideoChip video_chip);
    void SelectVideoChipForCartridge();
    bool SaveState(std::ostream& stream, size_t& size, bool screenshot);
    bool LoadState(std::istream& stream);
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
    bool m_bPaused;
    GC_Color_Format m_pixelFormat;
    u8* m_pFrameBuffer;
    u64 m_MasterClockCycles;
    GC_VideoChip m_requested_video_chip;
    GC_VideoChip m_video_chip;
};

#endif	/* CORE_H */
