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

#include <stdio.h>
#include <string.h>
#include "gui_adam.h"

#include "imgui.h"
#include "config.h"
#include "emu.h"
#include "gui.h"
#include "application.h"
#include "gui_menus.h"
#include "gui_filedialogs.h"
#include "gui_debug_constants.h"
#include "utils.h"
#include "Adam.h"

enum AdamMediaPendingAction
{
    AdamMediaPendingNone = 0,
    AdamMediaPendingReplace,
    AdamMediaPendingEject,
    AdamMediaPendingSwap
};

struct AdamFirmwareInspection
{
    char path[4096];
    size_t actual_size;
    u32 crc;
    bool readable;
    bool valid;
};

static bool show_adam_media = false;
static bool show_adam_firmware = false;
static bool open_missing_firmware = false;
static bool missing_adam_firmware = false;
static bool open_quit_confirmation = false;
static bool open_dirty_confirmation = false;
static int pending_insert_slot = -1;
static bool pending_insert_discard_changes = false;
static int pending_save_as_slot = -1;
static int pending_firmware_browse = -1;
static int pending_dirty_slot = -1;
static int pending_swap_index = -1;
static AdamMediaPendingAction pending_dirty_action = AdamMediaPendingNone;
static AdamFirmwareInspection firmware_inspections[GC_ADAM_FIRMWARE_COUNT];

static void draw_media_window(void);
static void draw_media_row(GC_AdamMediaSlot slot, const char* label);
static void draw_dirty_confirmation(void);
static void complete_pending_action(bool save);
static void draw_firmware_window(void);
static void draw_firmware_row(GC_AdamFirmware firmware, char* path, size_t path_size);
static void draw_missing_firmware(void);
static void draw_quit_confirmation(void);
static void reset_firmware_paths(void);
static void refresh_firmware_inspection(GC_AdamFirmware firmware, const char* path);
static bool apply_firmware_path(GC_AdamFirmware firmware, const char* path);

void gui_adam_open_media(void)
{
    show_adam_media = true;
}

void gui_adam_open_firmware(void)
{
    reset_firmware_paths();
    show_adam_firmware = true;
}

void gui_adam_open_missing_firmware(bool adam)
{
    missing_adam_firmware = adam;
    open_missing_firmware = true;
}

void gui_adam_open_quit_confirmation(void)
{
    open_quit_confirmation = true;
}

void gui_adam_windows(void)
{
    if ((emu_get_machine() != GC_MACHINE_ADAM) || emu_is_empty())
        show_adam_media = false;

    if (show_adam_media)
        draw_media_window();
    if (show_adam_firmware)
        draw_firmware_window();

    if (open_missing_firmware)
    {
        open_missing_firmware = false;
        ImGui::OpenPopup("Firmware Required");
    }
    draw_missing_firmware();

    if (open_quit_confirmation)
    {
        open_quit_confirmation = false;
        ImGui::OpenPopup("Unsaved ADAM Media on Quit");
    }
    draw_quit_confirmation();

    if (pending_insert_slot >= 0)
    {
        int slot = pending_insert_slot;
        pending_insert_slot = -1;
        gui_file_dialog_insert_adam_media((GC_AdamMediaSlot)slot,
            pending_insert_discard_changes);
        pending_insert_discard_changes = false;
    }
    if (pending_save_as_slot >= 0)
    {
        int slot = pending_save_as_slot;
        pending_save_as_slot = -1;
        gui_file_dialog_save_adam_media((GC_AdamMediaSlot)slot);
    }
    if (pending_firmware_browse >= 0)
    {
        GC_AdamFirmware firmware = (GC_AdamFirmware)pending_firmware_browse;
        pending_firmware_browse = -1;
        if (firmware == GC_ADAM_FIRMWARE_OS7)
            gui_file_dialog_load_bios();
        else
            gui_file_dialog_load_adam_firmware(firmware);
    }
}

