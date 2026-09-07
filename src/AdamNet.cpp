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
#include "AdamNet.h"
#include "Adam.h"
#include "TraceLogger.h"

static const int kKeyRepeatDelayCycles = GC_MASTER_CLOCK_NTSC / 2;
static const int kKeyRepeatIntervalCycles = GC_MASTER_CLOCK_NTSC / 20;

static const AdamNet::DeviceMetadata kDevices[] =
{
    { 0x01, "Keyboard", AdamNet::DeviceKeyboard, -1, 1,
        AdamNet::TimingKeyboard, 0 },
    { 0x02, "Printer", AdamNet::DevicePrinter, -1, 16,
        AdamNet::TimingPrinter, 0 },
    { 0x04, "Disk 1", AdamNet::DeviceDisk, GC_ADAM_MEDIA_DISK_1,
        AdamMedia::kBlockSize, AdamNet::TimingFloppy, 0 },
    { 0x05, "Disk 2", AdamNet::DeviceDisk, GC_ADAM_MEDIA_DISK_2,
        AdamMedia::kBlockSize, AdamNet::TimingFloppy, 0 },
    { 0x08, "Data Pack 1", AdamNet::DeviceDataPack, GC_ADAM_MEDIA_DATA_PACK_1,
        AdamMedia::kBlockSize, AdamNet::TimingDataPack, 0 },
    { 0x18, "Data Pack 2", AdamNet::DeviceDataPack, GC_ADAM_MEDIA_DATA_PACK_2,
        AdamMedia::kBlockSize, AdamNet::TimingDataPack, 4 }
};

#define ADAM_KEY(normal, shifted, control, home, sends, repeats) \
    { normal, shifted, control, home, sends, repeats }

static const AdamNet::KeyDefinition kAdamKeys[GC_ADAM_KEY_COUNT] =
{
    ADAM_KEY('a', 'A', 0x01, 'a', true, true),
    ADAM_KEY('b', 'B', 0x02, 'b', true, true),
    ADAM_KEY('c', 'C', 0x03, 'c', true, true),
    ADAM_KEY('d', 'D', 0x04, 'd', true, true),
    ADAM_KEY('e', 'E', 0x05, 'e', true, true),
    ADAM_KEY('f', 'F', 0x06, 'f', true, true),
    ADAM_KEY('g', 'G', 0x07, 'g', true, true),
    ADAM_KEY('h', 'H', 0x08, 'h', true, true),
    ADAM_KEY('i', 'I', 0x09, 'i', true, true),
    ADAM_KEY('j', 'J', 0x0A, 'j', true, true),
    ADAM_KEY('k', 'K', 0x0B, 'k', true, true),
    ADAM_KEY('l', 'L', 0x0C, 'l', true, true),
    ADAM_KEY('m', 'M', 0x0D, 'm', true, true),
    ADAM_KEY('n', 'N', 0x0E, 'n', true, true),
    ADAM_KEY('o', 'O', 0x0F, 'o', true, true),
    ADAM_KEY('p', 'P', 0x10, 'p', true, true),
    ADAM_KEY('q', 'Q', 0x11, 'q', true, true),
    ADAM_KEY('r', 'R', 0x12, 'r', true, true),
    ADAM_KEY('s', 'S', 0x13, 's', true, true),
    ADAM_KEY('t', 'T', 0x14, 't', true, true),
    ADAM_KEY('u', 'U', 0x15, 'u', true, true),
    ADAM_KEY('v', 'V', 0x16, 'v', true, true),
    ADAM_KEY('w', 'W', 0x17, 'w', true, true),
    ADAM_KEY('x', 'X', 0x18, 'x', true, true),
    ADAM_KEY('y', 'Y', 0x19, 'y', true, true),
    ADAM_KEY('z', 'Z', 0x1A, 'z', true, true),
    ADAM_KEY('0', ')', '0', '0', true, true),
    ADAM_KEY('1', '!', '1', '1', true, true),
    ADAM_KEY('2', '@', 0x00, '2', true, true),
    ADAM_KEY('3', '#', '3', '3', true, true),
    ADAM_KEY('4', '$', '4', '4', true, true),
    ADAM_KEY('5', '%', '5', '5', true, true),
    ADAM_KEY('6', '_', 0x1F, '6', true, true),
    ADAM_KEY('7', '&', '7', '7', true, true),
    ADAM_KEY('8', '*', '8', '8', true, true),
    ADAM_KEY('9', '(', '9', '9', true, true),
    ADAM_KEY(' ', ' ', ' ', ' ', true, true),
    ADAM_KEY('-', '`', '-', '-', true, true),
    ADAM_KEY('+', '=', '+', '+', true, true),
    ADAM_KEY('^', '~', 0x1E, '^', true, true),
    ADAM_KEY(';', ':', ';', ';', true, true),
    ADAM_KEY('\'', '"', '\'', '\'', true, true),
    ADAM_KEY('[', '{', 0x1B, '[', true, true),
    ADAM_KEY(']', '}', 0x1D, ']', true, true),
    ADAM_KEY('\\', '|', 0x1C, '\\', true, true),
    ADAM_KEY(',', '<', ',', ',', true, true),
    ADAM_KEY('.', '>', '.', '.', true, true),
    ADAM_KEY('/', '?', '/', '/', true, true),
    ADAM_KEY(0x0D, 0x0D, 0x0D, 0x0D, true, false),
    ADAM_KEY(0x1B, 0x1B, 0x1B, 0x1B, true, false),
    ADAM_KEY(0x08, 0xB8, 0x08, 0x08, true, true),
    ADAM_KEY(0x09, 0xB9, 0x09, 0x09, true, true),
    ADAM_KEY(0x80, 0x80, 0x80, 0x80, true, false),
    ADAM_KEY(0x81, 0x89, 0x81, 0x81, true, false),
    ADAM_KEY(0x82, 0x8A, 0x82, 0x82, true, false),
    ADAM_KEY(0x83, 0x8B, 0x83, 0x83, true, false),
    ADAM_KEY(0x84, 0x8C, 0x84, 0x84, true, false),
    ADAM_KEY(0x85, 0x8D, 0x85, 0x85, true, false),
    ADAM_KEY(0x86, 0x8E, 0x86, 0x86, true, false),
    ADAM_KEY(0x90, 0x98, 0x90, 0x90, true, false),
    ADAM_KEY(0x91, 0x99, 0x91, 0x91, true, false),
    ADAM_KEY(0x92, 0x9A, 0x92, 0x92, true, false),
    ADAM_KEY(0x93, 0x9B, 0x93, 0x93, true, false),
    ADAM_KEY(0x94, 0x9C, 0x94, 0x94, true, false),
    ADAM_KEY(0x95, 0x9D, 0x95, 0x95, true, false),
    ADAM_KEY(0x96, 0x9E, 0x96, 0x96, true, false),
    ADAM_KEY(0x97, 0x9F, 0x7F, 0x97, true, false),
    ADAM_KEY(0xA0, 0xA0, 0xA4, 0xAC, true, true),
    ADAM_KEY(0xA1, 0xA1, 0xA5, 0xAD, true, true),
    ADAM_KEY(0xA2, 0xA2, 0xA6, 0xAE, true, true),
    ADAM_KEY(0xA3, 0xA3, 0xA7, 0xAF, true, true),
    ADAM_KEY(0x00, 0x00, 0x00, 0x00, false, false),
    ADAM_KEY(0x00, 0x00, 0x00, 0x00, false, false),
    ADAM_KEY(0x00, 0x00, 0x00, 0x00, false, false)
};

