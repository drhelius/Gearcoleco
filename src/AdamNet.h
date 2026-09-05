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

#ifndef ADAM_NET_H
#define ADAM_NET_H

#include "definitions.h"
#include "AdamMedia.h"
#include <iostream>

class Adam;
class TraceLogger;

class AdamNet
{
public:
    static const int kPCBSize = 4;
    static const int kDCBSize = 21;
    static const int kMaxDCBs = 15;
    static const u16 kInitialPCBAddress = 0xFEC0;
    static const int kResetCycles = 114;
    static const int kLinkByteCycles = 573;
    static const int kStatusCycles = kLinkByteCycles * 6;
    static const int kFloppyBlockCycles = (GC_MASTER_CLOCK_NTSC / GC_FRAMES_PER_SECOND_NTSC) * 5;
    static const int kDataPackBlockCycles = (GC_MASTER_CLOCK_NTSC / GC_FRAMES_PER_SECOND_NTSC) * 10;
    static const int kKeyboardFIFOSize = 16;
    static const int kPrinterSpoolSize = 4096;

    enum PCBOffset
    {
        PCBCommandStatus = 0,
        PCBAddressLow = 1,
        PCBAddressHigh = 2,
        PCBDeviceCount = 3
    };

    enum DCBOffset
    {
        DCBCommandStatus = 0,
        DCBBufferAddressLow = 1,
        DCBBufferAddressHigh = 2,
        DCBBufferLengthLow = 3,
        DCBBufferLengthHigh = 4,
        DCBBlock0 = 5,
        DCBBlock1 = 6,
        DCBBlock2 = 7,
        DCBBlock3 = 8,
        DCBSecondaryID = 9,
        DCBRetryLow = 14,
        DCBRetryHigh = 15,
        DCBAddressCode = 16,
        DCBMaxLengthLow = 17,
        DCBMaxLengthHigh = 18,
        DCBDeviceType = 19,
        DCBNodeStatus = 20
    };

    enum Command
    {
        CommandIdle = 0,
        CommandStatus = 1,
        CommandSoftReset = 2,
        CommandWrite = 3,
        CommandRead = 4
    };

    enum Response
    {
        ResponseSuccess = 0x80,
        ResponsePrinterBusy = 0x86,
        ResponseKeyboardEmpty = 0x8C,
        ResponseDeviceError = 0x96,
        ResponseTimeout = 0x9B
    };

    enum DeviceType
    {
        DeviceKeyboard = 0,
        DevicePrinter,
        DeviceDisk,
        DeviceDataPack
    };

    enum TimingClass
    {
        TimingKeyboard = 0,
        TimingPrinter,
        TimingFloppy,
        TimingDataPack
    };

    struct DeviceMetadata
    {
        u8 id;
        const char* name;
        DeviceType type;
        int slot;
        u16 max_transfer;
        TimingClass timing;
        u8 status_shift;
    };

    struct KeyDefinition
    {
        u8 normal;
        u8 shifted;
        u8 control;
        u8 home;
        bool sends_code;
        bool repeats;
    };

    AdamNet();
    ~AdamNet();
    void Init(Adam* adam);
    void SetTraceLogger(TraceLogger* trace_logger);
    static int GetDeviceCount();
    static const DeviceMetadata* GetDeviceMetadataByIndex(int index);
    static const DeviceMetadata* GetDeviceMetadata(u8 id);
    void Reset(bool cold);
    void Clock(unsigned int cycles);
    void KeyPressed(GC_AdamKey key);
    void KeyReleased(GC_AdamKey key);
    void ReleaseAllKeys();
    GC_AdamMediaError InsertMedia(GC_AdamMediaSlot slot, GC_AdamMediaType type,
        const u8* data, size_t size, bool write_protected, u32 base_crc = 0);
    void EjectMedia(GC_AdamMediaSlot slot);
    AdamMedia* GetMedia(GC_AdamMediaSlot slot);
    const AdamMedia* GetMedia(GC_AdamMediaSlot slot) const;
    const u8* GetPrinterData() const;
    int GetPrinterSize() const;
    void ClearPrinter();
    u16 GetPCBAddress() const;
    int GetCyclesUntilEvent() const;
    bool IsBusy() const;
    int GetKeyboardFIFOCount() const;
    bool DidKeyboardOverflow() const;
    void GetDebugState(GC_AdamDebugState* state) const;
    void SaveState(std::ostream& stream) const;
    bool LoadState(std::istream& stream);

private:
    struct Transfer
    {
        bool active;
        bool pcb;
        u8 dcb;
        u8 command;
        u8 device;
        u8 error;
        u16 buffer;
        u16 length;
        u16 pcb_address;
        u8 pcb_count;
        u32 block;
        u32 media_generation;
    };

