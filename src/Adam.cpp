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
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#include <string.h>
#include "Adam.h"
#include "AdamNet.h"
#include "Mapper.h"
#include "TraceLogger.h"

static const Adam::FirmwareMetadata kAdamFirmwareMetadata[GC_ADAM_FIRMWARE_COUNT] =
{
    {
        "OS-7", Adam::kOS7ROMSize, 0x3AA93EF3,
        "45bedc4cbdeac66c7df59e9e599195c778d86a92",
        { "colecovision.rom", "coleco.rom", "os7.u2", NULL }
    },
    {
        "EOS", Adam::kEOSROMSize, 0x05A37A34, NULL,
        { "eos.rom", NULL, NULL, NULL }
    },
    {
        "SmartWriter", Adam::kSmartWriterROMSize, 0x58D86A2A,
        "d4aec4efe1431e56fe52d83baf9118542c525255",
        { "writer.rom", "wp.rom", "wp_r80.rom", NULL }
    }
};

static u32 CalculateAdamCRC32(const u8* data, int size)
{
    u32 crc = ~0U;

    while (size-- > 0)
    {
        crc ^= *data++;

        for (int bit = 0; bit < 8; bit++)
        {
            u32 mask = 0U - (crc & 1U);
            crc = (crc >> 1) ^ (0xEDB88320U & mask);
        }
    }

    return crc ^ ~0U;
}

const Adam::FirmwareMetadata* Adam::GetFirmwareMetadata(GC_AdamFirmware firmware)
{
    if ((firmware < GC_ADAM_FIRMWARE_OS7) || (firmware >= GC_ADAM_FIRMWARE_COUNT))
        return NULL;
    return &kAdamFirmwareMetadata[firmware];
}

Adam::Adam()
{
    InitPointer(m_pSharedPorts);
    InitPointer(m_pMapper);
    InitPointer(m_pTraceLogger);
    InitPointer(m_pAdamNet);
    InitPointer(m_pOS7ROM);
    InitPointer(m_pEOSROM);
    InitPointer(m_pSmartWriterROM);
    InitPointer(m_pMainRAM);
    memset(m_FirmwareCRC, 0, sizeof(m_FirmwareCRC));
    memset(m_FirmwareLoaded, 0, sizeof(m_FirmwareLoaded));
    m_Enabled = false;
    m_MIOC = 0;
    m_Control = 0;
    m_BootMode = GC_ADAM_BOOT_COMPUTER;
    m_MemoryMapGeneration = 0;

    for (int i = 0; i < kPageCount; i++)
        MapOpenBus(i);
}

Adam::~Adam()
{
    SafeDelete(m_pAdamNet);
    SafeDeleteArray(m_pMainRAM);
    SafeDeleteArray(m_pSmartWriterROM);
    SafeDeleteArray(m_pEOSROM);
    SafeDeleteArray(m_pOS7ROM);
}

void Adam::Init(IOPorts* shared_ports)
{
    m_pSharedPorts = shared_ports;
    AllocateStorage();
    m_pAdamNet = new AdamNet();
    m_pAdamNet->Init(this);
}

void Adam::AllocateStorage()
{
    if (!IsValidPointer(m_pOS7ROM))
    {
        m_pOS7ROM = new u8[kOS7ROMSize];
        memset(m_pOS7ROM, 0xFF, kOS7ROMSize);
    }
    if (!IsValidPointer(m_pEOSROM))
    {
        m_pEOSROM = new u8[kEOSROMSize];
        memset(m_pEOSROM, 0xFF, kEOSROMSize);
    }
    if (!IsValidPointer(m_pSmartWriterROM))
    {
        m_pSmartWriterROM = new u8[kSmartWriterROMSize];
        memset(m_pSmartWriterROM, 0xFF, kSmartWriterROMSize);
    }
    if (!IsValidPointer(m_pMainRAM))
    {
        m_pMainRAM = new u8[kMainRAMSize];
        InitializeRAM();
    }
}

void Adam::SetMapper(Mapper* mapper)
{
    m_pMapper = mapper;
}

void Adam::SetTraceLogger(TraceLogger* trace_logger)
{
    m_pTraceLogger = trace_logger;
    if (IsValidPointer(m_pAdamNet))
        m_pAdamNet->SetTraceLogger(trace_logger);
}

