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

#ifndef F18A_GPU_H
#define F18A_GPU_H

#include "definitions.h"

class F18A;

class F18AGPU
{
public:
    F18AGPU();
    ~F18AGPU();

    void Reset(u16 pc, bool run);
    void Load(u16 pc, bool run);
    void Trigger();
    void Stop();
    void RunCycles(int cycles, F18A* video);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);

    bool IsRunning() const;
    u16 GetPC() const;
    u16 GetStatus() const;
    u16* GetRegisters();
    u8 GetUserStatus() const;
    void SetUserStatus(u8 status);
    s64 GetCycleBalance() const;
    u8 ReadDMARegister(u8 index) const;
    void WriteDMARegister(u8 index, u8 value, F18A* video);

private:
    struct Operand
    {
        u16 value;
        u16 address;
        u8 reg;
        bool is_register;
    };

    enum StatusFlag
    {
        STATUS_LGT = 0x8000,
        STATUS_AGT = 0x4000,
        STATUS_EQUAL = 0x2000,
        STATUS_CARRY = 0x1000,
        STATUS_OVERFLOW = 0x0800,
        STATUS_PARITY = 0x0400
    };

    int ExecuteInstruction(F18A* video);
    Operand ResolveOperand(F18A* video, u8 specification, bool byte, int& cycles);
    void WriteOperand(F18A* video, const Operand& operand, u16 value, bool byte, int& cycles);
    u16 ReadWord(F18A* video, u16 address, int& cycles);
    void WriteWord(F18A* video, u16 address, u16 value, int& cycles);
    void SetFlag(u16 flag, bool set);
    bool GetFlag(u16 flag) const;
    void SetLogicalFlags(u16 value, bool byte);
    void SetCompareFlags(u16 source, u16 destination, bool byte);
    void SetParity(u8 value);
    void RunDMA(F18A* video);
    void ExecutePIX(F18A* video, const Operand& source, u8 destination, int& cycles);

private:
    u16 m_registers[16];
    u16 m_pc;
    u16 m_status;
    bool m_running;
    bool m_spi_enabled;
    bool m_override_valid;
    u16 m_override_opcode;
    u8 m_user_status;
    s64 m_cycle_balance;
    u64 m_total_cycles;

    u16 m_dma_source;
    u16 m_dma_destination;
    u8 m_dma_width;
    u8 m_dma_height;
    s8 m_dma_stride;
    bool m_dma_decrement;
    bool m_dma_fill;
};

#endif
