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

#include "F18AGPU.h"
#include "F18A.h"

F18AGPU::F18AGPU()
{
    Reset(0x4000, false);
}

F18AGPU::~F18AGPU()
{
}

void F18AGPU::Reset(u16 pc, bool run)
{
    for (int i = 0; i < 16; i++)
        m_registers[i] = 0;

    m_pc = pc;
    m_status = 0;
    m_running = run;
    m_spi_enabled = false;
    m_override_valid = false;
    m_override_opcode = 0;
    m_user_status = 0;
    m_cycle_balance = 0;
    m_total_cycles = 0;
    m_dma_source = 0;
    m_dma_destination = 0;
    m_dma_width = 0;
    m_dma_height = 0;
    m_dma_stride = 0;
    m_dma_decrement = false;
    m_dma_fill = false;
}

void F18AGPU::Trigger()
{
    m_running = true;
}

void F18AGPU::Load(u16 pc, bool run)
{
    m_pc = pc;
    m_running = run;
    m_override_valid = false;
    m_override_opcode = 0;
    m_cycle_balance = 0;
}

void F18AGPU::Stop()
{
    m_running = false;
}

void F18AGPU::RunCycles(int cycles, F18A* video)
{
    if (!m_running || (cycles <= 0))
        return;

    m_cycle_balance += cycles;

    while (m_running && (m_cycle_balance > 0))
    {
        int used = ExecuteInstruction(video);
        if (used <= 0)
            used = 1;
        m_cycle_balance -= used;
        m_total_cycles += used;
    }
}

u16 F18AGPU::ReadWord(F18A* video, u16 address, int& cycles)
{
    cycles += 2;
    return video->ReadF18AGPUWord(address);
}

void F18AGPU::WriteWord(F18A* video, u16 address, u16 value, int& cycles)
{
    cycles += 2;
    video->WriteF18AGPUWord(address, value);
}

F18AGPU::Operand F18AGPU::ResolveOperand(F18A* video, u8 specification, bool byte, int& cycles)
{
    Operand operand;
    operand.value = 0;
    operand.address = 0;
    operand.reg = specification & 0x0F;
    operand.is_register = false;

    u8 mode = specification >> 4;

    switch (mode)
    {
        case 0:
            operand.value = m_registers[operand.reg];
            operand.address = operand.reg << 1;
            operand.is_register = true;
            cycles += 1;
            break;

        case 1:
            operand.address = m_registers[operand.reg];
            operand.value = byte ? ((u16)video->ReadF18AGPUByte(operand.address) << 8) :
                ReadWord(video, operand.address, cycles);
            if (byte)
                cycles++;
            break;

        case 2:
        {
            u16 displacement = ReadWord(video, m_pc, cycles);
            m_pc += 2;
            operand.address = displacement;
            if (operand.reg != 0)
                operand.address += m_registers[operand.reg];
            operand.value = byte ? ((u16)video->ReadF18AGPUByte(operand.address) << 8) :
                ReadWord(video, operand.address, cycles);
            if (byte)
                cycles++;
            break;
        }

        case 3:
            operand.address = m_registers[operand.reg];
            operand.value = byte ? ((u16)video->ReadF18AGPUByte(operand.address) << 8) :
                ReadWord(video, operand.address, cycles);
            if (byte)
                cycles++;
            m_registers[operand.reg] += byte ? 1 : 2;
            break;
    }

    return operand;
}

void F18AGPU::WriteOperand(F18A* video, const Operand& operand, u16 value, bool byte, int& cycles)
{
    if (operand.is_register)
    {
        if (byte)
            m_registers[operand.reg] = (value & 0xFF00) | (m_registers[operand.reg] & 0x00FF);
        else
            m_registers[operand.reg] = value;
        cycles++;
    }
    else if (byte)
    {
        video->WriteF18AGPUByte(operand.address, (u8)(value >> 8));
        cycles++;
    }
    else
    {
        WriteWord(video, operand.address, value, cycles);
    }
}