void Adam::SetEnabled(bool enabled)
{
    m_Enabled = enabled;

    if (m_Enabled)
        SetMemoryMap();
    else
    {
        for (int i = 0; i < kPageCount; i++)
            MapOpenBus(i);
        m_MemoryMapGeneration++;
    }
}

bool Adam::IsFirmwareReady() const
{
    for (int i = 0; i < GC_ADAM_FIRMWARE_COUNT; i++)
    {
        if (!m_FirmwareLoaded[i])
            return false;
    }

    return true;
}

bool Adam::LoadFirmware(const u8* os7, int os7_size, const u8* eos, int eos_size,
    const u8* smartwriter, int smartwriter_size)
{
    if (!IsValidPointer(os7) || (os7_size != kOS7ROMSize) ||
        !IsValidPointer(eos) || (eos_size != kEOSROMSize) ||
        !IsValidPointer(smartwriter) || (smartwriter_size != kSmartWriterROMSize))
    {
        return false;
    }

    AllocateStorage();
    memcpy(m_pOS7ROM, os7, kOS7ROMSize);
    memcpy(m_pEOSROM, eos, kEOSROMSize);
    memcpy(m_pSmartWriterROM, smartwriter, kSmartWriterROMSize);
    m_FirmwareCRC[GC_ADAM_FIRMWARE_OS7] = CalculateAdamCRC32(os7, os7_size);
    m_FirmwareCRC[GC_ADAM_FIRMWARE_EOS] = CalculateAdamCRC32(eos, eos_size);
    m_FirmwareCRC[GC_ADAM_FIRMWARE_SMARTWRITER] = CalculateAdamCRC32(smartwriter, smartwriter_size);
    memset(m_FirmwareLoaded, true, sizeof(m_FirmwareLoaded));

    if (m_Enabled)
        SetMemoryMap();

    return true;
}

bool Adam::LoadFirmware(GC_AdamFirmware firmware, const u8* data, int size)
{
    if ((firmware < GC_ADAM_FIRMWARE_OS7) || (firmware >= GC_ADAM_FIRMWARE_COUNT) ||
        !IsValidPointer(data))
    {
        return false;
    }

    int expected_size = 0;
    u8* destination = NULL;
    AllocateStorage();

    switch (firmware)
    {
        case GC_ADAM_FIRMWARE_OS7:
            expected_size = kOS7ROMSize;
            destination = m_pOS7ROM;
            break;
        case GC_ADAM_FIRMWARE_EOS:
            expected_size = kEOSROMSize;
            destination = m_pEOSROM;
            break;
        case GC_ADAM_FIRMWARE_SMARTWRITER:
            expected_size = kSmartWriterROMSize;
            destination = m_pSmartWriterROM;
            break;
        default:
            return false;
    }

    if (size != expected_size)
        return false;

    memcpy(destination, data, size);
    m_FirmwareCRC[firmware] = CalculateAdamCRC32(data, size);
    m_FirmwareLoaded[firmware] = true;

    if (m_Enabled)
        SetMemoryMap();

    return true;
}

void Adam::UnloadFirmware()
{
    memset(m_FirmwareCRC, 0, sizeof(m_FirmwareCRC));
    memset(m_FirmwareLoaded, 0, sizeof(m_FirmwareLoaded));

    if (IsValidPointer(m_pOS7ROM))
        memset(m_pOS7ROM, 0xFF, kOS7ROMSize);
    if (IsValidPointer(m_pEOSROM))
        memset(m_pEOSROM, 0xFF, kEOSROMSize);
    if (IsValidPointer(m_pSmartWriterROM))
        memset(m_pSmartWriterROM, 0xFF, kSmartWriterROMSize);

    if (m_Enabled)
        SetMemoryMap();
}

void Adam::InitializeRAM()
{
    if (!IsValidPointer(m_pMainRAM))
        return;

    for (int i = 0; i < kMainRAMSize; i++)
        m_pMainRAM[i] = (i & 1) ? 0xFF : 0x00;
}

void Adam::Reset(bool cold, GC_AdamBootMode boot_mode)
{
    AllocateStorage();

    if (cold)
        InitializeRAM();

    m_BootMode = boot_mode;
    m_Control = 0;
    m_MIOC = (m_BootMode == GC_ADAM_BOOT_CARTRIDGE) ? 0x0F : 0x00;
    SetMemoryMap();
    m_pAdamNet->Reset(cold);
}

