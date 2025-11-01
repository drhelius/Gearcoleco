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

#include <math.h>
#include "imgui/imgui.h"
#include "nfd/nfd.h"
#include "nfd/nfd_sdl2.h"
#include "config.h"
#include "emu.h"
#include "renderer.h"
#include "../../src/gearcoleco.h"
#include "gui.h"
#include "gui_debug_constants.h"
#include "gui_debug_memory.h"
#include "application.h"
#include "utils.h"

#define GUI_DEBUG_IMPORT
#include "gui_debug.h"

struct DebugSymbol
{
    int bank;
    u16 address;
    std::string text;
};

struct DisassmeblerLine
{
    bool is_symbol;
    bool is_breakpoint;
    Memory::stDisassembleRecord* record;
    std::string symbol;
};

static std::vector<DebugSymbol> symbols;
static Memory::stDisassembleRecord* selected_record = NULL;
static char brk_address_cpu[5] = "";
static char brk_address_mem[10] = "";
static bool brk_new_mem_read = true;
static bool brk_new_mem_write = true;
static char goto_address[5] = "";
static bool goto_address_requested = false;
static u16 goto_address_target = 0;
static bool goto_back_requested = false;
static int goto_back = 0;

static void debug_window_processor(void);
static void debug_window_disassembler(void);
static void debug_window_vram_registers(void);
static void debug_window_vram(void);
static void debug_window_vram_background(void);
static void debug_window_vram_tiles(void);
static void debug_window_vram_sprites(void);
static void debug_window_vram_regs(void);
static void add_symbol(const char* line);
static void add_breakpoint_cpu(void);
static void add_breakpoint_mem(void);
static void request_goto_address(u16 addr);
static bool is_return_instruction(u8 opcode1, u8 opcode2);

void gui_debug_windows(void)
{
    if (config_debug.debug)
    {
        if (config_debug.show_processor)
            debug_window_processor();
        if (config_debug.show_memory)
            gui_debug_window_memory();
        if (config_debug.show_disassembler)
            debug_window_disassembler();
        if (config_debug.show_video)
            debug_window_vram();
        if (config_debug.show_video_registers)
            debug_window_vram_registers();

        gui_debug_memory_watches_window();
        gui_debug_memory_search_window();
    }
}

void gui_debug_reset(void)
{
    gui_debug_reset_breakpoints_cpu();
    gui_debug_reset_breakpoints_mem();
    gui_debug_reset_symbols();
    gui_debug_memory_reset();
    selected_record = NULL;
}

void gui_debug_reset_symbols(void)
{
    symbols.clear();
    
    for (int i = 0; i < gui_debug_symbols_count; i++)
        add_symbol(gui_debug_symbols[i]);
}

void gui_debug_load_symbols_file(const char* path)
{
    Log("Loading symbol file %s", path);

    std::ifstream file;
    open_ifstream_utf8(file, path, std::ios::in);

    if (file.is_open())
    {
        std::string line;
        bool valid_section = true;

        while (std::getline(file, line))
        {
            size_t comment = line.find_first_of(';');
            if (comment != std::string::npos)
                line = line.substr(0, comment);
            line = line.erase(0, line.find_first_not_of(" \t\r\n"));
            line = line.erase(line.find_last_not_of(" \t\r\n") + 1);

            if (line.empty())
                continue;

            if (line.find("[") != std::string::npos)
            {
                valid_section = (line.find("[labels]") != std::string::npos);
                continue;
            }

            if (valid_section)
                add_symbol(line.c_str());
        }

        file.close();
    }
}

void gui_debug_toggle_breakpoint(void)
{
    if (IsValidPointer(selected_record))
    {
        bool found = false;
        std::vector<Memory::stDisassembleRecord*>* breakpoints = emu_get_core()->GetMemory()->GetBreakpointsCPU();

        for (long unsigned int b = 0; b < breakpoints->size(); b++)
        {
            if ((*breakpoints)[b] == selected_record)
            {
                found = true;
                 InitPointer((*breakpoints)[b]);
                break;
            }
        }

        if (!found)
        {
            breakpoints->push_back(selected_record);
        }
    }
}

void gui_debug_runtocursor(void)
{
    if (IsValidPointer(selected_record))
    {
        emu_get_core()->GetMemory()->SetRunToBreakpoint(selected_record);
        emu_debug_continue();
    }
}

void gui_debug_reset_breakpoints_cpu(void)
{
    emu_get_core()->GetMemory()->GetBreakpointsCPU()->clear();
    brk_address_cpu[0] = 0;
}

void gui_debug_reset_breakpoints_mem(void)
{
    emu_get_core()->GetMemory()->GetBreakpointsMem()->clear();
    brk_address_mem[0] = 0;
}

void gui_debug_go_back(void)
{
    goto_back_requested = true;
}