#undef ADAM_KEY

AdamNet::AdamNet()
{
    InitPointer(m_pAdam);
    InitPointer(m_pTraceLogger);
    m_State = GC_ADAMNET_CONTROLLER_INITIALIZING;
    memset(&m_Transfer, 0, sizeof(m_Transfer));
    m_PCBAddress = kInitialPCBAddress;
    m_ScanIndex = 0;
    m_CyclesUntilEvent = kResetCycles;
    ResetMediaCache();
    ResetKeyboard();
    memset(m_PrinterSpool, 0, sizeof(m_PrinterSpool));
    m_PrinterSize = 0;
}

AdamNet::~AdamNet()
{
}

void AdamNet::Init(Adam* adam)
{
    m_pAdam = adam;
    Reset(true);
}

void AdamNet::SetTraceLogger(TraceLogger* trace_logger)
{
    m_pTraceLogger = trace_logger;
}

int AdamNet::GetDeviceCount()
{
    return (int)(sizeof(kDevices) / sizeof(kDevices[0]));
}

const AdamNet::DeviceMetadata* AdamNet::GetDeviceMetadataByIndex(int index)
{
    return (index >= 0) && (index < GetDeviceCount()) ? &kDevices[index] : NULL;
}

const AdamNet::DeviceMetadata* AdamNet::GetDeviceMetadata(u8 id)
{
    for (int i = 0; i < GetDeviceCount(); i++)
    {
        if (kDevices[i].id == id)
            return &kDevices[i];
    }
    return NULL;
}

void AdamNet::Reset(bool cold)
{
    m_State = GC_ADAMNET_CONTROLLER_INITIALIZING;
    memset(&m_Transfer, 0, sizeof(m_Transfer));
    m_PCBAddress = kInitialPCBAddress;
    m_ScanIndex = 0;
    m_CyclesUntilEvent = kResetCycles;
    ResetMediaCache();
    ResetKeyboard();
    if (cold)
    {
        memset(m_PrinterSpool, 0, sizeof(m_PrinterSpool));
        m_PrinterSize = 0;
    }
}

void AdamNet::ResetKeyboard()
{
    memset(m_KeyboardFIFO, 0, sizeof(m_KeyboardFIFO));
    m_KeyboardRead = 0;
    m_KeyboardWrite = 0;
    m_KeyboardCount = 0;
    m_KeyboardOverflow = false;
    memset(m_KeyPressed, 0, sizeof(m_KeyPressed));
    m_Lock = false;
    m_RepeatKey = GC_ADAM_KEY_COUNT;
    m_RepeatCycles = 0;
}

void AdamNet::InitializePCB(bool clear_dcbs)
{
    if (clear_dcbs)
    {
        SetPCB(PCBCommandStatus, 0);

        for (int dcb = 0; dcb < kMaxDCBs; dcb++)
        {
            for (int offset = 0; offset < kDCBSize; offset++)
                SetDCB((u8)dcb, offset, 0);
        }
    }

    SetPCB(PCBAddressLow, (u8)m_PCBAddress);
    SetPCB(PCBAddressHigh, (u8)(m_PCBAddress >> 8));
    SetPCB(PCBDeviceCount, kMaxDCBs);

    for (int dcb = 0; dcb < kMaxDCBs; dcb++)
    {
        SetDCB((u8)dcb, DCBSecondaryID, 0);
        SetDCB((u8)dcb, DCBAddressCode, (u8)dcb);
    }
}

void AdamNet::Clock(unsigned int cycles)
{
    ClockKeyboard(cycles);

    while (cycles > 0)
    {
        if (m_CyclesUntilEvent <= 0)
        {
            ProcessEvent();
            continue;
        }

        if (cycles < static_cast<unsigned int>(m_CyclesUntilEvent))
        {
            m_CyclesUntilEvent -= static_cast<int>(cycles);
            break;
        }

        cycles -= static_cast<unsigned int>(m_CyclesUntilEvent);
        m_CyclesUntilEvent = 0;
        ProcessEvent();
    }
}

void AdamNet::ProcessEvent()
{
    switch (m_State)
    {
        case GC_ADAMNET_CONTROLLER_INITIALIZING:
            InitializePCB(true);
            m_State = GC_ADAMNET_CONTROLLER_IDLE;
            m_CyclesUntilEvent = kLinkByteCycles;
            break;
        case GC_ADAMNET_CONTROLLER_IDLE:
            if (!ScanCommands())
                m_CyclesUntilEvent = kLinkByteCycles;
            break;
        case GC_ADAMNET_CONTROLLER_BUSY:
            if (m_Transfer.pcb)
                CompletePCBCommand();
            else
                CompleteDCBCommand();
            memset(&m_Transfer, 0, sizeof(m_Transfer));
            if (m_State == GC_ADAMNET_CONTROLLER_BUSY)
            {
                m_State = GC_ADAMNET_CONTROLLER_IDLE;
                m_CyclesUntilEvent = kLinkByteCycles;
            }
            break;
    }
}

bool AdamNet::ScanCommands()
{
    u8 pcb_command = GetPCB(PCBCommandStatus);
    if ((pcb_command > 0) && (pcb_command < 0x80) && (pcb_command <= 5))
    {
        StartPCBCommand(pcb_command);
        return true;
    }

    int count = GetPCB(PCBDeviceCount);
    if (count > kMaxDCBs)
        count = kMaxDCBs;

    for (int checked = 0; checked < count; checked++)
    {
        u8 dcb = (u8)((m_ScanIndex + checked) % count);
        u8 command = GetDCB(dcb, DCBCommandStatus);

        if ((command > CommandIdle) && (command <= CommandRead) && !(command & 0x80))
        {
            m_ScanIndex = (u8)((dcb + 1) % count);
            StartDCBCommand(dcb, command);
            return true;
        }
    }

    return false;
}