void Adam::Reset()
{
    Reset(false, m_BootMode);
}

void Adam::Clock(unsigned int cycles)
{
    if (IsValidPointer(m_pAdamNet))
        m_pAdamNet->Clock(cycles);
}

void Adam::KeyPressed(GC_AdamKey key)
{
    m_pAdamNet->KeyPressed(key);
}

void Adam::KeyReleased(GC_AdamKey key)
{
    m_pAdamNet->KeyReleased(key);
}

void Adam::ReleaseAllKeys()
{
    m_pAdamNet->ReleaseAllKeys();
}

GC_AdamMediaError Adam::InsertMedia(GC_AdamMediaSlot slot, GC_AdamMediaType type,
    const u8* data, size_t size, bool write_protected, u32 base_crc)
{
    return m_pAdamNet->InsertMedia(slot, type, data, size, write_protected, base_crc);
}

void Adam::EjectMedia(GC_AdamMediaSlot slot)
{
    m_pAdamNet->EjectMedia(slot);
}

AdamMedia* Adam::GetMedia(GC_AdamMediaSlot slot)
{
    return m_pAdamNet->GetMedia(slot);
}

const AdamMedia* Adam::GetMedia(GC_AdamMediaSlot slot) const
{
    return m_pAdamNet->GetMedia(slot);
}

AdamNet* Adam::GetAdamNet()
{
    return m_pAdamNet;
}

const AdamNet* Adam::GetAdamNet() const
{
    return m_pAdamNet;
}

void Adam::SaveState(std::ostream& stream) const
{
    u8 enabled = m_Enabled ? 1 : 0;
    u8 boot_mode = (u8)m_BootMode;

    stream.write(reinterpret_cast<const char*>(&enabled), sizeof(enabled));
    stream.write(reinterpret_cast<const char*>(&boot_mode), sizeof(boot_mode));
    stream.write(reinterpret_cast<const char*>(&m_MIOC), sizeof(m_MIOC));
    stream.write(reinterpret_cast<const char*>(&m_Control), sizeof(m_Control));
    stream.write(reinterpret_cast<const char*>(m_FirmwareCRC), sizeof(m_FirmwareCRC));
    stream.write(reinterpret_cast<const char*>(m_pMainRAM), kMainRAMSize);
    m_pAdamNet->SaveState(stream);
}

bool Adam::LoadState(std::istream& stream)
{
    u8 enabled = 0;
    u8 boot_mode = 0;
    u8 mioc = 0;
    u8 control = 0;
    u32 firmware_crc[GC_ADAM_FIRMWARE_COUNT];
    u8* ram = new u8[kMainRAMSize];

    stream.read(reinterpret_cast<char*>(&enabled), sizeof(enabled));
    stream.read(reinterpret_cast<char*>(&boot_mode), sizeof(boot_mode));
    stream.read(reinterpret_cast<char*>(&mioc), sizeof(mioc));
    stream.read(reinterpret_cast<char*>(&control), sizeof(control));
    stream.read(reinterpret_cast<char*>(firmware_crc), sizeof(firmware_crc));
    stream.read(reinterpret_cast<char*>(ram), kMainRAMSize);

    if (!stream.good() || (enabled != 1) || (boot_mode > GC_ADAM_BOOT_CARTRIDGE))
    {
        SafeDeleteArray(ram);
        return false;
    }

    for (int i = 0; i < GC_ADAM_FIRMWARE_COUNT; i++)
    {
        if (!m_FirmwareLoaded[i] || (firmware_crc[i] != m_FirmwareCRC[i]))
        {
            SafeDeleteArray(ram);
            return false;
        }
    }

    if (!m_pAdamNet->LoadState(stream))
    {
        SafeDeleteArray(ram);
        return false;
    }

    memcpy(m_pMainRAM, ram, kMainRAMSize);
    SafeDeleteArray(ram);
    m_Enabled = true;
    m_BootMode = (GC_AdamBootMode)boot_mode;
    m_MIOC = mioc;
    m_Control = control;
    SetMemoryMap();
    return true;
}

bool Adam::HandlesPort(u8 port) const
{
    u8 family = port & 0xE0;
    return (family == 0x20) || (family == 0x60);
}

