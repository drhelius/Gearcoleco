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
#include "GearcolecoCore.h"
#include "Memory.h"
#include "Processor.h"
#include "Audio.h"
#include "Video.h"
#include "Input.h"
#include "Cartridge.h"
#include "ColecoVisionIOPorts.h"
#include "no_bios.h"

GearcolecoCore::GearcolecoCore()
{
    InitPointer(m_pMemory);
    InitPointer(m_pProcessor);
    InitPointer(m_pAudio);
    InitPointer(m_pVideo);
    InitPointer(m_pInput);
    InitPointer(m_pCartridge);
    InitPointer(m_pColecoVisionIOPorts);
    m_bPaused = true;
    m_pixelFormat = GC_PIXEL_RGB888;
}

GearcolecoCore::~GearcolecoCore()
{
    SafeDelete(m_pColecoVisionIOPorts);
    SafeDelete(m_pCartridge);
    SafeDelete(m_pInput);
    SafeDelete(m_pVideo);
    SafeDelete(m_pAudio);
    SafeDelete(m_pProcessor);
    SafeDelete(m_pMemory);
}

void GearcolecoCore::Init(GC_Color_Format pixelFormat)
{
    Log("Loading %s core %s by Ignacio Sanchez", GEARCOLECO_TITLE, GEARCOLECO_VERSION);

    m_pixelFormat = pixelFormat;

    m_pCartridge = new Cartridge();
    m_pMemory = new Memory(m_pCartridge);
    m_pProcessor = new Processor(m_pMemory);
    m_pAudio = new Audio();
    m_pVideo = new Video(m_pMemory, m_pProcessor);
    m_pInput = new Input(m_pProcessor);
    m_pColecoVisionIOPorts = new ColecoVisionIOPorts(m_pAudio, m_pVideo, m_pInput, m_pCartridge, m_pMemory, m_pProcessor);

    m_pMemory->Init();
    m_pProcessor->Init();
    m_pAudio->Init();
    m_pVideo->Init();
    m_pInput->Init();
    m_pCartridge->Init();

    m_pProcessor->SetIOPOrts(m_pColecoVisionIOPorts);
}

bool GearcolecoCore::RunToVBlank(u8* pFrameBuffer, s16* pSampleBuffer, int* pSampleCount, bool step, bool stopOnBreakpoints)
{
    bool breakpoint = false;

    if (!m_bPaused && m_pCartridge->IsReady())
    {
        bool vblank = false;
        int totalClocks = 0;
        while (!vblank)
        {
#ifdef PERFORMANCE
            unsigned int clockCycles = m_pProcessor->RunFor(75);
#else
            unsigned int clockCycles = m_pProcessor->RunFor(1);
#endif
            vblank = m_pVideo->Tick(clockCycles);
            m_pAudio->Tick(clockCycles);

            totalClocks += clockCycles;

#ifndef GEARCOLECO_DISABLE_DISASSEMBLER
            if ((step || (stopOnBreakpoints && m_pProcessor->BreakpointHit())) && !m_pProcessor->DuringInputOpcode())
            {
                vblank = true;
                if (m_pProcessor->BreakpointHit())
                    breakpoint = true;
            }
#endif

            if (totalClocks > 702240)
                vblank = true;
        }

        m_pAudio->EndFrame(pSampleBuffer, pSampleCount);
        RenderFrameBuffer(pFrameBuffer);
    }

    return breakpoint;
}

bool GearcolecoCore::LoadROM(const char* szFilePath, Cartridge::ForceConfiguration* config)
{
    if (m_pCartridge->LoadFromFile(szFilePath))
    {
        if (IsValidPointer(config))
            m_pCartridge->ForceConfig(*config);

        Reset();

        m_pMemory->ResetRomDisassembledMemory();
        m_pProcessor->DisassembleNextOpcode();

        return true;
    }
    else
        return false;
}

bool GearcolecoCore::LoadROMFromBuffer(const u8* buffer, int size, Cartridge::ForceConfiguration* config)
{
    if (m_pCartridge->LoadFromBuffer(buffer, size))
    {
        if (IsValidPointer(config))
            m_pCartridge->ForceConfig(*config);

        Reset();

        m_pMemory->ResetRomDisassembledMemory();
        m_pProcessor->DisassembleNextOpcode();

        return true;
    }
    else
        return false;
}