void AdamNet::StartPCBCommand(u8 command)
{
    memset(&m_Transfer, 0, sizeof(m_Transfer));
    m_Transfer.active = true;
    m_Transfer.pcb = true;
    m_Transfer.command = command;
    m_Transfer.pcb_address = (u16)(GetPCB(PCBAddressLow) | (GetPCB(PCBAddressHigh) << 8));
    m_Transfer.pcb_count = GetPCB(PCBDeviceCount);
    m_State = GC_ADAMNET_CONTROLLER_BUSY;
    m_CyclesUntilEvent = kLinkByteCycles;
    TraceTransfer(TRACE_ADAM_COMMAND_ACCEPT);
}

void AdamNet::StartDCBCommand(u8 dcb, u8 command)
{
    memset(&m_Transfer, 0, sizeof(m_Transfer));
    m_Transfer.active = true;
    m_Transfer.pcb = false;
    m_Transfer.dcb = dcb;
    m_Transfer.command = command;
    m_Transfer.device = GetDeviceID(dcb);
    m_Transfer.buffer = GetDCB16(dcb, DCBBufferAddressLow);
    m_Transfer.length = GetDCB16(dcb, DCBBufferLengthLow);
    m_Transfer.block = GetDCB32(dcb, DCBBlock0);
    AdamMedia* media = GetDeviceMedia(m_Transfer.device);
    m_Transfer.media_generation = IsValidPointer(media) ? media->GetGeneration() : 0;
    m_State = GC_ADAMNET_CONTROLLER_BUSY;
    m_CyclesUntilEvent = GetTransferCycles(m_Transfer.device, command, m_Transfer.length);
    TraceTransfer(TRACE_ADAM_COMMAND_ACCEPT);
}

void AdamNet::TraceTransfer(u8 event, u8 response, u8 error) const
{
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    if (!IsValidPointer(m_pTraceLogger) ||
        !m_pTraceLogger->IsEventEnabled(TRACE_ADAM, event))
    {
        return;
    }
    GC_Trace_Entry entry = {};
    entry.type = TRACE_ADAM;
    entry.adam.event = event;
    entry.adam.command = m_Transfer.command;
    entry.adam.response = response;
    entry.adam.device = m_Transfer.device;
    entry.adam.dcb = m_Transfer.dcb;
    entry.adam.error = error;
    entry.adam.pcb = m_Transfer.pcb;
    entry.adam.buffer = m_Transfer.pcb ? m_Transfer.pcb_address : m_Transfer.buffer;
    entry.adam.length = m_Transfer.pcb ? m_Transfer.pcb_count : m_Transfer.length;
    entry.adam.block = m_Transfer.block;
    m_pTraceLogger->TraceLog(entry);
#else
    UNUSED(event);
    UNUSED(response);
    UNUSED(error);
#endif
}

int AdamNet::GetTransferCycles(u8 device, u8 command, u16 length) const
{
    const DeviceMetadata* metadata = GetDeviceMetadata(device);
    if (!IsValidPointer(metadata))
        return kStatusCycles;
    switch (metadata->timing)
    {
        case TimingFloppy:
            return (command == CommandRead || command == CommandWrite) ?
                kFloppyBlockCycles : kStatusCycles;
        case TimingDataPack:
            return (command == CommandRead || command == CommandWrite) ?
                kDataPackBlockCycles : kStatusCycles;
        case TimingPrinter:
            return kLinkByteCycles * (4 + (length > metadata->max_transfer ?
                metadata->max_transfer : length));
        case TimingKeyboard:
            return kLinkByteCycles * 2;
        default:
            return kStatusCycles;
    }
}

void AdamNet::CompletePCBCommand()
{
    u8 response = ResponseTimeout;
    switch (m_Transfer.command)
    {
        case 1:
        case 2:
        case 5:
            SetPCB(PCBCommandStatus, ResponseSuccess | m_Transfer.command);
            response = ResponseSuccess | m_Transfer.command;
            break;
        case 3:
            m_PCBAddress = m_Transfer.pcb_address;
            if (m_Transfer.pcb_count > kMaxDCBs)
                m_Transfer.pcb_count = kMaxDCBs;
            InitializePCB(false);
            SetPCB(PCBDeviceCount, m_Transfer.pcb_count);
            SetPCB(PCBCommandStatus, ResponseSuccess | 3);
            response = ResponseSuccess | 3;
            break;
        case 4:
            response = ResponseSuccess | 4;
            TraceTransfer(TRACE_ADAM_COMMAND_COMPLETE, response);
            Reset(false);
            return;
        default:
            break;
    }
    TraceTransfer(TRACE_ADAM_COMMAND_COMPLETE, response);
    if ((response & 0xF0) != ResponseSuccess)
        TraceTransfer(TRACE_ADAM_ERROR, response);
}

void AdamNet::CompleteDCBCommand()
{
    u8 response = ResponseTimeout;
    const DeviceMetadata* metadata = GetDeviceMetadata(m_Transfer.device);
    if (IsValidPointer(metadata))
    {
        switch (metadata->type)
        {
            case DeviceKeyboard:
                CompleteKeyboard(m_Transfer.dcb, m_Transfer.command, &response);
                break;
            case DevicePrinter:
                CompletePrinter(m_Transfer.dcb, m_Transfer.command, &response);
                break;
            case DeviceDisk:
            case DeviceDataPack:
                CompleteMedia(m_Transfer.dcb, m_Transfer.device, m_Transfer.command, &response);
                break;
            default:
                break;
        }
    }

    SetDCB(m_Transfer.dcb, DCBCommandStatus, response);
    TraceTransfer(TRACE_ADAM_COMMAND_COMPLETE, response, m_Transfer.error);
    if ((response == ResponseSuccess) && ((m_Transfer.command == CommandRead) ||
        (m_Transfer.command == CommandWrite)))
    {
        TraceTransfer(TRACE_ADAM_DMA_COMPLETE, response);
    }
    else if (response != ResponseSuccess)
        TraceTransfer(TRACE_ADAM_ERROR, response, m_Transfer.error);
}

void AdamNet::CompleteKeyboard(u8 dcb, u8 command, u8* response)
{
    switch (command)
    {
        case CommandStatus:
            ReportDevice(dcb, GetDeviceMetadata(m_Transfer.device));
            SetDeviceStatus(dcb, m_Transfer.device, 0);
            *response = ResponseSuccess;
            break;
        case CommandSoftReset:
            ResetKeyboard();
            ReportDevice(dcb, GetDeviceMetadata(m_Transfer.device));
            *response = ResponseSuccess;
            break;
        case CommandRead:
        {
            u16 requested = m_Transfer.length;
            u16 transferred = 0;
            u8 code = 0;

            if ((requested > 0) && PopKey(&code))
            {
                m_pAdam->WritePhysicalRAM(m_Transfer.buffer, code);
                transferred = 1;
            }

            *response = (transferred == requested) ? ResponseSuccess : ResponseKeyboardEmpty;
            break;
        }
        case CommandWrite:
            *response = ResponseTimeout;
            break;
        default:
            *response = ResponseTimeout;
            break;
    }
}

