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

#include <iomanip>
#include <string>
#include <string.h>
#include "GearcolecoCore.h"
#include "Memory.h"
#include "Processor.h"
#include "Audio.h"
#include "Video.h"
#include "TMS9918A.h"
#include "F18A.h"
#include "Input.h"
#include "Cartridge.h"
#include "ColecoVisionIOPorts.h"
#include "Adam.h"
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
#include "TraceLogger.h"
#endif
#include "no_bios.h"
#include "common.h"
#include "memory_stream.h"
#include "random.h"

static bool IsValidStateIdentity(u8 machine, u8 content_type, u8 adam_boot_mode)
{
    if ((machine < GC_MACHINE_COLECOVISION) || (machine > GC_MACHINE_ADAM) ||
        (content_type > GC_CONTENT_ADAM_DISK) || (adam_boot_mode > GC_ADAM_BOOT_CARTRIDGE))
    {
        return false;
    }

    if (machine == GC_MACHINE_COLECOVISION)
        return (content_type == GC_CONTENT_CARTRIDGE) &&
            (adam_boot_mode == GC_ADAM_BOOT_COMPUTER);

    if (adam_boot_mode == GC_ADAM_BOOT_CARTRIDGE)
        return content_type == GC_CONTENT_CARTRIDGE;

    return content_type != GC_CONTENT_CARTRIDGE;
}

GearcolecoCore::GearcolecoCore()
{
    InitPointer(m_pMemory);
    InitPointer(m_pProcessor);
    InitPointer(m_pAudio);
    InitPointer(m_pTMS9918A);
    InitPointer(m_pF18A);
    InitPointer(m_pVideo);
    InitPointer(m_pInput);
    InitPointer(m_pCartridge);
    InitPointer(m_pColecoVisionIOPorts);
    InitPointer(m_pRandom);
    InitPointer(m_pTraceLogger);
    InitPointer(m_pAdam);
    InitPointer(m_pFrameBuffer);
    m_bPaused = true;
    m_pixelFormat = GC_PIXEL_RGBA8888;
    m_MasterClockCycles = 0;
    m_requested_video_chip = GC_VIDEO_CHIP_AUTO;
    m_video_chip = GC_VIDEO_CHIP_TMS9918A;
    m_machine = GC_MACHINE_COLECOVISION;
    m_content_type = GC_CONTENT_NONE;
    m_adam_boot_mode = GC_ADAM_BOOT_COMPUTER;
}

GearcolecoCore::~GearcolecoCore()
{
    SafeDelete(m_pAdam);
    SafeDelete(m_pColecoVisionIOPorts);
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    SafeDelete(m_pTraceLogger);
#endif
    SafeDelete(m_pCartridge);
    SafeDelete(m_pInput);
    SafeDelete(m_pF18A);
    SafeDelete(m_pTMS9918A);
    SafeDelete(m_pAudio);
    SafeDelete(m_pProcessor);
    SafeDelete(m_pMemory);
    SafeDelete(m_pRandom);
}

void GearcolecoCore::Init(GC_Color_Format pixelFormat)
{
    Log("Loading %s core %s by Ignacio Sanchez", GEARCOLECO_TITLE, GEARCOLECO_VERSION);

    m_pixelFormat = pixelFormat;

    m_pCartridge = new Cartridge();
    m_pRandom = new Random();
    m_pRandom->Seed((u32)time(NULL));
    m_pMemory = new Memory(m_pCartridge, m_pRandom);
    m_pProcessor = new Processor(m_pMemory);
    m_pAudio = new Audio();
    m_pTMS9918A = new TMS9918A(m_pMemory, m_pProcessor);
    m_pF18A = new F18A(m_pMemory, m_pProcessor);
    m_pVideo = m_pTMS9918A;
    m_pInput = new Input(m_pProcessor);
    m_pColecoVisionIOPorts = new ColecoVisionIOPorts(m_pAudio, m_pVideo, m_pInput, m_pCartridge, m_pMemory, m_pProcessor);
    m_pAdam = new Adam();
    m_pAdam->Init(m_pColecoVisionIOPorts);

    m_pMemory->Init();
    m_pProcessor->Init();
    m_pAudio->Init();
    m_pTMS9918A->Init();
    m_pF18A->Init();
    m_pInput->Init();
    m_pCartridge->Init();

    m_pProcessor->SetIOPOrts(m_pColecoVisionIOPorts);

#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    m_pTraceLogger = new TraceLogger(&m_MasterClockCycles);
    m_pProcessor->SetTraceLogger(m_pTraceLogger);
    m_pMemory->SetTraceLogger(m_pTraceLogger);
    m_pTMS9918A->SetTraceLogger(m_pTraceLogger);
    m_pF18A->SetTraceLogger(m_pTraceLogger);
    m_pInput->SetTraceLogger(m_pTraceLogger);
    m_pColecoVisionIOPorts->SetTraceLogger(m_pTraceLogger);
    m_pAdam->SetTraceLogger(m_pTraceLogger);
#endif
}

void GearcolecoCore::SetVideoChip(GC_VideoChip video_chip)
{
    if ((video_chip == GC_VIDEO_CHIP_AUTO) || (video_chip == GC_VIDEO_CHIP_TMS9918A) ||
        (video_chip == GC_VIDEO_CHIP_F18A))
    {
        m_requested_video_chip = video_chip;
    }
}

GC_VideoChip GearcolecoCore::GetVideoChip() const
{
    return m_video_chip;
}

void GearcolecoCore::SelectVideoChip(GC_VideoChip video_chip)
{
    if (video_chip == GC_VIDEO_CHIP_F18A)
    {
        m_pVideo = m_pF18A;
        m_video_chip = GC_VIDEO_CHIP_F18A;
    }
    else
    {
        m_pVideo = m_pTMS9918A;
        m_video_chip = GC_VIDEO_CHIP_TMS9918A;
    }

    if (IsValidPointer(m_pColecoVisionIOPorts))
        m_pColecoVisionIOPorts->SetVideo(m_pVideo);
}

void GearcolecoCore::SelectVideoChipForCartridge()
{
    GC_VideoChip video_chip = m_requested_video_chip;

    if (video_chip == GC_VIDEO_CHIP_AUTO)
    {
        video_chip = m_pCartridge->IsF18ARequired() ? GC_VIDEO_CHIP_F18A : GC_VIDEO_CHIP_TMS9918A;
    }

    SelectVideoChip(video_chip);
    Log("Video chip: %s", m_video_chip == GC_VIDEO_CHIP_F18A ? "F18A" : "TMS9918A");
}