    void InitializePCB(bool clear_dcbs);
    void ProcessEvent();
    bool ScanCommands();
    void StartPCBCommand(u8 command);
    void StartDCBCommand(u8 dcb, u8 command);
    void CompletePCBCommand();
    void CompleteDCBCommand();
    int GetTransferCycles(u8 device, u8 command, u16 length) const;
    u8 GetPCB(int offset) const;
    void SetPCB(int offset, u8 value);
    u8 GetDCB(u8 dcb, int offset) const;
    void SetDCB(u8 dcb, int offset, u8 value);
    u16 GetDCB16(u8 dcb, int offset) const;
    u32 GetDCB32(u8 dcb, int offset) const;
    void SetDCB16(u8 dcb, int offset, u16 value);
    u8 GetDeviceID(u8 dcb) const;
    void ReportDevice(u8 dcb, const DeviceMetadata* device);
    void SetDeviceStatus(u8 dcb, u8 device, u8 status);
    u8 GetMediaStatus(const AdamMedia* media) const;
    int GetMediaSlot(u8 device) const;
    AdamMedia* GetDeviceMedia(u8 device);
    const AdamMedia* GetDeviceMedia(u8 device) const;
    void ResetMediaCache();
    void InvalidateMediaCache(int slot);
    void CompleteKeyboard(u8 dcb, u8 command, u8* response);
    void CompletePrinter(u8 dcb, u8 command, u8* response);
    void CompleteMedia(u8 dcb, u8 device, u8 command, u8* response);
    void ResetKeyboard();
    void ClockKeyboard(unsigned int cycles);
    void QueueKey(u8 code);
    bool PopKey(u8* code);
    u8 TranslateKey(GC_AdamKey key) const;
    bool IsDirectionKey(GC_AdamKey key) const;
    u8 GetDiagonalCode(GC_AdamKey key) const;
    bool AppendPrinter(u8 value);
    void TraceTransfer(u8 event, u8 response = 0, u8 error = 0) const;
    bool ReadState(std::istream& stream);
    bool IsMediaStateCompatible(const AdamNet& state) const;

private:
    Adam* m_pAdam;
    TraceLogger* m_pTraceLogger;
    AdamMedia m_Media[GC_ADAM_MEDIA_SLOT_COUNT];
    bool m_MediaCacheValid[GC_ADAM_MEDIA_SLOT_COUNT];
    u32 m_MediaCacheBlock[GC_ADAM_MEDIA_SLOT_COUNT];
    u32 m_MediaCacheGeneration[GC_ADAM_MEDIA_SLOT_COUNT];
    GC_AdamNetControllerState m_State;
    Transfer m_Transfer;
    u16 m_PCBAddress;
    u8 m_ScanIndex;
    int m_CyclesUntilEvent;
    u8 m_KeyboardFIFO[kKeyboardFIFOSize];
    u8 m_KeyboardRead;
    u8 m_KeyboardWrite;
    u8 m_KeyboardCount;
    bool m_KeyboardOverflow;
    bool m_KeyPressed[GC_ADAM_KEY_COUNT];
    bool m_Lock;
    GC_AdamKey m_RepeatKey;
    int m_RepeatCycles;
    u8 m_PrinterSpool[kPrinterSpoolSize];
    int m_PrinterSize;
};

#endif /* ADAM_NET_H */