void AdamNet::CompletePrinter(u8 dcb, u8 command, u8* response)
{
    switch (command)
    {
        case CommandStatus:
            ReportDevice(dcb, GetDeviceMetadata(m_Transfer.device));
            SetDeviceStatus(dcb, m_Transfer.device, 0);
            *response = ResponseSuccess;
            break;
        case CommandSoftReset:
            ReportDevice(dcb, GetDeviceMetadata(m_Transfer.device));
            *response = ResponseSuccess;
            break;
        case CommandWrite:
        {
            const DeviceMetadata* metadata = GetDeviceMetadata(m_Transfer.device);
            if (!IsValidPointer(metadata) || (m_Transfer.length > metadata->max_transfer))
            {
                m_Transfer.error = GC_ADAM_MEDIA_ERROR_OUT_OF_RANGE;
                *response = ResponsePrinterBusy;
                break;
            }

            int original_size = m_PrinterSize;
            bool ok = true;
            for (u16 i = 0; i < m_Transfer.length; i++)
            {
                if (!AppendPrinter(m_pAdam->ReadPhysicalRAM((u16)(m_Transfer.buffer + i))))
                {
                    ok = false;
                    break;
                }
            }

            if (!ok)
            {
                m_PrinterSize = original_size;
                m_Transfer.error = GC_ADAM_MEDIA_ERROR_OUT_OF_RANGE;
            }
            *response = ok ? ResponseSuccess : ResponsePrinterBusy;
            break;
        }
        case CommandRead:
            *response = ResponseTimeout;
            break;
        default:
            *response = ResponseTimeout;
            break;
    }
}

void AdamNet::CompleteMedia(u8 dcb, u8 device, u8 command, u8* response)
{
    const DeviceMetadata* metadata = GetDeviceMetadata(device);
    AdamMedia* media = GetDeviceMedia(device);

    if (!IsValidPointer(metadata) || !IsValidPointer(media))
    {
        m_Transfer.error = GC_ADAM_MEDIA_ERROR_INVALID_ARGUMENT;
        SetDeviceStatus(dcb, device, 4);
        *response = ResponseTimeout;
        return;
    }

    if (command == CommandStatus)
    {
        ReportDevice(dcb, GetDeviceMetadata(device));
        // Status reports both units; each floppy controller has no secondary drive.
        u8 status = (u8)(GetMediaStatus(media) | 0x40);
        if (metadata->type == DeviceDataPack)
        {
            status = (u8)(GetMediaStatus(&m_Media[GC_ADAM_MEDIA_DATA_PACK_1]) |
                (GetMediaStatus(&m_Media[GC_ADAM_MEDIA_DATA_PACK_2]) << 4));
        }
        SetDCB(dcb, DCBNodeStatus, status);
        *response = ResponseSuccess;
        return;
    }

    if (command == CommandSoftReset)
    {
        InvalidateMediaCache(GetMediaSlot(device));
        SetDeviceStatus(dcb, device, GetMediaStatus(media));
        *response = ResponseSuccess;
        return;
    }

    if ((command != CommandRead) && (command != CommandWrite))
    {
        *response = ResponseTimeout;
        return;
    }

    if (media->GetGeneration() != m_Transfer.media_generation)
    {
        m_Transfer.error = GC_ADAM_MEDIA_ERROR_CHANGED;
        SetDeviceStatus(dcb, device, 3);
        *response = ResponseDeviceError;
        return;
    }

    if (!media->IsInserted())
    {
        m_Transfer.error = GC_ADAM_MEDIA_ERROR_NO_MEDIA;
        SetDeviceStatus(dcb, device, 3);
        *response = ResponseDeviceError;
        return;
    }

    if (m_Transfer.length > metadata->max_transfer)
    {
        m_Transfer.error = GC_ADAM_MEDIA_ERROR_OUT_OF_RANGE;
        SetDeviceStatus(dcb, device, 2);
        *response = ResponseDeviceError;
        return;
    }

    if (m_Transfer.block >= media->GetBlockCount())
    {
        m_Transfer.error = GC_ADAM_MEDIA_ERROR_OUT_OF_RANGE;
        SetDeviceStatus(dcb, device, 2);
        *response = ResponseDeviceError;
        return;
    }

    int media_slot = GetMediaSlot(device);
    if ((command == CommandRead) &&
        (!m_MediaCacheValid[media_slot] ||
        (m_MediaCacheBlock[media_slot] != m_Transfer.block) ||
        (m_MediaCacheGeneration[media_slot] != media->GetGeneration())))
    {
        m_MediaCacheValid[media_slot] = true;
        m_MediaCacheBlock[media_slot] = m_Transfer.block;
        m_MediaCacheGeneration[media_slot] = media->GetGeneration();
        SetDeviceStatus(dcb, device, 0);
        *response = ResponseTimeout;
        return;
    }

    u8 data[AdamMedia::kBlockSize];
    GC_AdamMediaError error = GC_ADAM_MEDIA_ERROR_NONE;

    if (command == CommandRead)
    {
        error = media->ReadBlock(m_Transfer.block, data, m_Transfer.length);
        if (error == GC_ADAM_MEDIA_ERROR_NONE)
        {
            for (u16 i = 0; i < m_Transfer.length; i++)
                m_pAdam->WritePhysicalRAM((u16)(m_Transfer.buffer + i), data[i]);
        }
    }
    else
    {
        InvalidateMediaCache(media_slot);
        for (u16 i = 0; i < m_Transfer.length; i++)
            data[i] = m_pAdam->ReadPhysicalRAM((u16)(m_Transfer.buffer + i));
        error = media->WriteBlock(m_Transfer.block, data, m_Transfer.length);
    }

    if (error == GC_ADAM_MEDIA_ERROR_NONE)
    {
        SetDeviceStatus(dcb, device, 0);
        SetDCB16(dcb, DCBBufferLengthLow, m_Transfer.length);
        *response = ResponseSuccess;
    }
    else
    {
        m_Transfer.error = error;
        u8 status = 2;
        if (error == GC_ADAM_MEDIA_ERROR_NO_MEDIA || error == GC_ADAM_MEDIA_ERROR_CHANGED)
            status = 3;
        else if (error == GC_ADAM_MEDIA_ERROR_WRITE_PROTECTED)
            status = 5;
        InvalidateMediaCache(media_slot);
        SetDeviceStatus(dcb, device, status);
        *response = ResponseDeviceError;
    }
}

u8 AdamNet::GetPCB(int offset) const
{
    return m_pAdam->ReadPhysicalRAM((u16)(m_PCBAddress + offset));
}

void AdamNet::SetPCB(int offset, u8 value)
{
    m_pAdam->WritePhysicalRAM((u16)(m_PCBAddress + offset), value);
}

u8 AdamNet::GetDCB(u8 dcb, int offset) const
{
    u16 address = (u16)(m_PCBAddress + kPCBSize + (dcb * kDCBSize) + offset);
    return m_pAdam->ReadPhysicalRAM(address);
}

