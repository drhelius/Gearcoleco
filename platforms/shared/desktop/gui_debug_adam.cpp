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

#include "gui_debug_adam.h"

#include "imgui.h"
#include "Adam.h"
#include "AdamNet.h"
#include "config.h"
#include "emu.h"
#include "gui.h"
#include "gui_filedialogs.h"
#include "gui_debug_constants.h"
#include "gui_debug_memory.h"
#include "gui_adam.h"
#include "utils.h"

static const char* kAdamMediaSlotNames[GC_ADAM_MEDIA_SLOT_COUNT] =
{
    "Disk 1", "Disk 2", "Data Pack 1", "Data Pack 2"
};

static const char* memory_source_name(int source)
{
    switch ((Adam::MemorySource)source)
    {
        case Adam::MemorySourceRAM: return "RAM";
        case Adam::MemorySourceOS7: return "OS-7";
        case Adam::MemorySourceEOS: return "EOS";
        case Adam::MemorySourceSmartWriter: return "SmartWriter";
        case Adam::MemorySourceCartridge: return "Cartridge";
        case Adam::MemorySourceOpenBus:
        default: return "Open bus";
    }
}

static const char* command_status_name(u8 value)
{
    switch (value)
    {
        case AdamNet::CommandIdle: return "Idle";
        case AdamNet::CommandStatus: return "Status";
        case AdamNet::CommandSoftReset: return "Soft reset";
        case AdamNet::CommandWrite: return "Write";
        case AdamNet::CommandRead: return "Read";
        case AdamNet::ResponseSuccess: return "Success";
        case AdamNet::ResponsePrinterBusy: return "Printer busy";
        case AdamNet::ResponseKeyboardEmpty: return "Keyboard empty";
        case AdamNet::ResponseDeviceError: return "Device error";
        case AdamNet::ResponseTimeout: return "Timeout";
        default: return (value >= 0x80) && (value <= 0x85) ? "PCB response" : "Unknown";
    }
}

static const char* device_name(u8 id)
{
    const AdamNet::DeviceMetadata* device = AdamNet::GetDeviceMetadata(id);
    return IsValidPointer(device) ? device->name : "Unknown";
}

static const char* dcb_device_type_name(u8 type)
{
    if (type == 0)
        return "Character";
    if (type == 1)
        return "Block";
    return "Unknown";
}

static const char* node_status_name(u8 value, u8 device_id)
{
    const AdamNet::DeviceMetadata* device = AdamNet::GetDeviceMetadata(device_id);
    u8 shift = IsValidPointer(device) ? device->status_shift : 0;
    switch ((value >> shift) & 0x0F)
    {
        case 0: return "Ready";
        case 1: return "CRC error";
        case 2: return "Block not found";
        case 3: return "No media";
        case 4: return "No device";
        case 5: return "Write protected";
        default: return "Unknown";
    }
}