bool GearcolecoCore::RunToVBlank(u8* pFrameBuffer, s16* pSampleBuffer, int* pSampleCount, GC_Debug_Run* debug, bool render)
{
    m_pFrameBuffer = pFrameBuffer;

    if (!IsReady())
    {
        if (render)
            RenderFrameBuffer(pFrameBuffer);
        return false;
    }

    if (!m_bPaused && IsReady())
    {
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
        bool debug_enable = false;
        bool instruction_completed = false;
        if (IsValidPointer(debug))
        {
            debug_enable = true;
            m_pProcessor->EnableBreakpoints(debug->stop_on_breakpoint, debug->stop_on_irq);
        }

        bool vblank = false;
        int totalClocks = 0;

        do
        {
            unsigned int clockCycles = debug_enable && debug->step_debugger ? m_pProcessor->RunInstruction() : m_pProcessor->RunFor(1);
            instruction_completed = true;
            m_MasterClockCycles += clockCycles;
            vblank = m_pVideo->Tick(clockCycles);
            m_pAudio->Tick(clockCycles);
            m_pMemory->Tick(clockCycles);
            if (m_machine == GC_MACHINE_ADAM)
                m_pAdam->Clock(clockCycles);
            totalClocks += clockCycles;

            if (debug_enable)
            {
                if (debug->step_debugger)
                    vblank = instruction_completed;

                if (m_pProcessor->MemoryBreakpointHit())
                    vblank = true;

                if (instruction_completed)
                {
                    if (m_pProcessor->BreakpointHit())
                        vblank = true;

                    if (debug->stop_on_run_to_breakpoint && m_pProcessor->RunToBreakpointHit())
                        vblank = true;
                }
            }

            if (totalClocks > 702240)
                vblank = true;
        }
        while (!vblank);

        m_pAudio->EndFrame(pSampleBuffer, pSampleCount);
        if (render)
            RenderFrameBuffer(pFrameBuffer);

        return m_pProcessor->BreakpointHit() || m_pProcessor->RunToBreakpointHit();
#else
        UNUSED(debug);
        bool vblank = false;
        int totalClocks = 0;

        do
        {
#ifdef PERFORMANCE
            unsigned int clockCycles = m_pProcessor->RunFor(75);
#else
            unsigned int clockCycles = m_pProcessor->RunFor(1);
#endif
            m_MasterClockCycles += clockCycles;
            vblank = m_pVideo->Tick(clockCycles);
            m_pAudio->Tick(clockCycles);
            m_pMemory->Tick(clockCycles);
            if (m_machine == GC_MACHINE_ADAM)
                m_pAdam->Clock(clockCycles);
            totalClocks += clockCycles;

            if (totalClocks > 702240)
                vblank = true;
        }
        while (!vblank);

        m_pAudio->EndFrame(pSampleBuffer, pSampleCount);
        if (render)
            RenderFrameBuffer(pFrameBuffer);

        return false;
#endif
    }

    return false;
}

bool GearcolecoCore::LoadROM(const char* szFilePath, Cartridge::ForceConfiguration* config, bool softpatching)
{
    if (m_pCartridge->LoadFromFile(szFilePath, softpatching))
    {
        if (IsValidPointer(config))
            m_pCartridge->ForceConfig(*config);

        m_machine = GC_MACHINE_COLECOVISION;
        m_content_type = GC_CONTENT_CARTRIDGE;
        m_adam_boot_mode = GC_ADAM_BOOT_COMPUTER;
        EjectAllAdamMedia();
        m_pAdam->SetEnabled(false);
        m_pMemory->SetAdam(NULL);
        SelectVideoChipForCartridge();
        Reset();

        m_pMemory->ResetRomDisassembledMemory();
        m_pProcessor->DisassembleNextOPCode();

        return true;
    }
    else
        return false;
}

bool GearcolecoCore::LoadROMFromBuffer(const u8* buffer, int size,
    Cartridge::ForceConfiguration* config, const char* path, bool softpatching)
{
    bool loaded;
    if (IsValidPointer(path))
        loaded = m_pCartridge->LoadFromBuffer(buffer, size, path, softpatching);
    else
    {
        m_pCartridge->Reset();
        loaded = m_pCartridge->LoadFromBuffer(buffer, size);
    }
    if (loaded)
    {
        if (IsValidPointer(config))
            m_pCartridge->ForceConfig(*config);

        m_machine = GC_MACHINE_COLECOVISION;
        m_content_type = GC_CONTENT_CARTRIDGE;
        m_adam_boot_mode = GC_ADAM_BOOT_COMPUTER;
        EjectAllAdamMedia();
        m_pAdam->SetEnabled(false);
        m_pMemory->SetAdam(NULL);
        SelectVideoChipForCartridge();
        Reset();

        m_pMemory->ResetRomDisassembledMemory();
        m_pProcessor->DisassembleNextOPCode();

        return true;
    }
    else
        return false;
}

bool GearcolecoCore::LoadAdamFirmware(const u8* os7, int os7_size, const u8* eos, int eos_size,
    const u8* smartwriter, int smartwriter_size)
{
    bool loaded = m_pAdam->LoadFirmware(os7, os7_size, eos, eos_size, smartwriter, smartwriter_size);

    if (loaded)
    {
        Log("ADAM firmware loaded (OS-7 %08X, EOS %08X, SmartWriter %08X)",
            m_pAdam->GetFirmwareCRC(GC_ADAM_FIRMWARE_OS7),
            m_pAdam->GetFirmwareCRC(GC_ADAM_FIRMWARE_EOS),
            m_pAdam->GetFirmwareCRC(GC_ADAM_FIRMWARE_SMARTWRITER));
    }
    else
    {
        Error("Invalid or incomplete ADAM firmware");
    }

    return loaded;
}

bool GearcolecoCore::LoadAdamFirmware(GC_AdamFirmware firmware, const u8* data, int size)
{
    bool loaded = m_pAdam->LoadFirmware(firmware, data, size);

    if (loaded)
        Log("ADAM firmware role %d loaded (%d bytes, CRC32 %08X)", firmware, size,
            m_pAdam->GetFirmwareCRC(firmware));
    else
        Error("Invalid ADAM firmware role %d (%d bytes)", firmware, size);

    return loaded;
}

void GearcolecoCore::UnloadAdamFirmware()
{
    m_pAdam->UnloadFirmware();

    if (m_machine == GC_MACHINE_ADAM)
    {
        m_pAdam->SetEnabled(false);
        m_pMemory->SetAdam(NULL);
        m_bPaused = true;
    }
}