void F18AGPU::SetFlag(u16 flag, bool set)
{
    if (set)
        m_status |= flag;
    else
        m_status &= ~flag;
}

bool F18AGPU::GetFlag(u16 flag) const
{
    return (m_status & flag) != 0;
}

void F18AGPU::SetParity(u8 value)
{
    value ^= value >> 4;
    value ^= value >> 2;
    value ^= value >> 1;
    SetFlag(STATUS_PARITY, (value & 1) != 0);
}

void F18AGPU::SetLogicalFlags(u16 value, bool byte)
{
    if (byte)
    {
        u8 result = (u8)(value >> 8);
        SetFlag(STATUS_LGT, result != 0);
        SetFlag(STATUS_AGT, ((s8)result) > 0);
        SetFlag(STATUS_EQUAL, result == 0);
    }
    else
    {
        SetFlag(STATUS_LGT, value != 0);
        SetFlag(STATUS_AGT, ((s16)value) > 0);
        SetFlag(STATUS_EQUAL, value == 0);
    }
}

void F18AGPU::SetCompareFlags(u16 source, u16 destination, bool byte)
{
    if (byte)
    {
        u8 source_byte = (u8)(source >> 8);
        u8 destination_byte = (u8)(destination >> 8);
        SetFlag(STATUS_LGT, destination_byte > source_byte);
        SetFlag(STATUS_AGT, ((s8)destination_byte) > ((s8)source_byte));
        SetFlag(STATUS_EQUAL, source_byte == destination_byte);
        SetParity(source_byte);
    }
    else
    {
        SetFlag(STATUS_LGT, destination > source);
        SetFlag(STATUS_AGT, ((s16)destination) > ((s16)source));
        SetFlag(STATUS_EQUAL, source == destination);
    }
}

void F18AGPU::ExecutePIX(F18A* video, const Operand& source, u8 destination, int& cycles)
{
    u8* video_registers = video->GetRegisters();
    u16 options = m_registers[destination];
    u8 x = (u8)(source.value >> 8);
    u8 y = (u8)source.value;
    u16 address;

    if ((options & 0x8000) != 0)
    {
        address = ((video_registers[4] & 0x04) << 11) + ((y >> 3) << 8) +
            (x & 0xF8) + (y & 0x07);
    }
    else
    {
        int width = video_registers[35];
        if (width == 0)
            width = 256;
        int stride = (width + 3) >> 2;
        address = (u16)(((video_registers[32] << 6) + (y * stride) + (x >> 2)) & 0x3FFF);
    }

    if ((options & 0xC000) != 0)
    {
        m_registers[destination] = address;
        cycles++;
        return;
    }

    u8 data = video->ReadF18AGPUByte(address);
    cycles++;
    int shift = 6 - ((x & 3) << 1);
    u8 old_pixel = (data >> shift) & 3;

    if ((options & 0x0800) != 0)
        m_registers[destination] = (options & 0xFFFC) | old_pixel;

    bool write = (options & 0x0400) == 0;
    if ((options & 0x0200) != 0)
    {
        u8 compare = (options >> 4) & 3;
        bool equal = old_pixel == compare;
        write = write && (((options & 0x0100) != 0) ? equal : !equal);
    }

    if (write)
    {
        data = (data & ~(3 << shift)) | ((options & 3) << shift);
        video->WriteF18AGPUByte(address, data);
        cycles++;
    }
}