static void debug_window_disassembler(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(159, 31), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(401, 641), ImGuiCond_FirstUseEver);

    ImGui::Begin("Disassembler", &config_debug.show_disassembler);

    GearcolecoCore* core = emu_get_core();
    Processor* processor = core->GetProcessor();
    Processor::ProcessorState* proc_state = processor->GetState();
    Memory* memory = core->GetMemory();
    std::vector<Memory::stDisassembleRecord*>* breakpoints_cpu = memory->GetBreakpointsCPU();
    std::vector<Memory::stMemoryBreakpoint>* breakpoints_mem = memory->GetBreakpointsMem();

    int pc = proc_state->PC->GetValue();

    if (ImGui::Button("Step Over"))
        emu_debug_step();
    ImGui::SameLine();
    if (ImGui::Button("Step Frame"))
    {
        emu_debug_next_frame();
        gui_debug_memory_step_frame();
    }
    ImGui::SameLine();
    if (ImGui::Button("Continue"))
        emu_debug_continue(); 
    ImGui::SameLine();
    if (ImGui::Button("Run To Cursor"))
        gui_debug_runtocursor();

    static bool follow_pc = true;
    static bool show_mem = true;
    static bool show_symbols = true;
    static bool show_segment = true;

    ImGui::Checkbox("Follow PC", &follow_pc); ImGui::SameLine();
    ImGui::Checkbox("Opcodes", &show_mem);  ImGui::SameLine();
    ImGui::Checkbox("Symbols", &show_symbols);  ImGui::SameLine();
    ImGui::Checkbox("Segments", &show_segment);

    ImGui::Separator();

    ImGui::Text("Go To Address: ");
    ImGui::SameLine();
    ImGui::PushItemWidth(45);
    if (ImGui::InputTextWithHint("##goto_address", "XXXX", goto_address, IM_ARRAYSIZE(goto_address), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
    {
        try
        {
            request_goto_address((u16)std::stoul(goto_address, 0, 16));
            follow_pc = false;
        }
        catch(const std::invalid_argument&)
        {
        }
        goto_address[0] = 0;
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Go", ImVec2(30, 0)))
    {
        try
        {
            request_goto_address((u16)std::stoul(goto_address, 0, 16));
            follow_pc = false;
        }
        catch(const std::invalid_argument&)
        {
        }
        goto_address[0] = 0;
    }

    ImGui::SameLine();
    if (ImGui::Button("Back", ImVec2(50, 0)))
    {
        goto_back_requested = true;
        follow_pc = false;
    }

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Processor Breakpoints"))
    {
        ImGui::Checkbox("Disable All##disable_all_cpu", &emu_debug_disable_breakpoints_cpu);

        ImGui::Columns(2, "breakpoints_cpu");
        ImGui::SetColumnOffset(1, 85);

        ImGui::Separator();

        if (IsValidPointer(selected_record))
            snprintf(brk_address_cpu, sizeof(brk_address_cpu), "%04X", selected_record->address);

        ImGui::PushItemWidth(70);
        if (ImGui::InputTextWithHint("##add_breakpoint_cpu", "XXXX", brk_address_cpu, IM_ARRAYSIZE(brk_address_cpu), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
        {
            add_breakpoint_cpu();
        }
        ImGui::PopItemWidth();


        if (ImGui::Button("Add##add_cpu", ImVec2(70, 0)))
        {
            add_breakpoint_cpu();
        }

        if (ImGui::Button("Clear All##clear_all_cpu", ImVec2(70, 0)))
        {
            gui_debug_reset_breakpoints_cpu();
        }

        ImGui::NextColumn();

        ImGui::BeginChild("breakpoints_cpu", ImVec2(0, 80), false);

        int remove = -1;

        for (long unsigned int b = 0; b < breakpoints_cpu->size(); b++)
        {
            if (!IsValidPointer((*breakpoints_cpu)[b]))
                continue;

            ImGui::PushID(b);
            if (ImGui::SmallButton("X"))
            {
               remove = b;
               ImGui::PopID();
               continue;
            }

            ImGui::PopID();

            ImGui::PushFont(gui_default_font);
            ImGui::SameLine();
            ImGui::TextColored(red, "%04X", (*breakpoints_cpu)[b]->address);
            ImGui::SameLine();
            ImGui::TextColored(gray, "%s", (*breakpoints_cpu)[b]->name);
            ImGui::PopFont();
        }

        if (remove >= 0)
        {
            breakpoints_cpu->erase(breakpoints_cpu->begin() + remove);
        }

        ImGui::EndChild();
        ImGui::Columns(1);
        ImGui::Separator();

    }

    if (ImGui::CollapsingHeader("Memory Breakpoints"))
    {
        ImGui::Checkbox("Disable All##diable_all_mem", &emu_debug_disable_breakpoints_mem);

        ImGui::Columns(2, "breakpoints_mem");
        ImGui::SetColumnOffset(1, 100);

        ImGui::Separator();

        ImGui::PushItemWidth(85);
        if (ImGui::InputTextWithHint("##add_breakpoint_mem", "XXXX-XXXX", brk_address_mem, IM_ARRAYSIZE(brk_address_mem), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
        {
            add_breakpoint_mem();
        }
        ImGui::PopItemWidth();

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Use XXXX format for single addresses or XXXX-XXXX for address ranges");

        ImGui::Checkbox("Read", &brk_new_mem_read);
        ImGui::Checkbox("Write", &brk_new_mem_write);

        if (ImGui::Button("Add##add_mem", ImVec2(85, 0)))
        {
            add_breakpoint_mem();
        }

        if (ImGui::Button("Clear All##clear_all_mem", ImVec2(85, 0)))
        {
            gui_debug_reset_breakpoints_mem();
        }

        ImGui::NextColumn();

        ImGui::BeginChild("breakpoints_mem", ImVec2(0, 130), false);

        int remove = -1;

        for (long unsigned int b = 0; b < breakpoints_mem->size(); b++)
        {
            ImGui::PushID(10000 + b);
            if (ImGui::SmallButton("X"))
            {
               remove = b;
               ImGui::PopID();
               continue;
            }

            ImGui::PopID();

            ImGui::PushFont(gui_default_font);
            ImGui::SameLine();
            if ((*breakpoints_mem)[b].range)
                ImGui::TextColored(red, "%04X-%04X", (*breakpoints_mem)[b].address1, (*breakpoints_mem)[b].address2);
            else
                ImGui::TextColored(red, "%04X", (*breakpoints_mem)[b].address1);
            if ((*breakpoints_mem)[b].read)
            {
                ImGui::SameLine(); ImGui::TextColored(gray, "R");
            }
            if ((*breakpoints_mem)[b].write)
            {
                ImGui::SameLine(); ImGui::TextColored(gray, "W");
            }
            ImGui::PopFont();
        }

        if (remove >= 0)
        {
            breakpoints_mem->erase(breakpoints_mem->begin() + remove);
        }

        ImGui::EndChild();
        ImGui::Columns(1);
        ImGui::Separator();
    }

    ImGui::PushFont(gui_default_font);

    bool window_visible = ImGui::BeginChild("##dis", ImVec2(ImGui::GetContentRegionAvail().x, 0), true, 0);
    
    if (window_visible)
    {
        int dis_size = 0;
        int pc_pos = 0;
        int goto_address_pos = 0;
        
        std::vector<DisassmeblerLine> vec(0x10000);
        
        for (int i = 0; i < 0x10000; i++)
        {
            Memory::stDisassembleRecord* record = memory->GetDisassembleRecord(i, false);

            if (IsValidPointer(record) && (record->name[0] != 0))
            {
                for (long unsigned int s = 0; s < symbols.size(); s++)
                {
                    if ((symbols[s].bank == record->bank) && (symbols[s].address == i) && show_symbols)
                    {
                        vec[dis_size].is_symbol = true;
                        vec[dis_size].symbol = symbols[s].text;
                        dis_size ++;
                    }
                }

                vec[dis_size].is_symbol = false;
                vec[dis_size].record = record;

                if (vec[dis_size].record->address == pc)
                    pc_pos = dis_size;

                if (goto_address_requested && (vec[dis_size].record->address <= goto_address_target))
                    goto_address_pos = dis_size;

                vec[dis_size].is_breakpoint = false;

                for (long unsigned int b = 0; b < breakpoints_cpu->size(); b++)
                {
                    if ((*breakpoints_cpu)[b] == vec[dis_size].record)
                    {
                        vec[dis_size].is_breakpoint = true;
                        break;
                    }
                }

                dis_size++;
            }
        }

        if (follow_pc)
        {
            float window_offset = ImGui::GetWindowHeight() / 2.0f;
            float offset = window_offset - (ImGui::GetTextLineHeightWithSpacing() - 2.0f);
            ImGui::SetScrollY((pc_pos * ImGui::GetTextLineHeightWithSpacing()) - offset);
        }

        if (goto_address_requested)
        {
            goto_address_requested = false;
            goto_back = (int)ImGui::GetScrollY();
            ImGui::SetScrollY((goto_address_pos * ImGui::GetTextLineHeightWithSpacing()) + 2);
        }

        if (goto_back_requested)
        {
            goto_back_requested = false;
            ImGui::SetScrollY((float)goto_back);
        }

        ImGuiListClipper clipper;
        clipper.Begin(dis_size, ImGui::GetTextLineHeightWithSpacing());

        while (clipper.Step())
        {
            for (int item = clipper.DisplayStart; item < clipper.DisplayEnd; item++)
            {
                if (vec[item].is_symbol)
                {
                    ImGui::TextColored(green, "%s:", vec[item].symbol.c_str());
                    continue;
                }

                ImGui::PushID(item);

                bool is_selected = (selected_record == vec[item].record);

                if (ImGui::Selectable("", is_selected, ImGuiSelectableFlags_AllowDoubleClick))
                {
                    if (ImGui::IsMouseDoubleClicked(0) && vec[item].record->jump)
                    {
                        follow_pc = false;
                        request_goto_address(vec[item].record->jump_address);
                    }
                    else if (is_selected)
                    {
                        InitPointer(selected_record);
                        brk_address_cpu[0] = 0;
                    }
                    else
                        selected_record = vec[item].record;
                }

                if (is_selected)
                    ImGui::SetItemDefaultFocus();

                ImVec4 color_segment = vec[item].is_breakpoint ? red : magenta;
                ImVec4 color_addr = vec[item].is_breakpoint ? red : cyan;
                ImVec4 color_opcodes = vec[item].is_breakpoint ? red : gray;
                ImVec4 color_name = vec[item].is_breakpoint ? red : white;

                if (show_segment)
                {
                    ImGui::SameLine();
                    ImGui::TextColored(color_segment, "%s", vec[item].record->segment);
                }

                ImGui::SameLine();
                if (strcmp(vec[item].record->segment, "ROM") == 0)
                    ImGui::TextColored(color_addr, "%02X:%04X ", vec[item].record->bank, vec[item].record->address);
                else
                    ImGui::TextColored(color_addr, "  %04X ", vec[item].record->address);


                if (show_mem)
                {
                    ImGui::SameLine();
                    ImGui::TextColored(color_opcodes, "%s ", vec[item].record->bytes);
                }

                ImGui::SameLine();
                if (vec[item].record->address == pc)
                {
                    ImGui::TextColored(yellow, "->");
                    color_name = yellow;
                }
                else
                {
                    ImGui::TextColored(yellow, "  ");
                }

                ImGui::SameLine();
                ImGui::TextColored(color_name, "%s", vec[item].record->name);

                bool is_ret = is_return_instruction(vec[item].record->opcodes[0], vec[item].record->opcodes[1]);
                if (is_ret)
                {
                    ImGui::PushStyleColor(ImGuiCol_Separator, (vec[item].record->opcodes[0] == 0xC9) ? gray : dark_gray);
                    ImGui::Separator();
                    ImGui::PopStyleColor();
                }

                ImGui::PopID();
            }
        }
    }

    ImGui::EndChild();
    
    ImGui::PopFont();

    ImGui::End();

    ImGui::PopStyleVar();
}

static void debug_window_processor(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(6, 31), ImGuiCond_FirstUseEver);

    ImGui::Begin("Z80 Status", &config_debug.show_processor, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);

    ImGui::PushFont(gui_default_font);

    GearcolecoCore* core = emu_get_core();
    Processor* processor = core->GetProcessor();
    Processor::ProcessorState* proc_state = processor->GetState();

    ImGui::Separator();

    ImGui::TextColored(light_brown, "  S Z Y H X P N C");
    ImGui::Text("  " BYTE_TO_BINARY_PATTERN_ALL_SPACED, BYTE_TO_BINARY(proc_state->AF->GetLow()));

    ImGui::Columns(2, "registers");
    ImGui::Separator();
    ImGui::TextColored(cyan, " A"); ImGui::SameLine();
    ImGui::Text(" $%02X", proc_state->AF->GetHigh());
    char buffer[20];
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->AF->GetHigh()));
    ImGui::Text("%s", buffer);

    ImGui::NextColumn();
    ImGui::TextColored(cyan, " F"); ImGui::SameLine();
    ImGui::Text(" $%02X", proc_state->AF->GetLow());
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->AF->GetLow()));
    ImGui::Text("%s", buffer);

    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::TextColored(violet, " A'"); ImGui::SameLine();
    ImGui::Text("$%02X", proc_state->AF2->GetHigh());
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->AF2->GetHigh()));
    ImGui::Text("%s", buffer);

    ImGui::NextColumn();
    ImGui::TextColored(violet, " F'"); ImGui::SameLine();
    ImGui::Text("$%02X", proc_state->AF2->GetLow());
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->AF2->GetLow()));
    ImGui::Text("%s", buffer);

    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::TextColored(cyan, " B"); ImGui::SameLine();
    ImGui::Text(" $%02X", proc_state->BC->GetHigh());
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->BC->GetHigh()));
    ImGui::Text("%s", buffer);

    ImGui::NextColumn();
    ImGui::TextColored(cyan, " C"); ImGui::SameLine();
    ImGui::Text(" $%02X", proc_state->BC->GetLow());
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->BC->GetLow()));
    ImGui::Text("%s", buffer);

    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::TextColored(violet, " B'"); ImGui::SameLine();
    ImGui::Text("$%02X", proc_state->BC2->GetHigh());
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->BC2->GetHigh()));
    ImGui::Text("%s", buffer);

    ImGui::NextColumn();
    ImGui::TextColored(violet, " C'"); ImGui::SameLine();
    ImGui::Text("$%02X", proc_state->BC2->GetLow());
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->BC2->GetLow()));
    ImGui::Text("%s", buffer);

    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::TextColored(cyan, " D"); ImGui::SameLine();
    ImGui::Text(" $%02X", proc_state->DE->GetHigh());
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->DE->GetHigh()));
    ImGui::Text("%s", buffer);

    ImGui::NextColumn();
    ImGui::TextColored(cyan, " E"); ImGui::SameLine();
    ImGui::Text(" $%02X", proc_state->DE->GetLow());
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->DE->GetLow()));
    ImGui::Text("%s", buffer);

    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::TextColored(violet, " D'"); ImGui::SameLine();
    ImGui::Text("$%02X", proc_state->DE2->GetHigh());
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->DE2->GetHigh()));
    ImGui::Text("%s", buffer);

    ImGui::NextColumn();
    ImGui::TextColored(violet, " E'"); ImGui::SameLine();
    ImGui::Text("$%02X", proc_state->DE2->GetLow());
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->DE2->GetLow()));
    ImGui::Text("%s", buffer);

    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::TextColored(cyan, " H"); ImGui::SameLine();
    ImGui::Text(" $%02X", proc_state->HL->GetHigh());
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->HL->GetHigh()));
    ImGui::Text("%s", buffer);

    ImGui::NextColumn();
    ImGui::TextColored(cyan, " L"); ImGui::SameLine();
    ImGui::Text(" $%02X", proc_state->HL->GetLow());
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->HL->GetLow()));
    ImGui::Text("%s", buffer);

    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::TextColored(violet, " H'"); ImGui::SameLine();
    ImGui::Text("$%02X", proc_state->HL2->GetHigh());
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->HL2->GetHigh()));
    ImGui::Text("%s", buffer);

    ImGui::NextColumn();
    ImGui::TextColored(violet, " L'"); ImGui::SameLine();
    ImGui::Text("$%02X", proc_state->HL2->GetLow());
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->HL2->GetLow()));
    ImGui::Text("%s", buffer);

    ImGui::NextColumn();
    ImGui::Separator();
    ImGui::TextColored(cyan, " I"); ImGui::SameLine();
    ImGui::Text(" $%02X", *proc_state->I);
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(*proc_state->I));
    ImGui::Text("%s", buffer);

    ImGui::NextColumn();
    ImGui::TextColored(cyan, " R"); ImGui::SameLine();
    ImGui::Text(" $%02X", *proc_state->R);
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(*proc_state->R));
    ImGui::Text("%s", buffer);

    ImGui::NextColumn();
    ImGui::Columns(1);

    ImGui::Separator();
    ImGui::TextColored(yellow, "    IX"); ImGui::SameLine();
    ImGui::Text("= $%04X", proc_state->IX->GetValue());
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->IX->GetHigh()), BYTE_TO_BINARY(proc_state->IX->GetLow()));
    ImGui::Text("%s", buffer);

    ImGui::Separator();
    ImGui::TextColored(yellow, "    IY"); ImGui::SameLine();
    ImGui::Text("= $%04X", proc_state->IY->GetValue());
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->IY->GetHigh()), BYTE_TO_BINARY(proc_state->IY->GetLow()));
    ImGui::Text("%s", buffer);

    ImGui::Separator();
    ImGui::TextColored(yellow, "    WZ"); ImGui::SameLine();
    ImGui::Text("= $%04X", proc_state->WZ->GetValue());
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->WZ->GetHigh()), BYTE_TO_BINARY(proc_state->WZ->GetLow()));
    ImGui::Text("%s", buffer);

    ImGui::Separator();
    ImGui::TextColored(yellow, "    SP"); ImGui::SameLine();
    ImGui::Text("= $%04X", proc_state->SP->GetValue());
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->SP->GetHigh()), BYTE_TO_BINARY(proc_state->SP->GetLow()));
    ImGui::Text("%s", buffer);

    ImGui::Separator();
    ImGui::TextColored(yellow, "    PC"); ImGui::SameLine();
    ImGui::Text("= $%04X", proc_state->PC->GetValue());
    snprintf(buffer, sizeof(buffer), BYTE_TO_BINARY_PATTERN_SPACED " " BYTE_TO_BINARY_PATTERN_SPACED, BYTE_TO_BINARY(proc_state->PC->GetHigh()), BYTE_TO_BINARY(proc_state->PC->GetLow()));
    ImGui::Text("%s", buffer);

    ImGui::Separator();

    ImGui::TextColored(*proc_state->IFF1 ? green : gray, " IFF1"); ImGui::SameLine();
    ImGui::TextColored(*proc_state->IFF2 ? green : gray, " IFF2"); ImGui::SameLine();
    ImGui::TextColored(*proc_state->Halt ? green : gray, " HALT");
    
    ImGui::TextColored(*proc_state->INT ? green : gray, "    INT"); ImGui::SameLine();
    ImGui::TextColored(*proc_state->NMI ? green : gray, "  NMI");

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

