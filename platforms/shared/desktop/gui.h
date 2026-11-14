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

#ifndef GUI_H
#define GUI_H

#include <SDL3/SDL.h>
#include "imgui.h"
#include "config.h"

#ifdef GUI_IMPORT
    #define EXTERN
#else
    #define EXTERN extern
#endif

EXTERN bool gui_in_use;
EXTERN bool gui_main_window_hovered;
EXTERN bool gui_main_window_focused;
EXTERN SDL_WindowID gui_main_window_sdl_window_id;
EXTERN bool gui_main_menu_hovered;
EXTERN ImFont* gui_default_font;
EXTERN ImFont* gui_default_fonts[4];
EXTERN ImFont* gui_roboto_font;
EXTERN ImFont* gui_material_icons_font;
EXTERN int gui_main_window_width;
EXTERN int gui_main_window_height;
EXTERN int gui_main_menu_height;
EXTERN SDL_Scancode* gui_configured_key;
EXTERN int* gui_configured_button;
EXTERN config_Hotkey* gui_configured_hotkey;
EXTERN bool gui_dialog_in_use;
EXTERN bool gui_shortcut_open_rom;

EXTERN ImVec4 gui_custom_palette[16];

EXTERN bool gui_init(void);
EXTERN void gui_apply_settings(void);
EXTERN void gui_destroy(void);
EXTERN void gui_render(void);
EXTERN void gui_hotkey(config_HotkeyIndex hotkey, bool pressed);
EXTERN bool gui_load_rom(const char* path, const char* symbol_path = NULL);
EXTERN bool gui_is_cartridge_file(const char* path);
EXTERN bool gui_open_rom(const char* path, bool adam_cartridge = false);
EXTERN void gui_drop_media(const char* path);
EXTERN bool gui_start_adam(const char* const* media_paths = NULL);
EXTERN bool gui_is_rom_loading(void);
EXTERN bool gui_finish_loading_rom(void);
EXTERN void gui_load_state_after_media(int slot, const char* path);
EXTERN Cartridge::ForceConfiguration gui_get_force_configuration(void);
EXTERN void gui_set_style(void);
EXTERN void gui_set_error_message(const char* message);
EXTERN void gui_center_next_window(void);
EXTERN bool gui_has_open_window(void);
EXTERN void gui_update_palette(void);

#undef GUI_IMPORT
#undef EXTERN
#endif /* GUI_H */