void AdamNet::SetDCB(u8 dcb, int offset, u8 value)
{
    u16 address = (u16)(m_PCBAddress + kPCBSize + (dcb * kDCBSize) + offset);
    m_pAdam->WritePhysicalRAM(address, value);
}

u16 AdamNet::GetDCB16(u8 dcb, int offset) const
{
    return (u16)(GetDCB(dcb, offset) | (GetDCB(dcb, offset + 1) << 8));
}

u32 AdamNet::GetDCB32(u8 dcb, int offset) const
{
    u32 value = GetDCB(dcb, offset);
    value |= static_cast<u32>(GetDCB(dcb, offset + 1)) << 8;
    value |= static_cast<u32>(GetDCB(dcb, offset + 2)) << 16;
    value |= static_cast<u32>(GetDCB(dcb, offset + 3)) << 24;
    return value;
}

void AdamNet::SetDCB16(u8 dcb, int offset, u16 value)
{
    SetDCB(dcb, offset, (u8)value);
    SetDCB(dcb, offset + 1, (u8)(value >> 8));
}

u8 AdamNet::GetDeviceID(u8 dcb) const
{
    return (u8)(((GetDCB(dcb, DCBSecondaryID) & 0x0F) << 4) |
        (GetDCB(dcb, DCBAddressCode) & 0x0F));
}

void AdamNet::ReportDevice(u8 dcb, const DeviceMetadata* device)
{
    if (!IsValidPointer(device))
        return;
    SetDCB16(dcb, DCBMaxLengthLow, device->max_transfer);
    bool block = (device->type == DeviceDisk) || (device->type == DeviceDataPack);
    SetDCB(dcb, DCBDeviceType, block ? 1 : 0);
}

void AdamNet::SetDeviceStatus(u8 dcb, u8 device, u8 status)
{
    u8 value = GetDCB(dcb, DCBNodeStatus);

    const DeviceMetadata* metadata = GetDeviceMetadata(device);
    u8 shift = IsValidPointer(metadata) ? metadata->status_shift : 0;
    u8 mask = (u8)(0x0F << shift);
    value = (u8)((value & ~mask) | ((status & 0x0F) << shift));

    SetDCB(dcb, DCBNodeStatus, value);
}

u8 AdamNet::GetMediaStatus(const AdamMedia* media) const
{
    return IsValidPointer(media) && media->IsInserted() ? 0 : 3;
}

int AdamNet::GetMediaSlot(u8 device) const
{
    const DeviceMetadata* metadata = GetDeviceMetadata(device);
    return IsValidPointer(metadata) ? metadata->slot : -1;
}

AdamMedia* AdamNet::GetDeviceMedia(u8 device)
{
    int slot = GetMediaSlot(device);
    return slot >= 0 ? &m_Media[slot] : NULL;
}

const AdamMedia* AdamNet::GetDeviceMedia(u8 device) const
{
    return const_cast<AdamNet*>(this)->GetDeviceMedia(device);
}

void AdamNet::ResetMediaCache()
{
    memset(m_MediaCacheValid, 0, sizeof(m_MediaCacheValid));
    memset(m_MediaCacheBlock, 0, sizeof(m_MediaCacheBlock));
    memset(m_MediaCacheGeneration, 0, sizeof(m_MediaCacheGeneration));
}

void AdamNet::InvalidateMediaCache(int slot)
{
    if ((slot < GC_ADAM_MEDIA_DISK_1) || (slot >= GC_ADAM_MEDIA_SLOT_COUNT))
        return;

    m_MediaCacheValid[slot] = false;
    m_MediaCacheBlock[slot] = 0;
    m_MediaCacheGeneration[slot] = 0;
}

GC_AdamMediaError AdamNet::InsertMedia(GC_AdamMediaSlot slot, GC_AdamMediaType type,
    const u8* data, size_t size, bool write_protected, u32 base_crc)
{
    if ((slot < GC_ADAM_MEDIA_DISK_1) || (slot >= GC_ADAM_MEDIA_SLOT_COUNT))
        return GC_ADAM_MEDIA_ERROR_INVALID_ARGUMENT;

    bool disk_slot = (slot == GC_ADAM_MEDIA_DISK_1) || (slot == GC_ADAM_MEDIA_DISK_2);
    if ((disk_slot && type != GC_ADAM_MEDIA_DISK) || (!disk_slot && type != GC_ADAM_MEDIA_DATA_PACK))
        return GC_ADAM_MEDIA_ERROR_INVALID_ARGUMENT;

    GC_AdamMediaError error = m_Media[slot].Insert(type, data, size, write_protected, base_crc);
    if (error == GC_ADAM_MEDIA_ERROR_NONE)
        InvalidateMediaCache(slot);
    return error;
}

void AdamNet::EjectMedia(GC_AdamMediaSlot slot)
{
    if ((slot >= GC_ADAM_MEDIA_DISK_1) && (slot < GC_ADAM_MEDIA_SLOT_COUNT))
    {
        m_Media[slot].Eject();
        InvalidateMediaCache(slot);
    }
}

AdamMedia* AdamNet::GetMedia(GC_AdamMediaSlot slot)
{
    if ((slot < GC_ADAM_MEDIA_DISK_1) || (slot >= GC_ADAM_MEDIA_SLOT_COUNT))
        return NULL;
    return &m_Media[slot];
}

const AdamMedia* AdamNet::GetMedia(GC_AdamMediaSlot slot) const
{
    return const_cast<AdamNet*>(this)->GetMedia(slot);
}

void AdamNet::QueueKey(u8 code)
{
    if (m_KeyboardCount >= kKeyboardFIFOSize)
    {
        m_KeyboardOverflow = true;
        return;
    }

    m_KeyboardFIFO[m_KeyboardWrite] = code;
    m_KeyboardWrite = (u8)((m_KeyboardWrite + 1) % kKeyboardFIFOSize);
    m_KeyboardCount++;
}

bool AdamNet::PopKey(u8* code)
{
    if ((m_KeyboardCount == 0) || !IsValidPointer(code))
        return false;

    *code = m_KeyboardFIFO[m_KeyboardRead];
    m_KeyboardRead = (u8)((m_KeyboardRead + 1) % kKeyboardFIFOSize);
    m_KeyboardCount--;
    return true;
}

u8 AdamNet::TranslateKey(GC_AdamKey key) const
{
    const KeyDefinition* definition = &kAdamKeys[key];

    if (m_KeyPressed[GC_ADAM_KEY_CONTROL])
        return definition->control;
    if (m_KeyPressed[GC_ADAM_KEY_HOME] && IsDirectionKey(key))
        return definition->home;
    if (m_KeyPressed[GC_ADAM_KEY_SHIFT] || m_Lock)
        return definition->shifted;
    return definition->normal;
}