void GearcolecoCore::SaveDisassembledROM()
{
    Memory::stDisassembleRecord** biosMap = m_pMemory->GetDisassembledBiosMemoryMap();
    Memory::stDisassembleRecord** romMap = m_pMemory->GetDisassembledRomMemoryMap();

    if (m_pCartridge->IsReady() && (strlen(m_pCartridge->GetFilePath()) > 0) && IsValidPointer(romMap))
    {
        using namespace std;

        char path[512];

        strcpy(path, m_pCartridge->GetFilePath());
        strcat(path, ".dis");

        Log("Saving Disassembled ROM %s...", path);

        ofstream myfile(path, ios::out | ios::trunc);

        if (myfile.is_open())
        {
            #define PAD_ADDR(digits) std::uppercase << std::hex << std::setw(digits) << std::setfill('0')
            #define PAD_MEM(chars) std::setw(chars) << std::setfill(' ')

            for (int i = 0; i < 0x2000; i++)
            {
                if (IsValidPointer(biosMap[i]) && (biosMap[i]->name[0] != 0))
                {
                    myfile << "BIOS $" << PAD_ADDR(4) << i << "   " << PAD_MEM(12) << biosMap[i]->bytes << "  " << biosMap[i]->name << "\n";
                }
            }

            for (int i = 0; i < MAX_ROM_SIZE; i++)
            {
                if (IsValidPointer(romMap[i]) && (romMap[i]->name[0] != 0))
                {
                    myfile << "ROM  $" << PAD_ADDR(4) << i + 0x8000 << "   " << PAD_MEM(12) << romMap[i]->bytes << "  " << romMap[i]->name << "\n";
                }
            }

            myfile.close();
        }

        Debug("Disassembled ROM Saved");
    }
}

bool GearcolecoCore::GetRuntimeInfo(GC_RuntimeInfo& runtime_info)
{
    runtime_info.screen_width = GC_RESOLUTION_WIDTH;
    runtime_info.screen_height = GC_RESOLUTION_HEIGHT;
    runtime_info.region = Region_NTSC;

    if (m_pCartridge->IsReady())
    {
        if (m_pVideo->GetOverscan() == Video::OverscanFull284)
            runtime_info.screen_width = GC_RESOLUTION_WIDTH + GC_RESOLUTION_SMS_OVERSCAN_H_284_L + GC_RESOLUTION_SMS_OVERSCAN_H_284_R;
        if (m_pVideo->GetOverscan() == Video::OverscanFull320)
            runtime_info.screen_width = GC_RESOLUTION_WIDTH + GC_RESOLUTION_SMS_OVERSCAN_H_320_L + GC_RESOLUTION_SMS_OVERSCAN_H_320_R;
        if (m_pVideo->GetOverscan() != Video::OverscanDisabled)
            runtime_info.screen_height = GC_RESOLUTION_HEIGHT + (2 * (m_pCartridge->IsPAL() ? GC_RESOLUTION_OVERSCAN_V_PAL : GC_RESOLUTION_OVERSCAN_V));
        runtime_info.region = m_pCartridge->IsPAL() ? Region_PAL : Region_NTSC;
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
        Log("Gearcoleco PAUSED");
    }
    else
    {
        Log("Gearcoleco RESUMED");
    }
    m_bPaused = paused;
}

bool GearcolecoCore::IsPaused()
{
    return m_bPaused;
}

void GearcolecoCore::ResetROM(Cartridge::ForceConfiguration* config)
{
    if (m_pCartridge->IsReady())
    {
        Log("Gearcoleco RESET");

        if (IsValidPointer(config))
            m_pCartridge->ForceConfig(*config);

        Reset();

        m_pProcessor->DisassembleNextOpcode();
    }
}

void GearcolecoCore::ResetROMPreservingRAM(Cartridge::ForceConfiguration* config)
{
    // TODO

    ResetROM(config);
}

void GearcolecoCore::ResetSound()
{
    m_pAudio->Reset(m_pCartridge->IsPAL());
}

void GearcolecoCore::SaveRam()
{
    SaveRam(NULL);
}

void GearcolecoCore::SaveRam(const char*, bool)
{
    // TODO
}

void GearcolecoCore::LoadRam()
{
    LoadRam(NULL);
}

void GearcolecoCore::LoadRam(const char*, bool)
{
    // TODO
}

void GearcolecoCore::SaveState(int index)
{
    Log("Creating save state %d...", index);

    SaveState(NULL, index);

    Debug("Save state %d created", index);
}

void GearcolecoCore::SaveState(const char* szPath, int index)
{
    Log("Creating save state...");

    using namespace std;

    size_t size;
    SaveState(NULL, size);

    u8* buffer = new u8[size];
    string path = "";

    if (IsValidPointer(szPath))
    {
        path += szPath;
        path += "/";
        path += m_pCartridge->GetFileName();
    }
    else
    {
        path = m_pCartridge->GetFilePath();
    }

    string::size_type i = path.rfind('.', path.length());

    if (i != string::npos) {
        path.replace(i + 1, 3, "state");
    }

    std::stringstream sstm;

    if (index < 0)
        sstm << szPath;
    else
        sstm << path << index;

    Log("Save state file: %s", sstm.str().c_str());

    ofstream file(sstm.str().c_str(), ios::out | ios::binary);

    SaveState(file, size);

    SafeDeleteArray(buffer);

    file.close();

    Debug("Save state created");
}

bool GearcolecoCore::SaveState(u8* buffer, size_t& size)
{
    bool ret = false;

    if (m_pCartridge->IsReady())
    {
        using namespace std;

        stringstream stream;

        if (SaveState(stream, size))
            ret = true;

        if (IsValidPointer(buffer))
        {
            Log("Saving state to buffer [%d bytes]...", size);
            memcpy(buffer, stream.str().c_str(), size);
            ret = true;
        }
    }
    else
    {
        Log("Invalid rom.");
    }

    return ret;
}

