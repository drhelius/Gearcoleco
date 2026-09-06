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

#ifndef EMU_ADAM_H
#define EMU_ADAM_H

#include "gearcoleco.h"

enum EmuDesktopContentType
{
    EmuDesktopContentInvalid = 0,
    EmuDesktopContentCartridge,
    EmuDesktopContentAdamDisk,
    EmuDesktopContentAdamDataPack
};

struct EmuDesktopContent
{
    EmuDesktopContentType type;
    u8* data;
    size_t size;
    char source_path[4096];
    char entry_name[512];
    bool archive;
    bool playlist;
};

bool emu_adam_validate_media(GC_AdamMediaSlot slot, const char* path);
bool emu_adam_prepare_session(const char* const* paths);
bool emu_adam_commit_session(void);
void emu_adam_clear_session(void);
bool emu_swap_adam_disks(void);
void emu_adam_init(void);
void emu_adam_prepare_load(void);
bool emu_adam_load_firmware(void);
bool emu_adam_classify_content(const char* path, GC_Machine machine,
    EmuDesktopContent* content);
void emu_adam_destroy_content(EmuDesktopContent* content);
bool emu_adam_load_content(const EmuDesktopContent* content, const char* requested_path,
    int boot_mode, Cartridge::ForceConfiguration* config, bool softpatching);
void emu_adam_clear_host_media(void);
bool emu_adam_get_state_path(int index, char* path, size_t path_size);

#endif /* EMU_ADAM_H */