bool GearcolecoCore::StartAdam(GC_AdamBootMode boot_mode)
{
    if (!m_pAdam->IsFirmwareReady())
    {
        Error("ADAM firmware is incomplete");
        return false;
    }

    if ((boot_mode == GC_ADAM_BOOT_CARTRIDGE) && !m_pCartridge->IsReady())
    {
        Error("ADAM cartridge boot requested without a cartridge");
        return false;
    }

    m_machine = GC_MACHINE_ADAM;
    if (boot_mode == GC_ADAM_BOOT_CARTRIDGE)
        m_content_type = GC_CONTENT_CARTRIDGE;
    else if ((m_content_type != GC_CONTENT_ADAM_DATA_PACK) &&
        (m_content_type != GC_CONTENT_ADAM_DISK))
        m_content_type = GC_CONTENT_NONE;
    m_adam_boot_mode = boot_mode;
    SelectVideoChip(GC_VIDEO_CHIP_TMS9918A);
    Reset(true);
    m_pMemory->ResetRomDisassembledMemory();
    m_pProcessor->DisassembleNextOPCode();
    Log("ADAM started in %s mode", boot_mode == GC_ADAM_BOOT_CARTRIDGE ? "cartridge" : "computer");
    return true;
}

bool GearcolecoCore::LoadAdamMediaFromBuffer(GC_AdamMediaSlot slot, GC_AdamMediaType type,
    const u8* data, size_t size, bool write_protected, u32 base_crc)
{
    if (!m_pAdam->IsFirmwareReady())
    {
        Error("ADAM firmware is incomplete");
        return false;
    }

    bool starting_adam = m_machine != GC_MACHINE_ADAM;
    GC_AdamMediaError error = m_pAdam->InsertMedia(slot, type, data, size, write_protected, base_crc);
    if (error != GC_ADAM_MEDIA_ERROR_NONE)
    {
        Error("Invalid ADAM media for slot %d (error %d)", slot, error);
        return false;
    }

    if (starting_adam)
        m_content_type = (type == GC_ADAM_MEDIA_DATA_PACK) ? GC_CONTENT_ADAM_DATA_PACK :
            GC_CONTENT_ADAM_DISK;

    if (starting_adam)
    {
        m_machine = GC_MACHINE_ADAM;
        m_adam_boot_mode = GC_ADAM_BOOT_COMPUTER;
        SelectVideoChip(GC_VIDEO_CHIP_TMS9918A);
        Reset(true);
        m_pMemory->ResetRomDisassembledMemory();
        m_pProcessor->DisassembleNextOPCode();
    }

    Log("ADAM %s inserted in slot %d (%zu bytes%s)",
        type == GC_ADAM_MEDIA_DATA_PACK ? "data pack" : "disk", slot, size,
        write_protected ? ", write protected" : "");
    return true;
}

void GearcolecoCore::EjectAdamMedia(GC_AdamMediaSlot slot)
{
    m_pAdam->EjectMedia(slot);
}

void GearcolecoCore::EjectAllAdamMedia()
{
    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
        m_pAdam->EjectMedia((GC_AdamMediaSlot)i);
}

void GearcolecoCore::UnloadContent()
{
    m_pAdam->ReleaseAllKeys();
    EjectAllAdamMedia();
    m_pAdam->SetEnabled(false);
    m_pMemory->SetAdam(NULL);
    m_pCartridge->Reset();
    m_pProcessor->SetIOPOrts(m_pColecoVisionIOPorts);
    m_machine = GC_MACHINE_COLECOVISION;
    m_content_type = GC_CONTENT_NONE;
    m_adam_boot_mode = GC_ADAM_BOOT_COMPUTER;
    m_bPaused = true;
}

AdamMedia* GearcolecoCore::GetAdamMedia(GC_AdamMediaSlot slot)
{
    return m_pAdam->GetMedia(slot);
}

void GearcolecoCore::AdamKeyPressed(GC_AdamKey key)
{
    if (m_machine == GC_MACHINE_ADAM)
        m_pAdam->KeyPressed(key);
}

void GearcolecoCore::AdamKeyReleased(GC_AdamKey key)
{
    if (m_machine == GC_MACHINE_ADAM)
        m_pAdam->KeyReleased(key);
}

void GearcolecoCore::AdamReleaseAllKeys()
{
    if (m_machine == GC_MACHINE_ADAM)
        m_pAdam->ReleaseAllKeys();
}

bool GearcolecoCore::IsReady() const
{
    if (m_machine == GC_MACHINE_ADAM)
        return m_pAdam->IsEnabled() && m_pAdam->IsFirmwareReady();

    return m_pCartridge->IsReady() && m_pMemory->IsBiosLoaded();
}

GC_Machine GearcolecoCore::GetMachine() const
{
    return m_machine;
}

GC_ContentType GearcolecoCore::GetContentType() const
{
    return m_content_type;
}

GC_AdamBootMode GearcolecoCore::GetAdamBootMode() const
{
    return m_adam_boot_mode;
}

void GearcolecoCore::SaveDisassembledROM()
{
    GC_Disassembler_Record** biosMap = m_pMemory->GetDisassemblerBiosMap();
    GC_Disassembler_Record** romMap = m_pMemory->GetDisassemblerRomMap();

    if (m_pCartridge->IsReady() && (strlen(m_pCartridge->GetFilePath()) > 0) && IsValidPointer(romMap))
    {
        using namespace std;

        string path = string(m_pCartridge->GetFilePath()) + ".dis";

        Log("Saving Disassembled ROM %s...", path.c_str());

        ofstream myfile;
        open_ofstream_utf8(myfile, path.c_str(), ios::out | ios::trunc);

        if (myfile.is_open())
        {
            #define PAD_ADDR(digits) std::uppercase << std::hex << std::setw(digits) << std::setfill('0')
            #define PAD_MEM(chars) std::setw(chars) << std::setfill(' ')

            for (int i = 0; i < 0x2000; i++)
            {
                if (IsValidPointer(biosMap[i]) && (biosMap[i]->name[0] != 0))
                {
                    myfile << "BIOS $" << PAD_ADDR(4) << i << "   " << PAD_MEM(25) << biosMap[i]->bytes << "  " << biosMap[i]->name << "\n";
                }
            }

            for (int i = 0; i < MAX_ROM_SIZE; i++)
            {
                if (IsValidPointer(romMap[i]) && (romMap[i]->name[0] != 0))
                {
                    myfile << "ROM  $" << PAD_ADDR(4) << i + 0x8000 << "   " << PAD_MEM(25) << romMap[i]->bytes << "  " << romMap[i]->name << "\n";
                }
            }

            myfile.close();
        }

        Debug("Disassembled ROM Saved");
    }
}