void gui_debug_window_adam_system(const GC_AdamDebugState* state)
{
    if (!IsValidPointer(state))
        return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    gui_adam_prepare_window(850.0f, 350.0f);
    ImGui::Begin("ADAM Memory Map and Ports###ADAM System", &config_debug.show_adam_system);
    gui_adam_keep_window_visible();
    ImGui::PushFont(gui_default_font);

    ImGui::TextColored(violet, "MIOC $60-$7F"); ImGui::SameLine();
    ImGui::Text("$%02X  ", state->mioc); ImGui::SameLine(0, 0);
    ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(state->mioc));
    ImGui::TextColored(violet, "Control $20-$3F"); ImGui::SameLine();
    ImGui::Text("$%02X  ", state->control); ImGui::SameLine(0, 0);
    ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(state->control));
    ImGui::TextColored(violet, "ADAMnet reset"); ImGui::SameLine();
    ImGui::TextColored(state->adamnet_reset ? orange : green, "%s",
        state->adamnet_reset ? "Asserted" : "Released");
    ImGui::SameLine();
    ImGui::TextColored(violet, "EOS overlay"); ImGui::SameLine();
    ImGui::TextColored(state->eos_enabled ? green : gray, "%s",
        state->eos_enabled ? "Enabled" : "Disabled");
    ImGui::Separator();
    ImGui::TextWrapped("Select an address range to inspect CPU memory. Read and write mappings can differ when ROM overlays RAM.");
    ImGui::TextColored(brown, "CPU MEMORY MAP");
    if (ImGui::BeginTable("##adam_memory_map", 6,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX,
        ImVec2(0, 0), 700.0f))
    {
        ImGui::TableSetupColumn("Range");
        ImGui::TableSetupColumn("Read source");
        ImGui::TableSetupColumn("Read offset");
        ImGui::TableSetupColumn("Write destination");
        ImGui::TableSetupColumn("Write offset");
        ImGui::TableSetupColumn("Bank");
        ImGui::TableHeadersRow();
        for (int page = 0; page < GC_ADAM_DEBUG_PAGE_COUNT; page++)
        {
            const GC_AdamDebugMemoryPage* debug_page = &state->pages[page];
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            char range[32];
            snprintf(range, sizeof(range), "$%04X-$%04X", debug_page->start, debug_page->end);
            if (ImGui::Selectable(range))
            {
                config_debug.show_memory = true;
                gui_debug_memory_goto(MEMORY_EDITOR_ADAM_MAPPED, debug_page->start);
            }
            ImGui::TableNextColumn(); ImGui::TextUnformatted(
                memory_source_name(debug_page->read_source));
            ImGui::TableNextColumn();
            if (debug_page->read_source == Adam::MemorySourceOpenBus)
                ImGui::TextDisabled("-");
            else
                ImGui::Text("$%05X", debug_page->read_offset);
            ImGui::TableNextColumn();
            if (debug_page->write_source == Adam::MemorySourceOpenBus)
                ImGui::TextDisabled("-");
            else
                ImGui::TextUnformatted(memory_source_name(debug_page->write_source));
            ImGui::TableNextColumn();
            if (debug_page->write_source == Adam::MemorySourceRAM)
                ImGui::Text("$%04X", debug_page->write_offset);
            else
                ImGui::TextDisabled("-");
            ImGui::TableNextColumn();
            if (debug_page->cartridge_bank >= 0)
                ImGui::Text("%d", debug_page->cartridge_bank);
            else
                ImGui::TextDisabled("-");
        }
        ImGui::EndTable();
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_adam_net(const GC_AdamDebugState* state)
{
    if (!IsValidPointer(state))
        return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    gui_adam_prepare_window(1100.0f, 620.0f);
    ImGui::Begin("ADAMnet / EOS Requests###ADAMnet", &config_debug.show_adam_net);
    gui_adam_keep_window_visible();
    ImGui::PushFont(gui_default_font);

    ImGui::TextWrapped("Inspect EOS device requests in physical RAM. Select a DCB or buffer address to open it in the memory editor.");
    ImGui::TextColored(violet, "PCB address "); ImGui::SameLine();
    char pcb_label[24];
    snprintf(pcb_label, sizeof(pcb_label), "$%04X##pcb", state->pcb_address);
    if (ImGui::SmallButton(pcb_label))
    {
        config_debug.show_memory = true;
        gui_debug_memory_goto(MEMORY_EDITOR_ADAM_RAM, state->pcb_address);
    }
    ImGui::SameLine(); ImGui::TextColored(violet, "Command/status"); ImGui::SameLine();
    ImGui::Text("$%02X %s", state->pcb_command_status,
        command_status_name(state->pcb_command_status));
    ImGui::TextColored(violet, "DCB count   "); ImGui::SameLine();
    ImGui::Text("%u", state->configured_dcb_count);
    if (state->configured_dcb_count > GC_ADAM_DEBUG_DCB_COUNT)
        ImGui::TextColored(orange, "Invalid DCB count; the PCB may not be initialized yet.");
    ImGui::Separator();
    ImGui::TextColored(brown, "ACTIVE TRANSFER");
    if (!state->transfer_active)
        ImGui::TextDisabled("None");
    else if (state->transfer_pcb)
    {
        ImGui::Text("PCB command $%02X (%s), new PCB $%04X, DCB count %u",
            state->transfer_command, command_status_name(state->transfer_command),
            state->transfer_pcb_address, state->transfer_pcb_count);
    }
    else
    {
        const AdamNet::DeviceMetadata* device =
            AdamNet::GetDeviceMetadata(state->transfer_device);
        ImGui::Text("DCB %u, device $%02X (%s), slot %s, command $%02X (%s)",
            state->transfer_dcb, state->transfer_device, device_name(state->transfer_device),
            IsValidPointer(device) && (device->slot >= 0) ?
            kAdamMediaSlotNames[device->slot] : "None", state->transfer_command,
            command_status_name(state->transfer_command));
        ImGui::Text("Block %u, buffer $%04X, length %u bytes",
            state->transfer_block, state->transfer_buffer, state->transfer_length);
    }

    ImGui::Separator();
    ImGui::TextColored(brown, "DEVICE CONTROL BLOCKS");
    static bool show_capabilities = false;
    ImGui::Checkbox("Device capabilities", &show_capabilities);
    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("##adam_dcbs", show_capabilities ? 10 : 7, flags, ImVec2(0, 275)))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("DCB address", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Command/status", ImGuiTableColumnFlags_WidthFixed, 130.0f);
        ImGui::TableSetupColumn("Device", ImGuiTableColumnFlags_WidthFixed, 115.0f);
        ImGui::TableSetupColumn("Buffer", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Length", ImGuiTableColumnFlags_WidthFixed, 55.0f);
        ImGui::TableSetupColumn("Block", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("Node status", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        if (show_capabilities)
        {
            ImGui::TableSetupColumn("Retry", ImGuiTableColumnFlags_WidthFixed, 55.0f);
            ImGui::TableSetupColumn("Max length", ImGuiTableColumnFlags_WidthFixed, 72.0f);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 95.0f);
        }
        ImGui::TableHeadersRow();
        for (int dcb = 0; dcb < state->configured_dcb_count && dcb < GC_ADAM_DEBUG_DCB_COUNT; dcb++)
        {
            const GC_AdamDebugDCB* debug_dcb = &state->dcbs[dcb];
            ImGui::TableNextRow();
            ImGui::PushID(dcb);
            ImGui::TableNextColumn();
            u16 address = (u16)(state->pcb_address + AdamNet::kPCBSize + dcb * AdamNet::kDCBSize);
            char address_label[32];
            snprintf(address_label, sizeof(address_label), "%d: $%04X", dcb, address);
            if (ImGui::Selectable(address_label))
            {
                config_debug.show_memory = true;
                gui_debug_memory_goto(MEMORY_EDITOR_ADAM_RAM, address);
            }
            ImGui::TableNextColumn(); ImGui::Text("$%02X %s", debug_dcb->command_status,
                command_status_name(debug_dcb->command_status));
            ImGui::TableNextColumn(); ImGui::Text("$%02X %s", debug_dcb->device,
                device_name(debug_dcb->device));
            ImGui::TableNextColumn();
            snprintf(address_label, sizeof(address_label), "$%04X##buffer", debug_dcb->buffer);
            if (ImGui::Selectable(address_label))
            {
                config_debug.show_memory = true;
                gui_debug_memory_goto(MEMORY_EDITOR_ADAM_RAM, debug_dcb->buffer);
            }
            ImGui::TableNextColumn(); ImGui::Text("%u", debug_dcb->length);
            ImGui::TableNextColumn(); ImGui::Text("%u", debug_dcb->block);
            ImGui::TableNextColumn(); ImGui::Text("$%02X %s", debug_dcb->node_status,
                node_status_name(debug_dcb->node_status, debug_dcb->device));
            if (show_capabilities)
            {
                ImGui::TableNextColumn(); ImGui::Text("%u", debug_dcb->retry);
                ImGui::TableNextColumn(); ImGui::Text("%u", debug_dcb->max_length);
                ImGui::TableNextColumn(); ImGui::Text("$%02X %s", debug_dcb->device_type,
                    dcb_device_type_name(debug_dcb->device_type));
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_adam_media_printer(const GC_AdamDebugState* state)
{
    if (!IsValidPointer(state))
        return;

    static bool show_hex = false;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    gui_adam_prepare_window(640.0f, 410.0f);
    ImGui::Begin("ADAM Printer Output###ADAM Media and Printer", &config_debug.show_adam_printer);
    gui_adam_keep_window_visible();


    ImGui::TextColored(cyan, "Captured:"); ImGui::SameLine();
    ImGui::Text("%d / %d bytes", state->printer_size, GC_ADAM_DEBUG_PRINTER_SIZE);
    if (ImGui::Button("Save..."))
        gui_file_dialog_save_adam_printer();
    ImGui::SameLine();
    if (ImGui::Button("Clear"))
        emu_clear_adam_printer();
    ImGui::SameLine();
    ImGui::Checkbox("Hexadecimal", &show_hex);

    ImGui::PushFont(gui_default_font);
    ImGui::BeginChild("##adam_printer_output", ImVec2(0, 0), true,
        ImGuiWindowFlags_HorizontalScrollbar);

    char output[(GC_ADAM_DEBUG_PRINTER_SIZE * 4) + 1];
    int position = 0;
    if (show_hex)
    {
        for (int i = 0; (i < state->printer_size) &&
            (position < (int)sizeof(output) - 16); i++)
        {
            if ((i & 0x0F) == 0)
            {
                position += snprintf(output + position, sizeof(output) - (size_t)position,
                    "%04X: ", i);
            }
            position += snprintf(output + position, sizeof(output) - (size_t)position,
                "%02X%s", state->printer_data[i],
                ((i & 0x0F) == 0x0F) || (i + 1 == state->printer_size) ? "\n" : " ");
        }
    }
    else
    {
        for (int i = 0; i < state->printer_size &&
            position < GC_ADAM_DEBUG_PRINTER_SIZE; i++)
        {
            u8 value = state->printer_data[i];
            if (value == '\r')
            {
                if ((i + 1 >= state->printer_size) || (state->printer_data[i + 1] != '\n'))
                    output[position++] = '\n';
            }
            else if ((value == '\n') || (value == '\t') ||
                ((value >= 0x20) && (value < 0x7F)))
            {
                output[position++] = (char)value;
            }
            else
                output[position++] = '.';
        }
    }
    output[position] = '\0';
    ImGui::TextUnformatted(output);

    ImGui::EndChild();

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleVar();
}
