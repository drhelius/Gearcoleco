/*
 * Gearcoleco - ColecoVision Emulator
 * Copyright (C) 2013  Ignacio Sanchez

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

#include "mcp_debug_adapter.h"
#include "log.h"
#include "../utils.h"
#include "../emu.h"
#include "../gui.h"
#include "../gui_actions.h"
#include "../gui_debug_disassembler.h"
#include "../gui_debug_memory.h"
#include "../gui_debug_memeditor.h"
#include "../gui_debug_rewind.h"
#include "../config.h"
#include "../rewind.h"
#include <cstring>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>

struct DisassemblerBookmark
{
    u16 address;
    char name[32];
};

static int MemoryEditorOffsetToDisplayAddress(const MemoryAreaInfo& info, int offset)
{
    return offset + (int)info.display_base;
}

static int MemoryEditorDisplayAddressToOffset(const MemoryAreaInfo& info, int address)
{
    return address - (int)info.display_base;
}

void DebugAdapter::Pause()
{
    emu_debug_break();
}

void DebugAdapter::Resume()
{
    emu_debug_continue();
}

void DebugAdapter::StepInto()
{
    emu_debug_step_into();
}

void DebugAdapter::StepOver()
{
    emu_debug_step_over();
}

void DebugAdapter::StepOut()
{
    emu_debug_step_out();
}

void DebugAdapter::StepFrame()
{
    emu_debug_step_frame();
}

void DebugAdapter::Reset()
{
    emu_reset(gui_get_force_configuration());
}

json DebugAdapter::GetDebugStatus()
{
    json result;

    if (!m_core)
    {
        result["error"] = "Core not initialized";
        return result;
    }

    bool is_paused = emu_is_debug_idle();

    result["paused"] = is_paused;

    if (is_paused)
    {
        Processor* cpu = m_core->GetProcessor();
        u16 pc = cpu->GetState()->PC->GetValue();

        bool at_breakpoint = cpu->BreakpointHit();

        result["at_breakpoint"] = at_breakpoint;

        std::ostringstream pc_ss;
        pc_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << pc;
        result["pc"] = pc_ss.str();
    }
    else
    {
        result["at_breakpoint"] = false;
    }

    return result;
}

void DebugAdapter::SetBreakpoint(u16 address, int type, bool read, bool write, bool execute)
{
    Processor* cpu = m_core->GetProcessor();

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%04X", address);

    if (type == Processor::GC_BREAKPOINT_TYPE_ROMRAM && execute && !read && !write)
    {
        cpu->AddBreakpoint(address);
    }
    else
    {
        cpu->AddBreakpoint(type, buffer, read, write, execute);
    }
}

void DebugAdapter::SetBreakpointRange(u16 start_address, u16 end_address, int type, bool read, bool write, bool execute)
{
    Processor* cpu = m_core->GetProcessor();

    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%04X-%04X", start_address, end_address);

    cpu->AddBreakpoint(type, buffer, read, write, execute);
}

void DebugAdapter::ClearBreakpointByAddress(u16 address, int type, u16 end_address)
{
    Processor* cpu = m_core->GetProcessor();
    std::vector<Processor::GC_Breakpoint>* breakpoints = cpu->GetBreakpoints();

    for (int i = (int)breakpoints->size() - 1; i >= 0; i--)
    {
        Processor::GC_Breakpoint& bp = (*breakpoints)[i];

        if (bp.type != type)
            continue;

        if (end_address > 0 && end_address >= address)
        {
            if (bp.range && bp.address1 == address && bp.address2 == end_address)
                breakpoints->erase(breakpoints->begin() + i);
        }
        else
        {
            if (!bp.range && bp.address1 == address)
                breakpoints->erase(breakpoints->begin() + i);
        }
    }
}

std::vector<BreakpointInfo> DebugAdapter::ListBreakpoints()
{
    std::vector<BreakpointInfo> result;
    Processor* cpu = m_core->GetProcessor();
    std::vector<Processor::GC_Breakpoint>* breakpoints = cpu->GetBreakpoints();

    for (const Processor::GC_Breakpoint& brk : *breakpoints)
    {
        BreakpointInfo info;
        info.enabled = brk.enabled;
        info.type = brk.type;
        info.address1 = brk.address1;
        info.address2 = brk.address2;
        info.read = brk.read;
        info.write = brk.write;
        info.execute = brk.execute;
        info.range = brk.range;
        info.type_name = GetBreakpointTypeName(brk.type);
        result.push_back(info);
    }

    return result;
}

RegistersSnapshot DebugAdapter::GetRegisters()
{
    Debug("[MCP] GetRegisters: start");

    Processor* cpu = m_core->GetProcessor();
    Processor::ProcessorState* state = cpu->GetState();

    Debug("[MCP] GetRegisters: creating snapshot");
    RegistersSnapshot snapshot;

    snapshot.AF = state->AF->GetValue();
    snapshot.BC = state->BC->GetValue();
    snapshot.DE = state->DE->GetValue();
    snapshot.HL = state->HL->GetValue();
    snapshot.AF2 = state->AF2->GetValue();
    snapshot.BC2 = state->BC2->GetValue();
    snapshot.DE2 = state->DE2->GetValue();
    snapshot.HL2 = state->HL2->GetValue();
    snapshot.IX = state->IX->GetValue();
    snapshot.IY = state->IY->GetValue();
    snapshot.SP = state->SP->GetValue();
    snapshot.PC = state->PC->GetValue();
    snapshot.WZ = state->WZ->GetValue();
    snapshot.I = *state->I;
    snapshot.R = *state->R;
    snapshot.IFF1 = *state->IFF1;
    snapshot.IFF2 = *state->IFF2;
    snapshot.Halt = *state->Halt;
    snapshot.InterruptMode = *state->InterruptMode;

    Debug("[MCP] GetRegisters: done (PC=%04X)", snapshot.PC);
    return snapshot;
}

void DebugAdapter::SetRegister(const std::string& name, u32 value)
{
    Processor* cpu = m_core->GetProcessor();
    Processor::ProcessorState* state = cpu->GetState();

    if (name == "AF")
        state->AF->SetValue((u16)value);
    else if (name == "BC")
        state->BC->SetValue((u16)value);
    else if (name == "DE")
        state->DE->SetValue((u16)value);
    else if (name == "HL")
        state->HL->SetValue((u16)value);
    else if (name == "AF'" || name == "AF2")
        state->AF2->SetValue((u16)value);
    else if (name == "BC'" || name == "BC2")
        state->BC2->SetValue((u16)value);
    else if (name == "DE'" || name == "DE2")
        state->DE2->SetValue((u16)value);
    else if (name == "HL'" || name == "HL2")
        state->HL2->SetValue((u16)value);
    else if (name == "IX")
        state->IX->SetValue((u16)value);
    else if (name == "IY")
        state->IY->SetValue((u16)value);
    else if (name == "SP")
        state->SP->SetValue((u16)value);
    else if (name == "PC")
        state->PC->SetValue((u16)value);
    else if (name == "WZ")
        state->WZ->SetValue((u16)value);
    else if (name == "A")
        state->AF->SetHigh((u8)value);
    else if (name == "F")
        state->AF->SetLow((u8)value);
    else if (name == "B")
        state->BC->SetHigh((u8)value);
    else if (name == "C")
        state->BC->SetLow((u8)value);
    else if (name == "D")
        state->DE->SetHigh((u8)value);
    else if (name == "E")
        state->DE->SetLow((u8)value);
    else if (name == "H")
        state->HL->SetHigh((u8)value);
    else if (name == "L")
        state->HL->SetLow((u8)value);
    else if (name == "I")
        *state->I = (u8)value;
    else if (name == "R")
        *state->R = (u8)value;
}

std::vector<MemoryAreaInfo> DebugAdapter::ListMemoryAreas()
{
    std::vector<MemoryAreaInfo> result;

    for (int i = 0; i < MEMORY_EDITOR_MAX; i++)
    {
        MemoryAreaInfo info = GetMemoryAreaInfo(i);
        if (info.data != NULL && info.size > 0)
        {
            result.push_back(info);
        }
    }

    return result;
}

std::vector<u8> DebugAdapter::ReadMemoryArea(int area, u32 offset, size_t size)
{
    std::vector<u8> result;
    MemoryAreaInfo info = GetMemoryAreaInfo(area);

    if (info.data == NULL || offset >= info.size)
        return result;

    u32 bytes_to_read = (u32)size;
    if (offset + bytes_to_read > info.size)
        bytes_to_read = info.size - offset;

    for (u32 i = 0; i < bytes_to_read; i++)
    {
        result.push_back(info.data[offset + i]);
    }

    return result;
}

void DebugAdapter::WriteMemoryArea(int area, u32 offset, const std::vector<u8>& data)
{
    MemoryAreaInfo info = GetMemoryAreaInfo(area);

    if (info.data == NULL || offset >= info.size)
        return;

    for (size_t i = 0; i < data.size() && (offset + i) < info.size; i++)
    {
        info.data[offset + i] = data[i];
    }
}

std::vector<DisasmLine> DebugAdapter::GetDisassembly(u16 start_address, u16 end_address, int bank, bool resolve_symbols)
{
    std::vector<DisasmLine> result;
    Memory* memory = m_core->GetMemory();

    bool use_explicit_bank = (bank >= 0 && bank <= 0xFF);

    // Scan backwards to find any instruction that might span into our range
    u16 scan_start = start_address;
    const int MAX_INSTRUCTION_SIZE = 7;

    for (int lookback = 1; lookback < MAX_INSTRUCTION_SIZE && scan_start > 0; lookback++)
    {
        u16 check_addr = start_address - lookback;

        if (use_explicit_bank)
        {
            u16 start_offset = start_address & 0x1FFF;
            if (lookback > start_offset)
                break;
            check_addr = (start_address & 0xE000) | (start_offset - lookback);
        }

        GC_Disassembler_Record* record = NULL;

        if (use_explicit_bank)
        {
            record = memory->GetDisassemblerRecord(check_addr, (u8)bank);
        }
        else
        {
            record = memory->GetDisassemblerRecord(check_addr);
        }

        if (IsValidPointer(record) && record->name[0] != 0)
        {
            u16 instr_end = check_addr + record->size - 1;
            if (instr_end >= start_address)
            {
                scan_start = check_addr;
                break;
            }
        }
    }

    u32 addr = scan_start;

    while (addr <= (u32)end_address)
    {
        GC_Disassembler_Record* record = NULL;

        if (use_explicit_bank)
            record = memory->GetDisassemblerRecord((u16)addr, (u8)bank);
        else
            record = memory->GetDisassemblerRecord((u16)addr);

        if (IsValidPointer(record) && record->name[0] != 0)
        {
            DisasmLine line;
            line.address = (u16)addr;
            line.bank = record->bank;
            line.name = record->name;
            strip_color_tags(line.name);
            line.bytes = record->bytes;
            line.segment = record->segment;
            line.size = record->size;
            line.jump = record->jump;
            line.jump_address = record->jump_address;
            line.jump_bank = record->jump_bank;
            line.has_operand_address = record->has_operand_address;
            line.operand_address = record->operand_address;
            line.subroutine = record->subroutine;
            line.irq = record->irq;

            if (resolve_symbols)
            {
                std::string instr = line.name;
                if (!gui_debug_resolve_symbol(record, instr, "", ""))
                    gui_debug_resolve_label(record, instr, "", "");
                line.name = instr;
            }

            result.push_back(line);

            u32 next_addr;

            if (use_explicit_bank)
            {
                u16 offset_in_bank = (u16)addr & 0x1FFF;
                offset_in_bank += (u16)record->size;
                if (offset_in_bank >= 0x2000)
                {
                    break;
                }
                next_addr = ((u16)addr & 0xE000) | offset_in_bank;
            }
            else
            {
                next_addr = addr + (u32)record->size;
            }

            if ((record->size == 0) || (next_addr <= addr))
                next_addr = addr + 1;

            addr = next_addr;
        }
        else
        {
            addr++;
        }
    }

    return result;
}

const char* DebugAdapter::GetBreakpointTypeName(int type)
{
    switch (type)
    {
        case Processor::GC_BREAKPOINT_TYPE_ROMRAM:
            return "ROM/RAM";
        case Processor::GC_BREAKPOINT_TYPE_VRAM:
            return "VRAM";
        case Processor::GC_BREAKPOINT_TYPE_VDP_REGISTER:
            return "VDP REG";
        default:
            return "UNKNOWN";
    }
}

MemoryAreaInfo DebugAdapter::GetMemoryAreaInfo(int area)
{
    MemoryAreaInfo info;
    info.id = area;
    info.data = NULL;
    info.size = 0;
    info.display_base = 0;

    Memory* memory = m_core->GetMemory();
    Cartridge* cart = m_core->GetCartridge();
    Video* video = m_core->GetVideo();

    switch (area)
    {
        case MEMORY_EDITOR_BIOS:
            info.name = "BIOS";
            info.data = memory->GetBios();
            info.size = 0x2000;
            break;
        case MEMORY_EDITOR_RAM:
            info.name = "RAM";
            info.data = memory->GetRam();
            info.size = 0x400;
            info.display_base = 0x6000;
            break;
        case MEMORY_EDITOR_SGM_RAM:
            info.name = "SGM RAM";
            info.data = memory->GetSGMRam();
            info.size = 0x8000;
            break;
        case MEMORY_EDITOR_VRAM:
            info.name = "VRAM";
            info.data = video->GetVRAM();
            info.size = 0x4000;
            break;
        case MEMORY_EDITOR_ROM:
            info.name = "FULL ROM";
            info.data = cart->GetROM();
            info.size = cart->GetROMSize();
            break;
        default:
            break;
    }

    return info;
}

json DebugAdapter::GetMediaInfo()
{
    json info;
    Cartridge* cart = m_core->GetCartridge();

    info["emulator"] = GEARCOLECO_TITLE;
    info["emulator_version"] = GEARCOLECO_VERSION;
    info["ready"] = cart->IsReady();
    info["file_path"] = cart->GetFilePath();
    info["file_name"] = cart->GetFileName();
    info["file_directory"] = cart->GetFileDirectory();

    std::ostringstream crc_ss;
    crc_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << cart->GetCRC();
    info["crc"] = crc_ss.str();

    info["is_pal"] = cart->IsPAL();
    info["has_sram"] = cart->HasSRAM();
    info["valid_rom"] = cart->IsValidROM();
    info["rom_size"] = cart->GetROMSize();
    info["rom_bank_count"] = cart->GetROMBankCount();

    Cartridge::CartridgeTypes type = cart->GetType();
    const char* type_names[] = {
        "ColecoVision", "MegaCart", "Activision", "OCM", "Not Supported"
    };
    int type_idx = (int)type;
    if (type_idx >= 0 && type_idx < (int)(sizeof(type_names) / sizeof(type_names[0])))
        info["cartridge_type"] = type_names[type_idx];
    else
        info["cartridge_type"] = "Unknown";

    info["cartridge_system"] = "ColecoVision";

    return info;
}

json DebugAdapter::GetZ80Status()
{
    json status;
    Processor* cpu = m_core->GetProcessor();
    Processor::ProcessorState* state = cpu->GetState();
    Memory* memory = m_core->GetMemory();

    std::ostringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');

    // Main register pairs
    ss << std::setw(4) << state->AF->GetValue();
    status["AF"] = ss.str(); ss.str("");

    ss << std::setw(4) << state->BC->GetValue();
    status["BC"] = ss.str(); ss.str("");

    ss << std::setw(4) << state->DE->GetValue();
    status["DE"] = ss.str(); ss.str("");

    ss << std::setw(4) << state->HL->GetValue();
    status["HL"] = ss.str(); ss.str("");

    // Shadow register pairs
    ss << std::setw(4) << state->AF2->GetValue();
    status["AF'"] = ss.str(); ss.str("");

    ss << std::setw(4) << state->BC2->GetValue();
    status["BC'"] = ss.str(); ss.str("");

    ss << std::setw(4) << state->DE2->GetValue();
    status["DE'"] = ss.str(); ss.str("");

    ss << std::setw(4) << state->HL2->GetValue();
    status["HL'"] = ss.str(); ss.str("");

    // Index, stack, PC
    ss << std::setw(4) << state->IX->GetValue();
    status["IX"] = ss.str(); ss.str("");

    ss << std::setw(4) << state->IY->GetValue();
    status["IY"] = ss.str(); ss.str("");

    ss << std::setw(4) << state->SP->GetValue();
    status["SP"] = ss.str(); ss.str("");

    ss << std::setw(4) << state->PC->GetValue();
    status["PC"] = ss.str(); ss.str("");

    ss << std::setw(4) << state->WZ->GetValue();
    status["WZ"] = ss.str(); ss.str("");

    // Individual 8-bit registers
    u8 a = state->AF->GetHigh();
    u8 f = state->AF->GetLow();

    ss << std::setw(2) << (int)a;
    status["A"] = ss.str(); ss.str("");

    ss << std::setw(2) << (int)f;
    status["F"] = ss.str(); ss.str("");

    ss << std::setw(2) << (int)state->BC->GetHigh();
    status["B"] = ss.str(); ss.str("");

    ss << std::setw(2) << (int)state->BC->GetLow();
    status["C"] = ss.str(); ss.str("");

    ss << std::setw(2) << (int)state->DE->GetHigh();
    status["D"] = ss.str(); ss.str("");

    ss << std::setw(2) << (int)state->DE->GetLow();
    status["E"] = ss.str(); ss.str("");

    ss << std::setw(2) << (int)state->HL->GetHigh();
    status["H"] = ss.str(); ss.str("");

    ss << std::setw(2) << (int)state->HL->GetLow();
    status["L"] = ss.str(); ss.str("");

    ss << std::setw(2) << (int)*state->I;
    status["I"] = ss.str(); ss.str("");

    ss << std::setw(2) << (int)*state->R;
    status["R"] = ss.str(); ss.str("");

    // Flags decoded from F register
    status["flag_S"] = (f & 0x80) != 0;
    status["flag_Z"] = (f & 0x40) != 0;
    status["flag_Y"] = (f & 0x20) != 0;
    status["flag_H"] = (f & 0x10) != 0;
    status["flag_X"] = (f & 0x08) != 0;
    status["flag_PV"] = (f & 0x04) != 0;
    status["flag_N"] = (f & 0x02) != 0;
    status["flag_C"] = (f & 0x01) != 0;

    // Physical PC and bank
    ss << std::setw(6) << memory->GetPhysicalAddress(state->PC->GetValue());
    status["physical_PC"] = ss.str(); ss.str("");

    ss << std::setw(2) << (int)memory->GetBank(state->PC->GetValue());
    status["bank"] = ss.str(); ss.str("");

    // Interrupt state
    status["IFF1"] = *state->IFF1;
    status["IFF2"] = *state->IFF2;
    status["IM"] = *state->InterruptMode;
    status["Halt"] = *state->Halt;
    status["INT"] = *state->INT;
    status["NMI"] = *state->NMI;

    return status;
}

json DebugAdapter::GetVDPRegisters()
{
    json registers = json::array();
    Video* video = m_core->GetVideo();
    u8* regs = video->GetRegisters();

    const char* reg_names[] = {
        "CONTROL 0", "CONTROL 1", "NAME TABLE",
        "COLOR TABLE", "PATTERN TABLE", "SPRITE ATTR",
        "SPRITE PAT", "BACKDROP COLOR"
    };

    for (int i = 0; i < 8; i++)
    {
        json reg;
        reg["index"] = i;

        std::ostringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)regs[i];
        reg["value"] = ss.str();
        reg["description"] = reg_names[i];

        registers.push_back(reg);
    }

    json decoded;
    decoded["screen_enabled"] = (regs[1] & 0x40) != 0;
    decoded["nmi_enabled"] = (regs[1] & 0x20) != 0;
    decoded["sprite_size_16x16"] = (regs[1] & 0x02) != 0;
    decoded["sprite_magnification"] = (regs[1] & 0x01) != 0;
    decoded["external_video"] = (regs[0] & 0x01) != 0;
    decoded["tms9918_mode"] = video->GetMode();
    decoded["name_table_addr"] = (regs[2] & 0x0F) << 10;
    decoded["color_table_addr"] = regs[3] << 6;
    decoded["pattern_table_addr"] = (regs[4] & 0x07) << 11;
    decoded["sprite_attr_addr"] = (regs[5] & 0x7F) << 7;
    decoded["sprite_pattern_addr"] = (regs[6] & 0x07) << 11;
    decoded["backdrop_color"] = regs[7] & 0x0F;
    decoded["text_color"] = (regs[7] >> 4) & 0x0F;

    json result;
    result["registers"] = registers;
    result["decoded"] = decoded;

    return result;
}

json DebugAdapter::GetVDPStatus()
{
    json status;
    Video* video = m_core->GetVideo();

    u8 flags = video->GetStatusReg();

    status["status_flags"] = flags;
    status["frame_interrupt"] = (flags & 0x80) != 0;
    status["sprite_overflow"] = (flags & 0x40) != 0;
    status["sprite_collision"] = (flags & 0x20) != 0;
    status["fifth_sprite"] = flags & 0x1F;

    status["tms9918_mode"] = video->GetMode();
    status["render_line"] = video->GetRenderLine();
    status["cycle_counter"] = video->GetCycleCounter();
    status["display_enabled"] = (video->GetRegisters()[1] & 0x40) != 0;
    status["is_pal"] = video->IsPAL();

    return status;
}

json DebugAdapter::GetPSGStatus()
{
    json status;
    Audio* audio = m_core->GetAudio();
    Sms_Apu* psg = audio->GetPSG();
    Sms_Apu_State psg_state = psg->GetState();

    GC_RuntimeInfo runtime;
    m_core->GetRuntimeInfo(runtime);
    int master_clock = (runtime.region == Region_PAL) ? GC_MASTER_CLOCK_PAL : GC_MASTER_CLOCK_NTSC;

    std::ostringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');

    json channels = json::array();
    for (int c = 0; c < 4; c++)
    {
        Sms_Apu_State::Channel* ch = &psg_state.channels[c];
        json channel;

        channel["index"] = c;
        channel["type"] = (c < 3) ? "tone" : "noise";
        channel["volume_reg"] = ch->volume_reg;
        channel["mute"] = *ch->mute;

        if (ch->volume_reg == 15)
            channel["volume_info"] = "OFF";
        else if (ch->volume_reg == 0)
            channel["volume_info"] = "MAX";
        else
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "-%.1f dB", ch->volume_reg * 2.0f);
            channel["volume_info"] = buf;
        }

        if (c < 3)
        {
            int raw_period = ch->period >> 4;
            channel["period"] = raw_period;
            channel["phase"] = ch->phase;

            if (raw_period > 0)
            {
                float freq_hz = (float)master_clock / (32.0f * raw_period);
                channel["frequency_hz"] = freq_hz;
            }
            else
            {
                channel["frequency_hz"] = 0.0f;
            }
        }
        else
        {
            static const char* noise_rate_names[4] = { "N/512", "N/1024", "N/2048", "Tone 3" };
            channel["noise_white"] = psg_state.noise_white;
            channel["noise_rate"] = psg_state.noise_rate;
            channel["noise_rate_name"] = noise_rate_names[psg_state.noise_rate];
            channel["noise_shifter"] = psg_state.noise_shifter;

            if (psg_state.noise_rate < 3)
            {
                static const int noise_dividers[3] = { 512, 1024, 2048 };
                float freq_hz = (float)master_clock / (float)noise_dividers[psg_state.noise_rate];
                channel["frequency_hz"] = freq_hz;
            }
            else
            {
                int ch3_raw = psg_state.channels[2].period >> 4;
                if (ch3_raw > 0)
                {
                    float freq_hz = (float)master_clock / (32.0f * ch3_raw);
                    channel["frequency_hz"] = freq_hz;
                }
            }
        }

        channels.push_back(channel);
    }

    status["channels"] = channels;
    status["latch"] = psg_state.latch;

    ss << std::setw(2) << psg_state.ggstereo;
    status["gg_stereo"] = ss.str(); ss.str("");

    status["noise_feedback"] = psg_state.noise_feedback;

    return status;
}

json DebugAdapter::GetAY8910Status()
{
    json status;
    Audio* audio = m_core->GetAudio();
    AY8910* ay = audio->GetAY8910();

    const u8* regs = ay->GetRegisters();
    const u16* tone_periods = ay->GetTonePeriods();
    const u8* amplitudes = ay->GetAmplitudes();
    const bool* tone_disable = ay->GetToneDisable();
    const bool* noise_disable = ay->GetNoiseDisable();
    const bool* envelope_mode = ay->GetEnvelopeMode();
    int clock_rate = ay->GetClockRate();

    json channels = json::array();
    for (int c = 0; c < 3; c++)
    {
        json channel;
        channel["index"] = c;
        channel["name"] = std::string(1, (char)('A' + c));
        channel["tone_enabled"] = !tone_disable[c];
        channel["noise_enabled"] = !noise_disable[c];
        channel["envelope_enabled"] = envelope_mode[c];
        channel["tone_period"] = tone_periods[c];
        channel["tone_reg_lo"] = regs[c * 2];
        channel["tone_reg_hi"] = regs[c * 2 + 1] & 0x0F;
        channel["amplitude"] = amplitudes[c];
        channel["amplitude_reg"] = regs[8 + c];

        bool* mute = ay->GetChannelMute(c);
        channel["mute"] = mute ? *mute : false;

        if (tone_periods[c] > 0 && clock_rate > 0)
            channel["frequency_hz"] = (float)clock_rate / (16.0f * tone_periods[c]);
        else
            channel["frequency_hz"] = 0.0f;

        channels.push_back(channel);
    }
    status["channels"] = channels;

    json noise;
    noise["period"] = ay->GetNoisePeriod();
    noise["period_reg"] = regs[6];
    noise["lfsr"] = ay->GetNoiseShift() & 0x1FFFF;
    if (ay->GetNoisePeriod() > 0 && clock_rate > 0)
        noise["frequency_hz"] = (float)clock_rate / (16.0f * ay->GetNoisePeriod());
    else
        noise["frequency_hz"] = 0.0f;
    status["noise"] = noise;

    json envelope;
    envelope["period"] = ay->GetEnvelopePeriod();
    envelope["period_lo"] = regs[11];
    envelope["period_hi"] = regs[12];
    envelope["shape"] = regs[13] & 0x0F;
    envelope["step"] = ay->GetEnvelopeStep();
    envelope["volume"] = ay->GetEnvelopeVolume();
    if (ay->GetEnvelopePeriod() > 0 && clock_rate > 0)
        envelope["frequency_hz"] = (float)clock_rate / (256.0f * ay->GetEnvelopePeriod());
    else
        envelope["frequency_hz"] = 0.0f;
    status["envelope"] = envelope;

    std::ostringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0');

    json registers = json::array();
    for (int i = 0; i < 16; i++)
    {
        ss << std::setw(2) << (int)regs[i];
        registers.push_back(ss.str());
        ss.str("");
    }
    status["registers"] = registers;
    status["selected_register"] = ay->GetSelectedRegister();
    status["mixer"] = regs[7];

    return status;
}

// Base64 encoding table
static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64_encode(const unsigned char* data, int size)
{
    std::string result;
    result.reserve(((size + 2) / 3) * 4);

    int i = 0;
    while (i < size)
    {
        unsigned char byte1 = data[i++];
        bool has_byte2 = i < size;
        unsigned char byte2 = has_byte2 ? data[i++] : 0;
        bool has_byte3 = i < size;
        unsigned char byte3 = has_byte3 ? data[i++] : 0;

        result.push_back(base64_chars[byte1 >> 2]);
        result.push_back(base64_chars[((byte1 & 0x03) << 4) | (byte2 >> 4)]);
        result.push_back(has_byte2 ? base64_chars[((byte2 & 0x0F) << 2) | (byte3 >> 6)] : '=');
        result.push_back(has_byte3 ? base64_chars[byte3 & 0x3F] : '=');
    }

    return result;
}

json DebugAdapter::GetScreenshot()
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    GC_RuntimeInfo runtime;
    m_core->GetRuntimeInfo(runtime);

    unsigned char* png_buffer = NULL;
    int png_size = emu_get_screenshot_png(&png_buffer);

    if (png_size == 0 || !png_buffer)
    {
        result["error"] = "Failed to capture screenshot";
        return result;
    }

    std::string base64_png = base64_encode(png_buffer, png_size);
    free(png_buffer);

    result["__mcp_image"] = true;
    result["data"] = base64_png;
    result["mimeType"] = "image/png";
    result["width"] = runtime.screen_width;
    result["height"] = runtime.screen_height;

    return result;
}

json DebugAdapter::LoadMedia(const std::string& file_path)
{
    json result;

    if (file_path.empty())
    {
        result["error"] = "File path is required";
        Log("[MCP] LoadMedia failed: File path is required");
        return result;
    }

    emu_load_media_async(file_path.c_str(), gui_get_force_configuration());

    int timeout_ms = 180000;
    int elapsed_ms = 0;
    while (emu_is_media_loading() && elapsed_ms < timeout_ms)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        elapsed_ms += 500;
    }

    if (emu_is_media_loading())
    {
        result["error"] = "Loading timed out";
        Log("[MCP] LoadMedia timed out: %s", file_path.c_str());
        return result;
    }

    if (!emu_finish_media_loading() || !m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "Failed to load media file";
        Log("[MCP] LoadMedia failed: %s", file_path.c_str());
        return result;
    }

    result["success"] = true;
    result["file_path"] = file_path;
    result["rom_name"] = m_core->GetCartridge()->GetFileName();
    result["is_pal"] = m_core->GetCartridge()->IsPAL();

    return result;
}

json DebugAdapter::LoadSymbols(const std::string& file_path)
{
    json result;

    if (file_path.empty())
    {
        result["error"] = "File path is required";
        Log("[MCP] LoadSymbols failed: File path is required");
        return result;
    }

    gui_debug_load_symbols_file(file_path.c_str());

    result["success"] = true;
    result["file_path"] = file_path;

    return result;
}

json DebugAdapter::ListSaveStateSlots()
{
    json result;
    json slots = json::array();

    for (int i = 0; i < 5; i++)
    {
        json slot;
        slot["slot"] = i + 1;
        slot["selected"] = (config_emulator.save_slot == i);

        if (emu_savestates[i].rom_name[0] != 0)
        {
            slot["rom_name"] = emu_savestates[i].rom_name;
            slot["timestamp"] = emu_savestates[i].timestamp;
            slot["version"] = emu_savestates[i].version;
            slot["valid"] = (emu_savestates[i].version >= GC_SAVESTATE_MIN_VERSION && emu_savestates[i].version <= GC_SAVESTATE_VERSION);
            slot["has_screenshot"] = IsValidPointer(emu_savestates_screenshots[i].data);

            if (emu_savestates[i].emu_build[0] != 0)
                slot["emu_build"] = emu_savestates[i].emu_build;
        }
        else
        {
            slot["empty"] = true;
        }

        slots.push_back(slot);
    }

    result["slots"] = slots;
    result["current_slot"] = config_emulator.save_slot + 1;

    return result;
}

json DebugAdapter::SelectSaveStateSlot(int slot)
{
    json result;

    if (slot < 1 || slot > 5)
    {
        result["error"] = "Invalid slot number (must be 1-5)";
        Log("[MCP] SelectSaveStateSlot failed: Invalid slot %d", slot);
        return result;
    }

    config_emulator.save_slot = slot - 1;

    result["success"] = true;
    result["slot"] = slot;

    return result;
}

json DebugAdapter::SaveState()
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        Log("[MCP] SaveState failed: No media loaded");
        return result;
    }

    int slot = config_emulator.save_slot + 1;
    emu_save_state_slot(slot);

    result["success"] = true;
    result["slot"] = slot;
    result["rom_name"] = m_core->GetCartridge()->GetFileName();

    return result;
}

json DebugAdapter::LoadState()
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        Log("[MCP] LoadState failed: No media loaded");
        return result;
    }

    int slot = config_emulator.save_slot + 1;

    if (emu_savestates[config_emulator.save_slot].rom_name[0] == 0)
    {
        result["error"] = "Save state slot is empty";
        Log("[MCP] LoadState failed: Slot %d is empty", slot);
        return result;
    }

    emu_load_state_slot(slot);

    result["success"] = true;
    result["slot"] = slot;

    return result;
}

json DebugAdapter::SetFastForwardSpeed(int speed)
{
    json result;

    if (speed < 0 || speed > 4)
    {
        result["error"] = "Invalid speed (must be 0-4: 0=1.5x, 1=2x, 2=2.5x, 3=3x, 4=Unlimited)";
        Log("[MCP] SetFastForwardSpeed failed: Invalid speed %d", speed);
        return result;
    }

    config_emulator.ffwd_speed = speed;

    result["success"] = true;
    result["speed"] = speed;

    const char* speed_names[] = {"1.5x", "2x", "2.5x", "3x", "Unlimited"};
    result["speed_name"] = speed_names[speed];

    return result;
}

json DebugAdapter::ToggleFastForward(bool enabled)
{
    json result;

    config_emulator.ffwd = enabled;
    gui_action_ffwd();

    result["success"] = true;
    result["enabled"] = enabled;
    result["speed"] = config_emulator.ffwd_speed;

    return result;
}

json DebugAdapter::GetRewindStatus()
{
    json result;

    result["active"] = rewind_is_active();
    result["snapshot_count"] = rewind_get_snapshot_count();
    result["capacity"] = rewind_get_capacity();
    result["frames_per_snapshot"] = rewind_get_frames_per_snapshot();
    result["memory_usage"] = rewind_get_memory_usage();
    result["enabled"] = config_rewind.enabled;
    result["buffer_seconds"] = config_rewind.buffer_seconds;
    result["speed"] = config_rewind.speed;

    return result;
}

json DebugAdapter::RewindSeek(int snapshot)
{
    json result;

    int count = rewind_get_snapshot_count();
    if (snapshot < 0 || snapshot >= count)
    {
        result["error"] = "Invalid snapshot index";
        return result;
    }

    bool success = gui_debug_rewind_seek(snapshot);
    result["success"] = success;
    result["snapshot"] = snapshot;
    result["total_snapshots"] = count;

    return result;
}

json DebugAdapter::ControllerButton(int player, const std::string& button, const std::string& action)
{
    json result;

    if (action != "press" && action != "release" && action != "press_and_release")
    {
        result["error"] = "Invalid action (must be: press, release, press_and_release)";
        return result;
    }

    if (player < 1 || player > 2)
    {
        result["error"] = "Invalid player number (must be 1-2)";
        return result;
    }
    GC_Controllers joypad = static_cast<GC_Controllers>(player - 1);

    std::string button_lower = button;
    std::transform(button_lower.begin(), button_lower.end(), button_lower.begin(), ::tolower);

    GC_Keys key = Key_Up; // default to a valid key, overwritten below
    bool found = false;
    if (button_lower == "up") { key = Key_Up; found = true; }
    else if (button_lower == "down") { key = Key_Down; found = true; }
    else if (button_lower == "left") { key = Key_Left; found = true; }
    else if (button_lower == "right") { key = Key_Right; found = true; }
    else if (button_lower == "1") { key = Keypad_1; found = true; }
    else if (button_lower == "2") { key = Keypad_2; found = true; }
    else if (button_lower == "3") { key = Keypad_3; found = true; }
    else if (button_lower == "4") { key = Keypad_4; found = true; }
    else if (button_lower == "5") { key = Keypad_5; found = true; }
    else if (button_lower == "6") { key = Keypad_6; found = true; }
    else if (button_lower == "7") { key = Keypad_7; found = true; }
    else if (button_lower == "8") { key = Keypad_8; found = true; }
    else if (button_lower == "9") { key = Keypad_9; found = true; }
    else if (button_lower == "0") { key = Keypad_0; found = true; }
    else if (button_lower == "asterisk" || button_lower == "*") { key = Keypad_Asterisk; found = true; }
    else if (button_lower == "hash" || button_lower == "#") { key = Keypad_Hash; found = true; }
    else if (button_lower == "left_button" || button_lower == "yellow" || button_lower == "fire1") { key = Key_Left_Button; found = true; }
    else if (button_lower == "right_button" || button_lower == "red" || button_lower == "fire2") { key = Key_Right_Button; found = true; }
    else if (button_lower == "blue") { key = Key_Blue; found = true; }
    else if (button_lower == "purple") { key = Key_Purple; found = true; }

    if (!found)
    {
        result["error"] = "Invalid button name (up, down, left, right, 0-9, asterisk, hash, left_button, right_button, yellow, red, fire1, fire2, blue, purple)";
        return result;
    }

    if (action == "press")
    {
        emu_key_pressed(joypad, key);
    }
    else if (action == "release")
    {
        emu_key_released(joypad, key);
    }
    else if (action == "press_and_release")
    {
        emu_key_pressed(joypad, key);
        result["__delayed_release"] = true;
    }

    result["success"] = true;
    result["player"] = player;
    result["button"] = button;
    result["action"] = action;

    return result;
}

json DebugAdapter::ListSprites()
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    Video* video = m_core->GetVideo();
    u8* vram = video->GetVRAM();
    u8* regs = video->GetRegisters();

    bool sprite_16x16 = (regs[1] & 0x02) != 0;
    int sprite_height = sprite_16x16 ? 16 : 8;

    json sprites = json::array();

    u16 sat_addr = (regs[5] & 0x7F) << 7;

    for (int s = 0; s < GC_MAX_SPRITES; s++)
    {
        u8 y = vram[sat_addr + s * 4];
        if (y == 0xD0) break;
        u8 x = vram[sat_addr + s * 4 + 1];
        u8 pattern = vram[sat_addr + s * 4 + 2];
        u8 color_ec = vram[sat_addr + s * 4 + 3];

        json sprite_info;
        sprite_info["index"] = s;
        sprite_info["x"] = (int)x;
        sprite_info["y"] = (int)y;
        sprite_info["pattern"] = (int)pattern;
        sprite_info["color"] = (int)(color_ec & 0x0F);
        sprite_info["early_clock"] = (color_ec & 0x80) != 0;
        sprite_info["size"] = std::string(sprite_16x16 ? "16x" : "8x") + std::to_string(sprite_height);

        sprites.push_back(sprite_info);
    }

    result["sprites"] = sprites;

    return result;
}

json DebugAdapter::GetSpriteImage(int sprite_index)
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (sprite_index < 0 || sprite_index >= GC_MAX_SPRITES)
    {
        result["error"] = "Invalid sprite index (must be 0-31)";
        return result;
    }

    unsigned char* png_buffer = NULL;
    int png_size = emu_get_sprite_png(sprite_index, &png_buffer);

    if (png_size == 0 || !png_buffer)
    {
        result["error"] = "Failed to capture sprite";
        return result;
    }

    Video* video = m_core->GetVideo();
    u8* regs = video->GetRegisters();
    bool sprite_16x16 = (regs[1] & 0x02) != 0;
    int width = sprite_16x16 ? 16 : 8;
    int height = sprite_16x16 ? 16 : 8;

    std::string base64_png = base64_encode(png_buffer, png_size);
    free(png_buffer);

    result["__mcp_image"] = true;
    result["data"] = base64_png;
    result["mimeType"] = "image/png";
    result["width"] = width;
    result["height"] = height;
    result["sprite_index"] = sprite_index;

    return result;
}

// Disassembler operations

json DebugAdapter::RunToAddress(u16 address)
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    gui_debug_runto_address(address);

    result["success"] = true;
    result["address"] = address;
    result["message"] = "Running to address";

    return result;
}

json DebugAdapter::AddDisassemblerBookmark(u16 address, const std::string& name)
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    gui_debug_add_disassembler_bookmark(address, name.c_str());

    result["success"] = true;
    result["address"] = address;
    result["name"] = name.empty() ? "auto-generated" : name;

    return result;
}

json DebugAdapter::RemoveDisassemblerBookmark(u16 address)
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    gui_debug_remove_disassembler_bookmark(address);

    result["success"] = true;
    result["address"] = address;

    return result;
}

json DebugAdapter::AddSymbol(u8 bank, u16 address, const std::string& name)
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    char symbol[128];
    snprintf(symbol, sizeof(symbol), "%02X:%04X %s", bank, address, name.c_str());
    gui_debug_add_symbol(symbol);

    result["success"] = true;
    result["bank"] = bank;
    result["address"] = address;
    result["name"] = name;

    return result;
}

json DebugAdapter::RemoveSymbol(u8 bank, u16 address)
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    gui_debug_remove_symbol(bank, address);

    result["success"] = true;
    result["bank"] = bank;
    result["address"] = address;

    return result;
}

// Memory editor operations

json DebugAdapter::SelectMemoryRange(int editor, int start_address, int end_address)
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (editor < 0 || editor >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid editor number";
        return result;
    }

    gui_debug_memory_select_range(editor, start_address, end_address);

    result["success"] = true;
    result["editor"] = editor;
    result["start_address"] = start_address;
    result["end_address"] = end_address;

    return result;
}

json DebugAdapter::SetMemorySelectionValue(int editor, u8 value)
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (editor < 0 || editor >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid editor number";
        return result;
    }

    gui_debug_memory_set_selection_value(editor, value);

    result["success"] = true;
    result["editor"] = editor;
    result["value"] = value;

    return result;
}

json DebugAdapter::AddMemoryBookmark(int editor, int address, const std::string& name)
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (editor < 0 || editor >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid editor number";
        return result;
    }

    MemoryAreaInfo info = GetMemoryAreaInfo(editor);
    int display_address = MemoryEditorOffsetToDisplayAddress(info, address);
    gui_debug_memory_add_bookmark(editor, display_address, name.c_str());

    result["success"] = true;
    result["editor"] = editor;
    result["address"] = address;
    result["name"] = name.empty() ? "auto-generated" : name;

    return result;
}

json DebugAdapter::RemoveMemoryBookmark(int editor, int address)
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (editor < 0 || editor >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid editor number";
        return result;
    }

    MemoryAreaInfo info = GetMemoryAreaInfo(editor);
    int display_address = MemoryEditorOffsetToDisplayAddress(info, address);
    gui_debug_memory_remove_bookmark(editor, display_address);

    result["success"] = true;
    result["editor"] = editor;
    result["address"] = address;

    return result;
}

json DebugAdapter::AddMemoryWatch(int editor, int address, const std::string& notes, int size)
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (editor < 0 || editor >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid editor number";
        return result;
    }

    MemoryAreaInfo info = GetMemoryAreaInfo(editor);
    int display_address = MemoryEditorOffsetToDisplayAddress(info, address);
    gui_debug_memory_add_watch(editor, display_address, notes.c_str(), size);

    result["success"] = true;
    result["editor"] = editor;
    result["address"] = address;
    result["notes"] = notes;
    result["size"] = size;

    return result;
}

json DebugAdapter::RemoveMemoryWatch(int editor, int address)
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (editor < 0 || editor >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid editor number";
        return result;
    }

    MemoryAreaInfo info = GetMemoryAreaInfo(editor);
    int display_address = MemoryEditorOffsetToDisplayAddress(info, address);
    gui_debug_memory_remove_watch(editor, display_address);

    result["success"] = true;
    result["editor"] = editor;
    result["address"] = address;

    return result;
}

json DebugAdapter::ListDisassemblerBookmarks()
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    void* bookmarks_ptr = NULL;
    int count = gui_debug_get_disassembler_bookmarks(&bookmarks_ptr);

    std::vector<DisassemblerBookmark>* bookmarks = (std::vector<DisassemblerBookmark>*)bookmarks_ptr;

    json bookmarks_array = json::array();

    if (bookmarks)
    {
        for (const DisassemblerBookmark& bookmark : *bookmarks)
        {
            json bookmark_obj;

            std::ostringstream addr_ss;
            addr_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << bookmark.address;
            bookmark_obj["address"] = addr_ss.str();
            bookmark_obj["name"] = bookmark.name;

            bookmarks_array.push_back(bookmark_obj);
        }
    }

    result["bookmarks"] = bookmarks_array;
    result["count"] = count;

    return result;
}

json DebugAdapter::ListSymbols()
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    void* symbols_ptr = NULL;
    gui_debug_get_symbols(&symbols_ptr);

    DebugSymbol*** fixed_symbols = (DebugSymbol***)symbols_ptr;

    json symbols_array = json::array();

    if (fixed_symbols)
    {
        for (int bank = 0; bank < 0x100; bank++)
        {
            if (!fixed_symbols[bank])
                continue;

            for (int address = 0; address < 0x10000; address++)
            {
                if (fixed_symbols[bank][address])
                {
                    json symbol_obj;

                    std::ostringstream bank_ss, addr_ss;
                    bank_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << bank;
                    addr_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << address;

                    symbol_obj["bank"] = bank_ss.str();
                    symbol_obj["address"] = addr_ss.str();
                    symbol_obj["name"] = fixed_symbols[bank][address]->text;

                    symbols_array.push_back(symbol_obj);
                }
            }
        }
    }

    result["symbols"] = symbols_array;
    result["count"] = symbols_array.size();

    return result;
}

json DebugAdapter::ListCallStack()
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    Memory* memory = m_core->GetMemory();
    Processor* processor = m_core->GetProcessor();
    std::stack<Processor::GC_CallStackEntry> temp_stack = *processor->GetDisassemblerCallStack();

    void* symbols_ptr = NULL;
    gui_debug_get_symbols(&symbols_ptr);
    DebugSymbol*** fixed_symbols = (DebugSymbol***)symbols_ptr;

    json stack_array = json::array();

    while (!temp_stack.empty())
    {
        Processor::GC_CallStackEntry entry = temp_stack.top();
        temp_stack.pop();

        json entry_obj;

        std::ostringstream dest_ss, src_ss, back_ss;
        dest_ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << entry.dest;
        src_ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << entry.src;
        back_ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << entry.back;

        entry_obj["function"] = dest_ss.str();
        entry_obj["source"] = src_ss.str();
        entry_obj["return"] = back_ss.str();

        GC_Disassembler_Record* record = memory->GetDisassemblerRecord(entry.dest);
        if (IsValidPointer(record) && record->name[0] != 0)
        {
            if (fixed_symbols && fixed_symbols[record->bank] && fixed_symbols[record->bank][entry.dest])
            {
                entry_obj["symbol"] = fixed_symbols[record->bank][entry.dest]->text;
            }
        }

        stack_array.push_back(entry_obj);
    }

    result["stack"] = stack_array;
    result["depth"] = stack_array.size();

    return result;
}

json DebugAdapter::ListMemoryBookmarks(int area)
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (area < 0 || area >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid area number";
        return result;
    }

    void* bookmarks_ptr = NULL;
    int count = gui_debug_memory_get_bookmarks(area, &bookmarks_ptr);
    MemoryAreaInfo info = GetMemoryAreaInfo(area);

    std::vector<MemEditor::Bookmark>* bookmarks = (std::vector<MemEditor::Bookmark>*)bookmarks_ptr;

    json bookmarks_array = json::array();

    if (bookmarks)
    {
        for (const MemEditor::Bookmark& bookmark : *bookmarks)
        {
            json bookmark_obj;
            int offset = MemoryEditorDisplayAddressToOffset(info, bookmark.address);

            std::ostringstream addr_ss;
            addr_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << offset;
            bookmark_obj["address"] = addr_ss.str();
            bookmark_obj["name"] = bookmark.name;

            bookmarks_array.push_back(bookmark_obj);
        }
    }

    result["area"] = area;
    result["bookmarks"] = bookmarks_array;
    result["count"] = count;

    return result;
}

json DebugAdapter::ListMemoryWatches(int area)
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (area < 0 || area >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid area number";
        return result;
    }

    void* watches_ptr = NULL;
    int count = gui_debug_memory_get_watches(area, &watches_ptr);
    MemoryAreaInfo info = GetMemoryAreaInfo(area);

    std::vector<MemEditor::Watch>* watches = (std::vector<MemEditor::Watch>*)watches_ptr;

    json watches_array = json::array();

    if (watches)
    {
        const char* size_names[] = {"8", "16", "24", "32"};
        const char* format_names[] = {"hex", "binary", "decimal_unsigned", "decimal_signed"};

        for (const MemEditor::Watch& watch : *watches)
        {
            json watch_obj;
            int offset = MemoryEditorDisplayAddressToOffset(info, watch.address);

            std::ostringstream addr_ss;
            addr_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << offset;
            watch_obj["address"] = addr_ss.str();
            watch_obj["notes"] = watch.notes;

            int size_idx = (watch.size >= 0 && watch.size <= 3) ? watch.size : 0;
            int fmt_idx = (watch.format >= 0 && watch.format <= 3) ? watch.format : 0;
            watch_obj["size"] = size_names[size_idx];
            watch_obj["format"] = format_names[fmt_idx];

            watches_array.push_back(watch_obj);
        }
    }

    result["area"] = area;
    result["watches"] = watches_array;
    result["count"] = count;

    return result;
}

json DebugAdapter::GetMemorySelection(int area)
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (area < 0 || area >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid area number";
        return result;
    }

    int start = -1;
    int end = -1;
    gui_debug_memory_get_selection(area, &start, &end);

    result["area"] = area;

    if (start >= 0 && end >= 0 && start <= end)
    {
        std::ostringstream start_ss, end_ss;
        start_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << start;
        end_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << end;

        result["start"] = start_ss.str();
        result["end"] = end_ss.str();
        result["size"] = end - start + 1;
    }
    else
    {
        result["start"] = NULL;
        result["end"] = NULL;
        result["size"] = 0;
        result["note"] = "No selection";
    }

    return result;
}

json DebugAdapter::MemorySearchCapture(int area)
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (area < 0 || area >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid area number";
        return result;
    }

    gui_debug_memory_search_capture(area);

    result["success"] = true;
    result["area"] = area;
    result["message"] = "Memory snapshot captured";

    return result;
}

json DebugAdapter::MemorySearch(int area, const std::string& op, const std::string& compare_type, int compare_value, const std::string& data_type)
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (area < 0 || area >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid area number";
        return result;
    }

    int op_index = 0;
    if (op == "<") op_index = 0;
    else if (op == ">") op_index = 1;
    else if (op == "==") op_index = 2;
    else if (op == "!=") op_index = 3;
    else if (op == "<=") op_index = 4;
    else if (op == ">=") op_index = 5;
    else
    {
        result["error"] = "Invalid operator";
        return result;
    }

    int compare_type_index = 0;
    if (compare_type == "previous") compare_type_index = 0;
    else if (compare_type == "value") compare_type_index = 1;
    else if (compare_type == "address") compare_type_index = 2;
    else
    {
        result["error"] = "Invalid compare_type";
        return result;
    }

    int data_type_index = 0;
    if (data_type == "hex") data_type_index = 0;
    else if (data_type == "signed") data_type_index = 1;
    else if (data_type == "unsigned") data_type_index = 2;
    else
    {
        result["error"] = "Invalid data_type";
        return result;
    }

    void* results_ptr = NULL;
    int count = gui_debug_memory_search(area, op_index, compare_type_index, compare_value, data_type_index, &results_ptr);

    result["area"] = area;
    result["count"] = count;
    result["results"] = json::array();

    if (count > 0 && results_ptr != NULL)
    {
        std::vector<MemEditor::Search>* results = (std::vector<MemEditor::Search>*)results_ptr;

        int max_results = (count > 1000) ? 1000 : count;

        for (int i = 0; i < max_results; i++)
        {
            MemEditor::Search& search = (*results)[i];
            json item;

            std::ostringstream addr_ss;
            addr_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << search.address;

            item["address"] = addr_ss.str();
            item["value"] = search.value;
            item["previous"] = search.prev_value;

            result["results"].push_back(item);
        }

        if (count > 1000)
        {
            result["note"] = "Results limited to first 1000 matches";
            result["total_matches"] = count;
        }
    }

    return result;
}

json DebugAdapter::MemoryFindBytes(int area, const std::string& hex_bytes)
{
    json result;

    if (!m_core || !m_core->GetCartridge()->IsReady())
    {
        result["error"] = "No media loaded";
        return result;
    }

    if (area < 0 || area >= MEMORY_EDITOR_MAX)
    {
        result["error"] = "Invalid area number";
        return result;
    }

    if (hex_bytes.empty())
    {
        result["error"] = "Empty hex byte string";
        return result;
    }

    int addresses[100];
    int count = gui_debug_memory_find_bytes(area, hex_bytes.c_str(), addresses, 100);

    result["area"] = area;
    result["count"] = count;
    result["results"] = json::array();

    int max_results = (count > 100) ? 100 : count;

    for (int i = 0; i < max_results; i++)
    {
        json item;
        std::ostringstream addr_ss;
        addr_ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << addresses[i];
        item["address"] = addr_ss.str();
        result["results"].push_back(item);
    }

    if (count > 100)
    {
        result["note"] = "Results limited to first 100 matches";
        result["total_matches"] = count;
    }

    return result;
}

json DebugAdapter::GetTraceLog(int start, int count)
{
    json result;

#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    TraceLogger* logger = m_core->GetTraceLogger();
    if (!logger)
    {
        result["error"] = "Trace logger not available";
        return result;
    }

    u32 total = logger->GetCount();
    result["total"] = total;
    result["enabled_flags"] = logger->GetEnabledFlags();

    if (start < 0) start = 0;
    if (count <= 0) count = 100;
    if (count > 1000) count = 1000;
    if ((u32)start >= total) start = (total > 0) ? total - 1 : 0;

    json entries = json::array();
    Memory* memory = m_core->GetMemory();

    for (int i = start; i < start + count && (u32)i < total; i++)
    {
        const GC_Trace_Entry& e = logger->GetEntry(i);
        json entry;
        entry["index"] = i;
        entry["type"] = (int)e.type;

        static const char* type_names[] = {
            "cpu", "cpu_irq", "vdp_write", "vdp_status",
            "psg", "ay8910", "io_port", "sgm"
        };
        if (e.type < TRACE_TYPE_COUNT)
            entry["type_name"] = type_names[e.type];

        switch (e.type)
        {
            case TRACE_CPU:
            {
                std::ostringstream ss;
                ss << std::hex << std::uppercase << std::setfill('0');
                ss << std::setw(4) << e.cpu.pc;
                entry["pc"] = ss.str();
                entry["bank"] = e.cpu.bank;

                GC_Disassembler_Record* record = memory->GetDisassemblerRecord(e.cpu.pc);
                if (IsValidPointer(record) && record->name[0] != 0)
                    entry["disasm"] = record->name;

                break;
            }
            case TRACE_CPU_IRQ:
            {
                std::ostringstream ss;
                ss << std::hex << std::uppercase << std::setfill('0');
                ss << std::setw(4) << e.irq.pc;
                entry["pc"] = ss.str();
                ss.str(""); ss << std::setw(4) << e.irq.vector;
                entry["vector"] = ss.str();
                entry["irq_type"] = (int)e.irq.type;
                break;
            }
            case TRACE_VDP_WRITE:
                entry["reg"] = e.vdp_write.reg;
                entry["value"] = e.vdp_write.value;
                break;
            case TRACE_VDP_STATUS:
                entry["event"] = e.vdp_status.event;
                entry["line"] = e.vdp_status.line;
                break;
            case TRACE_PSG:
                entry["value"] = e.psg.value;
                break;
            case TRACE_AY8910:
                entry["reg"] = e.ay8910.reg;
                entry["value"] = e.ay8910.value;
                entry["is_write"] = e.ay8910.is_write;
                break;
            case TRACE_IO_PORT:
                entry["port"] = e.io_port.port;
                entry["value"] = e.io_port.value;
                entry["is_write"] = e.io_port.is_write;
                break;
            case TRACE_SGM:
                entry["port"] = e.sgm.port;
                entry["value"] = e.sgm.value;
                break;
            default:
                break;
        }

        entries.push_back(entry);
    }

    result["entries"] = entries;
#else
    UNUSED(start);
    UNUSED(count);
    result["error"] = "Trace logging not available in this build";
#endif

    return result;
}

json DebugAdapter::SetTraceLog(bool enabled, u32 flags)
{
    json result;

#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    TraceLogger* logger = m_core->GetTraceLogger();
    if (!logger)
    {
        result["error"] = "Trace logger not available";
        return result;
    }

    logger->SetEnabledFlags(enabled ? flags : 0);
    result["success"] = true;
    result["enabled"] = enabled;
    result["flags"] = logger->GetEnabledFlags();
#else
    UNUSED(enabled);
    UNUSED(flags);
    result["error"] = "Trace logging not available in this build";
#endif

    return result;
}