int F18AGPU::ExecuteInstruction(F18A* video)
{
    int cycles = 1;
    u16 opcode;

    if (m_override_valid)
    {
        opcode = m_override_opcode;
        m_override_valid = false;
    }
    else
    {
        cycles = 5;
        opcode = ReadWord(video, m_pc, cycles);
        m_pc += 2;
    }

    if ((opcode & 0x8000) != 0)
    {
        bool byte = (opcode & 0x1000) != 0;
        Operand source = ResolveOperand(video, opcode & 0x3F, byte, cycles);
        Operand destination = ResolveOperand(video, (opcode >> 6) & 0x3F, byte, cycles);
        u16 source_value = byte ? (source.value & 0xFF00) : source.value;
        u16 destination_value = byte ? (destination.value & 0xFF00) : destination.value;
        u16 result = 0;

        switch ((opcode >> 13) & 3)
        {
            case 0:
                SetCompareFlags(source_value, destination_value, byte);
                return cycles;

            case 1:
            {
                u32 sum = (u32)destination_value + source_value;
                result = (u16)sum;
                SetLogicalFlags(result, byte);
                SetFlag(STATUS_CARRY, sum > 0xFFFF);
                if (byte)
                {
                    s16 signed_sum = (s8)(destination_value >> 8) + (s8)(source_value >> 8);
                    SetFlag(STATUS_OVERFLOW, (signed_sum < -128) || (signed_sum > 127));
                }
                else
                {
                    s32 signed_sum = (s16)destination_value + (s16)source_value;
                    SetFlag(STATUS_OVERFLOW, (signed_sum < -32768) || (signed_sum > 32767));
                }
                if (byte)
                    SetParity((u8)(result >> 8));
                break;
            }

            case 2:
                result = source_value;
                SetLogicalFlags(result, byte);
                if (byte)
                    SetParity((u8)(result >> 8));
                break;

            case 3:
                result = destination_value | source_value;
                SetLogicalFlags(result, byte);
                if (byte)
                    SetParity((u8)(result >> 8));
                break;
        }

        WriteOperand(video, destination, result, byte, cycles);
        return cycles;
    }

    if ((opcode & 0x4000) != 0)
    {
        bool byte = (opcode & 0x1000) != 0;
        Operand source = ResolveOperand(video, opcode & 0x3F, byte, cycles);
        Operand destination = ResolveOperand(video, (opcode >> 6) & 0x3F, byte, cycles);
        u16 source_value = byte ? (source.value & 0xFF00) : source.value;
        u16 destination_value = byte ? (destination.value & 0xFF00) : destination.value;
        u16 result;

        if ((opcode & 0x2000) == 0)
        {
            result = destination_value & ~source_value;
            SetLogicalFlags(result, byte);
        }
        else
        {
            if (byte)
            {
                u8 source_byte = (u8)(source_value >> 8);
                u8 destination_byte = (u8)(destination_value >> 8);
                u8 byte_result = destination_byte - source_byte;
                result = (u16)byte_result << 8;
                SetFlag(STATUS_CARRY, destination_byte >= source_byte);
                s16 signed_result = (s8)destination_byte - (s8)source_byte;
                SetFlag(STATUS_OVERFLOW, (signed_result < -128) || (signed_result > 127));
            }
            else
            {
                result = destination_value - source_value;
                SetFlag(STATUS_CARRY, destination_value >= source_value);
                s32 signed_result = (s16)destination_value - (s16)source_value;
                SetFlag(STATUS_OVERFLOW, (signed_result < -32768) || (signed_result > 32767));
            }
            SetLogicalFlags(result, byte);
        }

        if (byte)
            SetParity((u8)(result >> 8));
        WriteOperand(video, destination, result, byte, cycles);
        return cycles;
    }

    if ((opcode & 0x2000) != 0)
    {
        int operation = (opcode >> 10) & 7;
        u8 destination = (opcode >> 6) & 0x0F;
        Operand source = ResolveOperand(video, opcode & 0x3F, false, cycles);

        switch (operation)
        {
            case 0:
                SetFlag(STATUS_EQUAL, ((~m_registers[destination]) & source.value) == 0);
                break;

            case 1:
                SetFlag(STATUS_EQUAL, (m_registers[destination] & source.value) == 0);
                break;

            case 2:
                m_registers[destination] ^= source.value;
                SetLogicalFlags(m_registers[destination], false);
                break;

            case 3:
                ExecutePIX(video, source, destination, cycles);
                break;

            case 4:
                SetLogicalFlags(0xFF00, true);
                SetParity(0xFF);
                break;

            case 5:
                WriteOperand(video, source, 0xFF00, true, cycles);
                SetLogicalFlags(0xFF00, true);
                SetParity(0xFF);
                break;

            case 6:
            {
                u32 product = (u32)source.value * m_registers[destination];
                m_registers[destination] = (u16)(product >> 16);
                m_registers[(destination + 1) & 0x0F] = (u16)product;
                cycles += 3;
                break;
            }

            case 7:
                if ((source.value == 0) || (m_registers[destination] >= source.value))
                {
                    SetFlag(STATUS_OVERFLOW, true);
                }
                else
                {
                    u32 dividend = ((u32)m_registers[destination] << 16) |
                        m_registers[(destination + 1) & 0x0F];
                    m_registers[destination] = (u16)(dividend / source.value);
                    m_registers[(destination + 1) & 0x0F] = (u16)(dividend % source.value);
                    SetFlag(STATUS_OVERFLOW, false);
                    cycles += 16;
                }
                break;
        }

        return cycles;
    }

    if ((opcode & 0x1000) != 0)
    {
        bool jump = false;
        switch ((opcode >> 8) & 0x0F)
        {
            case 0: jump = true; break;
            case 1: jump = !GetFlag(STATUS_AGT) && !GetFlag(STATUS_EQUAL); break;
            case 2: jump = !GetFlag(STATUS_LGT) || GetFlag(STATUS_EQUAL); break;
            case 3: jump = GetFlag(STATUS_EQUAL); break;
            case 4: jump = GetFlag(STATUS_LGT) || GetFlag(STATUS_EQUAL); break;
            case 5: jump = GetFlag(STATUS_AGT); break;
            case 6: jump = !GetFlag(STATUS_EQUAL); break;
            case 7: jump = !GetFlag(STATUS_CARRY); break;
            case 8: jump = GetFlag(STATUS_CARRY); break;
            case 9: jump = !GetFlag(STATUS_OVERFLOW); break;
            case 10: jump = !GetFlag(STATUS_LGT) && !GetFlag(STATUS_EQUAL); break;
            case 11: jump = GetFlag(STATUS_LGT) && !GetFlag(STATUS_EQUAL); break;
            case 12: jump = GetFlag(STATUS_PARITY); break;
            default: break;
        }

        if (jump)
            m_pc += (s16)((s8)(opcode & 0xFF) * 2);
        return cycles + 1;
    }

    if ((opcode & 0x0800) != 0)
    {
        int operation = (opcode >> 8) & 7;

        if (operation <= 3 || operation == 6)
        {
            int count = (opcode >> 4) & 0x0F;
            u8 reg = opcode & 0x0F;
            if (count == 0)
            {
                count = m_registers[0] & 0x0F;
                if (count == 0)
                    count = 16;
            }

            u16 value = m_registers[reg];
            bool carry = false;
            bool overflow = false;
            bool original_sign = (value & 0x8000) != 0;

            for (int i = 0; i < count; i++)
            {
                if (operation == 0)
                {
                    carry = (value & 1) != 0;
                    value = (value >> 1) | (value & 0x8000);
                }
                else if (operation == 1)
                {
                    carry = (value & 1) != 0;
                    value >>= 1;
                }
                else if (operation == 2)
                {
                    carry = (value & 0x8000) != 0;
                    value <<= 1;
                    if (((value & 0x8000) != 0) != original_sign)
                        overflow = true;
                }
                else if (operation == 3)
                {
                    carry = (value & 1) != 0;
                    value = (value >> 1) | (carry ? 0x8000 : 0);
                }
                else
                {
                    carry = (value & 0x8000) != 0;
                    value = (value << 1) | (carry ? 1 : 0);
                }
            }

            m_registers[reg] = value;
            SetLogicalFlags(value, false);
            SetFlag(STATUS_CARRY, carry);
            if (operation == 2)
                SetFlag(STATUS_OVERFLOW, overflow);
            return cycles + count + 3;
        }

        if (operation == 4)
        {
            if ((opcode & 0x0080) == 0)
            {
                m_registers[15] += 2;
                m_pc = ReadWord(video, m_registers[15], cycles);
            }
            else
            {
                Operand source = ResolveOperand(video, opcode & 0x3F, false, cycles);
                WriteWord(video, m_registers[15], m_pc, cycles);
                m_registers[15] -= 2;
                m_pc = source.address;
            }
        }
        else if (operation == 5)
        {
            Operand source = ResolveOperand(video, opcode & 0x3F, false, cycles);
            WriteWord(video, m_registers[15], source.value, cycles);
            m_registers[15] -= 2;
        }
        else if (operation == 7)
        {
            m_registers[15] += 2;
            u16 value = ReadWord(video, m_registers[15], cycles);
            Operand destination = ResolveOperand(video, opcode & 0x3F, false, cycles);
            WriteOperand(video, destination, value, false, cycles);
        }

        return cycles + 2;
    }

    if ((opcode & 0x0400) != 0)
    {
        int operation = (opcode >> 6) & 0x0F;
        if ((operation == 0) || (operation >= 14))
            return cycles;

        Operand source = ResolveOperand(video, opcode & 0x3F, false, cycles);
        u16 result = source.value;
        bool store = true;
        bool update_flags = true;

        switch (operation)
        {
            case 1:
                m_pc = source.address;
                return cycles + 1;
            case 2:
                m_override_opcode = source.value;
                m_override_valid = true;
                return cycles + ExecuteInstruction(video);
            case 3:
                result = 0;
                update_flags = false;
                break;
            case 4:
                result = (u16)(0 - source.value);
                SetFlag(STATUS_CARRY, source.value == 0);
                SetFlag(STATUS_OVERFLOW, source.value == 0x8000);
                break;
            case 5:
                result = ~source.value;
                break;
            case 6:
                result = source.value + 1;
                SetFlag(STATUS_CARRY, result == 0);
                SetFlag(STATUS_OVERFLOW, source.value == 0x7FFF);
                break;
            case 7:
                result = source.value + 2;
                SetFlag(STATUS_CARRY, result < source.value);
                SetFlag(STATUS_OVERFLOW, (source.value == 0x7FFF) || (source.value == 0x7FFE));
                break;
            case 8:
                result = source.value - 1;
                SetFlag(STATUS_CARRY, source.value != 0);
                SetFlag(STATUS_OVERFLOW, source.value == 0x8000);
                break;
            case 9:
                result = source.value - 2;
                SetFlag(STATUS_CARRY, source.value >= 2);
                SetFlag(STATUS_OVERFLOW, (source.value == 0x8000) || (source.value == 0x8001));
                break;
            case 10:
                m_registers[11] = m_pc;
                m_pc = source.address;
                return cycles + 2;
            case 11:
                result = (source.value << 8) | (source.value >> 8);
                update_flags = false;
                break;
            case 12:
                result = 0xFFFF;
                update_flags = false;
                break;
            case 13:
                if ((source.value & 0x8000) != 0)
                    result = (u16)(0 - source.value);
                SetFlag(STATUS_LGT, source.value != 0);
                SetFlag(STATUS_AGT, ((s16)source.value) > 0);
                SetFlag(STATUS_EQUAL, source.value == 0);
                SetFlag(STATUS_CARRY, source.value == 0);
                SetFlag(STATUS_OVERFLOW, source.value == 0x8000);
                update_flags = false;
                break;
        }

        if (update_flags)
            SetLogicalFlags(result, false);
        if (store)
            WriteOperand(video, source, result, false, cycles);
        return cycles;
    }

    u16 immediate_operation = opcode & 0xFFE0;
    u8 reg = opcode & 0x0F;

    if ((immediate_operation >= 0x0200) && (immediate_operation <= 0x0280))
    {
        u16 immediate = ReadWord(video, m_pc, cycles);
        m_pc += 2;

        switch (immediate_operation)
        {
            case 0x0200:
                m_registers[reg] = immediate;
                SetLogicalFlags(immediate, false);
                break;
            case 0x0220:
            {
                u16 old = m_registers[reg];
                u32 sum = (u32)old + immediate;
                m_registers[reg] = (u16)sum;
                SetLogicalFlags(m_registers[reg], false);
                SetFlag(STATUS_CARRY, sum > 0xFFFF);
                s32 signed_sum = (s16)old + (s16)immediate;
                SetFlag(STATUS_OVERFLOW, (signed_sum < -32768) || (signed_sum > 32767));
                break;
            }
            case 0x0240:
                m_registers[reg] &= immediate;
                SetLogicalFlags(m_registers[reg], false);
                break;
            case 0x0260:
                m_registers[reg] |= immediate;
                SetLogicalFlags(m_registers[reg], false);
                break;
            case 0x0280:
                SetCompareFlags(immediate, m_registers[reg], false);
                break;
        }
        return cycles + 2;
    }

    switch (immediate_operation)
    {
        case 0x02C0:
            m_registers[reg] = m_status & 0xFC00;
            break;
        case 0x0340:
            m_running = false;
            break;
        case 0x0380:
            m_pc = m_registers[14];
            m_status = m_registers[15] & 0xFC00;
            break;
        case 0x03A0:
            m_spi_enabled = true;
            break;
        case 0x03C0:
            m_spi_enabled = false;
            break;
        default:
            break;
    }

    return cycles;
}

