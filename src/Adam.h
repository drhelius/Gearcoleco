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

#ifndef ADAM_H
#define ADAM_H

#include "definitions.h"
#include "IOPorts.h"

class Mapper;
class AdamNet;
class AdamMedia;

class Adam : public IOPorts
{
public:
    enum MemorySource
    {
        MemorySourceOpenBus = 0,
        MemorySourceRAM,
        MemorySourceOS7,
        MemorySourceEOS,
        MemorySourceSmartWriter,
        MemorySourceCartridge
    };

    struct FirmwareMetadata
    {
        const char* role_name;
        int size;
        u32 crc;
        const char* sha1;
        const char* aliases[4];
    };

    static const int kOS7ROMSize = 0x2000;
    static const int kEOSROMSize = 0x2000;
    static const int kSmartWriterROMSize = 0x8000;
    static const int kMainRAMSize = 0x10000;
    static const int kPageSize = 0x2000;
    static const int kPageCount = 8;
    static const FirmwareMetadata* GetFirmwareMetadata(GC_AdamFirmware firmware);

    Adam();
    virtual ~Adam();
    void Init(IOPorts* shared_ports);
    void SetMapper(Mapper* mapper);
    void SetEnabled(bool enabled);
    INLINE bool IsEnabled() const { return m_Enabled; }
    bool IsFirmwareReady() const;
    bool LoadFirmware(const u8* os7, int os7_size, const u8* eos, int eos_size,
        const u8* smartwriter, int smartwriter_size);
    bool LoadFirmware(GC_AdamFirmware firmware, const u8* data, int size);
    void UnloadFirmware();
    void Reset(bool cold, GC_AdamBootMode boot_mode);
    void Clock(unsigned int cycles);
    void KeyPressed(GC_AdamKey key);
    void KeyReleased(GC_AdamKey key);
    void ReleaseAllKeys();
    GC_AdamMediaError InsertMedia(GC_AdamMediaSlot slot, GC_AdamMediaType type,
        const u8* data, size_t size, bool write_protected, u32 base_crc = 0);
    void EjectMedia(GC_AdamMediaSlot slot);
    AdamMedia* GetMedia(GC_AdamMediaSlot slot);
    const AdamMedia* GetMedia(GC_AdamMediaSlot slot) const;
    AdamNet* GetAdamNet();
    const AdamNet* GetAdamNet() const;
    void SaveState(std::ostream& stream) const;
    bool LoadState(std::istream& stream);
    virtual u8 In(u8 port);
    virtual void Out(u8 port, u8 value);
    virtual void Reset();
    u8 ReadMemory(u16 address);
    u8 DebugReadMemory(u16 address);
    void WriteMemory(u16 address, u8 value);
    u8 ReadPhysicalRAM(u16 address) const;
    void WritePhysicalRAM(u16 address, u8 value);
    u8* GetMainRAM();
    const u8* GetOS7ROM() const;
    const u8* GetEOSROM() const;
    const u8* GetSmartWriterROM() const;
    u32 GetFirmwareCRC(GC_AdamFirmware firmware) const;
    u8 GetMIOC() const;
    u8 GetControl() const;
    GC_AdamBootMode GetBootMode() const;
    bool HandlesPort(u8 port) const;
    MemorySource GetMemorySource(u16 address) const;
    u32 GetMemorySourceOffset(u16 address) const;
    u32 GetMemoryMapGeneration() const;

private:
    enum PageType
    {
        PageOpenBus = 0,
        PageMemory,
        PageCartridge
    };

    struct MemoryPage
    {
        const u8* read;
        u8* write;
        PageType type;
        MemorySource source;
        u32 source_offset;
    };

    void AllocateStorage();
    void InitializeRAM();
    void SetMemoryMap();
    void MapOpenBus(int page);
    void MapReadOnly(int page, const u8* memory, MemorySource source, u32 source_offset);
    void MapReadWrite(int page, u8* memory, u32 source_offset);
    void MapCartridge(int page);
    void WriteMIOC(u8 value);
    void WriteControl(u8 value);

private:
    IOPorts* m_pSharedPorts;
    Mapper* m_pMapper;
    AdamNet* m_pAdamNet;
    u8* m_pOS7ROM;
    u8* m_pEOSROM;
    u8* m_pSmartWriterROM;
    u8* m_pMainRAM;
    MemoryPage m_Pages[kPageCount];
    u32 m_FirmwareCRC[GC_ADAM_FIRMWARE_COUNT];
    bool m_FirmwareLoaded[GC_ADAM_FIRMWARE_COUNT];
    bool m_Enabled;
    u8 m_MIOC;
    u8 m_Control;
    GC_AdamBootMode m_BootMode;
    u32 m_MemoryMapGeneration;
};

#endif /* ADAM_H */
