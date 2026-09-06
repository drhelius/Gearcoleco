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

#ifndef GUI_ADAM_H
#define GUI_ADAM_H

#include "gearcoleco.h"

void gui_adam_prepare_window(float width, float height);
void gui_adam_keep_window_visible(void);
void gui_adam_media_menu(void);
void gui_adam_keyboard_menu(void);
bool gui_adam_menu_requested(void);
void gui_adam_request_menu(void);
bool gui_adam_select_media(GC_AdamMediaSlot slot, const char* first, const char* second,
    char* error = NULL, size_t error_size = 0);
void gui_adam_start(void);
void gui_adam_drop_media(const char* path, bool disk);
void gui_adam_remember_media(GC_AdamMediaSlot slot, const char* path);
void gui_adam_firmware_menu(const char* label, GC_AdamFirmware firmware);
void gui_adam_open_missing_firmware(bool adam);
void gui_adam_open_quit_confirmation(void);
void gui_adam_windows(void);

#endif /* GUI_ADAM_H */