bool AdamNet::IsDirectionKey(GC_AdamKey key) const
{
    return (key >= GC_ADAM_KEY_UP) && (key <= GC_ADAM_KEY_LEFT);
}

u8 AdamNet::GetDiagonalCode(GC_AdamKey key) const
{
    if ((key == GC_ADAM_KEY_UP && m_KeyPressed[GC_ADAM_KEY_RIGHT]) ||
        (key == GC_ADAM_KEY_RIGHT && m_KeyPressed[GC_ADAM_KEY_UP]))
        return 0xA8;
    if ((key == GC_ADAM_KEY_RIGHT && m_KeyPressed[GC_ADAM_KEY_DOWN]) ||
        (key == GC_ADAM_KEY_DOWN && m_KeyPressed[GC_ADAM_KEY_RIGHT]))
        return 0xA9;
    if ((key == GC_ADAM_KEY_DOWN && m_KeyPressed[GC_ADAM_KEY_LEFT]) ||
        (key == GC_ADAM_KEY_LEFT && m_KeyPressed[GC_ADAM_KEY_DOWN]))
        return 0xAA;
    if ((key == GC_ADAM_KEY_LEFT && m_KeyPressed[GC_ADAM_KEY_UP]) ||
        (key == GC_ADAM_KEY_UP && m_KeyPressed[GC_ADAM_KEY_LEFT]))
        return 0xAB;
    return 0;
}

void AdamNet::KeyPressed(GC_AdamKey key)
{
    if ((key < GC_ADAM_KEY_A) || (key >= GC_ADAM_KEY_COUNT) || m_KeyPressed[key])
        return;

    m_KeyPressed[key] = true;

    if (key == GC_ADAM_KEY_LOCK)
    {
        m_Lock = !m_Lock;
        return;
    }

    const KeyDefinition* definition = &kAdamKeys[key];
    if (!definition->sends_code)
        return;

    u8 code = IsDirectionKey(key) ? GetDiagonalCode(key) : 0;
    if (code == 0)
        code = TranslateKey(key);
    QueueKey(code);

    if (definition->repeats)
    {
        m_RepeatKey = key;
        m_RepeatCycles = kKeyRepeatDelayCycles;
    }
}

void AdamNet::KeyReleased(GC_AdamKey key)
{
    if ((key < GC_ADAM_KEY_A) || (key >= GC_ADAM_KEY_COUNT))
        return;

    m_KeyPressed[key] = false;
    if (m_RepeatKey == key)
    {
        m_RepeatKey = GC_ADAM_KEY_COUNT;
        m_RepeatCycles = 0;
    }
}

void AdamNet::ReleaseAllKeys()
{
    memset(m_KeyPressed, 0, sizeof(m_KeyPressed));
    m_RepeatKey = GC_ADAM_KEY_COUNT;
    m_RepeatCycles = 0;
}

void AdamNet::ClockKeyboard(unsigned int cycles)
{
    if ((m_RepeatKey >= GC_ADAM_KEY_COUNT) || !m_KeyPressed[m_RepeatKey])
        return;

    while (cycles >= static_cast<unsigned int>(m_RepeatCycles))
    {
        cycles -= static_cast<unsigned int>(m_RepeatCycles);
        u8 code = IsDirectionKey(m_RepeatKey) ? GetDiagonalCode(m_RepeatKey) : 0;
        QueueKey(code ? code : TranslateKey(m_RepeatKey));
        m_RepeatCycles = kKeyRepeatIntervalCycles;
    }

    m_RepeatCycles -= static_cast<int>(cycles);
}

bool AdamNet::AppendPrinter(u8 value)
{
    if (m_PrinterSize >= kPrinterSpoolSize)
        return false;

    m_PrinterSpool[m_PrinterSize++] = value;
    return true;
}

const u8* AdamNet::GetPrinterData() const
{
    return m_PrinterSpool;
}

int AdamNet::GetPrinterSize() const
{
    return m_PrinterSize;
}

void AdamNet::ClearPrinter()
{
    m_PrinterSize = 0;
}

u16 AdamNet::GetPCBAddress() const
{
    return m_PCBAddress;
}

int AdamNet::GetCyclesUntilEvent() const
{
    return m_CyclesUntilEvent;
}

bool AdamNet::IsBusy() const
{
    return m_State == GC_ADAMNET_CONTROLLER_BUSY;
}

int AdamNet::GetKeyboardFIFOCount() const
{
    return m_KeyboardCount;
}

bool AdamNet::DidKeyboardOverflow() const
{
    return m_KeyboardOverflow;
}

void AdamNet::GetDebugState(GC_AdamDebugState* state) const
{
    if (!IsValidPointer(state) || !IsValidPointer(m_pAdam))
        return;

    state->controller_state = m_State;
    state->pcb_address = m_PCBAddress;
    state->pcb_command_status = GetPCB(PCBCommandStatus);
    state->configured_dcb_count = GetPCB(PCBDeviceCount);
    state->next_scan_index = m_ScanIndex;
    state->cycles_until_event = m_CyclesUntilEvent;
    state->transfer_active = m_Transfer.active;
    state->transfer_pcb = m_Transfer.pcb;
    state->transfer_dcb = m_Transfer.dcb;
    state->transfer_device = m_Transfer.device;
    state->transfer_command = m_Transfer.command;
    state->transfer_error = m_Transfer.error;
    state->transfer_buffer = m_Transfer.buffer;
    state->transfer_length = m_Transfer.length;
    state->transfer_pcb_address = m_Transfer.pcb_address;
    state->transfer_pcb_count = m_Transfer.pcb_count;
    state->transfer_block = m_Transfer.block;
    state->transfer_media_generation = m_Transfer.media_generation;

    for (int dcb = 0; dcb < kMaxDCBs; dcb++)
    {
        GC_AdamDebugDCB* debug_dcb = &state->dcbs[dcb];
        debug_dcb->command_status = GetDCB((u8)dcb, DCBCommandStatus);
        debug_dcb->device = GetDeviceID((u8)dcb);
        debug_dcb->buffer = GetDCB16((u8)dcb, DCBBufferAddressLow);
        debug_dcb->length = GetDCB16((u8)dcb, DCBBufferLengthLow);
        debug_dcb->block = GetDCB32((u8)dcb, DCBBlock0);
        debug_dcb->retry = GetDCB16((u8)dcb, DCBRetryLow);
        debug_dcb->max_length = GetDCB16((u8)dcb, DCBMaxLengthLow);
        debug_dcb->device_type = GetDCB((u8)dcb, DCBDeviceType);
        debug_dcb->node_status = GetDCB((u8)dcb, DCBNodeStatus);
    }

    state->keyboard_fifo_count = m_KeyboardCount;
    state->keyboard_overflow = m_KeyboardOverflow;
    state->keyboard_lock = m_Lock;
    state->keyboard_shift = m_KeyPressed[GC_ADAM_KEY_SHIFT];
    state->keyboard_control = m_KeyPressed[GC_ADAM_KEY_CONTROL];
    state->keyboard_home = m_KeyPressed[GC_ADAM_KEY_HOME];
    state->keyboard_repeat_key = m_RepeatKey;
    state->keyboard_repeat_cycles = m_RepeatCycles;

    for (int slot = 0; slot < GC_ADAM_MEDIA_SLOT_COUNT; slot++)
    {
        const AdamMedia* media = &m_Media[slot];
        GC_AdamDebugMediaState* debug_media = &state->media[slot];
        debug_media->inserted = media->IsInserted();
        debug_media->type = media->GetType();
        debug_media->size = static_cast<u32>(media->GetSize());
        debug_media->block_count = media->GetBlockCount();
        debug_media->position = media->GetPosition();
        debug_media->generation = media->GetGeneration();
        debug_media->cache_valid = m_MediaCacheValid[slot];
        debug_media->cached_block = m_MediaCacheBlock[slot];
        debug_media->cached_generation = m_MediaCacheGeneration[slot];
        debug_media->write_protected = media->IsWriteProtected();
        debug_media->dirty = media->IsDirty();
        debug_media->base_crc = media->GetBaseCRC();
    }

    state->printer_size = m_PrinterSize;
    memcpy(state->printer_data, m_PrinterSpool, (size_t)m_PrinterSize);
}