static void reset_firmware_paths(void)
{
    emu_get_adam_firmware_path(GC_ADAM_FIRMWARE_OS7, gui_bios_path,
        sizeof(gui_bios_path));
    emu_get_adam_firmware_path(GC_ADAM_FIRMWARE_EOS, gui_adam_eos_path,
        sizeof(gui_adam_eos_path));
    emu_get_adam_firmware_path(GC_ADAM_FIRMWARE_SMARTWRITER,
        gui_adam_smartwriter_path, sizeof(gui_adam_smartwriter_path));

    memset(firmware_inspections, 0, sizeof(firmware_inspections));
    refresh_firmware_inspection(GC_ADAM_FIRMWARE_OS7, gui_bios_path);
    refresh_firmware_inspection(GC_ADAM_FIRMWARE_EOS, gui_adam_eos_path);
    refresh_firmware_inspection(GC_ADAM_FIRMWARE_SMARTWRITER,
        gui_adam_smartwriter_path);
}

static void refresh_firmware_inspection(GC_AdamFirmware firmware, const char* path)
{
    AdamFirmwareInspection* inspection = &firmware_inspections[firmware];
    strncpy_fit(inspection->path, path, sizeof(inspection->path));
    inspection->actual_size = 0;
    inspection->crc = 0;
    inspection->valid = emu_inspect_adam_firmware(firmware, path,
        &inspection->actual_size, &inspection->crc);
    inspection->readable = inspection->actual_size > 0;
}

static bool apply_firmware_path(GC_AdamFirmware firmware, const char* path)
{
    if (!emu_load_adam_firmware(firmware, path))
    {
        const Adam::FirmwareMetadata* metadata = Adam::GetFirmwareMetadata(firmware);
        char message[256];
        snprintf(message, sizeof(message),
            "Invalid %s firmware. Expected exactly %d bytes.", metadata->role_name,
            metadata->size);
        gui_set_error_message(message);
        return false;
    }

    if (firmware == GC_ADAM_FIRMWARE_OS7)
        config_emulator.bios_path.assign(path);
    else if (firmware == GC_ADAM_FIRMWARE_EOS)
        config_emulator.adam_eos_path.assign(path);
    else
        config_emulator.adam_smartwriter_path.assign(path);

    const Adam::FirmwareMetadata* metadata = Adam::GetFirmwareMetadata(firmware);
    char message[128];
    snprintf(message, sizeof(message), "%s firmware configured", metadata->role_name);
    gui_set_status_message(message, 3000);
    return true;
}

static void draw_firmware_window(void)
{
    bool adam_running = !emu_is_empty() && (emu_get_machine() == GC_MACHINE_ADAM);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(1020, 280), ImGuiCond_FirstUseEver);
    ImGui::Begin("Firmware Setup", &show_adam_firmware);
    ImGui::PushFont(gui_default_font);

    if (adam_running)
        ImGui::TextColored(orange,
            "ADAM firmware cannot be replaced while ADAM is running. Unload or switch content first.");
    else
        ImGui::TextDisabled("Press Enter or Apply to commit a validated path. Unknown revisions are allowed.");

    if (ImGui::BeginTable("##adam_firmware", 7,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Role", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Path");
        ImGui::TableSetupColumn("Expected", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("Actual", ImGuiTableColumnFlags_WidthFixed, 65.0f);
        ImGui::TableSetupColumn("CRC32", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Revision", ImGuiTableColumnFlags_WidthFixed, 105.0f);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 135.0f);
        ImGui::TableHeadersRow();

        ImGui::BeginDisabled(adam_running);
        draw_firmware_row(GC_ADAM_FIRMWARE_OS7, gui_bios_path, sizeof(gui_bios_path));
        draw_firmware_row(GC_ADAM_FIRMWARE_EOS, gui_adam_eos_path,
            sizeof(gui_adam_eos_path));
        draw_firmware_row(GC_ADAM_FIRMWARE_SMARTWRITER, gui_adam_smartwriter_path,
            sizeof(gui_adam_smartwriter_path));
        ImGui::EndDisabled();
        ImGui::EndTable();
    }

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleVar();
}

static void draw_firmware_row(GC_AdamFirmware firmware, char* path, size_t path_size)
{
    const Adam::FirmwareMetadata* metadata = Adam::GetFirmwareMetadata(firmware);
    AdamFirmwareInspection* inspection = &firmware_inspections[firmware];

    ImGui::PushID((int)firmware);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(metadata->role_name);
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1.0f);
    bool apply = ImGui::InputText("##path", path, path_size,
        ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);
    if ((apply || !ImGui::IsItemActive()) && strcmp(inspection->path, path))
        refresh_firmware_inspection(firmware, path);

    ImGui::TableNextColumn();
    ImGui::Text("%d B", metadata->size);
    ImGui::TableNextColumn();
    if (inspection->readable)
        ImGui::Text("%zu B", inspection->actual_size);
    else
        ImGui::TextDisabled("-");
    ImGui::TableNextColumn();
    if (inspection->readable)
        ImGui::Text("%08X", inspection->crc);
    else
        ImGui::TextDisabled("-");
    ImGui::TableNextColumn();
    if (!inspection->readable)
        ImGui::TextColored(red, "Missing");
    else if (!inspection->valid)
        ImGui::TextColored(red, "Invalid size");
    else if (inspection->crc == metadata->crc)
        ImGui::TextColored(green, "Known");
    else
        ImGui::TextColored(orange, "Unknown");

    ImGui::TableNextColumn();
    if (ImGui::Button("Browse..."))
        pending_firmware_browse = firmware;
    ImGui::SameLine();
    ImGui::BeginDisabled(!inspection->valid);
    if ((ImGui::Button("Apply") || apply) && apply_firmware_path(firmware, path))
        refresh_firmware_inspection(firmware, path);
    ImGui::EndDisabled();
    ImGui::PopID();
}