bool GearcolecoCore::GetRuntimeInfo(GC_RuntimeInfo& runtime_info)
{
    bool pal = (m_machine != GC_MACHINE_ADAM) && m_pCartridge->IsPAL();
    double master_clock = pal ? GC_MASTER_CLOCK_PAL : GC_MASTER_CLOCK_NTSC;
    int lines_per_frame = pal ? GC_LINES_PER_FRAME_PAL : GC_LINES_PER_FRAME_NTSC;

    runtime_info.screen_width = m_pVideo->GetScreenWidth();
    runtime_info.screen_height = m_pVideo->GetScreenHeight();
    runtime_info.region = Region_NTSC;
    runtime_info.fps = master_clock / (GC_CYCLES_PER_LINE * lines_per_frame);

    if (IsReady())
    {
        if (!m_pVideo->IsF18AHardware() && (m_pVideo->GetOverscan() == Video::OverscanFull284))
            runtime_info.screen_width = GC_RESOLUTION_WIDTH + GC_RESOLUTION_SMS_OVERSCAN_H_284_L + GC_RESOLUTION_SMS_OVERSCAN_H_284_R;
        if (!m_pVideo->IsF18AHardware() && (m_pVideo->GetOverscan() == Video::OverscanFull320))
            runtime_info.screen_width = GC_RESOLUTION_WIDTH + GC_RESOLUTION_SMS_OVERSCAN_H_320_L + GC_RESOLUTION_SMS_OVERSCAN_H_320_R;
        if (!m_pVideo->IsF18AHardware() && (m_pVideo->GetOverscan() != Video::OverscanDisabled))
            runtime_info.screen_height = GC_RESOLUTION_HEIGHT + (2 * (pal ?
                GC_RESOLUTION_OVERSCAN_V_PAL : GC_RESOLUTION_OVERSCAN_V));
        runtime_info.region = pal ? Region_PAL : Region_NTSC;
        return true;
    }

    return false;
}

Memory* GearcolecoCore::GetMemory()
{
    return m_pMemory;
}

Cartridge* GearcolecoCore::GetCartridge()
{
    return m_pCartridge;
}

Processor* GearcolecoCore::GetProcessor()
{
    return m_pProcessor;
}

Audio* GearcolecoCore::GetAudio()
{
    return m_pAudio;
}

Video* GearcolecoCore::GetVideo()
{
    return m_pVideo;
}

Input* GearcolecoCore::GetInput()
{
    return m_pInput;
}

TraceLogger* GearcolecoCore::GetTraceLogger()
{
    return m_pTraceLogger;
}

Adam* GearcolecoCore::GetAdam()
{
    return m_pAdam;
}

u64 GearcolecoCore::GetMasterClockCycles()
{
    return m_MasterClockCycles;
}

bool GearcolecoCore::GetAdamDebugState(GC_AdamDebugState* state)
{
    if (!IsValidPointer(state))
        return false;

    m_pAdam->GetDebugState(state);
    state->machine = m_machine;
    state->content_type = m_content_type;
    state->master_clock_cycles = m_MasterClockCycles;

    if (m_machine != GC_MACHINE_ADAM)
    {
        state->valid = false;
        return false;
    }

    for (int page = 0; page < GC_ADAM_DEBUG_PAGE_COUNT; page++)
    {
        GC_AdamDebugMemoryPage* debug_page = &state->pages[page];
        if (debug_page->read_source == Adam::MemorySourceCartridge)
        {
            debug_page->read_offset = m_pMemory->GetPhysicalAddress(debug_page->start);
            debug_page->cartridge_bank = m_pMemory->GetBank(debug_page->start);
        }
    }

    return state->valid;
}

void GearcolecoCore::KeyPressed(GC_Controllers controller, GC_Keys key)
{
    m_pInput->KeyPressed(controller, key);
}

void GearcolecoCore::KeyReleased(GC_Controllers controller, GC_Keys key)
{
    m_pInput->KeyReleased(controller, key);
}

void GearcolecoCore::Spinner1(int movement)
{
    m_pInput->Spinner1(movement);
}

void GearcolecoCore::Spinner2(int movement)
{
    m_pInput->Spinner2(movement);
}

void GearcolecoCore::Pause(bool paused)
{
    if (paused)
    {
        Log(GEARCOLECO_TITLE " PAUSED");
    }
    else
    {
        Log(GEARCOLECO_TITLE " RESUMED");
    }
    m_bPaused = paused;
}

bool GearcolecoCore::IsPaused()
{
    return m_bPaused;
}

void GearcolecoCore::ResetROM(Cartridge::ForceConfiguration* config)
{
    if (m_machine == GC_MACHINE_ADAM)
    {
        if (!IsReady())
            return;

        Log(GEARCOLECO_TITLE " RESET");

        if (IsValidPointer(config) && m_pCartridge->IsReady())
            m_pCartridge->ForceConfig(*config);

        SelectVideoChip(GC_VIDEO_CHIP_TMS9918A);
        Reset(false);
        m_pProcessor->DisassembleNextOPCode();
        return;
    }

    if (m_pCartridge->IsReady())
    {
        Log(GEARCOLECO_TITLE " RESET");

        if (IsValidPointer(config))
            m_pCartridge->ForceConfig(*config);

        SelectVideoChipForCartridge();
        Reset();

        m_pProcessor->DisassembleNextOPCode();
    }
}

void GearcolecoCore::ResetROMPreservingRAM(Cartridge::ForceConfiguration* config)
{
    if ((m_machine == GC_MACHINE_ADAM) && !m_pCartridge->IsReady())
    {
        ResetROM(config);
        return;
    }

    if (m_pCartridge->IsReady())
    {
        Mapper* pMapper = m_pMemory->GetMapper();
        u8* pSaveData = IsValidPointer(pMapper) ? pMapper->GetSaveData() : NULL;
        int saveDataSize = IsValidPointer(pMapper) ? pMapper->GetSaveDataSize() : 0;

        if (IsValidPointer(pSaveData) && (saveDataSize > 0))
        {
            Debug("Resetting preserving RAM...");

            u8* pSavedData = new u8[saveDataSize];
            memcpy(pSavedData, pSaveData, saveDataSize);

            ResetROM(config);

            pMapper = m_pMemory->GetMapper();
            pSaveData = IsValidPointer(pMapper) ? pMapper->GetSaveData() : NULL;

            if (IsValidPointer(pSaveData) && (pMapper->GetSaveDataSize() == saveDataSize))
                memcpy(pSaveData, pSavedData, saveDataSize);

            SafeDeleteArray(pSavedData);
        }
        else
        {
            ResetROM(config);
        }
    }
}

void GearcolecoCore::ResetSound()
{
    m_pAudio->Reset((m_machine == GC_MACHINE_ADAM) ? false : m_pCartridge->IsPAL());
}

void GearcolecoCore::SaveRam()
{
    SaveRam(NULL);
}

void GearcolecoCore::SaveRam(const char* szPath, bool fullPath)
{
    if (!m_pCartridge->IsReady())
        return;

    Mapper* pMapper = m_pMemory->GetMapper();

    if (!IsValidPointer(pMapper) || pMapper->GetSaveDataSize() <= 0)
        return;

    Log("Saving RAM...");

    using namespace std;

    string path = "";

    if (fullPath)
    {
        path = szPath;
    }
    else if (IsValidPointer(szPath))
    {
        path += szPath;
        append_path_component(path, m_pCartridge->GetFileName());
    }
    else
    {
        path = m_pCartridge->GetFilePath();
    }

    string::size_type i = path.rfind('.', path.length());

    if (i != string::npos)
    {
        path.replace(i + 1, path.length() - i - 1, "sav");
    }

    Log("Save CSV file: %s", path.c_str());

    ofstream file;
    open_ofstream_utf8(file, path.c_str(), ios::out | ios::binary);

    if (file.is_open())
    {
        file.write(reinterpret_cast<const char*>(pMapper->GetSaveData()), pMapper->GetSaveDataSize());
        file.close();
        Log("RAM saved to %s", path.c_str());
    }
    else
    {
        Log("Unable to open RAM file for writing: %s", path.c_str());
    }
}

