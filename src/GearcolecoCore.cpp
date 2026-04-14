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
#include "common.h"
#include "memory_stream.h"

GearcolecoCore::GearcolecoCore()
{
    InitPointer(m_pMemory);
    InitPointer(m_pProcessor);
    InitPointer(m_pAudio);
    InitPointer(m_pVideo);
    InitPointer(m_pInput);
    InitPointer(m_pCartridge);
    InitPointer(m_pColecoVisionIOPorts);
    InitPointer(m_pFrameBuffer);
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
    m_pFrameBuffer = pFrameBuffer;

    if (!m_pMemory->IsBiosLoaded())
    {
        RenderFrameBuffer(pFrameBuffer);
        return false;
    }

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
            m_pMemory->Tick(clockCycles);

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

        m_pMemory->SetupMapper();
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

        m_pMemory->SetupMapper();
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

        ofstream myfile;
        open_ofstream_utf8(myfile, path, ios::out | ios::trunc);

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

    if (m_pCartridge->IsReady() && m_pMemory->IsBiosLoaded())
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
        path += "/";
        path += m_pCartridge->GetFileName();
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

    if (!IsValidPointer(pMapper) || pMapper->GetSaveDataSize() <= 0)
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
        path += "/";
        path += m_pCartridge->GetFileName();
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
        file.read(reinterpret_cast<char*>(pMapper->GetSaveData()), pMapper->GetSaveDataSize());
        file.close();
        Log("RAM loaded from %s", path.c_str());
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
        sav_path += "/";
        sav_path += m_pCartridge->GetFileName();
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
        sstm << path;
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

    size_t size;
    SaveState(file, size, screenshot);

    file.close();

    Debug("Save state created");
    return true;
}

bool GearcolecoCore::SaveState(u8* buffer, size_t& size, bool screenshot)
{
    using namespace std;

    Debug("Saving state to buffer [%d bytes]...", size);

    if (!m_pCartridge->IsReady())
    {
        Error("Cartridge is not ready when trying to save state");
        return false;
    }

    if (!IsValidPointer(buffer))
    {
        stringstream stream;
        if (!SaveState(stream, size, screenshot))
        {
            Error("Failed to save state to stream to calculate size");
            return false;
        }
        return true;
    }
    else
    {
        memory_stream direct_stream(reinterpret_cast<char*>(buffer), size);

        if (!SaveState(direct_stream, size, screenshot))
        {
            Error("Failed to save state to buffer");
            return false;
        }

        size = direct_stream.size();
        return true;
    }
}

bool GearcolecoCore::SaveState(std::ostream& stream, size_t& size, bool screenshot)
{
    if (m_pCartridge->IsReady())
    {
        Debug("Gathering save state data...");

        m_pMemory->SaveState(stream);
        m_pProcessor->SaveState(stream);
        m_pAudio->SaveState(stream);
        m_pVideo->SaveState(stream);
        m_pInput->SaveState(stream);

#if defined(__LIBRETRO__)
        GC_SaveState_Header_Libretro header;
        memset(&header, 0, sizeof(header));
        header.magic = GC_SAVESTATE_MAGIC;
        header.version = GC_SAVESTATE_VERSION;
        Debug("Save state header magic: 0x%08x", header.magic);
        Debug("Save state header version: %d", header.version);
#else
        GC_SaveState_Header header;
        memset(&header, 0, sizeof(header));
        header.magic = GC_SAVESTATE_MAGIC;
        header.version = GC_SAVESTATE_VERSION;
        header.timestamp = time(NULL);
        strncpy_fit(header.rom_name, m_pCartridge->GetFileName(), sizeof(header.rom_name));
        header.rom_crc = m_pCartridge->GetCRC();
        strncpy_fit(header.emu_build, GEARCOLECO_VERSION, sizeof(header.emu_build));

        Debug("Save state header magic: 0x%08x", header.magic);
        Debug("Save state header version: %d", header.version);

        if (screenshot && IsValidPointer(m_pFrameBuffer))
        {
            GC_RuntimeInfo runtime_info;
            GetRuntimeInfo(runtime_info);
            header.screenshot_width = runtime_info.screen_width;
            header.screenshot_height = runtime_info.screen_height;

            int bytes_per_pixel = 3;
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
        if (LoadState(file))
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

    if (!m_pCartridge->IsReady())
    {
        Error("Cartridge is not ready when trying to load state");
        return false;
    }

    if (!IsValidPointer(buffer) || (size == 0))
    {
        Error("Invalid load state buffer");
        return false;
    }

    memory_input_stream direct_stream(reinterpret_cast<const char*>(buffer), size);
    return LoadState(direct_stream);
}

bool GearcolecoCore::LoadState(std::istream& stream)
{
    if (m_pCartridge->IsReady())
    {
        using namespace std;

        stream.seekg(0, ios::end);
        size_t size = static_cast<size_t>(stream.tellg());

        Debug("Load state stream size: %d", size);

        GC_SaveState_Header_Libretro header;
#if !defined(__LIBRETRO__)
        bool is_desktop_savestate = false;
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
                Log("Loading desktop save state");
            }
        }

        // Fallback to libretro header
        if (header.magic != GC_SAVESTATE_MAGIC)
        {
            stream.seekg(size - sizeof(header), ios::beg);
            stream.read(reinterpret_cast<char*>(&header), sizeof(header));
        }

        stream.seekg(0, ios::beg);

        Debug("Load state header magic: 0x%08x", header.magic);
        Debug("Load state header version: %d", header.version);

        if (header.magic == GC_SAVESTATE_MAGIC && header.version >= GC_SAVESTATE_MIN_VERSION)
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
            }
#endif

            Log("Loading state (v%d)...", header.version);

            m_pMemory->LoadState(stream);
            m_pProcessor->LoadState(stream);
            m_pAudio->LoadState(stream);
            m_pVideo->LoadState(stream);
            m_pInput->LoadState(stream, header.version);

            return true;
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

            if ((v1_size == size) && (v1_magic == GC_SAVESTATE_MAGIC))
            {
                Log("Loading legacy state...");

                m_pMemory->LoadState(stream);
                m_pProcessor->LoadState(stream);
                m_pAudio->LoadState(stream);
                m_pVideo->LoadState(stream);
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
        stream.close();

        if ((header->magic == GC_SAVESTATE_MAGIC) && (header->size == savestate_size))
            return true;
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
        stream.close();

        if ((v1_magic == GC_SAVESTATE_MAGIC) && (v1_size == savestate_size))
        {
            memset(header, 0, sizeof(GC_SaveState_Header));
            header->magic = v1_magic;
            header->version = GC_SAVESTATE_VERSION_V1;
            header->size = v1_size;
            strncpy_fit(header->rom_name, m_pCartridge->GetFileName(), sizeof(header->rom_name));
            return true;
        }
    }

    stream.close();
    return false;
}

bool GearcolecoCore::GetSaveStateScreenshot(int index, const char* path, GC_SaveState_Screenshot* screenshot)
{
    using namespace std;

    if (!IsValidPointer(screenshot->data) || (screenshot->size == 0))
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

    return true;
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
    int size = m_pMemory->IsBiosLoaded() ? GC_RESOLUTION_WIDTH_WITH_OVERSCAN * GC_RESOLUTION_HEIGHT_WITH_OVERSCAN : GC_RESOLUTION_WIDTH * GC_RESOLUTION_HEIGHT;
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