static void draw_missing_firmware(void)
{
    if (!ImGui::BeginPopupModal("Firmware Required", NULL,
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    gui_dialog_in_use = true;
    if (missing_adam_firmware)
    {
        ImGui::TextUnformatted("ADAM requires valid OS-7, EOS, and SmartWriter firmware.");
        for (int i = 0; i < GC_ADAM_FIRMWARE_COUNT; i++)
        {
            GC_AdamFirmware firmware = (GC_AdamFirmware)i;
            const Adam::FirmwareMetadata* metadata = Adam::GetFirmwareMetadata(firmware);
            char path[4096];
            size_t actual_size = 0;
            u32 crc = 0;
            emu_get_adam_firmware_path(firmware, path, sizeof(path));
            bool valid = emu_inspect_adam_firmware(firmware, path, &actual_size, &crc);
            ImGui::BulletText("%s: %s", metadata->role_name,
                valid ? get_filename(path) : "missing or invalid");
        }
    }
    else
    {
        ImGui::TextUnformatted("ColecoVision requires a valid 8 KiB OS-7 BIOS.");
    }

    ImGui::Separator();
    if (ImGui::Button("Configure Firmware...", ImVec2(175, 0)))
    {
        gui_adam_open_firmware();
        gui_dialog_in_use = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(100, 0)))
    {
        gui_dialog_in_use = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

static void draw_quit_confirmation(void)
{
    if (!ImGui::BeginPopupModal("Unsaved ADAM Media on Quit", NULL,
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    gui_dialog_in_use = true;
    ImGui::TextUnformatted("One or more ADAM working copies could not be saved.");
    ImGui::TextUnformatted("Choose Save As for affected media, then retry quitting.");
    ImGui::Separator();

    static const char* labels[GC_ADAM_MEDIA_SLOT_COUNT] = {
        "Disk 1", "Disk 2", "Data Pack 1", "Data Pack 2"
    };
    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
    {
        Emu_AdamMediaInfo info;
        emu_get_adam_media_info((GC_AdamMediaSlot)i, &info);
        if (!info.dirty)
            continue;

        ImGui::PushID(i);
        ImGui::Text("%s: %s", labels[i],
            info.path[0] ? get_filename(info.path) : "State snapshot");
        ImGui::SameLine();
        if (ImGui::Button("Save As..."))
            pending_save_as_slot = i;
        ImGui::PopID();
    }

    ImGui::Separator();
    if (ImGui::Button("Retry", ImVec2(100, 0)))
    {
        if (emu_flush_adam_media())
        {
            gui_dialog_in_use = false;
            ImGui::CloseCurrentPopup();
            application_confirm_quit();
        }
        else
            gui_set_error_message("Unable to save one or more ADAM working copies.");
    }
    ImGui::SameLine();
    if (ImGui::Button("Discard and Quit", ImVec2(140, 0)))
    {
        if (emu_discard_all_adam_media_changes())
        {
            gui_dialog_in_use = false;
            ImGui::CloseCurrentPopup();
            application_confirm_quit();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(100, 0)))
    {
        gui_dialog_in_use = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

static void draw_media_window(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(80, 80), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(1200, 320), ImGuiCond_FirstUseEver);
    ImGui::Begin("ADAM Media", &show_adam_media);
    ImGui::PushFont(gui_default_font);

    ImGui::Checkbox("Working-copy persistence", &config_emulator.adam_media_persistence);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Applies to newly inserted media. Source images are never overwritten.");

    if (ImGui::BeginTable("##adam_media", 7,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthFixed, 105.0f);
        ImGui::TableSetupColumn("Media");
        ImGui::TableSetupColumn("Working copy");
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 95.0f);
        ImGui::TableSetupColumn("Write protect", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 235.0f);
        ImGui::TableHeadersRow();

        draw_media_row(GC_ADAM_MEDIA_DISK_1, "Disk 1");
        draw_media_row(GC_ADAM_MEDIA_DISK_2, "Disk 2");
        draw_media_row(GC_ADAM_MEDIA_DATA_PACK_1, "Data Pack 1");
        draw_media_row(GC_ADAM_MEDIA_DATA_PACK_2, "Data Pack 2");
        ImGui::EndTable();
    }

    if (open_dirty_confirmation)
    {
        open_dirty_confirmation = false;
        ImGui::OpenPopup("Unsaved ADAM Media");
    }
    draw_dirty_confirmation();

    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleVar();
}

static void draw_media_row(GC_AdamMediaSlot slot, const char* label)
{
    Emu_AdamMediaInfo info;
    emu_get_adam_media_info(slot, &info);

    ImGui::PushID((int)slot);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(label);
    ImGui::TableNextColumn();
    if (info.inserted)
    {
        ImGui::TextUnformatted(info.path[0] ? get_filename(info.path) : "State snapshot");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            if (info.path[0])
                ImGui::TextWrapped("Source: %s", info.path);
            if (info.working_path[0])
                ImGui::TextWrapped("Working copy: %s", info.working_path);
            ImGui::Text("Base CRC32: %08X", info.base_crc);
            ImGui::EndTooltip();
        }

        int playlist_count = emu_get_adam_playlist_count(slot);
        if (playlist_count > 0)
        {
            int current = emu_get_adam_playlist_index(slot);
            ImGui::TextDisabled("M3U: %s", get_filename(emu_get_adam_playlist_path(slot)));
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::BeginCombo("##playlist_entry",
                emu_get_adam_playlist_name(slot, current)))
            {
                for (int i = 0; i < playlist_count; i++)
                {
                    bool selected = i == current;
                    if (ImGui::Selectable(emu_get_adam_playlist_name(slot, i), selected) &&
                        !selected)
                    {
                        if (info.dirty)
                        {
                            pending_dirty_slot = slot;
                            pending_swap_index = i;
                            pending_dirty_action = AdamMediaPendingSwap;
                            open_dirty_confirmation = true;
                        }
                        else if (!emu_select_adam_playlist_entry(slot, i, false))
                            gui_set_error_message("Unable to load the selected ADAM playlist entry.");
                    }
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }
    }
    else
        ImGui::TextDisabled("Empty");

    ImGui::TableNextColumn();
    if (!info.inserted)
        ImGui::TextDisabled("-");
    else if (info.working_path[0])
        ImGui::TextWrapped("%s", info.working_path);
    else if (info.state_owned)
        ImGui::TextDisabled("Save As required");
    else
        ImGui::TextDisabled("Disabled");

    ImGui::TableNextColumn();
    if (info.inserted)
        ImGui::Text("%zu KiB", info.size / 1024);
    else
        ImGui::TextDisabled("-");

    ImGui::TableNextColumn();
    if (info.dirty)
        ImGui::TextColored(orange, "Modified");
    else if (info.state_owned)
        ImGui::TextDisabled("State-owned");
    else if (info.inserted)
        ImGui::TextDisabled("Clean");
    else
        ImGui::TextDisabled("-");

    ImGui::TableNextColumn();
    bool write_protected = info.write_protected;
    if (info.inserted)
    {
        if (ImGui::Checkbox("##write_protected", &write_protected))
        {
            if (emu_set_adam_media_write_protected(slot, write_protected))
                config_emulator.adam_media_write_protected[slot] = write_protected;
        }
        if (info.state_owned && ImGui::IsItemHovered())
            ImGui::SetTooltip("Save the state snapshot to a file before enabling writes.");
    }
    else
        ImGui::TextDisabled("-");

    ImGui::TableNextColumn();
    if (ImGui::Button(info.inserted ? "Replace..." : "Insert..."))
    {
        if (info.dirty)
        {
            pending_dirty_slot = slot;
            pending_dirty_action = AdamMediaPendingReplace;
            open_dirty_confirmation = true;
        }
        else
        {
            pending_insert_discard_changes = false;
            pending_insert_slot = slot;
        }
    }

    if (info.inserted)
    {
        ImGui::SameLine();
        if (info.state_owned)
        {
            if (ImGui::Button("Save As..."))
                pending_save_as_slot = slot;
        }
        else
        {
            bool can_save = info.dirty && info.working_path[0];
            ImGui::BeginDisabled(!can_save);
            if (ImGui::Button("Save") && !emu_save_adam_media(slot))
                gui_set_error_message("Unable to save the ADAM working copy.");
            ImGui::EndDisabled();
        }

        ImGui::SameLine();
        if (ImGui::Button("Eject"))
        {
            if (info.dirty)
            {
                pending_dirty_slot = slot;
                pending_dirty_action = AdamMediaPendingEject;
                open_dirty_confirmation = true;
            }
            else if (!emu_eject_adam_media(slot))
                gui_set_error_message("Unable to eject ADAM media.");
        }
    }
    ImGui::PopID();
}

static void draw_dirty_confirmation(void)
{
    if (!ImGui::BeginPopupModal("Unsaved ADAM Media", NULL,
        ImGuiWindowFlags_AlwaysAutoResize))
    {
        return;
    }

    gui_dialog_in_use = true;
    ImGui::TextUnformatted("This media contains unsaved changes.");
    ImGui::Separator();

    if (ImGui::Button("Save and Continue", ImVec2(145, 0)))
        complete_pending_action(true);
    ImGui::SameLine();
    if (ImGui::Button("Discard", ImVec2(100, 0)))
        complete_pending_action(false);
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(100, 0)))
    {
        pending_dirty_slot = -1;
        pending_swap_index = -1;
        pending_dirty_action = AdamMediaPendingNone;
        gui_dialog_in_use = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

static void complete_pending_action(bool save)
{
    GC_AdamMediaSlot slot = (GC_AdamMediaSlot)pending_dirty_slot;
    bool ready = true;

    if (pending_dirty_action == AdamMediaPendingReplace)
    {
        if (save)
            ready = emu_save_adam_media(slot);
        if (ready)
        {
            pending_insert_discard_changes = !save;
            pending_insert_slot = slot;
        }
    }
    else if (pending_dirty_action == AdamMediaPendingEject)
    {
        if (save)
            ready = emu_eject_adam_media(slot);
        else
            ready = emu_discard_adam_media_changes(slot) && emu_eject_adam_media(slot);
    }
    else if (pending_dirty_action == AdamMediaPendingSwap)
    {
        if (save)
            ready = emu_save_adam_media(slot);
        if (ready)
            ready = emu_select_adam_playlist_entry(slot, pending_swap_index, !save);
    }

    if (!ready)
    {
        if (pending_dirty_action == AdamMediaPendingSwap)
            gui_set_error_message("Unable to load the selected ADAM playlist entry.");
        else
            gui_set_error_message(save ? "Unable to save the ADAM working copy." :
                "Unable to discard ADAM media changes.");
        return;
    }

    pending_dirty_slot = -1;
    pending_swap_index = -1;
    pending_dirty_action = AdamMediaPendingNone;
    gui_dialog_in_use = false;
    ImGui::CloseCurrentPopup();
}