void AdamNet::SaveState(std::ostream& stream) const
{
    u8 state = (u8)m_State;
    u8 transfer_flags = 0;
    if (m_Transfer.active)
        transfer_flags |= 0x01;
    if (m_Transfer.pcb)
        transfer_flags |= 0x02;
    s32 cycles_until_event = m_CyclesUntilEvent;
    u8 keyboard_overflow = m_KeyboardOverflow ? 1 : 0;
    u8 lock = m_Lock ? 1 : 0;
    u8 repeat_key = (u8)m_RepeatKey;
    s32 repeat_cycles = m_RepeatCycles;
    u16 printer_size = (u16)m_PrinterSize;

    stream.write(reinterpret_cast<const char*>(&state), sizeof(state));
    stream.write(reinterpret_cast<const char*>(&transfer_flags), sizeof(transfer_flags));
    stream.write(reinterpret_cast<const char*>(&m_Transfer.dcb), sizeof(m_Transfer.dcb));
    stream.write(reinterpret_cast<const char*>(&m_Transfer.command), sizeof(m_Transfer.command));
    stream.write(reinterpret_cast<const char*>(&m_Transfer.device), sizeof(m_Transfer.device));
    stream.write(reinterpret_cast<const char*>(&m_Transfer.buffer), sizeof(m_Transfer.buffer));
    stream.write(reinterpret_cast<const char*>(&m_Transfer.length), sizeof(m_Transfer.length));
    stream.write(reinterpret_cast<const char*>(&m_Transfer.pcb_address), sizeof(m_Transfer.pcb_address));
    stream.write(reinterpret_cast<const char*>(&m_Transfer.pcb_count), sizeof(m_Transfer.pcb_count));
    stream.write(reinterpret_cast<const char*>(&m_Transfer.block), sizeof(m_Transfer.block));
    stream.write(reinterpret_cast<const char*>(&m_Transfer.media_generation),
        sizeof(m_Transfer.media_generation));
    stream.write(reinterpret_cast<const char*>(&m_PCBAddress), sizeof(m_PCBAddress));
    stream.write(reinterpret_cast<const char*>(&m_ScanIndex), sizeof(m_ScanIndex));
    stream.write(reinterpret_cast<const char*>(&cycles_until_event), sizeof(cycles_until_event));
    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
    {
        u8 cache_valid = m_MediaCacheValid[i] ? 1 : 0;
        stream.write(reinterpret_cast<const char*>(&cache_valid), sizeof(cache_valid));
        stream.write(reinterpret_cast<const char*>(&m_MediaCacheBlock[i]),
            sizeof(m_MediaCacheBlock[i]));
        stream.write(reinterpret_cast<const char*>(&m_MediaCacheGeneration[i]),
            sizeof(m_MediaCacheGeneration[i]));
    }
    stream.write(reinterpret_cast<const char*>(m_KeyboardFIFO), sizeof(m_KeyboardFIFO));
    stream.write(reinterpret_cast<const char*>(&m_KeyboardRead), sizeof(m_KeyboardRead));
    stream.write(reinterpret_cast<const char*>(&m_KeyboardWrite), sizeof(m_KeyboardWrite));
    stream.write(reinterpret_cast<const char*>(&m_KeyboardCount), sizeof(m_KeyboardCount));
    stream.write(reinterpret_cast<const char*>(&keyboard_overflow), sizeof(keyboard_overflow));

    for (int i = 0; i < GC_ADAM_KEY_COUNT; i++)
    {
        u8 pressed = m_KeyPressed[i] ? 1 : 0;
        stream.write(reinterpret_cast<const char*>(&pressed), sizeof(pressed));
    }

    stream.write(reinterpret_cast<const char*>(&lock), sizeof(lock));
    stream.write(reinterpret_cast<const char*>(&repeat_key), sizeof(repeat_key));
    stream.write(reinterpret_cast<const char*>(&repeat_cycles), sizeof(repeat_cycles));
    stream.write(reinterpret_cast<const char*>(&printer_size), sizeof(printer_size));
    if (printer_size > 0)
        stream.write(reinterpret_cast<const char*>(m_PrinterSpool), printer_size);

    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
        m_Media[i].SaveState(stream);
}

