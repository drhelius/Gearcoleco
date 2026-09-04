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

#include "gui_adam.h"

#include "imgui.h"
#include "config.h"
#include "emu.h"
#include "gui.h"
#include "gui_filedialogs.h"
#include "gui_debug_constants.h"
#include "utils.h"

enum AdamMediaPendingAction
{
    AdamMediaPendingNone = 0,
    AdamMediaPendingReplace,
    AdamMediaPendingEject
};

static bool show_adam_media = false;
static bool open_dirty_confirmation = false;
static int pending_insert_slot = -1;
static bool pending_insert_discard_changes = false;
static int pending_save_as_slot = -1;
static int pending_dirty_slot = -1;
static AdamMediaPendingAction pending_dirty_action = AdamMediaPendingNone;

static void draw_media_window(void);
static void draw_media_row(GC_AdamMediaSlot slot, const char* label);
static void draw_dirty_confirmation(void);
static void complete_pending_action(bool save);

void gui_adam_open_media(void)
{
    show_adam_media = true;
}

void gui_adam_windows(void)
{
    if ((emu_get_machine() != GC_MACHINE_ADAM) || emu_is_empty())
    {
        show_adam_media = false;
        return;
    }

    if (show_adam_media)
        draw_media_window();

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
}

static void draw_media_window(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(80, 80), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(900, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("ADAM Media", &show_adam_media);
    ImGui::PushFont(gui_default_font);

    ImGui::Checkbox("Working-copy persistence", &config_emulator.adam_media_persistence);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Applies to newly inserted media. Source images are never overwritten.");

    if (ImGui::BeginTable("##adam_media", 6,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthFixed, 105.0f);
        ImGui::TableSetupColumn("Media");
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
    }
    else
        ImGui::TextDisabled("Empty");

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

    if (!ready)
    {
        gui_set_error_message(save ? "Unable to save the ADAM working copy." :
            "Unable to discard ADAM media changes.");
        return;
    }

    pending_dirty_slot = -1;
    pending_dirty_action = AdamMediaPendingNone;
    gui_dialog_in_use = false;
    ImGui::CloseCurrentPopup();
}