void F18AGPU::RunDMA(F18A* video)
{
    int width = m_dma_width == 0 ? 256 : m_dma_width;
    int height = m_dma_height == 0 ? 256 : m_dma_height;
    int direction = m_dma_decrement ? -1 : 1;
    u16 source_row = m_dma_source;
    u16 destination_row = m_dma_destination;
    u8 fill = video->ReadF18AGPUByte(source_row & 0x3FFF);

    for (int y = 0; y < height; y++)
    {
        u16 source = source_row;
        u16 destination = destination_row;

        for (int x = 0; x < width; x++)
        {
            u8 value = m_dma_fill ? fill : video->ReadF18AGPUByte(source & 0x3FFF);
            video->WriteF18AGPUByte(destination & 0x3FFF, value);
            source += direction;
            destination += direction;
        }

        source_row += m_dma_stride;
        destination_row += m_dma_stride;
    }

    int transfers = width * height;
    int dma_cycles = (m_dma_fill ? transfers + 1 : transfers * 2) + 4;
    m_cycle_balance -= dma_cycles;
    m_total_cycles += dma_cycles;
}

void F18AGPU::SaveState(std::ostream& stream)
{
    stream.write(reinterpret_cast<const char*>(m_registers), sizeof(m_registers));
    stream.write(reinterpret_cast<const char*>(&m_pc), sizeof(m_pc));
    stream.write(reinterpret_cast<const char*>(&m_status), sizeof(m_status));
    stream.write(reinterpret_cast<const char*>(&m_running), sizeof(m_running));
    stream.write(reinterpret_cast<const char*>(&m_spi_enabled), sizeof(m_spi_enabled));
    stream.write(reinterpret_cast<const char*>(&m_override_valid), sizeof(m_override_valid));
    stream.write(reinterpret_cast<const char*>(&m_override_opcode), sizeof(m_override_opcode));
    stream.write(reinterpret_cast<const char*>(&m_user_status), sizeof(m_user_status));
    stream.write(reinterpret_cast<const char*>(&m_cycle_balance), sizeof(m_cycle_balance));
    stream.write(reinterpret_cast<const char*>(&m_total_cycles), sizeof(m_total_cycles));
    stream.write(reinterpret_cast<const char*>(&m_dma_source), sizeof(m_dma_source));
    stream.write(reinterpret_cast<const char*>(&m_dma_destination), sizeof(m_dma_destination));
    stream.write(reinterpret_cast<const char*>(&m_dma_width), sizeof(m_dma_width));
    stream.write(reinterpret_cast<const char*>(&m_dma_height), sizeof(m_dma_height));
    stream.write(reinterpret_cast<const char*>(&m_dma_stride), sizeof(m_dma_stride));
    stream.write(reinterpret_cast<const char*>(&m_dma_decrement), sizeof(m_dma_decrement));
    stream.write(reinterpret_cast<const char*>(&m_dma_fill), sizeof(m_dma_fill));
}

