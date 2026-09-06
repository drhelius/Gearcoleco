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
#include "AdamNet.h"
#include "config.h"
#include "emu.h"
#include "gui.h"
#include "gui_filedialogs.h"
#include "gui_debug_constants.h"
#include "gui_debug_memory.h"
#include "utils.h"

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
        default: return "Unknown";
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

static void memory_address(const char* id, u16 address)
{
    ImGui::PushID(id);
    char label[16];
    snprintf(label, sizeof(label), "$%04X", address);
    if (ImGui::SmallButton(label))
    {
        config_debug.show_memory = true;
        gui_debug_memory_goto(MEMORY_EDITOR_ADAM_RAM, address);
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Open physical RAM at $%04X", address);
    ImGui::PopID();
}

static void port_value(const char* label, u8 value)
{
    ImGui::TextColored(violet, "%s", label); ImGui::SameLine();
    ImGui::Text("$%02X  ", value); ImGui::SameLine(0, 0);
    ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(value));
}

void gui_debug_window_adam_system(const GC_AdamDebugState* state)
{
    if (!IsValidPointer(state))
        return;

    static const char* lower_names[] = { "SmartWriter", "RAM", "Expansion RAM", "OS-7 + RAM" };
    static const char* upper_names[] = { "RAM", "Expansion ROM", "Expansion RAM", "Cartridge" };
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(180, 45), ImGuiCond_FirstUseEver);
    ImGui::Begin("ADAM Ports", &config_debug.show_adam_system,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);
    ImGui::PushFont(gui_default_font);

    port_value("$7F MIOC ", state->mioc);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Bits 1-0: lower 32 KiB. Bits 3-2: upper 32 KiB. Mirrored at $60-$7F.");
    ImGui::TextColored(violet, " LOWER   "); ImGui::SameLine();
    ImGui::TextUnformatted(lower_names[state->mioc & 3]);
    ImGui::TextColored(violet, " UPPER   "); ImGui::SameLine();
    ImGui::TextUnformatted(upper_names[(state->mioc >> 2) & 3]);

    ImGui::Separator();
    port_value("$3F CTRL ", state->control);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Mirrored at $20-$3F.");
    ImGui::TextColored(violet, " EOS EN  "); ImGui::SameLine();
    ImGui::TextColored(state->control & 2 ? green : gray, "%u", (state->control >> 1) & 1);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Bit 1 selects EOS at $6000-$7FFF when the lower bank is SmartWriter.");
    ImGui::TextColored(violet, " NET RST "); ImGui::SameLine();
    ImGui::Text("%u", state->control & 1);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Bit 0: write 1 then 0 to reset ADAMnet.");

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_adam_net(const GC_AdamDebugState* state)
{
    if (!IsValidPointer(state))
        return;

    static int selected_dcb = 0;
    static bool selected_device = false;
    int count = state->configured_dcb_count;
    if (count > GC_ADAM_DEBUG_DCB_COUNT)
        count = GC_ADAM_DEBUG_DCB_COUNT;
    if (selected_dcb >= count)
        selected_dcb = 0;
    if (!selected_device && state->configured_dcb_count <= GC_ADAM_DEBUG_DCB_COUNT)
    {
        for (int i = 0; i < count; i++)
        {
            if (IsValidPointer(AdamNet::GetDeviceMetadata(state->dcbs[i].device)))
            {
                selected_dcb = i;
                selected_device = true;
                break;
            }
        }
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(380, 80), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(330, 370), ImGuiCond_FirstUseEver);
    ImGui::Begin("ADAMnet###ADAMnet Devices", &config_debug.show_adam_net,
        ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PushFont(gui_default_font);

    ImGui::TextColored(brown, "PCB");
    ImGui::Separator();
    ImGui::TextColored(violet, " ADDRESS "); ImGui::SameLine();
    memory_address("pcb", state->pcb_address);
    ImGui::TextColored(violet, " STATUS  "); ImGui::SameLine();
    ImGui::Text("$%02X", state->pcb_command_status);
    ImGui::TextColored(violet, " DCBs    "); ImGui::SameLine();
    ImGui::Text("%u", state->configured_dcb_count);
    if (state->configured_dcb_count > GC_ADAM_DEBUG_DCB_COUNT)
        ImGui::TextColored(orange, "Invalid DCB count");

    ImGui::Separator();
    if (count > 0)
    {
        const GC_AdamDebugDCB* dcb = &state->dcbs[selected_dcb];
        char label[64];
        snprintf(label, sizeof(label), "%d: $%02X %s", selected_dcb, dcb->device, device_name(dcb->device));
        ImGui::SetNextItemWidth(ImGui::CalcTextSize("00: $00 Data Pack 2             ").x);
        if (ImGui::BeginCombo("##adam_dcb", label))
        {
            for (int i = 0; i < count; i++)
            {
                snprintf(label, sizeof(label), "%d: $%02X %s", i, state->dcbs[i].device, device_name(state->dcbs[i].device));
                if (ImGui::Selectable(label, selected_dcb == i))
                {
                    selected_dcb = i;
                    selected_device = true;
                }
                if (selected_dcb == i)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        dcb = &state->dcbs[selected_dcb];
        ImGui::TextColored(violet, " DCB     "); ImGui::SameLine();
        memory_address("dcb", (u16)(state->pcb_address + AdamNet::kPCBSize + selected_dcb * AdamNet::kDCBSize));
        ImGui::TextColored(violet, " CMD/STS "); ImGui::SameLine();
        ImGui::Text("$%02X %s", dcb->command_status, command_status_name(dcb->command_status));
        ImGui::TextColored(violet, " BUFFER  "); ImGui::SameLine();
        memory_address("buffer", dcb->buffer);
        ImGui::TextColored(violet, " LENGTH  "); ImGui::SameLine();
        ImGui::Text("$%04X (%u)", dcb->length, dcb->length);
        ImGui::TextColored(violet, " BLOCK   "); ImGui::SameLine();
        ImGui::Text("$%08X", dcb->block);
        ImGui::TextColored(violet, " NODE    "); ImGui::SameLine();
        ImGui::Text("$%02X %s", dcb->node_status, node_status_name(dcb->node_status, dcb->device));

        ImGui::Separator();
        ImGui::TextColored(violet, " RETRY   "); ImGui::SameLine();
        ImGui::Text("$%04X", dcb->retry);
        ImGui::TextColored(violet, " MAX LEN "); ImGui::SameLine();
        ImGui::Text("$%04X (%u)", dcb->max_length, dcb->max_length);
        ImGui::TextColored(violet, " TYPE    "); ImGui::SameLine();
        ImGui::Text("$%02X %s", dcb->device_type, dcb_device_type_name(dcb->device_type));
    }
    else
        ImGui::TextDisabled("No device control blocks");

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
    ImGui::SetNextWindowPos(ImVec2(460, 160), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 260), ImGuiCond_FirstUseEver);
    ImGui::Begin("ADAM Printer Output", &config_debug.show_adam_printer);

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