bool GearcolecoCore::SaveState(std::ostream& stream, size_t& size)
{
    if (m_pCartridge->IsReady())
    {
        Debug("Gathering save state data...");

        m_pMemory->SaveState(stream);
        m_pProcessor->SaveState(stream);
        m_pAudio->SaveState(stream);
        m_pVideo->SaveState(stream);
        m_pInput->SaveState(stream);

        size = static_cast<size_t>(stream.tellp());
        size += (sizeof(u32) * 2);

        u32 header_magic = GC_SAVESTATE_MAGIC;
        u32 header_size = static_cast<u32>(size);

        stream.write(reinterpret_cast<const char*> (&header_magic), sizeof(header_magic));
        stream.write(reinterpret_cast<const char*> (&header_size), sizeof(header_size));

        Debug("Save state size: %d", static_cast<size_t>(stream.tellp()));

        return true;
    }

    Log("Invalid rom.");

    return false;
}

void GearcolecoCore::LoadState(int index)
{
    Log("Loading save state %d...", index);

    LoadState(NULL, index);

    Debug("State %d file loaded", index);
}

void GearcolecoCore::LoadState(const char* szPath, int index)
{
    Log("Loading save state...");

    using namespace std;

    string sav_path = "";

    if (IsValidPointer(szPath))
    {
        sav_path += szPath;
        sav_path += "/";
        sav_path += m_pCartridge->GetFileName();
    }
    else
    {
        sav_path = m_pCartridge->GetFilePath();
    }

    string rom_path = sav_path;

    string::size_type i = sav_path.rfind('.', sav_path.length());

    if (i != string::npos) {
        sav_path.replace(i + 1, 3, "state");
    }

    std::stringstream sstm;

    if (index < 0)
        sstm << szPath;
    else
        sstm << sav_path << index;

    Log("Opening save file: %s", sstm.str().c_str());

    ifstream file;

    file.open(sstm.str().c_str(), ios::in | ios::binary);

    if (!file.fail())
    {
        if (LoadState(file))
        {
            Debug("Save state loaded");
        }
    }
    else
    {
        Log("Save state file doesn't exist");
    }

    file.close();
}

bool GearcolecoCore::LoadState(const u8* buffer, size_t size)
{
    if (m_pCartridge->IsReady() && (size > 0) && IsValidPointer(buffer))
    {
        Debug("Gathering load state data [%d bytes]...", size);

        using namespace std;

        stringstream stream;

        stream.write(reinterpret_cast<const char*> (buffer), size);

        return LoadState(stream);
    }

    Log("Invalid rom or memory.");

    return false;
}

bool GearcolecoCore::LoadState(std::istream& stream)
{
    if (m_pCartridge->IsReady())
    {
        using namespace std;

        u32 header_magic = 0;
        u32 header_size = 0;

        stream.seekg(0, ios::end);
        size_t size = static_cast<size_t>(stream.tellg());
        stream.seekg(0, ios::beg);

        Debug("Load state stream size: %d", size);

        stream.seekg(size - (2 * sizeof(u32)), ios::beg);
        stream.read(reinterpret_cast<char*> (&header_magic), sizeof(header_magic));
        stream.read(reinterpret_cast<char*> (&header_size), sizeof(header_size));
        stream.seekg(0, ios::beg);

        Debug("Load state magic: 0x%08x", header_magic);
        Debug("Load state size: %d", header_size);

        if ((header_size == size) && (header_magic == GC_SAVESTATE_MAGIC))
        {
            Log("Loading state...");

            m_pMemory->LoadState(stream);
            m_pProcessor->LoadState(stream);
            m_pAudio->LoadState(stream);
            m_pVideo->LoadState(stream);
            m_pInput->LoadState(stream);

            return true;
        }
        else
        {
            Log("Invalid save state size or header");
        }
    }
    else
    {
        Log("Invalid rom");
    }

    return false;
}

void GearcolecoCore::Reset()
{
    m_pMemory->Reset();
    m_pProcessor->Reset();
    m_pAudio->Reset(m_pCartridge->IsPAL());
    m_pVideo->Reset(m_pCartridge->IsPAL());
    m_pInput->Reset();
    m_pColecoVisionIOPorts->Reset();
    m_bPaused = false;
}

void GearcolecoCore::RenderFrameBuffer(u8* finalFrameBuffer)
{
    int size = GC_RESOLUTION_WIDTH_WITH_OVERSCAN * GC_RESOLUTION_HEIGHT_WITH_OVERSCAN;
    u16* srcBuffer = (m_pMemory->IsBiosLoaded() ? m_pVideo->GetFrameBuffer() : kNoBiosImage);

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
        case GC_PIXEL_RGB888:
        case GC_PIXEL_BGR888:
        {
            m_pVideo->Render24bit(srcBuffer, finalFrameBuffer, m_pixelFormat, size, true);
            break;
        }
    }
}
