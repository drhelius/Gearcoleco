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

#ifndef CONFIG_DATA_H
#define CONFIG_DATA_H

#include <SDL3/SDL.h>
#include <string>
#include "gearcoleco.h"

static const int config_version = 7;
static const int config_minimum_version = 2;
static const int config_max_recent_roms = 15;
static const int config_memory_editor_count = 5;

enum config_ShaderMode
{
    config_ShaderMode_PixelPerfect = 0,
    config_ShaderMode_External = 1
};

enum config_Theme
{
    config_Theme_Light = 0,
    config_Theme_Dark = 1,
    config_Theme_Count = 2
};

enum config_VideoSync
{
    config_VideoSync_Disabled = 0,
    config_VideoSync_Fixed = 1,
    config_VideoSync_VRR = 2
};

struct config_Emulator
{
    bool maximized;
    bool fullscreen;
    int fullscreen_mode;
    bool always_show_menu;
    int theme;
    bool paused;
    int save_slot;
    bool start_paused;
    bool pause_when_inactive;
    bool softpatching;
    bool ffwd;
    int ffwd_speed;
    int runahead;
    int mapper;
    int region;
    bool show_info;
    std::string recent_roms[config_max_recent_roms];
    std::string bios_path;
    int savefiles_dir_option;
    std::string savefiles_path;
    int savestates_dir_option;
    std::string savestates_path;
    int screenshots_dir_option;
    std::string screenshots_path;
    std::string last_open_path;
    int window_width;
    int window_height;
    int spinner;
    int spinner_sensitivity;
    bool capture_mouse;
    bool status_messages;
    bool allow_screensaver;
    int mcp_tcp_port;
    std::string mcp_http_address;
};

struct config_Video
{
    int video_chip;
    int scale;
    int scale_manual;
    int ratio;
    int overscan;
    bool fps;
    bool sprite_limit;
    int sync_mode;
    float background_color[config_Theme_Count][3];
    float background_color_debugger[config_Theme_Count][3];
    int palette;
    int shader_mode;
    std::string shader_preset_path;
    GC_Color color[16];
};

struct config_Audio
{
    bool enable;
    bool sync;
    float master_volume;
    int buffer_count;
};

struct config_Rewind
{
    bool enabled;
    int buffer_seconds;
    int frames_per_snapshot;
    float speed;
};

struct config_Input
{
    SDL_Scancode key_left;
    SDL_Scancode key_right;
    SDL_Scancode key_up;
    SDL_Scancode key_down;
    SDL_Scancode key_left_button;
    SDL_Scancode key_right_button;
    SDL_Scancode key_blue;
    SDL_Scancode key_purple;
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
    bool allow_up_down;
    bool gamepad;
    int gamepad_directional;
    bool gamepad_invert_x_axis;
    bool gamepad_invert_y_axis;
    int gamepad_left_button;
    int gamepad_right_button;
    int gamepad_blue;
    int gamepad_purple;
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

enum config_HotkeyIndex
{
    config_HotkeyIndex_OpenROM = 0,
    config_HotkeyIndex_ReloadROM,
    config_HotkeyIndex_Quit,
    config_HotkeyIndex_Reset,
    config_HotkeyIndex_Pause,
    config_HotkeyIndex_FFWD,
    config_HotkeyIndex_SaveState,
    config_HotkeyIndex_LoadState,
    config_HotkeyIndex_Screenshot,
    config_HotkeyIndex_Fullscreen,
    config_HotkeyIndex_CaptureMouse,
    config_HotkeyIndex_ShowMainMenu,
    config_HotkeyIndex_DebugStepInto,
    config_HotkeyIndex_DebugStepOver,
    config_HotkeyIndex_DebugStepOut,
    config_HotkeyIndex_DebugStepFrame,
    config_HotkeyIndex_DebugContinue,
    config_HotkeyIndex_DebugBreak,
    config_HotkeyIndex_DebugRunToCursor,
    config_HotkeyIndex_DebugBreakpoint,
    config_HotkeyIndex_DebugGoBack,
    config_HotkeyIndex_SelectSlot1,
    config_HotkeyIndex_SelectSlot2,
    config_HotkeyIndex_SelectSlot3,
    config_HotkeyIndex_SelectSlot4,
    config_HotkeyIndex_SelectSlot5,
    config_HotkeyIndex_Rewind,
    config_HotkeyIndex_Mute,
    config_HotkeyIndex_COUNT
};

struct config_Input_Gamepad_Shortcuts
{
    int gamepad_shortcuts[config_HotkeyIndex_COUNT];
};

struct config_Hotkey
{
    SDL_Scancode key;
    SDL_Keymod mod;
    char str[64];
};

struct config_Debug
{
    bool debug;
    bool show_screen;
    bool show_disassembler;
    bool show_processor;
    bool show_call_stack;
    bool show_breakpoints;
    bool show_symbols;
    bool show_memory;
    bool show_video;
    bool show_tms9918a_nametable;
    bool show_tms9918a_patterns;
    bool show_tms9918a_sprites;
    bool show_tms9918a_palettes;
    bool show_f18a_nametables;
    bool show_f18a_patterns;
    bool show_f18a_sprites;
    bool show_f18a_palette;
    bool show_tms9918a_regs;
    bool show_f18a_regs;
    bool show_f18a_extended_regs;
    bool show_psg;
    bool show_ay8910;
    bool show_trace_logger;
    bool show_rewind;
    bool trace_counter;
    bool trace_cycles;
    bool trace_bank;
    bool trace_registers;
    bool trace_flags;
    bool trace_bytes;
    bool trace_cpu_enabled;
    bool trace_cpu;
    bool trace_cpu_irq;
    bool trace_psg;
    bool trace_ay8910;
    bool trace_sgm;
    bool trace_vdp;
    bool trace_input;
    bool trace_io;
    bool trace_mapper;
    int trace_vdp_events;
    int trace_psg_events;
    int trace_ay8910_events;
    int trace_io_events;
    int trace_input_events;
    int trace_sgm_events;
    int trace_mapper_events;
    int trace_output;
    int trace_capacity;
    int trace_disk_dir_option;
    int trace_disk_size;
    std::string trace_disk_path;
    bool dis_show_mem;
    bool dis_show_symbols;
    bool dis_show_segment;
    bool dis_show_bank;
    bool dis_show_auto_symbols;
    bool dis_dim_auto_symbols;
    bool dis_replace_symbols;
    bool dis_replace_labels;
    int dis_syntax;
    int dis_look_ahead_count;
    int font_size;
    int scale;
    bool multi_viewport;
    bool single_instance;
    bool auto_debug_settings;
    int mem_editor_bytes_per_row[config_memory_editor_count];
    int mem_editor_preview_data_type[config_memory_editor_count];
    int mem_editor_preview_endianess[config_memory_editor_count];
    bool mem_editor_uppercase_hex[config_memory_editor_count];
    bool mem_editor_gray_out_zeros[config_memory_editor_count];
};

EXTERN config_Emulator config_emulator;
EXTERN config_Video config_video;
EXTERN config_Audio config_audio;
EXTERN config_Rewind config_rewind;
EXTERN config_Input config_input[2];
EXTERN config_Input_Gamepad_Shortcuts config_input_gamepad_shortcuts[2];
EXTERN config_Hotkey config_hotkeys[config_HotkeyIndex_COUNT];
EXTERN config_Debug config_debug;

#endif /* CONFIG_DATA_H */