u8 Adam::In(u8 port)
{
    switch (port & 0xE0)
    {
        case 0x20:
            return m_Control & 0x0F;
        case 0x60:
            return m_MIOC & 0x0F;
        case 0xA0:
        case 0xE0:
            return IsValidPointer(m_pSharedPorts) ? m_pSharedPorts->In(port) : 0xFF;
        default:
            return 0xFF;
    }
}

void Adam::Out(u8 port, u8 value)
{
    switch (port & 0xE0)
    {
        case 0x20:
            WriteControl(value);
            break;
        case 0x60:
            WriteMIOC(value);
            break;
        case 0x80:
        case 0xA0:
        case 0xC0:
        case 0xE0:
            if (IsValidPointer(m_pSharedPorts))
                m_pSharedPorts->Out(port, value);
            break;
        default:
            break;
    }
}

void Adam::WriteMIOC(u8 value)
{
    u8 old_value = m_MIOC;
    bool mapping_changed = (old_value & 0x0F) != (value & 0x0F);
    m_MIOC = value;
    if (old_value != value)
        TraceMapChange(0, old_value, value, false);
    if (mapping_changed)
        SetMemoryMap();
}

void Adam::WriteControl(u8 value)
{
    u8 old_value = m_Control;
    bool reset = (old_value & 0x01) && !(value & 0x01);
    bool mapping_changed = (old_value & 0x02) != (value & 0x02);
    m_Control = value;
    if (old_value != value)
        TraceMapChange(1, old_value, value, reset);
    if (mapping_changed)
        SetMemoryMap();

    if (reset && IsValidPointer(m_pAdamNet))
        m_pAdamNet->Reset(false);
}

void Adam::TraceMapChange(u8 target, u8 old_value, u8 new_value, bool reset) const
{
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    if (!IsValidPointer(m_pTraceLogger) ||
        !m_pTraceLogger->IsEventEnabled(TRACE_ADAM, TRACE_ADAM_MAP))
    {
        return;
    }
    GC_Trace_Entry entry = {};
    entry.type = TRACE_ADAM;
    entry.adam.event = TRACE_ADAM_MAP;
    entry.adam.target = target;
    entry.adam.old_value = old_value;
    entry.adam.new_value = new_value;
    entry.adam.error = reset ? 1 : 0;
    m_pTraceLogger->TraceLog(entry);
#else
    UNUSED(target);
    UNUSED(old_value);
    UNUSED(new_value);
    UNUSED(reset);
#endif
}

void Adam::MapOpenBus(int page)
{
    m_Pages[page].read = NULL;
    m_Pages[page].write = NULL;
    m_Pages[page].type = PageOpenBus;
    m_Pages[page].source = MemorySourceOpenBus;
    m_Pages[page].source_offset = 0;
}

void Adam::MapReadOnly(int page, const u8* memory, MemorySource source, u32 source_offset)
{
    m_Pages[page].read = memory;
    m_Pages[page].write = NULL;
    m_Pages[page].type = PageMemory;
    m_Pages[page].source = source;
    m_Pages[page].source_offset = source_offset;
}

void Adam::MapReadWrite(int page, u8* memory, u32 source_offset)
{
    m_Pages[page].read = memory;
    m_Pages[page].write = memory;
    m_Pages[page].type = PageMemory;
    m_Pages[page].source = MemorySourceRAM;
    m_Pages[page].source_offset = source_offset;
}

void Adam::MapCartridge(int page)
{
    m_Pages[page].read = NULL;
    m_Pages[page].write = NULL;
    m_Pages[page].type = PageCartridge;
    m_Pages[page].source = MemorySourceCartridge;
    m_Pages[page].source_offset = 0;
}