static void debug_window_vram_registers(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(567, 560), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(260, 329), ImGuiCond_FirstUseEver);

    ImGui::Begin("VDP Registers", &config_debug.show_video_registers);

    debug_window_vram_regs();

    ImGui::End();
    ImGui::PopStyleVar();
}

static void debug_window_vram(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(896, 31), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(670, 648), ImGuiCond_FirstUseEver);

    ImGui::Begin("VDP Viewer", &config_debug.show_video);

    ImGui::PushFont(gui_default_font);

    Video* video = emu_get_core()->GetVideo();
    u8* regs = video->GetRegisters();

    ImGui::TextColored(light_brown, "  VIDEO MODE:");ImGui::SameLine();
    ImGui::Text("$%02X", video->GetMode()); ImGui::SameLine();

    ImGui::TextColored(cyan, "  M1:");ImGui::SameLine();
    ImGui::Text("%d", (regs[1] >> 4) & 0x01); ImGui::SameLine();
    ImGui::TextColored(cyan, "  M2:");ImGui::SameLine();
    ImGui::Text("%d", (regs[0] >> 1) & 0x01); ImGui::SameLine();
    ImGui::TextColored(cyan, "  M3:");ImGui::SameLine();
    ImGui::Text("%d", (regs[1] >> 3) & 0x01);

    ImGui::PopFont();

    if (ImGui::BeginTabBar("##vram_tabs", ImGuiTabBarFlags_None))
    {
        if (ImGui::BeginTabItem("Name Table"))
        {
            debug_window_vram_background();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Pattern Table"))
        {
            debug_window_vram_tiles();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Sprites"))
        {
            debug_window_vram_sprites();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

static void debug_window_vram_background(void)
{
    Video* video = emu_get_core()->GetVideo();
    GC_RuntimeInfo runtime;
    emu_get_runtime(runtime);
    u8* regs = video->GetRegisters();
    u8* vram = video->GetVRAM();
    int mode = video->GetMode();

    static bool show_grid = true;
    int cols = (mode == 1) ? 40 : 32;
    int rows = 24;
    float scale = 2.0f;
    float tile_width = (mode == 1) ? 6.0f : 8.0f;
    float size_h = ((mode == 1) ? (6.0f * 40.0f) : 256.0f) * scale;
    float size_v = 8.0f * rows * scale;
    float spacing_h = ((mode == 1) ? 6.0f : 8.0f) * scale;
    float spacing_v = 8.0f * scale;
    float uv_h = (mode == 1) ? 0.0234375f : 1.0f / 32.0f;
    float uv_v = 1.0f / 32.0f;

    ImGui::Checkbox("Show Grid##grid_bg", &show_grid);

    ImGui::NewLine();

    ImGui::PushFont(gui_default_font);

    ImGui::Columns(2, "bg", false);
    ImGui::SetColumnOffset(1, size_h + 10.0f);

    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();

    ImGui::Image((ImTextureID)(intptr_t)renderer_emu_debug_vram_background, ImVec2(size_h, size_v), ImVec2(0.0f, 0.0f), ImVec2(uv_h * cols, uv_v * rows));

    if (show_grid)
    {
        float x = p.x;
        for (int n = 0; n <= cols; n++)
        {
            draw_list->AddLine(ImVec2(x, p.y), ImVec2(x, p.y + size_v), ImColor(dark_gray), 1.0f);
            x += spacing_h;
        }

        float y = p.y;  
        for (int n = 0; n <= rows; n++)
        {
            draw_list->AddLine(ImVec2(p.x, y), ImVec2(p.x + size_h, y), ImColor(dark_gray), 1.0f);
            y += spacing_v;
        }
    }

    int name_table_addr = regs[2] << 10;
    int color_table_addr = regs[3] << 6;
    if (mode == 2)
        color_table_addr &= 0x2000;

    ImGui::NewLine();
    ImGui::TextColored(cyan, "Name Table Addr:"); ImGui::SameLine();
    ImGui::Text("$%04X", name_table_addr);

    float mouse_x = io.MousePos.x - p.x;
    float mouse_y = io.MousePos.y - p.y;

    int tile_x = -1;
    int tile_y = -1;
    if (ImGui::IsWindowHovered() && (mouse_x >= 0.0f) && (mouse_x < size_h) && (mouse_y >= 0.0f) && (mouse_y < size_v))
    {
        tile_x = (int)(mouse_x / spacing_h);
        tile_y = (int)(mouse_y / spacing_v);

        draw_list->AddRect(ImVec2(p.x + (tile_x * spacing_h), p.y + (tile_y * spacing_v)), ImVec2(p.x + ((tile_x + 1) * spacing_h), p.y + ((tile_y + 1) * spacing_v)), ImColor(cyan), 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);

        ImGui::NextColumn();

        ImGui::Image((ImTextureID)(intptr_t)renderer_emu_debug_vram_background, ImVec2(tile_width * 16.0f, 128.0f), ImVec2(uv_h * tile_x, uv_v * tile_y), ImVec2(uv_h * (tile_x + 1), uv_v * (tile_y + 1)));

        ImGui::TextColored(light_brown, "INFO:");

        ImGui::TextColored(cyan, " X:"); ImGui::SameLine();
        ImGui::Text("$%02X", tile_x);
        ImGui::TextColored(cyan, " Y:"); ImGui::SameLine();
        ImGui::Text("$%02X", tile_y);

        int pattern_table_addr = regs[4] << 11;
        int region = (tile_y & 0x18) << 5;

        int tile_number = (tile_y * cols) + tile_x;
        int name_tile_addr = (name_table_addr + tile_number) & 0x3FFF;
        int name_tile = vram[name_tile_addr];

        if (mode == 2)
        {
            pattern_table_addr &= 0x2000;
            name_tile += region;
        }
        else if(mode == 4)
        {
            pattern_table_addr &= 0x2000;
        }

        int tile_addr = (pattern_table_addr + (name_tile << 3)) & 0x3FFF;

        int color_mask = ((regs[3] & 0x7F) << 3) | 0x07;

        int color_tile_addr = 0;

        if (mode == 2)
            color_tile_addr = color_table_addr + ((name_tile & color_mask) << 3);
        else if (mode == 0)
            color_tile_addr = color_table_addr + (name_tile >> 3);

        ImGui::TextColored(cyan, " Name Addr:"); ImGui::SameLine();
        ImGui::Text(" $%04X", name_tile_addr);
        ImGui::TextColored(cyan, " Tile Number:"); ImGui::SameLine();
        ImGui::Text("$%03X", name_tile);
        ImGui::TextColored(cyan, " Tile Addr:"); ImGui::SameLine();
        ImGui::Text(" $%04X", tile_addr);
        ImGui::TextColored(cyan, " Color Addr:"); ImGui::SameLine();
        ImGui::Text("$%04X", color_tile_addr);

        if (ImGui::IsMouseClicked(0))
        {
            gui_debug_memory_goto(MEMORY_EDITOR_VRAM, name_tile_addr);
        }
    }

    ImGui::Columns(1);

    ImGui::PopFont();
}

static void debug_window_vram_tiles(void)
{
    Video* video = emu_get_core()->GetVideo();
    u8* regs = video->GetRegisters();
    int mode = video->GetMode();

    static bool show_grid = true;
    static bool show_color = true;

    bool split_mode2 = (mode == 2);
    int sections = split_mode2 ? 3 : 1;
    int lines_per_section = split_mode2 ? 8 : 32;

    float scale = 2.0f;
    float width = 8.0f * 32.0f * scale;
    float section_height = 8.0f * lines_per_section * scale;
    float spacing = 8.0f * scale;
    float section_margin = 10.0f;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();

    ImGui::Checkbox("Show Grid##grid_tiles", &show_grid);
    ImGui::SameLine();
    ImGui::Checkbox("Show Color##color_tiles", &show_color);

    ImGui::NewLine();

    emu_debug_tile_color_mode = show_color;

    ImGui::PushFont(gui_default_font);

    ImGui::Columns(2, "tiles", false);
    ImGui::SetColumnOffset(1, width + 10.0f);

    int pattern_table_addr = (regs[4] & (mode == 2 ? 0x04 : 0x07)) << 11;

    int hovered_tile_x = -1;
    int hovered_tile_y = -1;
    int hovered_section = -1;

    for (int section = 0; section < sections; section++)
    {
        ImVec2 p = ImGui::GetCursorScreenPos();

        float tex_y_start = (float)(section * lines_per_section) / 32.0f;
        float tex_y_end = (float)((section + 1) * lines_per_section) / 32.0f;

        ImGui::Image((ImTextureID)(intptr_t)renderer_emu_debug_vram_tiles, 
                    ImVec2(width, section_height), 
                    ImVec2(0.0f, tex_y_start), 
                    ImVec2(1.0f, tex_y_end));

        if (show_grid)
        {
            float x = p.x;
            for (int n = 0; n <= 32; n++)
            {
                draw_list->AddLine(ImVec2(x, p.y), ImVec2(x, p.y + section_height), ImColor(dark_gray), 1.0f);
                x += spacing;
            }

            float y = p.y;
            for (int n = 0; n <= lines_per_section; n++)
            {
                draw_list->AddLine(ImVec2(p.x, y), ImVec2(p.x + width, y), ImColor(dark_gray), 1.0f);
                y += spacing;
            }
        }

        float mouse_x = io.MousePos.x - p.x;
        float mouse_y = io.MousePos.y - p.y;

        if (ImGui::IsWindowHovered() && (mouse_x >= 0.0f) && (mouse_x < width) && 
            (mouse_y >= 0.0f) && (mouse_y < section_height))
        {
            int tile_x = (int)(mouse_x / spacing);
            int tile_y = (int)(mouse_y / spacing);

            draw_list->AddRect(ImVec2(p.x + (tile_x * spacing), p.y + (tile_y * spacing)), 
                             ImVec2(p.x + ((tile_x + 1) * spacing), p.y + ((tile_y + 1) * spacing)), 
                             ImColor(cyan), 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);

            hovered_tile_x = tile_x;
            hovered_tile_y = tile_y + (section * lines_per_section);
            hovered_section = section;
        }

        if (section < sections - 1)
        {
            ImGui::Dummy(ImVec2(0.0f, section_margin));
        }
    }

    if (hovered_tile_x >= 0)
    {
        ImGui::NextColumn();

        float tex_x = (1.0f / 32.0f) * hovered_tile_x;
        float tex_y = (1.0f / 32.0f) * hovered_tile_y;

        ImGui::Image((ImTextureID)(intptr_t)renderer_emu_debug_vram_tiles, 
                    ImVec2(128.0f, 128.0f), 
                    ImVec2(tex_x, tex_y), 
                    ImVec2(tex_x + (1.0f / 32.0f), tex_y + (1.0f / 32.0f)));

        ImGui::TextColored(light_brown, "DETAILS:");

        int tile = (hovered_tile_y << 5) + hovered_tile_x;

        int tile_addr;
        if (split_mode2)
        {
            int section_base = pattern_table_addr + (hovered_section * 0x800);
            int tile_in_section = ((hovered_tile_y % lines_per_section) << 5) + hovered_tile_x;
            tile_addr = (section_base + (tile_in_section << 3)) & 0x3FFF;

            ImGui::TextColored(cyan, " Table Addr:"); ImGui::SameLine();
            ImGui::Text("$%04X", section_base);
        }
        else
        {
            tile_addr = (pattern_table_addr + (tile << 3)) & 0x3FFF;
            ImGui::TextColored(cyan, " Table Addr:"); ImGui::SameLine();
            ImGui::Text("$%04X", pattern_table_addr);
        }

        ImGui::TextColored(cyan, " Tile Number:"); ImGui::SameLine();
        ImGui::Text("$%03X", tile); 
        ImGui::TextColored(cyan, " Tile Addr: "); ImGui::SameLine();
        ImGui::Text("$%04X", tile_addr); 

        if (ImGui::IsMouseClicked(0))
        {
            gui_debug_memory_goto(MEMORY_EDITOR_VRAM, tile_addr);
        }
    }

    ImGui::Columns(1);

    ImGui::PopFont();
}

static void debug_window_vram_sprites(void)
{
    float scale = 4.0f;
    float size_8 = 8.0f * scale;
    float size_16 = 16.0f * scale;

    GearcolecoCore* core = emu_get_core();
    Video* video = core->GetVideo();
    u8* regs = video->GetRegisters();
    u8* vram = video->GetVRAM();
    GC_RuntimeInfo runtime;
    emu_get_runtime(runtime);
    bool sprites_16 = IsSetBit(regs[1], 1);

    float width = 0.0f;
    float height = 0.0f;

    width = sprites_16 ? size_16 : size_8;
    height = sprites_16 ? size_16 : size_8;

    ImVec2 p[64];

    ImGuiIO& io = ImGui::GetIO();

    ImGui::PushFont(gui_default_font);

    ImGui::Columns(2, "spr", false);
    ImGui::SetColumnOffset(1, sprites_16 ? 330.0f : 200.0f);

    ImGui::BeginChild("sprites", ImVec2(0, 0.0f), true);
    bool window_hovered = ImGui::IsWindowHovered();

    for (int s = 0; s < 32; s++)
    {
        p[s] = ImGui::GetCursorScreenPos();

        ImGui::Image((ImTextureID)(intptr_t)renderer_emu_debug_vram_sprites[s], ImVec2(width, height), ImVec2(0.0f, 0.0f), ImVec2((1.0f / 16.0f) * (width / scale), (1.0f / 16.0f) * (height / scale)));

        float mouse_x = io.MousePos.x - p[s].x;
        float mouse_y = io.MousePos.y - p[s].y;

        if (window_hovered && (mouse_x >= 0.0f) && (mouse_x < width) && (mouse_y >= 0.0f) && (mouse_y < height))
        {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRect(ImVec2(p[s].x, p[s].y), ImVec2(p[s].x + width, p[s].y + height), ImColor(cyan), 2.0f, ImDrawFlags_RoundCornersAll, 3.0f);
        }

        if (s % 4 < 3)
            ImGui::SameLine();
    }

    ImGui::EndChild();

    ImGui::NextColumn();

    ImVec2 p_screen = ImGui::GetCursorScreenPos();

    float screen_scale = 1.0f;
    float tex_h = (float)runtime.screen_width / (float)(GC_RESOLUTION_WIDTH_WITH_OVERSCAN);
    float tex_v = (float)runtime.screen_height / (float)(GC_RESOLUTION_HEIGHT_WITH_OVERSCAN);

    ImGui::Image((ImTextureID)(intptr_t)renderer_emu_texture, ImVec2(runtime.screen_width * screen_scale, runtime.screen_height * screen_scale), ImVec2(0, 0), ImVec2(tex_h, tex_v));

    for (int s = 0; s < 64; s++)
    {
        if ((p[s].x == 0) && (p[s].y == 0))
            continue;

        float mouse_x = io.MousePos.x - p[s].x;
        float mouse_y = io.MousePos.y - p[s].y;

        if (window_hovered && (mouse_x >= 0.0f) && (mouse_x < width) && (mouse_y >= 0.0f) && (mouse_y < height))
        {
            int x = 0;
            int y = 0;
            int tile = 0;
            int sprite_tile_addr = 0;
            int sprite_shift = 0;
            int sprite_color = 0;
            float real_x = 0.0f;
            float real_y = 0.0f;

            u16 sprite_attribute_addr = (regs[5] & 0x7F) << 7;
            u16 sprite_pattern_addr = (regs[6] & 0x07) << 11;
            int sprite_attribute_offset = sprite_attribute_addr + (s << 2);
            tile = vram[sprite_attribute_offset + 2];
            sprite_tile_addr = sprite_pattern_addr + (tile << 3);
            sprite_shift = (vram[sprite_attribute_offset + 3] & 0x80) ? 32 : 0;
            sprite_color = vram[sprite_attribute_offset + 3] & 0x0F;
            x = vram[sprite_attribute_offset + 1];
            y = vram[sprite_attribute_offset];

            int final_y = (y + 1) & 0xFF;

            if (final_y >= 0xE0)
                final_y = -(0x100 - final_y);

            real_x = (float)(x - sprite_shift);
            real_y = (float)final_y;

            float max_width = 8.0f;
            float max_height = sprites_16 ? 16.0f : 8.0f;

            if (sprites_16)
                max_width = 16.0f;

            if(IsSetBit(regs[1], 0))
            {
                max_width *= 2.0f;
                max_height *= 2.0f;
            }

            float rectx_min = p_screen.x + (real_x * screen_scale);
            float rectx_max = p_screen.x + ((real_x + max_width) * screen_scale);
            float recty_min = p_screen.y + (real_y * screen_scale);
            float recty_max = p_screen.y + ((real_y + max_height) * screen_scale);

            rectx_min = fminf(fmaxf(rectx_min, p_screen.x), p_screen.x + (runtime.screen_width * screen_scale));
            rectx_max = fminf(fmaxf(rectx_max, p_screen.x), p_screen.x + (runtime.screen_width * screen_scale));
            recty_min = fminf(fmaxf(recty_min, p_screen.y), p_screen.y + (runtime.screen_height * screen_scale));
            recty_max = fminf(fmaxf(recty_max, p_screen.y), p_screen.y + (runtime.screen_height * screen_scale));
            
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRect(ImVec2(rectx_min, recty_min), ImVec2(rectx_max, recty_max), ImColor(cyan), 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);

            ImGui::TextColored(light_brown, "DETAILS:");
            ImGui::TextColored(cyan, " Attribute Addr:"); ImGui::SameLine();
            ImGui::Text("$%04X", sprite_attribute_offset);

            ImGui::TextColored(cyan, " X:"); ImGui::SameLine();
            ImGui::Text("$%02X", x);
            ImGui::TextColored(cyan, " Y:"); ImGui::SameLine();
            ImGui::Text("$%02X", y);

            ImGui::TextColored(cyan, " Tile:"); ImGui::SameLine();
            ImGui::Text("$%02X", tile);

            ImGui::TextColored(cyan, " Tile Addr:"); ImGui::SameLine();
            ImGui::Text("$%04X", sprite_tile_addr);

            ImGui::TextColored(cyan, " Color:"); ImGui::SameLine();
            ImGui::Text("$%02X", sprite_color);

            ImGui::TextColored(cyan, " Early Clock:"); ImGui::SameLine();
            sprite_shift > 0 ? ImGui::TextColored(green, "ON ") : ImGui::TextColored(gray, "OFF");

            if (ImGui::IsMouseClicked(0))
            {
                gui_debug_memory_goto(MEMORY_EDITOR_VRAM, sprite_tile_addr);
            }
        }
    }

    ImGui::Columns(1);

    ImGui::PopFont();
}

static void debug_window_vram_regs(void)
{
    ImGui::PushFont(gui_default_font);

    Video* video = emu_get_core()->GetVideo();
    u8* regs = video->GetRegisters();

    ImGui::TextColored(light_brown, "VDP STATE:");

    ImGui::TextColored(violet, " PAL (50Hz)       "); ImGui::SameLine();
    video->IsPAL() ? ImGui::TextColored(green, "YES ") : ImGui::TextColored(gray, "NO  ");
    ImGui::TextColored(violet, " LATCH FIRST BYTE "); ImGui::SameLine();
    video->GetLatch() ? ImGui::TextColored(green, "YES ") : ImGui::TextColored(gray, "NO  ");
    ImGui::TextColored(violet, " INTERNAL BUFFER  "); ImGui::SameLine();
    ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", video->GetBufferReg(), BYTE_TO_BINARY(video->GetBufferReg()));
    ImGui::TextColored(violet, " INTERNAL STATUS  "); ImGui::SameLine();
    ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", video->GetStatusReg(), BYTE_TO_BINARY(video->GetStatusReg()));
    ImGui::TextColored(violet, " INTERNAL ADDRESS "); ImGui::SameLine();
    ImGui::Text("$%04X", video->GetAddressReg());
    ImGui::TextColored(violet, " RENDER LINE      "); ImGui::SameLine();
    ImGui::Text("%d", video->GetRenderLine());
    ImGui::TextColored(violet, " CYCLE COUNTER    "); ImGui::SameLine();
    ImGui::Text("%d", video->GetCycleCounter());

    ImGui::TextColored(light_brown, "VDP REGISTERS:");

    const char* reg_desc[] = {"CONTROL 0   ", "CONTROL 1   ", "PATTERN NAME", "COLOR TABLE ", "PATTERN GEN ", "SPRITE ATTR ", "SPRITE GEN  ", "COLORS      "};

    for (int i = 0; i < 8; i++)
    {
        ImGui::TextColored(cyan, " $%01X ", i); ImGui::SameLine();
        ImGui::TextColored(violet, "%s ", reg_desc[i]); ImGui::SameLine();
        ImGui::Text("$%02X (" BYTE_TO_BINARY_PATTERN_SPACED ")", regs[i], BYTE_TO_BINARY(regs[i]));
    }

    ImGui::PopFont();
}

static void add_symbol(const char* line)
{
    Debug("Loading symbol %s", line);

    DebugSymbol s;

    std::string str(line);

    str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
    str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());

    size_t first = str.find_first_not_of(' ');
    if (std::string::npos == first)
    {
        str = "";
    }
    else
    {
        size_t last = str.find_last_not_of(' ');
        str = str.substr(first, (last - first + 1));
    }

    std::size_t comment = str.find(";");

    if (comment != std::string::npos)
        str = str.substr(0 , comment);

    std::size_t space = str.find(" ");

    if (space != std::string::npos)
    {
        s.text = str.substr(space + 1 , std::string::npos);
        str = str.substr(0, space);

        std::size_t separator = str.find(":");

        try
        {
            if (separator != std::string::npos)
            {
                s.address = (u16)std::stoul(str.substr(separator + 1 , std::string::npos), 0, 16);

                s.bank = std::stoul(str.substr(0, separator), 0 , 16);
            }
            else
            {
                s.address = (u16)std::stoul(str, 0, 16);
                s.bank = 0;
            }

            symbols.push_back(s);
        }
        catch(const std::invalid_argument&)
        {
        }
    }
}

static void add_breakpoint_cpu(void)
{
    int input_len = (int)strlen(brk_address_cpu);
    u16 target_address = 0;

    try
    {
        if ((input_len == 7) && (brk_address_cpu[2] == ':'))
        {
            std::string str(brk_address_cpu);
            std::size_t separator = str.find(":");

            if (separator != std::string::npos)
            {
                target_address = (u16)std::stoul(str.substr(separator + 1 , std::string::npos), 0, 16);
            }
        } 
        else if (input_len == 4)
        {
            target_address = (u16)std::stoul(brk_address_cpu, 0, 16);
        }
        else
        {
            return;
        }
    }
    catch(const std::invalid_argument&)
    {
        return;
    }

    Memory::stDisassembleRecord* record = emu_get_core()->GetMemory()->GetDisassembleRecord(target_address, true);

    brk_address_cpu[0] = 0;

    bool found = false;
    std::vector<Memory::stDisassembleRecord*>* breakpoints = emu_get_core()->GetMemory()->GetBreakpointsCPU();

    if (IsValidPointer(record))
    {
        for (long unsigned int b = 0; b < breakpoints->size(); b++)
        {
            if ((*breakpoints)[b] == record)
            {
                found = true;
                break;
            }
        }
    }

    if (!found)
    {
        breakpoints->push_back(record);
    }
}

static void add_breakpoint_mem(void)
{
    int input_len = (int)strlen(brk_address_mem);
    u16 address1 = 0;
    u16 address2 = 0;
    bool range = false;

    try
    {
        if ((input_len == 9) && (brk_address_mem[4] == '-'))
        {
            std::string str(brk_address_mem);
            std::size_t separator = str.find("-");

            if (separator != std::string::npos)
            {
                address1 = (u16)std::stoul(str.substr(0, separator), 0 , 16);
                address2 = (u16)std::stoul(str.substr(separator + 1 , std::string::npos), 0, 16);
                range = true;
            }
        }
        else if (input_len == 4)
        {
            address1 = (u16)std::stoul(brk_address_mem, 0, 16);
        }
        else
        {
            return;
        }
    }
    catch(const std::invalid_argument&)
    {
        return;
    }

    bool found = false;
    std::vector<Memory::stMemoryBreakpoint>* breakpoints = emu_get_core()->GetMemory()->GetBreakpointsMem();

    for (long unsigned int b = 0; b < breakpoints->size(); b++)
    {
        Memory::stMemoryBreakpoint temp = (*breakpoints)[b];
        if ((temp.address1 == address1) && (temp.address2 == address2) && (temp.range == range))
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        Memory::stMemoryBreakpoint new_breakpoint;
        new_breakpoint.address1 = address1;
        new_breakpoint.address2 = address2;
        new_breakpoint.range = range;
        new_breakpoint.read = brk_new_mem_read;
        new_breakpoint.write = brk_new_mem_write;

        breakpoints->push_back(new_breakpoint);
    }

    brk_address_mem[0] = 0;
}

static void request_goto_address(u16 address)
{
    goto_address_requested = true;
    goto_address_target = address;
}

static bool is_return_instruction(u8 opcode1, u8 opcode2)
{
    switch (opcode1)
    {
        case 0xC9: // RET
        case 0xC0: // RET NZ
        case 0xC8: // RET Z
        case 0xD0: // RET NC
        case 0xD8: // RET C
        case 0xE0: // RET PO
        case 0xE8: // RET PE
        case 0xF0: // RET P
        case 0xF8: // RET M
            return true;
        case 0xED: // Extended instructions
            if (opcode2 == 0x45 || opcode2 == 0x4D) // RETN, RETI
                return true;
            else
                return false;
        default:
            return false;
    }
}