void GearcolecoCore::LoadRam()
{
    LoadRam(NULL);
}

void GearcolecoCore::LoadRam(const char* szPath, bool fullPath)
{
    if (!m_pCartridge->IsReady())
        return;

    Mapper* pMapper = m_pMemory->GetMapper();
    u8* pSaveData = IsValidPointer(pMapper) ? pMapper->GetSaveData() : NULL;
    int saveDataSize = IsValidPointer(pMapper) ? pMapper->GetSaveDataSize() : 0;

    if (!IsValidPointer(pSaveData) || saveDataSize <= 0)
        return;

    Log("Loading RAM...");

    using namespace std;

    string path = "";

    if (fullPath)
    {
        path = szPath;
    }
    else if (IsValidPointer(szPath))
    {
        path += szPath;
        append_path_component(path, m_pCartridge->GetFileName());
    }
    else
    {
        path = m_pCartridge->GetFilePath();
    }

    string::size_type i = path.rfind('.', path.length());

    if (i != string::npos)
    {
        path.replace(i + 1, path.length() - i - 1, "sav");
    }

    Log("Load RAM file: %s", path.c_str());

    ifstream file;
    open_ifstream_utf8(file, path.c_str(), ios::in | ios::binary);

    if (file.is_open())
    {
        u8* pLoadedData = new u8[saveDataSize];
        file.read(reinterpret_cast<char*>(pLoadedData), saveDataSize);

        if (file.gcount() == saveDataSize)
        {
            memcpy(pSaveData, pLoadedData, saveDataSize);
            Log("RAM loaded from %s", path.c_str());
        }
        else
        {
            Error("Unable to read complete RAM file: %s", path.c_str());
        }

        file.close();
        SafeDeleteArray(pLoadedData);
    }
    else
    {
        Log("Unable to open RAM file for reading: %s", path.c_str());
    }
}

std::string GearcolecoCore::GetSaveStatePath(const char* path, int index)
{
    using namespace std;

    string sav_path = "";

    if (IsValidPointer(path))
    {
        sav_path += path;
        append_path_component(sav_path, m_pCartridge->GetFileName());
    }
    else
    {
        sav_path = m_pCartridge->GetFilePath();
    }

    string::size_type i = sav_path.rfind('.', sav_path.length());

    if (i != string::npos) {
        sav_path.replace(i + 1, 3, "state");
    }

    std::stringstream sstm;

    if (index < 0)
    {
        if (IsValidPointer(path))
            sstm << path;
        else
            sstm << sav_path;
    }
    else
        sstm << sav_path << index;

    return sstm.str();
}

bool GearcolecoCore::SaveState(const char* path, int index, bool screenshot)
{
    Log("Creating save state...");

    using namespace std;

    string full_path = GetSaveStatePath(path, index);
    Log("Save state file: %s", full_path.c_str());

    ofstream file;
    open_ofstream_utf8(file, full_path.c_str(), ios::out | ios::binary);

    if (!file.is_open())
    {
        Error("Failed to open save state file for writing: %s", full_path.c_str());
        return false;
    }

    size_t size = 0;
    if (!SaveState(file, size, screenshot))
    {
        file.close();
        Error("Failed to save state to file: %s", full_path.c_str());
        return false;
    }

    file.close();

    if (!file.good())
    {
        Error("Failed to write save state file: %s", full_path.c_str());
        return false;
    }

    Debug("Save state created");
    return true;
}

bool GearcolecoCore::SaveState(u8* buffer, size_t& size, bool screenshot)
{
    using namespace std;
#if defined(__LIBRETRO__)
    size_t fixed_size = GetLibretroSaveStateSize();
#endif

    Debug("Saving state to buffer [%d bytes]...", size);

    if (!IsReady())
    {
        Error("Machine is not ready when trying to save state");
        return false;
    }

    if (!IsValidPointer(buffer))
    {
#if defined(__LIBRETRO__)
        size = fixed_size;
        return true;
#else
        counting_stream stream;
        if (!SaveState(stream, size, screenshot))
        {
            Error("Failed to save state to stream to calculate size");
            return false;
        }
        return true;
#endif
    }
    else
    {
#if defined(__LIBRETRO__)
        if (size < fixed_size)
        {
            Error("Failed to save state to buffer: output buffer is too small");
            return false;
        }
#endif

        memory_stream direct_stream(reinterpret_cast<char*>(buffer), size);

        if (!SaveState(direct_stream, size, screenshot))
        {
            Error("Failed to save state to buffer");
            return false;
        }

        if (!direct_stream.good())
        {
            Error("Failed to save state to buffer: output buffer is too small");
            return false;
        }

#if defined(__LIBRETRO__)
        if ((size < sizeof(GC_SaveState_Header_Libretro)) || (size > fixed_size))
        {
            Error("Invalid libretro save-state size: %zu", size);
            return false;
        }

        size_t source_header = size - sizeof(GC_SaveState_Header_Libretro);
        size_t destination_header = fixed_size - sizeof(GC_SaveState_Header_Libretro);
        memmove(buffer + destination_header, buffer + source_header, sizeof(GC_SaveState_Header_Libretro));
        memset(buffer + source_header, 0, fixed_size - size);
        size = fixed_size;
#else
        size = direct_stream.size();
#endif
        return true;
    }
}

