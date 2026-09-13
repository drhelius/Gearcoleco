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
#include "emu_adam.h"
#include "gui_actions.h"
#include "gui.h"
#include "application.h"
#include "gui_menus.h"
#include "gui_filedialogs.h"
#include "gui_debug_constants.h"
#include "gui_debug_adam.h"
#include "gui_debug.h"
#include "events.h"
#include "keyboard.h"
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

static bool open_missing_firmware = false;
static bool missing_adam_firmware = false;
static bool open_quit_confirmation = false;
static bool open_dirty_confirmation = false;
static int pending_insert_slot = -1;
static bool reopen_adam_menu = false;
static bool open_keyboard_binding = false;
static bool keyboard_binding_ready = false;
static int keyboard_binding_index = -1;
static SDL_Scancode keyboard_binding_candidate = SDL_SCANCODE_UNKNOWN;
static bool open_drop_target = false;
static bool dropped_disk = false;
static char dropped_media[4096];
static char selected_media[GC_ADAM_MEDIA_SLOT_COUNT][4096];
static char queued_media[GC_ADAM_MEDIA_SLOT_COUNT][4096];
static int pending_save_as_slot = -1;
static int pending_firmware_browse = -1;
static int pending_dirty_slot = -1;
static int pending_swap_index = -1;
static AdamMediaPendingAction pending_dirty_action = AdamMediaPendingNone;
static AdamFirmwareInspection firmware_inspections[GC_ADAM_FIRMWARE_COUNT];

static void draw_media_drive(GC_AdamMediaSlot slot, const char* label);
static void draw_keyboard_binding(int index);
static void draw_keyboard_binding_popup(void);
static void process_media_queue(void);
static void draw_dirty_confirmation(void);
static void complete_pending_action(bool save);
static void draw_missing_firmware(void);
static void draw_quit_confirmation(void);
static void reset_firmware_paths(void);
static void refresh_firmware_inspection(GC_AdamFirmware firmware, const char* path);

