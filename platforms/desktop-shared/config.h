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

#ifndef CONFIG_H
#define	CONFIG_H

#include <SDL.h>
#include "../../src/gearcoleco.h"
#define MINI_CASE_SENSITIVE
#include "mINI/ini.h"
#include "imgui/imgui.h"

#ifdef CONFIG_IMPORT
    #define EXTERN
#else
    #define EXTERN extern
#endif

static const int config_max_recent_roms = 10;

struct config_Emulator
{
    bool paused = false;
    int save_slot = 0;
    bool start_paused = false;
    bool ffwd = false;
    int ffwd_speed = 1;
    int region = 0;
    bool show_info = false;
    std::string recent_roms[config_max_recent_roms];
    std::string bios_path;
    int savefiles_dir_option = 0;
    std::string savefiles_path;
    int savestates_dir_option = 0;
    std::string savestates_path;
};

struct config_Video
{
    int scale = 0;
    int ratio = 0;
    bool fps = false;
    bool bilinear = false;
    bool mix_frames = true;
    float mix_frames_intensity = 0.30f;
    bool scanlines = true;
    float scanlines_intensity = 0.40f;
    bool sync = true;
    int palette = 0;
    GC_Color color[16] = {
        {24, 24, 24}, {0, 0, 0}, {33, 200, 66}, {94, 20, 120},
        {84, 85, 237}, {112, 118, 252}, {212, 82, 77}, {66, 235, 245},
        {252, 85, 84}, {255, 121, 120}, {212, 193, 84}, {230, 206, 128},
        {33, 176, 59}, {201, 91, 186}, {204, 204, 204}, {255, 255, 255}
    };
};

struct config_Audio
{
    bool enable = true;
    bool sync = true;
};

struct config_Input
{
    SDL_Scancode key_left;
    SDL_Scancode key_right;
    SDL_Scancode key_up;
    SDL_Scancode key_down;
    SDL_Scancode key_left_button;
    SDL_Scancode key_right_button;
    SDL_Scancode key_0;
    SDL_Scancode key_1;
    SDL_Scancode key_2;
    SDL_Scancode key_3;
    SDL_Scancode key_4;
    SDL_Scancode key_5;
    SDL_Scancode key_6;
    SDL_Scancode key_7;
    SDL_Scancode key_8;
    SDL_Scancode key_9;
    SDL_Scancode key_asterisk;
    SDL_Scancode key_hash;
    bool gamepad;
    int gamepad_directional;
    bool gamepad_invert_x_axis;
    bool gamepad_invert_y_axis;
    int gamepad_left_button;
    int gamepad_right_button;
    int gamepad_x_axis;
    int gamepad_y_axis;
    int gamepad_1;
    int gamepad_2;
    int gamepad_3;
    int gamepad_4;
    int gamepad_5;
    int gamepad_6;
    int gamepad_7;
    int gamepad_8;
    int gamepad_9;
    int gamepad_0;
    int gamepad_asterisk;
    int gamepad_hash;
};

struct config_Debug
{
    bool debug = false;
    bool show_screen = true;
    bool show_disassembler = true;
    bool show_processor = true;
    bool show_memory = true;
    bool show_video = false;
    bool show_video_registers = true;
    int font_size = 0;
};

EXTERN mINI::INIFile* config_ini_file;
EXTERN mINI::INIStructure config_ini_data;
EXTERN char* config_root_path;
EXTERN char config_emu_file_path[260];
EXTERN char config_imgui_file_path[260];
EXTERN config_Emulator config_emulator;
EXTERN config_Video config_video;
EXTERN config_Audio config_audio;
EXTERN config_Input config_input[2];
EXTERN config_Debug config_debug;

EXTERN void config_init(void);
EXTERN void config_destroy(void);
EXTERN void config_read(void);
EXTERN void config_write(void);

#undef CONFIG_IMPORT
#undef EXTERN
#endif	/* CONFIG_H */