bool GearcolecoCore::SaveState(std::ostream& stream, size_t& size, bool screenshot)
{
#if defined(__LIBRETRO__)
    UNUSED(screenshot);
#endif

    if (IsReady())
    {
        Debug("Gathering save state data...");

        u8 machine = (u8)m_machine;
        u8 content_type = (u8)m_content_type;
        u8 adam_boot_mode = (u8)m_adam_boot_mode;
        stream.write(reinterpret_cast<const char*>(&machine), sizeof(machine));
        stream.write(reinterpret_cast<const char*>(&content_type), sizeof(content_type));
        stream.write(reinterpret_cast<const char*>(&adam_boot_mode), sizeof(adam_boot_mode));
        if (m_machine == GC_MACHINE_ADAM)
        {
            u32 cartridge_crc = m_pCartridge->GetCRC();
            u32 cartridge_size = (u32)m_pCartridge->GetROMSize();
            u32 cartridge_type = (u32)m_pCartridge->GetType();
            stream.write(reinterpret_cast<const char*>(&cartridge_crc), sizeof(cartridge_crc));
            stream.write(reinterpret_cast<const char*>(&cartridge_size), sizeof(cartridge_size));
            stream.write(reinterpret_cast<const char*>(&cartridge_type), sizeof(cartridge_type));
        }
        stream.write(reinterpret_cast<const char*>(&m_video_chip), sizeof(m_video_chip));
        if (m_machine == GC_MACHINE_ADAM)
            m_pAdam->SaveState(stream);
        m_pMemory->SaveState(stream);
        m_pProcessor->SaveState(stream);
        m_pAudio->SaveState(stream);
        m_pVideo->SaveState(stream);
        m_pInput->SaveState(stream);

#if defined(__LIBRETRO__)
        GC_SaveState_Header_Libretro header = {};
        memset(&header, 0, sizeof(header));
        header.magic = GC_SAVESTATE_MAGIC;
        header.version = GC_SAVESTATE_VERSION;
        Debug("Save state header magic: 0x%08x", header.magic);
        Debug("Save state header version: %d", header.version);

        if (!stream.good())
            return false;

        size_t data_size = static_cast<size_t>(stream.tellp());

        size_t fixed_size = GetLibretroSaveStateSize();
        if ((data_size + sizeof(header)) > fixed_size)
        {
            Error("Libretro save state exceeds fixed %zu-byte size", fixed_size);
            return false;
        }
#else
        GC_SaveState_Header header;
        memset(&header, 0, sizeof(header));
        header.magic = GC_SAVESTATE_MAGIC;
        header.version = GC_SAVESTATE_VERSION;
        header.timestamp = time(NULL);
        if (m_machine == GC_MACHINE_ADAM)
        {
            strncpy_fit(header.rom_name, "ADAM", sizeof(header.rom_name));
            header.rom_crc = m_pAdam->GetFirmwareCRC(GC_ADAM_FIRMWARE_EOS);
        }
        else
        {
            strncpy_fit(header.rom_name, m_pCartridge->GetFileName(), sizeof(header.rom_name));
            header.rom_crc = m_pCartridge->GetCRC();
        }
        strncpy_fit(header.emu_build, GEARCOLECO_VERSION, sizeof(header.emu_build));

        Debug("Save state header magic: 0x%08x", header.magic);
        Debug("Save state header version: %d", header.version);

        if (screenshot && IsValidPointer(m_pFrameBuffer))
        {
            GC_RuntimeInfo runtime_info;
            GetRuntimeInfo(runtime_info);
            header.screenshot_width = runtime_info.screen_width;
            header.screenshot_height = runtime_info.screen_height;

            int bytes_per_pixel = (m_pixelFormat == GC_PIXEL_RGBA8888 || m_pixelFormat == GC_PIXEL_BGRA8888) ? 4 : 2;
            header.screenshot_size = header.screenshot_width * header.screenshot_height * bytes_per_pixel;
            stream.write(reinterpret_cast<const char*>(m_pFrameBuffer), header.screenshot_size);
        }
        else
        {
            header.screenshot_size = 0;
            header.screenshot_width = 0;
            header.screenshot_height = 0;
        }
#endif

        size = static_cast<size_t>(stream.tellp());
        size += sizeof(header);

#if !defined(__LIBRETRO__)
        header.size = static_cast<u32>(size);
        Debug("Save state header size: %d", header.size);
#endif

        stream.write(reinterpret_cast<const char*>(&header), sizeof(header));

        return true;
    }

    Log("Invalid rom.");

    return false;
}

size_t GearcolecoCore::GetLibretroSaveStateSize() const
{
    return m_machine == GC_MACHINE_ADAM ? GC_LIBRETRO_SAVESTATE_SIZE_ADAM :
        GC_LIBRETRO_SAVESTATE_SIZE_COLECOVISION;
}

bool GearcolecoCore::LoadState(const char* path, int index)
{
    Log("Loading save state...");

    using namespace std;

    string full_path = GetSaveStatePath(path, index);
    Log("Opening save file: %s", full_path.c_str());

    ifstream file;
    open_ifstream_utf8(file, full_path.c_str(), ios::in | ios::binary);

    if (!file.fail())
    {
        if (LoadStateTransactional(file))
        {
            Debug("Save state loaded");
            file.close();
            return true;
        }
    }
    else
    {
        Log("Save state file doesn't exist");
    }

    file.close();
    return false;
}

bool GearcolecoCore::LoadState(const u8* buffer, size_t size)
{
    using namespace std;

    Debug("Loading state from buffer [%d bytes]...", size);

    if (!IsReady())
    {
        Error("Machine is not ready when trying to load state");
        return false;
    }

    if (!IsValidPointer(buffer) || (size == 0))
    {
        Error("Invalid load state buffer");
        return false;
    }

    memory_input_stream direct_stream(reinterpret_cast<const char*>(buffer), size);
    return LoadStateTransactional(direct_stream);
}

bool GearcolecoCore::LoadStateTransactional(std::istream& stream)
{
#if defined(__LIBRETRO__)
    return LoadStateInternal(stream);
#else
    if (m_machine != GC_MACHINE_ADAM)
        return LoadStateInternal(stream);

    size_t backup_size = 0;
    if (!SaveState((u8*)NULL, backup_size, false))
    {
        Error("Unable to preserve live state before loading");
        return false;
    }

    u8* backup = new u8[backup_size];
    size_t written_size = backup_size;
    if (!SaveState(backup, written_size, false))
    {
        SafeDeleteArray(backup);
        Error("Unable to preserve live state before loading");
        return false;
    }

    bool loaded = LoadStateInternal(stream);
    if (!loaded)
    {
        Debug("Restoring live state after failed load");
        memory_input_stream backup_stream(reinterpret_cast<const char*>(backup), written_size);
        if (!LoadStateInternal(backup_stream))
            Error("Unable to restore live state after failed load");
    }

    SafeDeleteArray(backup);
    return loaded;
#endif
}