void gui_adam_prepare_window(float width, float height)
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float available_width = viewport->WorkSize.x > 32.0f ? viewport->WorkSize.x - 32.0f : 1.0f;
    float available_height = viewport->WorkSize.y > 32.0f ? viewport->WorkSize.y - 32.0f : 1.0f;

    if (width > available_width)
        width = available_width;

    if (height > available_height)
        height = available_height;

    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + 16.0f, viewport->WorkPos.y + 16.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_FirstUseEver);

    if (!(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
    {
        ImGui::SetNextWindowSizeConstraints(ImVec2(1.0f, 1.0f),
            ImVec2(available_width, available_height));
    }
}

void gui_adam_keep_window_visible(void)
{
    if (ImGui::IsWindowDocked() || (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable))
        return;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 position = ImGui::GetWindowPos();
    ImVec2 original_position = position;
    ImVec2 size = ImGui::GetWindowSize();

    float x = viewport->WorkPos.x + 16.0f;
    float y = viewport->WorkPos.y + 16.0f;
    float max_x = viewport->WorkPos.x + viewport->WorkSize.x - size.x - 16.0f;
    float max_y = viewport->WorkPos.y + viewport->WorkSize.y - size.y - 16.0f;

    if (position.x < x)
        position.x = x;

    if (position.y < y)
        position.y = y;

    if (position.x > max_x && max_x >= x)
        position.x = max_x;

    if (position.y > max_y && max_y >= y)
        position.y = max_y;

    if ((position.x != original_position.x) || (position.y != original_position.y))
        ImGui::SetWindowPos(position);
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
    if (open_keyboard_binding)
    {
        open_keyboard_binding = false;
        ImGui::OpenPopup("ADAM Key Mapping");
    }

    draw_keyboard_binding_popup();
    process_media_queue();

    if (open_drop_target)
    {
        open_drop_target = false;
        ImGui::OpenPopup("Insert ADAM Media");
    }

    if (ImGui::BeginPopupModal("Insert ADAM Media", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        gui_dialog_in_use = true;
        ImGui::Text("Insert %.48s into:", get_filename(dropped_media));

        for (int i = 0; i < 2; i++)
        {
            if (i) ImGui::SameLine();
            const char* label = dropped_disk ? (i ? "Disk 2" : "Disk 1") : (i ? "Data Pack 2" : "Data Pack 1");

            if (ImGui::Button(label))
            {
                GC_AdamMediaSlot slot = (GC_AdamMediaSlot)((dropped_disk ? 0 : 2) + i);

                if (!gui_adam_select_media(slot, dropped_media, NULL))
                    gui_set_error_message("Unable to insert the dropped image.");

                dropped_media[0] = '\0';
                gui_dialog_in_use = false;
                ImGui::CloseCurrentPopup();

                break;
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel"))
        {
            dropped_media[0] = '\0';
            gui_dialog_in_use = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (config_debug.show_adam_printer && !emu_is_empty() && emu_get_machine() == GC_MACHINE_ADAM)
    {
        GC_AdamDebugState state;
        if (emu_get_core()->GetAdamDebugState(&state))
            gui_debug_window_adam_media_printer(&state);
    }

    if (open_dirty_confirmation)
    {
        open_dirty_confirmation = false;
        ImGui::OpenPopup("Unsaved ADAM Media");
    }

    draw_dirty_confirmation();

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
        gui_file_dialog_select_adam_media((GC_AdamMediaSlot)slot, false);
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
    emu_get_adam_firmware_path(GC_ADAM_FIRMWARE_OS7, gui_bios_path, sizeof(gui_bios_path));
    emu_get_adam_firmware_path(GC_ADAM_FIRMWARE_EOS, gui_adam_eos_path, sizeof(gui_adam_eos_path));
    emu_get_adam_firmware_path(GC_ADAM_FIRMWARE_SMARTWRITER, gui_adam_smartwriter_path, sizeof(gui_adam_smartwriter_path));

    memset(firmware_inspections, 0, sizeof(firmware_inspections));
    refresh_firmware_inspection(GC_ADAM_FIRMWARE_OS7, gui_bios_path);
    refresh_firmware_inspection(GC_ADAM_FIRMWARE_EOS, gui_adam_eos_path);
    refresh_firmware_inspection(GC_ADAM_FIRMWARE_SMARTWRITER, gui_adam_smartwriter_path);
}

static void refresh_firmware_inspection(GC_AdamFirmware firmware, const char* path)
{
    AdamFirmwareInspection* inspection = &firmware_inspections[firmware];
    strncpy_fit(inspection->path, path, sizeof(inspection->path));
    inspection->actual_size = 0;
    inspection->crc = 0;
    inspection->valid = emu_inspect_adam_firmware(firmware, path, &inspection->actual_size, &inspection->crc);
    inspection->readable = inspection->actual_size > 0;
}

bool gui_adam_apply_firmware_path(GC_AdamFirmware firmware, const char* path)
{
    if (!emu_load_adam_firmware(firmware, path))
    {
        const Adam::FirmwareMetadata* metadata = Adam::GetFirmwareMetadata(firmware);
        char message[256];
        snprintf(message, sizeof(message), "Invalid %s firmware. Expected exactly %d bytes.", metadata->role_name, metadata->size);
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
    bool running = !emu_is_empty() && emu_get_machine() == GC_MACHINE_ADAM;

    snprintf(message, sizeof(message), running ? "%s firmware selected for next power-on" : "%s firmware configured", metadata->role_name);

    gui_set_status_message(message, 3000);
    return true;
}

void gui_adam_firmware_menu(const char* label, GC_AdamFirmware firmware)
{
    if (!ImGui::BeginMenu(label))
        return;

    if (ImGui::IsWindowAppearing())
        reset_firmware_paths();

    char* path = firmware == GC_ADAM_FIRMWARE_OS7 ? gui_bios_path : (firmware == GC_ADAM_FIRMWARE_EOS ? gui_adam_eos_path : gui_adam_smartwriter_path);

    size_t path_size = firmware == GC_ADAM_FIRMWARE_OS7 ? sizeof(gui_bios_path) : (firmware == GC_ADAM_FIRMWARE_EOS ? sizeof(gui_adam_eos_path) : sizeof(gui_adam_smartwriter_path));

    bool adam_running = !emu_is_empty() && emu_get_machine() == GC_MACHINE_ADAM;

    ImGui::BeginDisabled(emu_is_busy());

    if (ImGui::MenuItem(firmware == GC_ADAM_FIRMWARE_OS7 ? "Load BIOS..." : "Load ROM..."))
        pending_firmware_browse = firmware;

    ImGui::SetNextItemWidth(350.0f);

    bool apply = ImGui::InputText("##firmware_path", path, path_size, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);

    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("%s\nPress Enter to apply the path.", path);

    AdamFirmwareInspection* inspection = &firmware_inspections[firmware];

    if ((apply || !ImGui::IsItemActive()) && strcmp(inspection->path, path))
        refresh_firmware_inspection(firmware, path);

    if (apply && inspection->valid)
        gui_adam_apply_firmware_path(firmware, path);

    ImGui::EndDisabled();

    ImGui::Separator();

    bool loaded = firmware == GC_ADAM_FIRMWARE_OS7 ? emu_is_bios_loaded() :
        emu_is_adam_firmware_loaded(firmware);

    if (loaded)
        ImGui::TextColored(green, firmware == GC_ADAM_FIRMWARE_OS7 ? "BIOS loaded" : "ROM loaded");
    else
        ImGui::TextDisabled(firmware == GC_ADAM_FIRMWARE_OS7 ? "No BIOS loaded" : "No ROM loaded");

    if (!inspection->valid)
        ImGui::TextColored(orange, "Selected path is missing or invalid");
    if (adam_running)
        ImGui::TextDisabled("Changes apply on the next Power On.");

    ImGui::EndMenu();
}

static void draw_missing_firmware(void)
{
    if (!ImGui::BeginPopupModal("Firmware Required", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        return;

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
            ImGui::BulletText("%s: %s", metadata->role_name, valid ? get_filename(path) : "missing or invalid");
        }
    }
    else
    {
        ImGui::TextUnformatted("ColecoVision requires a valid 8 KiB OS-7 BIOS.");
    }

    ImGui::Separator();

    if (ImGui::Button("Configure Firmware...", ImVec2(175, 0)))
    {
        reset_firmware_paths();
        int count = missing_adam_firmware ? GC_ADAM_FIRMWARE_COUNT : 1;

        for (int i = 0; i < count; i++)
        {
            if (!firmware_inspections[i].valid)
            {
                pending_firmware_browse = i;
                break;
            }
        }

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
    if (!ImGui::BeginPopupModal("Unsaved ADAM Media on Quit", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        return;

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
        ImGui::Text("%s: %s", labels[i], info.path[0] ? get_filename(info.path) : "State snapshot");
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

static const char* keyboard_key_name(SDL_Scancode key)
{
    return key == SDL_SCANCODE_UNKNOWN ? "Unassigned" :
        SDL_GetKeyName(SDL_GetKeyFromScancode(key, SDL_KMOD_NONE, false));
}

static int keyboard_binding_duplicate(int index, SDL_Scancode key)
{
    if (key != SDL_SCANCODE_UNKNOWN)
    {
        for (int i = 0; i < config_adam_key_count; i++)
        {
            if (i != index && config_emulator.adam_keys[i] == key)
                return i;
        }
    }
    return -1;
}

static void open_key_binding(int index, SDL_Scancode candidate)
{
    events_release_adam_keys();
    keyboard_binding_index = index;
    keyboard_binding_candidate = candidate;
    keyboard_binding_ready = false;
    open_keyboard_binding = true;
}

void gui_adam_keyboard_menu(void)
{
    if (ImGui::BeginMenu("SmartKeys"))
    {
        for (int i = 0; i < 6; i++)
            draw_keyboard_binding(i);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Special Keys"))
    {
        for (int i = 6; i < config_adam_key_count; i++)
            draw_keyboard_binding(i);
        ImGui::EndMenu();
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Restore Defaults"))
    {
        events_release_adam_keys();
        for (int i = 0; i < config_adam_key_count; i++)
            config_emulator.adam_keys[i] = config_adam_keys[i].default_scancode;
    }
}

static void draw_keyboard_binding(int index)
{
    const config_AdamKeyDefinition* definition = &config_adam_keys[index];
    SDL_Scancode key = config_emulator.adam_keys[index];
    const char* reserved = events_adam_reserved_key(key);
    int duplicate = keyboard_binding_duplicate(index, key);
    bool conflict = reserved || duplicate >= 0;
    ImGui::PushID(index);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s:", definition->name);
    ImGui::SameLine(145.0f);

    if (conflict)
        ImGui::PushStyleColor(ImGuiCol_Text, orange);

    if (ImGui::Button(keyboard_key_name(key), ImVec2(125.0f, 0)))
        open_key_binding(index, SDL_SCANCODE_UNKNOWN);

    if (conflict)
        ImGui::PopStyleColor();

    if (ImGui::IsItemHovered())
    {
        if (reserved)
            ImGui::SetTooltip("Conflicts with %s. Assign another key or change that shortcut.", reserved);
        else if (duplicate >= 0)
            ImGui::SetTooltip("Also assigned to %s.", config_adam_keys[duplicate].name);
        else
            ImGui::SetTooltip("Default: %s", keyboard_key_name(definition->default_scancode));
    }

    ImGui::SameLine();

    if (ImGui::Button("X"))
    {
        events_release_adam_keys();
        config_emulator.adam_keys[index] = SDL_SCANCODE_UNKNOWN;
    }

    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Clear binding");

    ImGui::SameLine();

    if (ImGui::Button("Reset"))
        open_key_binding(index, definition->default_scancode);

    ImGui::PopID();
}

static void draw_keyboard_binding_popup(void)
{
    if (!ImGui::BeginPopupModal("ADAM Key Mapping", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        return;

    gui_dialog_in_use = true;

    ImGui::Text("ADAM key: %s", config_adam_keys[keyboard_binding_index].name);

    if (keyboard_binding_candidate == SDL_SCANCODE_UNKNOWN)
    {
        SDL_Scancode pressed = keyboard_get_first_pressed_scancode();

        if (!keyboard_binding_ready && pressed == SDL_SCANCODE_UNKNOWN)
            keyboard_binding_ready = true;

        else if (keyboard_binding_ready && pressed != SDL_SCANCODE_UNKNOWN)
            keyboard_binding_candidate = pressed;

        ImGui::TextUnformatted("Release held keys, then press the host key to use.");
    }
    else
    {
        ImGui::Text("Host key: %s", keyboard_key_name(keyboard_binding_candidate));

        if (ImGui::Button("Choose Another Key"))
        {
            keyboard_binding_candidate = SDL_SCANCODE_UNKNOWN;
            keyboard_binding_ready = false;
        }
    }

    const char* reserved = events_adam_reserved_key(keyboard_binding_candidate);

    int duplicate = keyboard_binding_duplicate(keyboard_binding_index, keyboard_binding_candidate);

    if (reserved)
        ImGui::TextColored(orange, "Reserved for %s. Choose another key.", reserved);
    else if (duplicate >= 0)
        ImGui::TextColored(orange, "Reassigning will clear the binding for %s.", config_adam_keys[duplicate].name);

    if (keyboard_binding_candidate != SDL_SCANCODE_UNKNOWN && !reserved && events_adam_typing_key(keyboard_binding_candidate) != GC_ADAM_KEY_COUNT)
        ImGui::TextColored(orange, "This will replace the key's normal ADAM typing function.");

    ImGui::Separator();

    ImGui::BeginDisabled(keyboard_binding_candidate == SDL_SCANCODE_UNKNOWN || reserved);

    if (ImGui::Button(duplicate >= 0 ? "Reassign" : "Assign", ImVec2(110.0f, 0)))
    {
        events_release_adam_keys();
        for (int i = 0; i < config_adam_key_count; i++)
        {
            if (config_emulator.adam_keys[i] == keyboard_binding_candidate)
                config_emulator.adam_keys[i] = SDL_SCANCODE_UNKNOWN;
        }

        config_emulator.adam_keys[keyboard_binding_index] = keyboard_binding_candidate;
        gui_dialog_in_use = false;
        ImGui::CloseCurrentPopup();
        reopen_adam_menu = true;
    }

    ImGui::EndDisabled();
    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(110.0f, 0)))
    {
        gui_dialog_in_use = false;
        ImGui::CloseCurrentPopup();
        reopen_adam_menu = true;
    }

    ImGui::EndPopup();
}

bool gui_adam_menu_requested(void)
{
    bool requested = reopen_adam_menu;
    reopen_adam_menu = false;
    return requested;
}

void gui_adam_request_menu(void)
{
    reopen_adam_menu = true;
}

void gui_adam_remember_media(GC_AdamMediaSlot slot, const char* path)
{
    int index = 0;

    while (index < 4 && config_emulator.adam_recent_media[slot][index] != path)
        index++;

    for (; index > 0; index--)
        config_emulator.adam_recent_media[slot][index] = config_emulator.adam_recent_media[slot][index - 1];

    config_emulator.adam_recent_media[slot][0] = path;
}

bool gui_adam_select_media(GC_AdamMediaSlot slot, const char* first, const char* second, char* error, size_t error_size)
{
    if (!first || !first[0] || !emu_adam_validate_media(slot, first, error, error_size))
        return false;

    if (second && second[0])
    {
        if ((slot != GC_ADAM_MEDIA_DISK_1 && slot != GC_ADAM_MEDIA_DATA_PACK_1) ||
            !emu_adam_validate_media((GC_AdamMediaSlot)(slot + 1), second, error, error_size))
            return false;
    }

    strncpy_fit(queued_media[slot], first, sizeof(queued_media[slot]));

    if (second && second[0])
        strncpy_fit(queued_media[slot + 1], second, sizeof(queued_media[slot + 1]));

    return true;
}

static void process_media_queue(void)
{
    if (pending_dirty_action != AdamMediaPendingNone || emu_is_busy())
        return;

    bool adam = emu_get_machine() == GC_MACHINE_ADAM;

    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
    {
        if (!queued_media[i][0])
            continue;

        GC_AdamMediaSlot slot = (GC_AdamMediaSlot)i;

        if (adam)
        {
            Emu_AdamMediaInfo info;
            emu_get_adam_media_info(slot, &info);

            if (info.dirty)
            {
                pending_dirty_slot = i;
                pending_dirty_action = AdamMediaPendingReplace;
                open_dirty_confirmation = true;
                return;
            }

            if (!emu_replace_adam_media(slot, queued_media[i], false))
            {
                memset(queued_media, 0, sizeof(queued_media));
                gui_set_error_message("Unable to insert ADAM media. The current image was kept.");
                return;
            }
        }
        else
            strncpy_fit(selected_media[i], queued_media[i], sizeof(selected_media[i]));

        gui_adam_remember_media(slot, queued_media[i]);

        queued_media[i][0] = '\0';
        reopen_adam_menu = true;
    }
}

void gui_adam_drop_media(const char* path, bool disk)
{
    if (dropped_media[0])
    {
        gui_set_status_message("Choose a drive for the previous dropped image first.", 3000);
        return;
    }

    int first = disk ? GC_ADAM_MEDIA_DISK_1 : GC_ADAM_MEDIA_DATA_PACK_1;
    bool adam = emu_get_machine() == GC_MACHINE_ADAM;

    for (int i = first; i < first + 2; i++)
    {
        Emu_AdamMediaInfo info = {};
        if (adam)
            emu_get_adam_media_info((GC_AdamMediaSlot)i, &info);

        bool occupied = adam ? info.inserted : selected_media[i][0] != '\0';

        if (!occupied && !queued_media[i][0])
        {
            if (!gui_adam_select_media((GC_AdamMediaSlot)i, path, NULL))
                gui_set_error_message("Unable to insert the dropped image.");
            return;
        }
    }

    strncpy_fit(dropped_media, path, sizeof(dropped_media));
    dropped_disk = disk;
    open_drop_target = true;
}

void gui_adam_start(void)
{
    if (!emu_is_empty() && emu_get_machine() == GC_MACHINE_ADAM)
    {
        gui_action_reset();
        return;
    }

    if (!emu_are_adam_firmware_paths_valid())
    {
        gui_adam_open_missing_firmware(true);
        return;
    }

    const char* paths[GC_ADAM_MEDIA_SLOT_COUNT];

    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
        paths[i] = selected_media[i];

    if (!gui_start_adam(emu_get_machine() == GC_MACHINE_ADAM ? NULL : paths))
        gui_set_error_message("Unable to start ADAM. Check the selected images and save directory.");
}

void gui_adam_update_title(void)
{
    if (emu_is_busy() || emu_get_machine() != GC_MACHINE_ADAM)
        return;

    if (emu_is_empty())
    {
        application_reset_title();
        return;
    }

    if (emu_get_core()->GetAdamBootMode() == GC_ADAM_BOOT_CARTRIDGE)
    {
        application_update_title_with_rom(emu_get_content_name());
        return;
    }

    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
    {
        Emu_AdamMediaInfo info;
        if (emu_get_adam_media_info((GC_AdamMediaSlot)i, &info) && info.inserted)
        {
            application_update_title_with_rom(info.path[0] ? get_filename(info.path) : "Saved-state image");
            return;
        }
    }

    application_update_title_with_rom("ADAM");
}

void gui_adam_power_off(void)
{
    gui_debug_auto_save_settings();

    if (!emu_power_off_adam())
    {
        gui_set_error_message("Unable to power off ADAM because media changes could not be saved. Use Save As for modified images, then try again.");
        return;
    }

    application_reset_title();
    gui_set_status_message("ADAM powered off", 3000);
}

void gui_adam_controller_menu(void)
{
    static const char* names[] = { "Not Connected", "Hand Controller (Keyboard)", "Hand Controller (Gamepad)" };
    static const char* sources[] = { "None", "Keyboard", "Gamepad" };

    for (int controller = 0; controller < 2; controller++)
    {
        char label[32];
        snprintf(label, sizeof(label), "Controller %d", controller + 1);
        int selected = config_emulator.adam_controller[controller];

        if (ImGui::BeginMenu(label))
        {
            ImGui::PushItemWidth(260.0f);

            if (ImGui::Combo("##controller", &config_emulator.adam_controller[controller], names, IM_ARRAYSIZE(names)))
            {
                if (emu_get_machine() == GC_MACHINE_ADAM)
                    events_sync_input();
            }

            selected = config_emulator.adam_controller[controller];

            if (selected == config_AdamController_Keyboard && ImGui::IsItemHovered())
            {
                char keypad_key[64];
                strncpy_fit(keypad_key, keyboard_key_name(config_input[controller].key_1), sizeof(keypad_key));
                ImGui::SetTooltip("Uses Player %d bindings from Input > Keyboard Configuration.\nKeypad 1: %s. Fire: %s.\nMapped keys control this controller; other keys still type on ADAM.", controller + 1, keypad_key, keyboard_key_name(config_input[controller].key_left_button));
            }

            if (selected == config_AdamController_Gamepad && ImGui::IsItemHovered())
                ImGui::SetTooltip("Uses the device and bindings selected in Input > Gamepads > Player %d.", controller + 1);

            ImGui::PopItemWidth();
            ImGui::EndMenu();
        }

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Port %d: %s", controller + 1, sources[selected]);
    }
}

void gui_adam_media_menu(void)
{
    draw_media_drive(GC_ADAM_MEDIA_DISK_1, "Disk 1");
    draw_media_drive(GC_ADAM_MEDIA_DISK_2, "Disk 2");
    draw_media_drive(GC_ADAM_MEDIA_DATA_PACK_1, "Data Pack 1");
    draw_media_drive(GC_ADAM_MEDIA_DATA_PACK_2, "Data Pack 2");

    if (ImGui::MenuItem("Swap Disks 1 and 2"))
    {
        if (emu_get_machine() == GC_MACHINE_ADAM)
        {
            if (!emu_swap_adam_disks())
                gui_set_error_message("Unable to swap disks. Save any modified images first.");
        }
        else
        {
            char path[4096];
            strncpy_fit(path, selected_media[0], sizeof(path));
            strncpy_fit(selected_media[0], selected_media[1], sizeof(selected_media[0]));
            strncpy_fit(selected_media[1], path, sizeof(selected_media[1]));
            bool protected_media = config_emulator.adam_media_write_protected[0];
            config_emulator.adam_media_write_protected[0] = config_emulator.adam_media_write_protected[1];
            config_emulator.adam_media_write_protected[1] = protected_media;
        }
    }
}

static void draw_media_drive(GC_AdamMediaSlot slot, const char* label)
{
    bool adam = emu_get_machine() == GC_MACHINE_ADAM;
    Emu_AdamMediaInfo info = {};

    if (adam)
        emu_get_adam_media_info(slot, &info);

    const char* path = adam ? info.path : selected_media[slot];
    bool inserted = adam ? info.inserted : path[0] != '\0';

    if (!ImGui::BeginMenu(label))
        return;

    if (ImGui::MenuItem("Insert..."))
        pending_insert_slot = slot;

    if (ImGui::MenuItem("Eject", NULL, false, inserted))
    {
        if (!adam)
            selected_media[slot][0] = '\0';
        else if (info.dirty)
        {
            pending_dirty_slot = slot;
            pending_dirty_action = AdamMediaPendingEject;
            open_dirty_confirmation = true;
        }
        else if (!emu_eject_adam_media(slot))
            gui_set_error_message("Unable to eject ADAM media.");
    }

    ImGui::Separator();

    if (inserted)
    {
        const char* name = path[0] ? get_filename(path) : "Saved-state image";
        ImGui::Text("%.32s%s", name, strlen(name) > 32 ? "..." : "");

        if (ImGui::IsItemHovered() && path[0])
            ImGui::SetTooltip("%s", path);

        if (info.dirty)
            ImGui::TextColored(orange, "Unsaved changes");
    }
    else
        ImGui::TextDisabled("Empty");

    ImGui::Separator();

    bool protected_media = adam ? info.write_protected : config_emulator.adam_media_write_protected[slot];

    if (ImGui::MenuItem("Write Protected", NULL, &protected_media))
    {
        if (!adam || !inserted || emu_set_adam_media_write_protected(slot, protected_media))
            config_emulator.adam_media_write_protected[slot] = protected_media;
    }

    if (ImGui::MenuItem("Save Changes", NULL, false, adam && info.dirty && info.working_path[0]))
    {
        if (!emu_save_adam_media(slot))
            gui_set_error_message("Unable to save ADAM media changes.");
    }

    if (ImGui::MenuItem("Save As...", NULL, false, adam && inserted))
        pending_save_as_slot = slot;

    if (ImGui::BeginMenu("Recent Images"))
    {
        for (int i = 0; i < 5; i++)
        {
            const char* recent = config_emulator.adam_recent_media[slot][i].c_str();

            if (!recent[0])
                continue;

            ImGui::PushID(i);

            if (ImGui::MenuItem(get_filename(recent)) && !gui_adam_select_media(slot, recent, NULL))
                gui_set_error_message("Unable to open the recent image.");

            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", recent);

            ImGui::PopID();
        }
        ImGui::EndMenu();
    }

    int count = adam ? emu_get_adam_playlist_count(slot) : 0;

    if (count > 0 && ImGui::BeginMenu("Playlist"))
    {
        int current = emu_get_adam_playlist_index(slot);

        for (int i = 0; i < count; i++)
        {
            ImGui::PushID(i);

            if (ImGui::MenuItem(emu_get_adam_playlist_name(slot, i), NULL, i == current) && i != current)
            {
                if (info.dirty)
                {
                    pending_dirty_slot = slot;
                    pending_swap_index = i;
                    pending_dirty_action = AdamMediaPendingSwap;
                    open_dirty_confirmation = true;
                }
                else if (!emu_select_adam_playlist_entry(slot, i, false))
                    gui_set_error_message("Unable to load the selected playlist image.");
            }

            ImGui::PopID();
        }

        ImGui::EndMenu();
    }

    ImGui::EndMenu();
}

static void draw_dirty_confirmation(void)
{
    if (!ImGui::BeginPopupModal("Unsaved ADAM Media", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        return;

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
        memset(queued_media, 0, sizeof(queued_media));
        reopen_adam_menu = true;
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
            ready = emu_replace_adam_media(slot, queued_media[slot], !save);

            if (ready)
            {
                gui_adam_remember_media(slot, queued_media[slot]);
                queued_media[slot][0] = '\0';
            }
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

    reopen_adam_menu = true;
    pending_dirty_slot = -1;
    pending_swap_index = -1;
    pending_dirty_action = AdamMediaPendingNone;
    gui_dialog_in_use = false;
    ImGui::CloseCurrentPopup();
}