void F18AGPU::LoadState(std::istream& stream)
{
    stream.read(reinterpret_cast<char*>(m_registers), sizeof(m_registers));
    stream.read(reinterpret_cast<char*>(&m_pc), sizeof(m_pc));
    stream.read(reinterpret_cast<char*>(&m_status), sizeof(m_status));
    stream.read(reinterpret_cast<char*>(&m_running), sizeof(m_running));
    stream.read(reinterpret_cast<char*>(&m_spi_enabled), sizeof(m_spi_enabled));
    stream.read(reinterpret_cast<char*>(&m_override_valid), sizeof(m_override_valid));
    stream.read(reinterpret_cast<char*>(&m_override_opcode), sizeof(m_override_opcode));
    stream.read(reinterpret_cast<char*>(&m_user_status), sizeof(m_user_status));
    stream.read(reinterpret_cast<char*>(&m_cycle_balance), sizeof(m_cycle_balance));
    stream.read(reinterpret_cast<char*>(&m_total_cycles), sizeof(m_total_cycles));
    stream.read(reinterpret_cast<char*>(&m_dma_source), sizeof(m_dma_source));
    stream.read(reinterpret_cast<char*>(&m_dma_destination), sizeof(m_dma_destination));
    stream.read(reinterpret_cast<char*>(&m_dma_width), sizeof(m_dma_width));
    stream.read(reinterpret_cast<char*>(&m_dma_height), sizeof(m_dma_height));
    stream.read(reinterpret_cast<char*>(&m_dma_stride), sizeof(m_dma_stride));
    stream.read(reinterpret_cast<char*>(&m_dma_decrement), sizeof(m_dma_decrement));
    stream.read(reinterpret_cast<char*>(&m_dma_fill), sizeof(m_dma_fill));
}