void Adam::SetMemoryMap()
{
    m_MemoryMapGeneration++;

    if (!m_Enabled || !IsValidPointer(m_pMainRAM))
    {
        for (int i = 0; i < kPageCount; i++)
            MapOpenBus(i);
        return;
    }

    switch (m_MIOC & 0x03)
    {
        case 0:
            MapReadOnly(0, m_pSmartWriterROM + 0x0000, MemorySourceSmartWriter, 0x0000);
            MapReadOnly(1, m_pSmartWriterROM + 0x2000, MemorySourceSmartWriter, 0x2000);
            MapReadOnly(2, m_pSmartWriterROM + 0x4000, MemorySourceSmartWriter, 0x4000);
            if (m_Control & 0x02)
                MapReadOnly(3, m_pEOSROM, MemorySourceEOS, 0x0000);
            else
                MapReadOnly(3, m_pSmartWriterROM + 0x6000, MemorySourceSmartWriter, 0x6000);
            break;
        case 1:
            for (int i = 0; i < 4; i++)
                MapReadWrite(i, m_pMainRAM + (i * kPageSize), i * kPageSize);
            break;
        case 2:
            for (int i = 0; i < 4; i++)
                MapOpenBus(i);
            break;
        case 3:
            MapReadOnly(0, m_pOS7ROM, MemorySourceOS7, 0x0000);
            for (int i = 1; i < 4; i++)
                MapReadWrite(i, m_pMainRAM + (i * kPageSize), i * kPageSize);
            break;
    }

    switch ((m_MIOC >> 2) & 0x03)
    {
        case 0:
            for (int i = 4; i < 8; i++)
                MapReadWrite(i, m_pMainRAM + (i * kPageSize), i * kPageSize);
            break;
        case 1:
        case 2:
            for (int i = 4; i < 8; i++)
                MapOpenBus(i);
            break;
        case 3:
            for (int i = 4; i < 8; i++)
                MapCartridge(i);
            break;
    }
}

u8 Adam::ReadMemory(u16 address)
{
    MemoryPage* page = &m_Pages[address >> 13];

    if (page->type == PageCartridge)
        return IsValidPointer(m_pMapper) ? m_pMapper->Read(address) : 0xFF;

    if (IsValidPointer(page->read))
        return page->read[address & 0x1FFF];

    return 0xFF;
}

u8 Adam::DebugReadMemory(u16 address)
{
    MemoryPage* page = &m_Pages[address >> 13];

    if (page->type == PageCartridge)
        return IsValidPointer(m_pMapper) ? m_pMapper->Peek(address) : 0xFF;

    if (IsValidPointer(page->read))
        return page->read[address & 0x1FFF];

    return 0xFF;
}

bool Adam::DebugWriteMemory(u16 address, u8 value)
{
    MemoryPage* page = &m_Pages[address >> 13];
    if (!IsValidPointer(page->write))
        return false;

    page->write[address & 0x1FFF] = value;
    return true;
}

bool Adam::CanWriteMemory(u16 address) const
{
    return IsValidPointer(m_Pages[address >> 13].write);
}

void Adam::WriteMemory(u16 address, u8 value)
{
    MemoryPage* page = &m_Pages[address >> 13];

    if (page->type == PageCartridge)
    {
        if (IsValidPointer(m_pMapper))
            m_pMapper->Write(address, value);
        return;
    }

    if (IsValidPointer(page->write))
        page->write[address & 0x1FFF] = value;
}

u8 Adam::ReadPhysicalRAM(u16 address) const
{
    return IsValidPointer(m_pMainRAM) ? m_pMainRAM[address] : 0xFF;
}

void Adam::WritePhysicalRAM(u16 address, u8 value)
{
    if (IsValidPointer(m_pMainRAM))
        m_pMainRAM[address] = value;
}

u8* Adam::GetMainRAM()
{
    return m_pMainRAM;
}

const u8* Adam::GetOS7ROM() const
{
    return m_pOS7ROM;
}

const u8* Adam::GetEOSROM() const
{
    return m_pEOSROM;
}

const u8* Adam::GetSmartWriterROM() const
{
    return m_pSmartWriterROM;
}

u32 Adam::GetFirmwareCRC(GC_AdamFirmware firmware) const
{
    if ((firmware < GC_ADAM_FIRMWARE_OS7) || (firmware >= GC_ADAM_FIRMWARE_COUNT))
        return 0;

    return m_FirmwareCRC[firmware];
}

u8 Adam::GetMIOC() const
{
    return m_MIOC;
}

u8 Adam::GetControl() const
{
    return m_Control;
}

GC_AdamBootMode Adam::GetBootMode() const
{
    return m_BootMode;
}

Adam::MemorySource Adam::GetMemorySource(u16 address) const
{
    return m_Pages[address >> 13].source;
}

u32 Adam::GetMemorySourceOffset(u16 address) const
{
    const MemoryPage* page = &m_Pages[address >> 13];
    return page->source_offset + (address & 0x1FFF);
}

u32 Adam::GetMemoryMapGeneration() const
{
    return m_MemoryMapGeneration;
}
