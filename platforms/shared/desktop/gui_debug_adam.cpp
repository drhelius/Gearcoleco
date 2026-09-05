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

static const char* controller_state_name(int state)
{
    switch ((GC_AdamNetControllerState)state)
    {
        case GC_ADAMNET_CONTROLLER_INITIALIZING: return "Initializing";
        case GC_ADAMNET_CONTROLLER_IDLE: return "Idle";
        case GC_ADAMNET_CONTROLLER_BUSY: return "Busy";
        default: return "Unknown";
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

static const char* media_type_name(int type)
{
    switch ((GC_AdamMediaType)type)
    {
        case GC_ADAM_MEDIA_DATA_PACK: return "Data pack";
        case GC_ADAM_MEDIA_DISK: return "Disk";
        case GC_ADAM_MEDIA_NONE:
        default: return "None";
    }
}

static void format_adam_key(int key, char* output, size_t output_size)
{
    static const char* names[] =
    {
        "Space", "Minus", "Plus", "Caret", "Semicolon", "Quote", "Open bracket",
        "Close bracket", "Backslash", "Comma", "Period", "Slash", "Return", "Escape",
        "Backspace", "Tab", "Home", "SmartKey I", "SmartKey II", "SmartKey III",
        "SmartKey IV", "SmartKey V", "SmartKey VI", "Wild Card", "Undo", "Move", "Store",
        "Insert", "Print", "Clear", "Delete", "Up", "Right", "Down", "Left", "Shift",
        "Control", "Lock"
    };

    if ((key >= GC_ADAM_KEY_A) && (key <= GC_ADAM_KEY_Z))
        snprintf(output, output_size, "%c", 'A' + key - GC_ADAM_KEY_A);
    else if ((key >= GC_ADAM_KEY_0) && (key <= GC_ADAM_KEY_9))
        snprintf(output, output_size, "%c", '0' + key - GC_ADAM_KEY_0);
    else if ((key >= GC_ADAM_KEY_SPACE) && (key < GC_ADAM_KEY_COUNT))
        snprintf(output, output_size, "%s", names[key - GC_ADAM_KEY_SPACE]);
    else
        snprintf(output, output_size, "None");
}

void gui_debug_window_adam_system(const GC_AdamDebugState* state)
{
    if (!IsValidPointer(state))
        return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(80, 80), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(850, 455), ImGuiCond_FirstUseEver);
    ImGui::Begin("ADAM System", &config_debug.show_adam_system);
    ImGui::PushFont(gui_default_font);

    ImGui::TextColored(violet, "Machine     "); ImGui::SameLine();
    ImGui::Text("ADAM (%s boot)", state->boot_mode == GC_ADAM_BOOT_CARTRIDGE ?
        "cartridge" : "computer");
    ImGui::TextColored(violet, "Master cycles"); ImGui::SameLine();
    ImGui::Text("%llu", (unsigned long long)state->master_clock_cycles);
    ImGui::TextColored(violet, "MIOC        "); ImGui::SameLine();
    ImGui::Text("$%02X  ", state->mioc); ImGui::SameLine(0, 0);
    ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(state->mioc));
    ImGui::TextColored(violet, "Control     "); ImGui::SameLine();
    ImGui::Text("$%02X  ", state->control); ImGui::SameLine(0, 0);
    ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(state->control));
    ImGui::TextColored(violet, "ADAMnet reset"); ImGui::SameLine();
    ImGui::TextColored(state->adamnet_reset ? orange : green, "%s",
        state->adamnet_reset ? "Asserted" : "Released");
    ImGui::SameLine();
    ImGui::TextColored(violet, "EOS overlay"); ImGui::SameLine();
    ImGui::TextColored(state->eos_enabled ? green : gray, "%s",
        state->eos_enabled ? "Enabled" : "Disabled");
    ImGui::TextColored(violet, "Map generation"); ImGui::SameLine();
    ImGui::Text("%u", state->mapping_generation);

    ImGui::Separator();
    ImGui::TextColored(brown, "FIRMWARE");
    if (ImGui::BeginTable("##adam_firmware", 4,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Role");
        ImGui::TableSetupColumn("Size");
        ImGui::TableSetupColumn("CRC32");
        ImGui::TableSetupColumn("Status");
        ImGui::TableHeadersRow();
        for (int firmware = 0; firmware < GC_ADAM_FIRMWARE_COUNT; firmware++)
        {
            const Adam::FirmwareMetadata* metadata =
                Adam::GetFirmwareMetadata((GC_AdamFirmware)firmware);
            const GC_AdamDebugFirmwareState* debug_firmware = &state->firmware[firmware];
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::TextUnformatted(metadata->role_name);
            ImGui::TableNextColumn(); ImGui::Text("%d KiB", debug_firmware->size / 1024);
            ImGui::TableNextColumn();
            if (debug_firmware->loaded)
                ImGui::Text("%08X", debug_firmware->crc);
            else
                ImGui::TextDisabled("-");
            ImGui::TableNextColumn();
            if (!debug_firmware->loaded)
                ImGui::TextColored(red, "Missing");
            else if (debug_firmware->known)
                ImGui::TextColored(green, "Known");
            else
                ImGui::TextColored(orange, "Unknown revision");
        }
        ImGui::EndTable();
    }

    ImGui::Separator();
    ImGui::TextColored(brown, "CPU MEMORY MAP");
    if (ImGui::BeginTable("##adam_memory_map", 7,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Range");
        ImGui::TableSetupColumn("Read source");
        ImGui::TableSetupColumn("Read offset");
        ImGui::TableSetupColumn("Write destination");
        ImGui::TableSetupColumn("Write offset");
        ImGui::TableSetupColumn("Bank");
        ImGui::TableSetupColumn("Debug write");
        ImGui::TableHeadersRow();
        for (int page = 0; page < GC_ADAM_DEBUG_PAGE_COUNT; page++)
        {
            const GC_AdamDebugMemoryPage* debug_page = &state->pages[page];
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("$%04X-$%04X", debug_page->start,
                debug_page->end);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(
                memory_source_name(debug_page->read_source));
            ImGui::TableNextColumn(); ImGui::Text("$%05X", debug_page->read_offset);
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
            ImGui::TableNextColumn();
            ImGui::TextColored(debug_page->writable ? green : gray, "%s",
                debug_page->writable ? "Yes" : "No");
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
    ImGui::SetNextWindowPos(ImVec2(115, 110), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(1100, 620), ImGuiCond_FirstUseEver);
    ImGui::Begin("ADAMnet", &config_debug.show_adam_net);
    ImGui::PushFont(gui_default_font);

    ImGui::TextColored(violet, "Controller  "); ImGui::SameLine();
    ImGui::TextColored(state->controller_state == GC_ADAMNET_CONTROLLER_BUSY ? orange : green,
        "%s", controller_state_name(state->controller_state));
    ImGui::TextColored(violet, "PCB address "); ImGui::SameLine();
    ImGui::Text("$%04X", state->pcb_address);
    ImGui::SameLine(); ImGui::TextColored(violet, "Command/status"); ImGui::SameLine();
    ImGui::Text("$%02X %s", state->pcb_command_status,
        command_status_name(state->pcb_command_status));
    ImGui::TextColored(violet, "DCB count   "); ImGui::SameLine();
    ImGui::Text("%u", state->configured_dcb_count);
    ImGui::SameLine(); ImGui::TextColored(violet, "Next scan"); ImGui::SameLine();
    ImGui::Text("%u", state->next_scan_index);
    ImGui::SameLine(); ImGui::TextColored(violet, "Cycles to event"); ImGui::SameLine();
    ImGui::Text("%d", state->cycles_until_event);

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
        ImGui::Text("DCB %u, device $%02X (%s), slot %d, command $%02X (%s)",
            state->transfer_dcb, state->transfer_device, device_name(state->transfer_device),
            IsValidPointer(device) ? device->slot : -1, state->transfer_command,
            command_status_name(state->transfer_command));
        ImGui::Text("Block %u, DMA $%04X, length %u, media generation %u, error %u",
            state->transfer_block, state->transfer_buffer, state->transfer_length,
            state->transfer_media_generation, state->transfer_error);
    }

    ImGui::Separator();
    ImGui::TextColored(brown, "DEVICE CONTROL BLOCKS");
    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("##adam_dcbs", 10, flags, ImVec2(0, 275)))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("DCB", ImGuiTableColumnFlags_WidthFixed, 38.0f);
        ImGui::TableSetupColumn("Command/status", ImGuiTableColumnFlags_WidthFixed, 130.0f);
        ImGui::TableSetupColumn("Device", ImGuiTableColumnFlags_WidthFixed, 115.0f);
        ImGui::TableSetupColumn("Buffer", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Length", ImGuiTableColumnFlags_WidthFixed, 55.0f);
        ImGui::TableSetupColumn("Block", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("Retry", ImGuiTableColumnFlags_WidthFixed, 55.0f);
        ImGui::TableSetupColumn("Max length", ImGuiTableColumnFlags_WidthFixed, 72.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 46.0f);
        ImGui::TableSetupColumn("Node status", ImGuiTableColumnFlags_WidthFixed, 76.0f);
        ImGui::TableHeadersRow();
        for (int dcb = 0; dcb < GC_ADAM_DEBUG_DCB_COUNT; dcb++)
        {
            const GC_AdamDebugDCB* debug_dcb = &state->dcbs[dcb];
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%d", dcb);
            ImGui::TableNextColumn(); ImGui::Text("$%02X %s", debug_dcb->command_status,
                command_status_name(debug_dcb->command_status));
            ImGui::TableNextColumn(); ImGui::Text("$%02X %s", debug_dcb->device,
                device_name(debug_dcb->device));
            ImGui::TableNextColumn(); ImGui::Text("$%04X", debug_dcb->buffer);
            ImGui::TableNextColumn(); ImGui::Text("%u", debug_dcb->length);
            ImGui::TableNextColumn(); ImGui::Text("%u", debug_dcb->block);
            ImGui::TableNextColumn(); ImGui::Text("%u", debug_dcb->retry);
            ImGui::TableNextColumn(); ImGui::Text("%u", debug_dcb->max_length);
            ImGui::TableNextColumn(); ImGui::Text("$%02X", debug_dcb->device_type);
            ImGui::TableNextColumn(); ImGui::Text("$%02X", debug_dcb->node_status);
        }
        ImGui::EndTable();
    }

    ImGui::Separator();
    ImGui::TextColored(brown, "KEYBOARD");
    ImGui::Text("FIFO %u / %d", state->keyboard_fifo_count, AdamNet::kKeyboardFIFOSize);
    ImGui::SameLine(); ImGui::TextColored(state->keyboard_overflow ? red : gray, "Overflow %s",
        state->keyboard_overflow ? "Yes" : "No");
    ImGui::SameLine(); ImGui::TextColored(state->keyboard_lock ? green : gray, "Lock %s",
        state->keyboard_lock ? "On" : "Off");
    ImGui::Text("Modifiers: Shift %s, Control %s, Home %s",
        state->keyboard_shift ? "On" : "Off", state->keyboard_control ? "On" : "Off",
        state->keyboard_home ? "On" : "Off");
    char repeat_key[32];
    format_adam_key(state->keyboard_repeat_key, repeat_key, sizeof(repeat_key));
    ImGui::SameLine(); ImGui::Text("  Repeat: %s (%d cycles)", repeat_key,
        state->keyboard_repeat_cycles);

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
    ImGui::SetNextWindowPos(ImVec2(150, 140), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(950, 410), ImGuiCond_FirstUseEver);
    ImGui::Begin("ADAM Media and Printer", &config_debug.show_adam_printer);

    ImGui::PushFont(gui_default_font);
    if (ImGui::BeginTabBar("##adam_media_printer_tabs"))
    {
        if (ImGui::BeginTabItem("Media"))
        {
            if (ImGui::BeginTable("##adam_core_media", 10,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX))
            {
                ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthFixed, 85.0f);
                ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 70.0f);
                ImGui::TableSetupColumn("Capacity", ImGuiTableColumnFlags_WidthFixed, 65.0f);
                ImGui::TableSetupColumn("Blocks", ImGuiTableColumnFlags_WidthFixed, 55.0f);
                ImGui::TableSetupColumn("Position", ImGuiTableColumnFlags_WidthFixed, 58.0f);
                ImGui::TableSetupColumn("Generation", ImGuiTableColumnFlags_WidthFixed, 70.0f);
                ImGui::TableSetupColumn("Cached block", ImGuiTableColumnFlags_WidthFixed, 105.0f);
                ImGui::TableSetupColumn("Write protect", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Dirty", ImGuiTableColumnFlags_WidthFixed, 45.0f);
                ImGui::TableSetupColumn("Base CRC32", ImGuiTableColumnFlags_WidthFixed, 75.0f);
                ImGui::TableHeadersRow();
                for (int slot = 0; slot < GC_ADAM_MEDIA_SLOT_COUNT; slot++)
                {
                    const GC_AdamDebugMediaState* media = &state->media[slot];
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn(); ImGui::TextUnformatted(kAdamMediaSlotNames[slot]);
                    ImGui::TableNextColumn(); ImGui::TextUnformatted(media_type_name(media->type));
                    ImGui::TableNextColumn();
                    if (media->inserted) ImGui::Text("%u KiB", media->size / 1024);
                    else ImGui::TextDisabled("-");
                    ImGui::TableNextColumn();
                    if (media->inserted) ImGui::Text("%u", media->block_count);
                    else ImGui::TextDisabled("-");
                    ImGui::TableNextColumn();
                    if (media->inserted) ImGui::Text("%u", media->position);
                    else ImGui::TextDisabled("-");
                    ImGui::TableNextColumn(); ImGui::Text("%u", media->generation);
                    ImGui::TableNextColumn();
                    if (media->cache_valid)
                        ImGui::Text("%u (gen %u)", media->cached_block, media->cached_generation);
                    else
                        ImGui::TextDisabled("-");
                    ImGui::TableNextColumn();
                    ImGui::TextColored(media->write_protected ? orange : gray, "%s",
                        media->write_protected ? "Yes" : "No");
                    ImGui::TableNextColumn();
                    ImGui::TextColored(media->dirty ? orange : gray, "%s",
                        media->dirty ? "Yes" : "No");
                    ImGui::TableNextColumn();
                    if (media->inserted) ImGui::Text("%08X", media->base_crc);
                    else ImGui::TextDisabled("-");
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Printer"))
        {
            ImGui::TextColored(cyan, "Captured:"); ImGui::SameLine();
            ImGui::Text("%d / %d bytes", state->printer_size, GC_ADAM_DEBUG_PRINTER_SIZE);
            if (ImGui::Button("Save..."))
                gui_file_dialog_save_adam_printer();
            ImGui::SameLine();
            if (ImGui::Button("Clear"))
                emu_clear_adam_printer();
            ImGui::SameLine();
            ImGui::Checkbox("Hexadecimal", &show_hex);

            ImGui::Separator();
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
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleVar();
}