bool GearcolecoCore::LoadStateInternal(std::istream& stream)
{
    if (IsReady())
    {
        using namespace std;

        stream.seekg(0, ios::end);
        size_t size = static_cast<size_t>(stream.tellg());

        Debug("Load state stream size: %d", size);

        GC_SaveState_Header_Libretro header = {};
#if !defined(__LIBRETRO__)
        bool is_desktop_savestate = false;
        size_t state_data_size = 0;
#endif

        // Try desktop header first (larger, contains all info)
        GC_SaveState_Header desktop_header;
        if (size >= sizeof(desktop_header))
        {
            stream.seekg(size - sizeof(desktop_header), ios::beg);
            stream.read(reinterpret_cast<char*>(&desktop_header), sizeof(desktop_header));

            if (desktop_header.magic == GC_SAVESTATE_MAGIC)
            {
                header.magic = desktop_header.magic;
                header.version = desktop_header.version;
#if !defined(__LIBRETRO__)
                is_desktop_savestate = true;
#endif
                Debug("Loading desktop save state");
            }
        }

        // Fallback to libretro header
        if ((header.magic != GC_SAVESTATE_MAGIC) && (size >= sizeof(header)))
        {
            stream.seekg(size - sizeof(header), ios::beg);
            stream.read(reinterpret_cast<char*>(&header), sizeof(header));
        }

        stream.seekg(0, ios::beg);

        Debug("Load state header magic: 0x%08x", header.magic);
        Debug("Load state header version: %d", header.version);

        if (header.magic == GC_SAVESTATE_MAGIC && header.version >= GC_SAVESTATE_MIN_VERSION && header.version <= GC_SAVESTATE_VERSION)
        {
#if !defined(__LIBRETRO__)
            if (is_desktop_savestate)
            {
                Debug("Load state header size: %d", desktop_header.size);
                Debug("Load state header timestamp: %d", desktop_header.timestamp);
                Debug("Load state header rom name: %s", desktop_header.rom_name);
                Debug("Load state header rom crc: 0x%08x", desktop_header.rom_crc);
                Debug("Load state header screenshot size: %d", desktop_header.screenshot_size);
                Debug("Load state header screenshot width: %d", desktop_header.screenshot_width);
                Debug("Load state header screenshot height: %d", desktop_header.screenshot_height);
                Debug("Load state header emu build: %s", desktop_header.emu_build);

                if (desktop_header.size != size)
                {
                    Error("Invalid save state size: %d", desktop_header.size);
                    return false;
                }
                if (desktop_header.screenshot_size > size - sizeof(desktop_header))
                {
                    Error("Invalid save state screenshot size: %u", desktop_header.screenshot_size);
                    return false;
                }
                state_data_size = size - sizeof(desktop_header) - desktop_header.screenshot_size;
            }
#endif

            Log("Loading state (v%d)...", header.version);

            GC_ContentType state_content_type = m_content_type;
            GC_AdamBootMode state_adam_boot_mode = m_adam_boot_mode;

            if (header.version >= 107)
            {
                u8 machine = 0;
                u8 content_type = 0;
                u8 adam_boot_mode = 0;
                stream.read(reinterpret_cast<char*>(&machine), sizeof(machine));
                stream.read(reinterpret_cast<char*>(&content_type), sizeof(content_type));
                stream.read(reinterpret_cast<char*>(&adam_boot_mode), sizeof(adam_boot_mode));

                if (!stream.good() || !IsValidStateIdentity(machine, content_type,
                    adam_boot_mode) || (machine != m_machine))
                {
                    Error("Incompatible machine, content, or boot mode in save state");
                    return false;
                }

                state_content_type = (GC_ContentType)content_type;
                state_adam_boot_mode = (GC_AdamBootMode)adam_boot_mode;
            }
            else if (m_machine != GC_MACHINE_COLECOVISION)
            {
                Error("Legacy save state is not an ADAM state");
                return false;
            }

            if (m_machine == GC_MACHINE_ADAM)
            {
                if (header.version < 108)
                {
                    Error("ADAM states before version 108 lack cartridge identity");
                    return false;
                }

                u32 cartridge_crc = 0;
                u32 cartridge_size = 0;
                u32 cartridge_type = 0;
                stream.read(reinterpret_cast<char*>(&cartridge_crc), sizeof(cartridge_crc));
                stream.read(reinterpret_cast<char*>(&cartridge_size), sizeof(cartridge_size));
                stream.read(reinterpret_cast<char*>(&cartridge_type), sizeof(cartridge_type));
                if (!stream.good() || (cartridge_crc != m_pCartridge->GetCRC()) ||
                    (cartridge_size != (u32)m_pCartridge->GetROMSize()) ||
                    (cartridge_type != (u32)m_pCartridge->GetType()) ||
                    ((state_adam_boot_mode == GC_ADAM_BOOT_CARTRIDGE) && !m_pCartridge->IsReady()))
                {
                    Error("Incompatible cartridge in ADAM save state");
                    return false;
                }
            }

            if (header.version >= 106)
            {
                GC_VideoChip video_chip;
                stream.read(reinterpret_cast<char*>(&video_chip), sizeof(video_chip));
                if ((video_chip != GC_VIDEO_CHIP_TMS9918A) && (video_chip != GC_VIDEO_CHIP_F18A))
                {
                    Error("Invalid video chip in save state");
                    return false;
                }
                SelectVideoChip(video_chip);
            }
            else
            {
                SelectVideoChip(GC_VIDEO_CHIP_TMS9918A);
            }

            if (m_machine == GC_MACHINE_ADAM)
            {
                if (m_video_chip != GC_VIDEO_CHIP_TMS9918A ||
                    !m_pAdam->LoadState(stream, state_adam_boot_mode))
                {
                    Error("Invalid or incompatible ADAM state");
                    return false;
                }

#if !defined(__LIBRETRO__)
                if (is_desktop_savestate)
                {
                    // Version 108 ADAM has a fixed component layout once the cartridge matches.
                    // A preview must never supply missing bytes from a truncated machine state.
                    counting_stream remaining;
                    m_pMemory->SaveState(remaining);
                    m_pProcessor->SaveState(remaining);
                    m_pAudio->SaveState(remaining);
                    m_pVideo->SaveState(remaining);
                    m_pInput->SaveState(remaining);
                    std::streampos position = stream.tellg();
                    if (!remaining.good() || (position < std::streampos(0)) ||
                        ((size_t)position > state_data_size) ||
                        (remaining.size() != state_data_size - (size_t)position))
                    {
                        Error("Invalid ADAM state data size");
                        return false;
                    }
                }
#endif
            }

            m_content_type = state_content_type;
            m_adam_boot_mode = state_adam_boot_mode;

            m_pMemory->LoadState(stream);
            if (!stream.good())
            {
                Error("Invalid memory or cartridge mapper state");
                return false;
            }
            m_pProcessor->LoadState(stream, header.version);

            if (header.version <= 102)
                m_pAudio->LoadStateV1(stream);
            else
                m_pAudio->LoadState(stream, header.version);

            m_pVideo->LoadState(stream, header.version);
            m_pInput->LoadState(stream, header.version);

            return stream.good();
        }

        // Try legacy V1 format (8-byte trailer: magic + size)
        if (size >= (2 * sizeof(u32)))
        {
            u32 v1_magic = 0;
            u32 v1_size = 0;

            stream.clear();
            stream.seekg(size - (2 * sizeof(u32)), ios::beg);
            stream.read(reinterpret_cast<char*>(&v1_magic), sizeof(v1_magic));
            stream.read(reinterpret_cast<char*>(&v1_size), sizeof(v1_size));
            stream.seekg(0, ios::beg);

            Debug("Load state V1 magic: 0x%08x", v1_magic);
            Debug("Load state V1 size: %d", v1_size);

            if ((m_machine == GC_MACHINE_COLECOVISION) && (v1_size == size) &&
                (v1_magic == GC_SAVESTATE_MAGIC))
            {
                Log("Loading legacy state...");

                SelectVideoChip(GC_VIDEO_CHIP_TMS9918A);
                m_pMemory->LoadState(stream);
                if (!stream.good())
                {
                    Error("Invalid memory or cartridge mapper state");
                    return false;
                }
                m_pProcessor->LoadState(stream, GC_SAVESTATE_VERSION_V1);
                m_pAudio->LoadStateV1(stream);
                m_pVideo->LoadState(stream, GC_SAVESTATE_VERSION_V1);
                m_pInput->LoadState(stream, GC_SAVESTATE_VERSION_V1);

                return true;
            }
        }

        Log("Invalid save state");
    }
    else
    {
        Log("Invalid rom");
    }

    return false;
}