bool AdamNet::ReadState(std::istream& stream, bool load_images)
{
    u8 state = 0;
    u8 transfer_flags = 0;
    s32 cycles_until_event = 0;
    u8 keyboard_overflow = 0;
    u8 pressed[GC_ADAM_KEY_COUNT];
    u8 lock = 0;
    u8 repeat_key = 0;
    s32 repeat_cycles = 0;
    u16 printer_size = 0;
    u8 cache_valid[GC_ADAM_MEDIA_SLOT_COUNT];
    u32 cache_block[GC_ADAM_MEDIA_SLOT_COUNT];
    u32 cache_generation[GC_ADAM_MEDIA_SLOT_COUNT];

    stream.read(reinterpret_cast<char*>(&state), sizeof(state));
    stream.read(reinterpret_cast<char*>(&transfer_flags), sizeof(transfer_flags));
    stream.read(reinterpret_cast<char*>(&m_Transfer.dcb), sizeof(m_Transfer.dcb));
    stream.read(reinterpret_cast<char*>(&m_Transfer.command), sizeof(m_Transfer.command));
    stream.read(reinterpret_cast<char*>(&m_Transfer.device), sizeof(m_Transfer.device));
    stream.read(reinterpret_cast<char*>(&m_Transfer.buffer), sizeof(m_Transfer.buffer));
    stream.read(reinterpret_cast<char*>(&m_Transfer.length), sizeof(m_Transfer.length));
    stream.read(reinterpret_cast<char*>(&m_Transfer.pcb_address), sizeof(m_Transfer.pcb_address));
    stream.read(reinterpret_cast<char*>(&m_Transfer.pcb_count), sizeof(m_Transfer.pcb_count));
    stream.read(reinterpret_cast<char*>(&m_Transfer.block), sizeof(m_Transfer.block));
    stream.read(reinterpret_cast<char*>(&m_Transfer.media_generation),
        sizeof(m_Transfer.media_generation));
    stream.read(reinterpret_cast<char*>(&m_PCBAddress), sizeof(m_PCBAddress));
    stream.read(reinterpret_cast<char*>(&m_ScanIndex), sizeof(m_ScanIndex));
    stream.read(reinterpret_cast<char*>(&cycles_until_event), sizeof(cycles_until_event));
    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
    {
        stream.read(reinterpret_cast<char*>(&cache_valid[i]), sizeof(cache_valid[i]));
        stream.read(reinterpret_cast<char*>(&cache_block[i]), sizeof(cache_block[i]));
        stream.read(reinterpret_cast<char*>(&cache_generation[i]), sizeof(cache_generation[i]));
    }
    stream.read(reinterpret_cast<char*>(m_KeyboardFIFO), sizeof(m_KeyboardFIFO));
    stream.read(reinterpret_cast<char*>(&m_KeyboardRead), sizeof(m_KeyboardRead));
    stream.read(reinterpret_cast<char*>(&m_KeyboardWrite), sizeof(m_KeyboardWrite));
    stream.read(reinterpret_cast<char*>(&m_KeyboardCount), sizeof(m_KeyboardCount));
    stream.read(reinterpret_cast<char*>(&keyboard_overflow), sizeof(keyboard_overflow));
    stream.read(reinterpret_cast<char*>(pressed), sizeof(pressed));
    stream.read(reinterpret_cast<char*>(&lock), sizeof(lock));
    stream.read(reinterpret_cast<char*>(&repeat_key), sizeof(repeat_key));
    stream.read(reinterpret_cast<char*>(&repeat_cycles), sizeof(repeat_cycles));
    stream.read(reinterpret_cast<char*>(&printer_size), sizeof(printer_size));

    bool transfer_active = (transfer_flags & 0x01) != 0;
    bool transfer_pcb = (transfer_flags & 0x02) != 0;

    if (!stream.good() || (state > GC_ADAMNET_CONTROLLER_BUSY) || (transfer_flags & ~0x03) ||
        (transfer_pcb && !transfer_active) ||
        ((state == GC_ADAMNET_CONTROLLER_BUSY) != transfer_active) ||
        (transfer_active && transfer_pcb && ((m_Transfer.command < 1) ||
        (m_Transfer.command > 5))) || (transfer_active && !transfer_pcb &&
        ((m_Transfer.command < CommandStatus) || (m_Transfer.command > CommandRead))) ||
        (m_Transfer.dcb >= kMaxDCBs) ||
        (m_Transfer.pcb_count > kMaxDCBs) || (m_ScanIndex >= kMaxDCBs) ||
        (cycles_until_event <= 0) || (cycles_until_event > kDataPackBlockCycles) ||
        (m_KeyboardRead >= kKeyboardFIFOSize) || (m_KeyboardWrite >= kKeyboardFIFOSize) ||
        (m_KeyboardCount > kKeyboardFIFOSize) || (keyboard_overflow > 1) ||
        (lock > 1) || (repeat_key > GC_ADAM_KEY_COUNT) || (repeat_cycles < 0) ||
        (repeat_cycles > kKeyRepeatDelayCycles) || (printer_size > kPrinterSpoolSize))
    {
        return false;
    }

    for (int i = 0; i < GC_ADAM_KEY_COUNT; i++)
    {
        if (pressed[i] > 1)
            return false;
    }

    if (printer_size > 0)
    {
        stream.read(reinterpret_cast<char*>(m_PrinterSpool), printer_size);
        if (!stream.good())
            return false;
    }

    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
    {
        if (!m_Media[i].LoadState(stream, load_images))
            return false;
        if ((cache_valid[i] > 1) ||
            (cache_valid[i] && (!m_Media[i].IsInserted() ||
            (cache_block[i] >= m_Media[i].GetBlockCount()) ||
            (cache_generation[i] != m_Media[i].GetGeneration()))) ||
            (!cache_valid[i] && ((cache_block[i] != 0) || (cache_generation[i] != 0))))
        {
            return false;
        }
    }

    m_State = (GC_AdamNetControllerState)state;
    m_Transfer.active = transfer_active;
    m_Transfer.pcb = transfer_pcb;
    m_CyclesUntilEvent = cycles_until_event;
    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
    {
        m_MediaCacheValid[i] = cache_valid[i] != 0;
        m_MediaCacheBlock[i] = cache_block[i];
        m_MediaCacheGeneration[i] = cache_generation[i];
    }
    m_KeyboardOverflow = keyboard_overflow != 0;
    for (int i = 0; i < GC_ADAM_KEY_COUNT; i++)
        m_KeyPressed[i] = pressed[i] != 0;
    m_Lock = lock != 0;
    m_RepeatKey = (GC_AdamKey)repeat_key;
    m_RepeatCycles = repeat_cycles;
    m_PrinterSize = printer_size;

    if ((m_RepeatKey < GC_ADAM_KEY_COUNT) &&
        (!m_KeyPressed[m_RepeatKey] || (m_RepeatCycles <= 0)))
        return false;
    if ((m_RepeatKey == GC_ADAM_KEY_COUNT) && (m_RepeatCycles != 0))
        return false;

    return stream.good();
}

bool AdamNet::IsMediaStateCompatible(const AdamNet& state) const
{
    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
    {
        const AdamMedia* current = &m_Media[i];
        const AdamMedia* saved = &state.m_Media[i];

#if defined(__LIBRETRO__)
        // The frontend prepares the saved disk selection before loading the core.
        if (current->IsInserted() != saved->IsInserted())
            return false;
#endif

        if (current->IsInserted() && saved->IsInserted())
        {
            if ((current->GetType() != saved->GetType()) ||
                (current->GetSize() != saved->GetSize()) ||
                (current->GetBaseCRC() != saved->GetBaseCRC()))
            {
                return false;
            }
        }
    }

    return true;
}

bool AdamNet::LoadState(std::istream& stream)
{
    std::streampos start = stream.tellg();
    AdamNet state;
    state.m_pAdam = m_pAdam;

    if (!state.ReadState(stream, false) || !IsMediaStateCompatible(state))
        return false;

    stream.clear();
    stream.seekg(start);
    if (!stream.good())
        return false;

    return ReadState(stream);
}