bool F18AGPU::IsRunning() const
{
    return m_running;
}

u16 F18AGPU::GetPC() const
{
    return m_pc;
}

u16 F18AGPU::GetStatus() const
{
    return m_status;
}

u16* F18AGPU::GetRegisters()
{
    return m_registers;
}

u8 F18AGPU::GetUserStatus() const
{
    return m_user_status;
}

void F18AGPU::SetUserStatus(u8 status)
{
    m_user_status = status & 0x7F;
}

s64 F18AGPU::GetCycleBalance() const
{
    return m_cycle_balance;
}

u8 F18AGPU::ReadDMARegister(u8 index) const
{
    switch (index & 0x0F)
    {
        case 0: return (u8)(m_dma_source >> 8);
        case 1: return (u8)m_dma_source;
        case 2: return (u8)(m_dma_destination >> 8);
        case 3: return (u8)m_dma_destination;
        case 4: return m_dma_width;
        case 5: return m_dma_height;
        case 6: return (u8)m_dma_stride;
        case 7: return (m_dma_decrement ? 2 : 0) | (m_dma_fill ? 1 : 0);
        default: return 0;
    }
}

void F18AGPU::WriteDMARegister(u8 index, u8 value, F18A* video)
{
    switch (index & 0x0F)
    {
        case 0: m_dma_source = (m_dma_source & 0x00FF) | (value << 8); break;
        case 1: m_dma_source = (m_dma_source & 0xFF00) | value; break;
        case 2: m_dma_destination = (m_dma_destination & 0x00FF) | (value << 8); break;
        case 3: m_dma_destination = (m_dma_destination & 0xFF00) | value; break;
        case 4: m_dma_width = value; break;
        case 5: m_dma_height = value; break;
        case 6: m_dma_stride = (s8)value; break;
        case 7:
            m_dma_decrement = IsSetBit(value, 1);
            m_dma_fill = IsSetBit(value, 0);
            break;
        case 8:
            RunDMA(video);
            break;
    }
}