bool GearcolecoCore::GetSaveStateHeader(int index, const char* path, GC_SaveState_Header* header)
{
    using namespace std;

    string full_path = GetSaveStatePath(path, index);
    Debug("Loading state header from %s...", full_path.c_str());

    ifstream stream;
    open_ifstream_utf8(stream, full_path.c_str(), ios::in | ios::binary);

    if (stream.fail())
    {
        Debug("Savestate file doesn't exist %s", full_path.c_str());
        stream.close();
        return false;
    }

    stream.seekg(0, ios::end);
    size_t savestate_size = static_cast<size_t>(stream.tellg());
    stream.seekg(0, ios::beg);

    if (savestate_size >= sizeof(GC_SaveState_Header))
    {
        stream.seekg(savestate_size - sizeof(GC_SaveState_Header), ios::beg);
        stream.read(reinterpret_cast<char*>(header), sizeof(GC_SaveState_Header));

        if ((header->magic == GC_SAVESTATE_MAGIC) && (header->size == savestate_size))
        {
            stream.close();
            return true;
        }
    }

    // Try legacy V1 format
    if (savestate_size >= (2 * sizeof(u32)))
    {
        u32 v1_magic = 0;
        u32 v1_size = 0;

        stream.clear();
        stream.seekg(savestate_size - (2 * sizeof(u32)), ios::beg);
        stream.read(reinterpret_cast<char*>(&v1_magic), sizeof(v1_magic));
        stream.read(reinterpret_cast<char*>(&v1_size), sizeof(v1_size));

        if ((v1_magic == GC_SAVESTATE_MAGIC) && (v1_size == savestate_size))
        {
            memset(header, 0, sizeof(GC_SaveState_Header));
            header->magic = v1_magic;
            header->version = GC_SAVESTATE_VERSION_V1;
            header->size = v1_size;
            strncpy_fit(header->rom_name, m_pCartridge->GetFileName(), sizeof(header->rom_name));
            stream.close();
            return true;
        }
    }

    stream.close();
    return false;
}

bool GearcolecoCore::GetSaveStateScreenshot(int index, const char* path, GC_SaveState_Screenshot* screenshot)
{
    using namespace std;

    if (!IsValidPointer(screenshot) || !IsValidPointer(screenshot->data) || (screenshot->size == 0))
    {
        Error("Invalid save state screenshot buffer");
        return false;
    }

    GC_SaveState_Header header;
    if (!GetSaveStateHeader(index, path, &header))
        return false;

    if (header.screenshot_size == 0)
    {
        Debug("No screenshot data");
        return false;
    }

    if (screenshot->size < header.screenshot_size)
    {
        Error("Invalid screenshot buffer size %d < %d", screenshot->size, header.screenshot_size);
        return false;
    }

    string full_path = GetSaveStatePath(path, index);
    Debug("Loading state screenshot from %s...", full_path.c_str());

    ifstream stream;
    open_ifstream_utf8(stream, full_path.c_str(), ios::in | ios::binary);

    if (stream.fail())
    {
        Error("Savestate file doesn't exist %s", full_path.c_str());
        stream.close();
        return false;
    }

    screenshot->size = header.screenshot_size;
    screenshot->width = header.screenshot_width;
    screenshot->height = header.screenshot_height;

    if (header.size < sizeof(header) + screenshot->size)
    {
        Error("Invalid screenshot offset");
        stream.close();
        return false;
    }

    stream.seekg(header.size - sizeof(header) - screenshot->size, ios::beg);
    stream.read(reinterpret_cast<char*>(screenshot->data), screenshot->size);
    stream.close();

    return stream.good();
}

void GearcolecoCore::Reset(bool cold)
{
    m_MasterClockCycles = 0;
    m_pMemory->SetupMapper();
    m_pAdam->SetMapper(m_pMemory->GetMapper());
    m_pAdam->SetEnabled(m_machine == GC_MACHINE_ADAM);
    m_pMemory->SetAdam(m_machine == GC_MACHINE_ADAM ? m_pAdam : NULL);
    m_pProcessor->SetIOPOrts(m_machine == GC_MACHINE_ADAM ? static_cast<IOPorts*>(m_pAdam) :
        static_cast<IOPorts*>(m_pColecoVisionIOPorts));
    m_pMemory->Reset();
    if (m_machine == GC_MACHINE_ADAM)
        m_pAdam->Reset(cold, m_adam_boot_mode);
    m_pProcessor->Reset();
    bool pal = (m_machine == GC_MACHINE_ADAM) ? false : m_pCartridge->IsPAL();
    m_pAudio->Reset(pal);
    m_pVideo->Reset(pal);
    m_pInput->Reset();
    m_pColecoVisionIOPorts->Reset();
    m_bPaused = false;
}

void GearcolecoCore::RenderFrameBuffer(u8* finalFrameBuffer)
{
    GC_RuntimeInfo runtime_info;
    GetRuntimeInfo(runtime_info);
    int size = IsReady() ? runtime_info.screen_width * runtime_info.screen_height :
        GC_RESOLUTION_WIDTH * GC_RESOLUTION_HEIGHT;
    u16* srcBuffer = (IsReady() ? m_pVideo->GetFrameBuffer() : kNoBiosImage);

    switch (m_pixelFormat)
    {
        case GC_PIXEL_RGB555:
        case GC_PIXEL_BGR555:
        case GC_PIXEL_RGB565:
        case GC_PIXEL_BGR565:
        {
            m_pVideo->Render16bit(srcBuffer, finalFrameBuffer, m_pixelFormat, size, true);
            break;
        }
        case GC_PIXEL_RGBA8888:
        case GC_PIXEL_BGRA8888:
        {
            m_pVideo->Render32bit(srcBuffer, finalFrameBuffer, m_pixelFormat, size, true);
            break;
        }
    }
}
