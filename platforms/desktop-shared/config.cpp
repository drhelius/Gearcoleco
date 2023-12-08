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

#include <SDL.h>
#include "../../src/gearcoleco.h"

#define MINI_CASE_SENSITIVE
#include "mINI/ini.h"

#define CONFIG_IMPORT
#include "config.h"

static bool check_portable(void);
static int read_int(const char* group, const char* key, int default_value);
static void write_int(const char* group, const char* key, int integer);
static float read_float(const char* group, const char* key, float default_value);
static void write_float(const char* group, const char* key, float value);
static bool read_bool(const char* group, const char* key, bool default_value);
static void write_bool(const char* group, const char* key, bool boolean);
static std::string read_string(const char* group, const char* key);
static void write_string(const char* group, const char* key, std::string value);

void config_init(void)
{
    if (check_portable())
        config_root_path = SDL_GetBasePath();
    else
        config_root_path = SDL_GetPrefPath("Geardome", GEARCOLECO_TITLE);

    strcpy(config_emu_file_path, config_root_path);
    strcat(config_emu_file_path, "config.ini");

    strcpy(config_imgui_file_path, config_root_path);
    strcat(config_imgui_file_path, "imgui.ini");

    config_input[0].key_left = SDL_SCANCODE_LEFT;
    config_input[0].key_right = SDL_SCANCODE_RIGHT;
    config_input[0].key_up = SDL_SCANCODE_UP;
    config_input[0].key_down = SDL_SCANCODE_DOWN;
    config_input[0].key_left_button = SDL_SCANCODE_A;
    config_input[0].key_right_button = SDL_SCANCODE_S;
    config_input[0].key_0 = SDL_SCANCODE_KP_0;
    config_input[0].key_1 = SDL_SCANCODE_KP_1;
    config_input[0].key_2 = SDL_SCANCODE_KP_2;
    config_input[0].key_3 = SDL_SCANCODE_KP_3;
    config_input[0].key_4 = SDL_SCANCODE_KP_4;
    config_input[0].key_5 = SDL_SCANCODE_KP_5;
    config_input[0].key_6 = SDL_SCANCODE_KP_6;
    config_input[0].key_7 = SDL_SCANCODE_KP_7;
    config_input[0].key_8 = SDL_SCANCODE_KP_8;
    config_input[0].key_9 = SDL_SCANCODE_KP_9;
    config_input[0].key_asterisk = SDL_SCANCODE_KP_MULTIPLY;
    config_input[0].key_hash = SDL_SCANCODE_KP_DIVIDE;
    config_input[0].gamepad = true;
    config_input[0].gamepad_invert_x_axis = false;
    config_input[0].gamepad_invert_y_axis = false;
    config_input[0].gamepad_left_button = SDL_CONTROLLER_BUTTON_A;
    config_input[0].gamepad_right_button = SDL_CONTROLLER_BUTTON_B;
    config_input[0].gamepad_x_axis = 0;
    config_input[0].gamepad_y_axis = 1;
    config_input[0].gamepad_1 = SDL_CONTROLLER_BUTTON_X;
    config_input[0].gamepad_2 = SDL_CONTROLLER_BUTTON_Y;
    config_input[0].gamepad_3 = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
    config_input[0].gamepad_4 = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
    config_input[0].gamepad_5 = SDL_CONTROLLER_BUTTON_RIGHTSTICK;
    config_input[0].gamepad_6 = SDL_CONTROLLER_BUTTON_LEFTSTICK;
    config_input[0].gamepad_7 = SDL_CONTROLLER_BUTTON_GUIDE;
    config_input[0].gamepad_8 = SDL_CONTROLLER_BUTTON_GUIDE;
    config_input[0].gamepad_9 = SDL_CONTROLLER_BUTTON_GUIDE;
    config_input[0].gamepad_0 = SDL_CONTROLLER_BUTTON_GUIDE;
    config_input[0].gamepad_asterisk = SDL_CONTROLLER_BUTTON_START;
    config_input[0].gamepad_hash = SDL_CONTROLLER_BUTTON_BACK;

    config_input[1].key_left = SDL_SCANCODE_J;
    config_input[1].key_right = SDL_SCANCODE_L;
    config_input[1].key_up = SDL_SCANCODE_I;
    config_input[1].key_down = SDL_SCANCODE_K;
    config_input[1].key_left_button = SDL_SCANCODE_G;
    config_input[1].key_right_button = SDL_SCANCODE_H;
    config_input[1].key_0 = SDL_SCANCODE_NONUSBACKSLASH;
    config_input[1].key_1 = SDL_SCANCODE_Z;
    config_input[1].key_2 = SDL_SCANCODE_X;
    config_input[1].key_3 = SDL_SCANCODE_C;
    config_input[1].key_4 = SDL_SCANCODE_V;
    config_input[1].key_5 = SDL_SCANCODE_B;
    config_input[1].key_6 = SDL_SCANCODE_N;
    config_input[1].key_7 = SDL_SCANCODE_M;
    config_input[1].key_8 = SDL_SCANCODE_COMMA;
    config_input[1].key_9 = SDL_SCANCODE_PERIOD;
    config_input[1].key_asterisk = SDL_SCANCODE_SLASH;
    config_input[1].key_hash = SDL_SCANCODE_RSHIFT;
    config_input[1].gamepad = true;
    config_input[1].gamepad_invert_x_axis = false;
    config_input[1].gamepad_invert_y_axis = false;
    config_input[1].gamepad_left_button = SDL_CONTROLLER_BUTTON_A;
    config_input[1].gamepad_right_button = SDL_CONTROLLER_BUTTON_B;
    config_input[1].gamepad_x_axis = 0;
    config_input[1].gamepad_y_axis = 1;
    config_input[1].gamepad_1 = SDL_CONTROLLER_BUTTON_X;
    config_input[1].gamepad_2 = SDL_CONTROLLER_BUTTON_Y;
    config_input[1].gamepad_3 = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
    config_input[1].gamepad_4 = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
    config_input[1].gamepad_5 = SDL_CONTROLLER_BUTTON_RIGHTSTICK;
    config_input[1].gamepad_6 = SDL_CONTROLLER_BUTTON_LEFTSTICK;
    config_input[1].gamepad_7 = SDL_CONTROLLER_BUTTON_GUIDE;
    config_input[1].gamepad_8 = SDL_CONTROLLER_BUTTON_GUIDE;
    config_input[1].gamepad_9 = SDL_CONTROLLER_BUTTON_GUIDE;
    config_input[1].gamepad_0 = SDL_CONTROLLER_BUTTON_GUIDE;
    config_input[1].gamepad_asterisk = SDL_CONTROLLER_BUTTON_START;
    config_input[1].gamepad_hash = SDL_CONTROLLER_BUTTON_BACK;

    config_ini_file = new mINI::INIFile(config_emu_file_path);
}

void config_destroy(void)
{
    SafeDelete(config_ini_file)
    SDL_free(config_root_path);
}

void config_read(void)
{
    if (!config_ini_file->read(config_ini_data))
    {
        Log("Unable to load settings from %s", config_emu_file_path);
        return;
    }

    Log("Loading settings from %s", config_emu_file_path);

    config_debug.debug = read_bool("Debug", "Debug", false);
    config_debug.show_disassembler = read_bool("Debug", "Disassembler", true);
    config_debug.show_screen = read_bool("Debug", "Screen", true);
    config_debug.show_memory = read_bool("Debug", "Memory", true);
    config_debug.show_processor = read_bool("Debug", "Processor", true);
    config_debug.show_video = read_bool("Debug", "Video", false);
    config_debug.show_video_registers = read_bool("Debug", "VideoRegisters", false);
    config_debug.font_size = read_int("Debug", "FontSize", 0);

    config_emulator.fullscreen = read_bool("Emulator", "FullScreen", false);
    config_emulator.show_menu = read_bool("Emulator", "ShowMenu", true);
    config_emulator.ffwd_speed = read_int("Emulator", "FFWD", 1);
    config_emulator.save_slot = read_int("Emulator", "SaveSlot", 0);
    config_emulator.start_paused = read_bool("Emulator", "StartPaused", false);
    config_emulator.region = read_int("Emulator", "Region", 0);
    config_emulator.bios_path = read_string("Emulator", "BiosPath");
    config_emulator.savefiles_dir_option = read_int("Emulator", "SaveFilesDirOption", 0);
    config_emulator.savefiles_path = read_string("Emulator", "SaveFilesPath");
    config_emulator.savestates_dir_option = read_int("Emulator", "SaveStatesDirOption", 0);
    config_emulator.savestates_path = read_string("Emulator", "SaveStatesPath");
    config_emulator.last_open_path = read_string("Emulator", "LastOpenPath");
    config_emulator.window_width = read_int("Emulator", "WindowWidth", 770);
    config_emulator.window_height = read_int("Emulator", "WindowHeight", 600);

    if (config_emulator.savefiles_path.empty())
    {
        config_emulator.savefiles_path = config_root_path;
    }
    if (config_emulator.savestates_path.empty())
    {
        config_emulator.savestates_path = config_root_path;
    }

    for (int i = 0; i < config_max_recent_roms; i++)
    {
        std::string item = "RecentROM" + std::to_string(i);
        config_emulator.recent_roms[i] = read_string("Emulator", item.c_str());
    }

    config_video.scale = read_int("Video", "Scale", 0);
    config_video.ratio = read_int("Video", "AspectRatio", 0);
    config_video.fps = read_bool("Video", "FPS", false);
    config_video.bilinear = read_bool("Video", "Bilinear", false);
    config_video.sprite_limit = read_bool("Video", "SpriteLimit", false);
    config_video.mix_frames = read_bool("Video", "MixFrames", true);
    config_video.mix_frames_intensity = read_float("Video", "MixFramesIntensity", 0.30f);
    config_video.scanlines = read_bool("Video", "Scanlines", true);
    config_video.scanlines_intensity = read_float("Video", "ScanlinesIntensity", 0.40f);
    config_video.palette = read_int("Video", "Palette", 0);

    for (int i = 0; i < 16; i++)
    {
        char pal_label_r[32];
        char pal_label_g[32];
        char pal_label_b[32];
        sprintf(pal_label_r, "CustomPalette%dR", i);
        sprintf(pal_label_g, "CustomPalette%dG", i);
        sprintf(pal_label_b, "CustomPalette%dB", i);
        config_video.color[i].red = read_int("Video", pal_label_r, config_video.color[i].red);
        config_video.color[i].green = read_int("Video", pal_label_g, config_video.color[i].green);
        config_video.color[i].blue = read_int("Video", pal_label_b, config_video.color[i].blue);
    }

    config_video.sync = read_bool("Video", "Sync", true);
    
    config_audio.enable = read_bool("Audio", "Enable", true);
    config_audio.sync = read_bool("Audio", "Sync", true);

    config_input[0].key_left = (SDL_Scancode)read_int("InputA", "KeyLeft", SDL_SCANCODE_LEFT);
    config_input[0].key_right = (SDL_Scancode)read_int("InputA", "KeyRight", SDL_SCANCODE_RIGHT);
    config_input[0].key_up = (SDL_Scancode)read_int("InputA", "KeyUp", SDL_SCANCODE_UP);
    config_input[0].key_down = (SDL_Scancode)read_int("InputA", "KeyDown", SDL_SCANCODE_DOWN);
    config_input[0].key_left_button = (SDL_Scancode)read_int("InputA", "KeyLeftButton", SDL_SCANCODE_A);
    config_input[0].key_right_button = (SDL_Scancode)read_int("InputA", "KeyRightButton", SDL_SCANCODE_S);
    config_input[0].key_0 = (SDL_Scancode)read_int("InputA", "Key0", SDL_SCANCODE_KP_0);
    config_input[0].key_1 = (SDL_Scancode)read_int("InputA", "Key1", SDL_SCANCODE_KP_1);
    config_input[0].key_2 = (SDL_Scancode)read_int("InputA", "Key2", SDL_SCANCODE_KP_2);
    config_input[0].key_3 = (SDL_Scancode)read_int("InputA", "Key3", SDL_SCANCODE_KP_3);
    config_input[0].key_4 = (SDL_Scancode)read_int("InputA", "Key4", SDL_SCANCODE_KP_4);
    config_input[0].key_5 = (SDL_Scancode)read_int("InputA", "Key5", SDL_SCANCODE_KP_5);
    config_input[0].key_6 = (SDL_Scancode)read_int("InputA", "Key6", SDL_SCANCODE_KP_6);
    config_input[0].key_7 = (SDL_Scancode)read_int("InputA", "Key7", SDL_SCANCODE_KP_7);
    config_input[0].key_8 = (SDL_Scancode)read_int("InputA", "Key8", SDL_SCANCODE_KP_8);
    config_input[0].key_9 = (SDL_Scancode)read_int("InputA", "Key9", SDL_SCANCODE_KP_9);
    config_input[0].key_asterisk = (SDL_Scancode)read_int("InputA", "KeyAsterisk", SDL_SCANCODE_KP_MULTIPLY);
    config_input[0].key_hash = (SDL_Scancode)read_int("InputA", "KeyHash", SDL_SCANCODE_KP_DIVIDE);

    config_input[0].gamepad = read_bool("InputA", "Gamepad", true);
    config_input[0].gamepad_directional = read_int("InputA", "GamepadDirectional", 0);
    config_input[0].gamepad_invert_x_axis = read_bool("InputA", "GamepadInvertX", false);
    config_input[0].gamepad_invert_y_axis = read_bool("InputA", "GamepadInvertY", false);
    config_input[0].gamepad_left_button = read_int("InputA", "GamepadLeft", SDL_CONTROLLER_BUTTON_A);
    config_input[0].gamepad_right_button = read_int("InputA", "GamepadRight", SDL_CONTROLLER_BUTTON_B);
    config_input[0].gamepad_x_axis = read_int("InputA", "GamepadX", SDL_CONTROLLER_AXIS_LEFTX);
    config_input[0].gamepad_y_axis = read_int("InputA", "GamepadY", SDL_CONTROLLER_AXIS_LEFTY);
    config_input[0].gamepad_1 = read_int("InputA", "Gamepad1", SDL_CONTROLLER_BUTTON_X);
    config_input[0].gamepad_2 = read_int("InputA", "Gamepad2", SDL_CONTROLLER_BUTTON_Y);
    config_input[0].gamepad_3 = read_int("InputA", "Gamepad3", SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    config_input[0].gamepad_4 = read_int("InputA", "Gamepad4", SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    config_input[0].gamepad_5 = read_int("InputA", "Gamepad5", SDL_CONTROLLER_BUTTON_RIGHTSTICK);
    config_input[0].gamepad_6 = read_int("InputA", "Gamepad6", SDL_CONTROLLER_BUTTON_LEFTSTICK);
    config_input[0].gamepad_7 = read_int("InputA", "Gamepad7", SDL_CONTROLLER_BUTTON_GUIDE);
    config_input[0].gamepad_8 = read_int("InputA", "Gamepad8", SDL_CONTROLLER_BUTTON_GUIDE);
    config_input[0].gamepad_9 = read_int("InputA", "Gamepad9", SDL_CONTROLLER_BUTTON_GUIDE);
    config_input[0].gamepad_0 = read_int("InputA", "Gamepad0", SDL_CONTROLLER_BUTTON_GUIDE);
    config_input[0].gamepad_asterisk = read_int("InputA", "GamepadAsterisk", SDL_CONTROLLER_BUTTON_START);
    config_input[0].gamepad_hash = read_int("InputA", "GamepadHash", SDL_CONTROLLER_BUTTON_BACK);

    config_input[1].key_left = (SDL_Scancode)read_int("InputB", "KeyLeft", SDL_SCANCODE_J);
    config_input[1].key_right = (SDL_Scancode)read_int("InputB", "KeyRight", SDL_SCANCODE_L);
    config_input[1].key_up = (SDL_Scancode)read_int("InputB", "KeyUp", SDL_SCANCODE_I);
    config_input[1].key_down = (SDL_Scancode)read_int("InputB", "KeyDown", SDL_SCANCODE_K);
    config_input[1].key_left_button = (SDL_Scancode)read_int("InputB", "KeyLeftButton", SDL_SCANCODE_G);
    config_input[1].key_right_button = (SDL_Scancode)read_int("InputB", "KeyRightButton", SDL_SCANCODE_H);
    config_input[1].key_0 = (SDL_Scancode)read_int("InputB", "Key0", SDL_SCANCODE_NONUSBACKSLASH);
    config_input[1].key_1 = (SDL_Scancode)read_int("InputB", "Key1", SDL_SCANCODE_Z);
    config_input[1].key_2 = (SDL_Scancode)read_int("InputB", "Key2", SDL_SCANCODE_X);
    config_input[1].key_3 = (SDL_Scancode)read_int("InputB", "Key3", SDL_SCANCODE_C);
    config_input[1].key_4 = (SDL_Scancode)read_int("InputB", "Key4", SDL_SCANCODE_V);
    config_input[1].key_5 = (SDL_Scancode)read_int("InputB", "Key5", SDL_SCANCODE_B);
    config_input[1].key_6 = (SDL_Scancode)read_int("InputB", "Key6", SDL_SCANCODE_N);
    config_input[1].key_7 = (SDL_Scancode)read_int("InputB", "Key7", SDL_SCANCODE_M);
    config_input[1].key_8 = (SDL_Scancode)read_int("InputB", "Key8", SDL_SCANCODE_COMMA);
    config_input[1].key_9 = (SDL_Scancode)read_int("InputB", "Key9", SDL_SCANCODE_PERIOD);
    config_input[1].key_asterisk = (SDL_Scancode)read_int("InputB", "KeyAsterisk", SDL_SCANCODE_SLASH);
    config_input[1].key_hash = (SDL_Scancode)read_int("InputB", "KeyHash", SDL_SCANCODE_RSHIFT);

    config_input[1].gamepad = read_bool("InputB", "Gamepad", true);
    config_input[1].gamepad_directional = read_int("InputB", "GamepadDirectional", 0);
    config_input[1].gamepad_invert_x_axis = read_bool("InputB", "GamepadInvertX", false);
    config_input[1].gamepad_invert_y_axis = read_bool("InputB", "GamepadInvertY", false);
    config_input[1].gamepad_left_button = read_int("InputB", "GamepadLeft", SDL_CONTROLLER_BUTTON_A);
    config_input[1].gamepad_right_button = read_int("InputB", "GamepadRight", SDL_CONTROLLER_BUTTON_B);
    config_input[1].gamepad_x_axis = read_int("InputB", "GamepadX", SDL_CONTROLLER_AXIS_LEFTX);
    config_input[1].gamepad_y_axis = read_int("InputB", "GamepadY", SDL_CONTROLLER_AXIS_LEFTY);
    config_input[1].gamepad_1 = read_int("InputB", "Gamepad1", SDL_CONTROLLER_BUTTON_X);
    config_input[1].gamepad_2 = read_int("InputB", "Gamepad2", SDL_CONTROLLER_BUTTON_Y);
    config_input[1].gamepad_3 = read_int("InputB", "Gamepad3", SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    config_input[1].gamepad_4 = read_int("InputB", "Gamepad4", SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    config_input[1].gamepad_5 = read_int("InputB", "Gamepad5", SDL_CONTROLLER_BUTTON_RIGHTSTICK);
    config_input[1].gamepad_6 = read_int("InputB", "Gamepad6", SDL_CONTROLLER_BUTTON_LEFTSTICK);
    config_input[1].gamepad_7 = read_int("InputB", "Gamepad7", SDL_CONTROLLER_BUTTON_GUIDE);
    config_input[1].gamepad_8 = read_int("InputB", "Gamepad8", SDL_CONTROLLER_BUTTON_GUIDE);
    config_input[1].gamepad_9 = read_int("InputB", "Gamepad9", SDL_CONTROLLER_BUTTON_GUIDE);
    config_input[1].gamepad_0 = read_int("InputB", "Gamepad0", SDL_CONTROLLER_BUTTON_GUIDE);
    config_input[1].gamepad_asterisk = read_int("InputB", "GamepadAsterisk", SDL_CONTROLLER_BUTTON_START);
    config_input[1].gamepad_hash = read_int("InputB", "GamepadHash", SDL_CONTROLLER_BUTTON_BACK);

    Log("Settings loaded");
}

void config_write(void)
{
    Log("Saving settings to %s", config_emu_file_path);

    write_bool("Debug", "Debug", config_debug.debug);
    write_bool("Debug", "Disassembler", config_debug.show_disassembler);
    write_bool("Debug", "Screen", config_debug.show_screen);
    write_bool("Debug", "Memory", config_debug.show_memory);
    write_bool("Debug", "Processor", config_debug.show_processor);
    write_bool("Debug", "Video", config_debug.show_video);
    write_bool("Debug", "VideoRegisters", config_debug.show_video_registers);
    write_int("Debug", "FontSize", config_debug.font_size);

    write_bool("Emulator", "FullScreen", config_emulator.fullscreen);
    write_bool("Emulator", "ShowMenu", config_emulator.show_menu);
    write_int("Emulator", "FFWD", config_emulator.ffwd_speed);
    write_int("Emulator", "SaveSlot", config_emulator.save_slot);
    write_bool("Emulator", "StartPaused", config_emulator.start_paused);
    write_int("Emulator", "Region", config_emulator.region);
    write_string("Emulator", "BiosPath", config_emulator.bios_path);
    write_int("Emulator", "SaveFilesDirOption", config_emulator.savefiles_dir_option);
    write_string("Emulator", "SaveFilesPath", config_emulator.savefiles_path);
    write_int("Emulator", "SaveStatesDirOption", config_emulator.savestates_dir_option);
    write_string("Emulator", "SaveStatesPath", config_emulator.savestates_path);
    write_string("Emulator", "LastOpenPath", config_emulator.last_open_path);
    write_int("Emulator", "WindowWidth", config_emulator.window_width);
    write_int("Emulator", "WindowHeight", config_emulator.window_height);

    for (int i = 0; i < config_max_recent_roms; i++)
    {
        std::string item = "RecentROM" + std::to_string(i);
        write_string("Emulator", item.c_str(), config_emulator.recent_roms[i]);
    }

    write_int("Video", "Scale", config_video.scale);
    write_int("Video", "AspectRatio", config_video.ratio);
    write_bool("Video", "FPS", config_video.fps);
    write_bool("Video", "Bilinear", config_video.bilinear);
    write_bool("Video", "SpriteLimit", config_video.sprite_limit);
    write_bool("Video", "MixFrames", config_video.mix_frames);
    write_float("Video", "MixFramesIntensity", config_video.mix_frames_intensity);
    write_bool("Video", "Scanlines", config_video.scanlines);
    write_float("Video", "ScanlinesIntensity", config_video.scanlines_intensity);
    write_int("Video", "Palette", config_video.palette);
    for (int i = 0; i < 16; i++)
    {
        char pal_label_r[32];
        char pal_label_g[32];
        char pal_label_b[32];
        sprintf(pal_label_r, "CustomPalette%dR", i);
        sprintf(pal_label_g, "CustomPalette%dG", i);
        sprintf(pal_label_b, "CustomPalette%dB", i);
        write_int("Video", pal_label_r, config_video.color[i].red);
        write_int("Video", pal_label_g, config_video.color[i].green);
        write_int("Video", pal_label_b, config_video.color[i].blue);
    }
    write_bool("Video", "Sync", config_video.sync);

    write_bool("Audio", "Enable", config_audio.enable);
    write_bool("Audio", "Sync", config_audio.sync);

    write_int("InputA", "KeyLeft", config_input[0].key_left);
    write_int("InputA", "KeyRight", config_input[0].key_right);
    write_int("InputA", "KeyUp", config_input[0].key_up);
    write_int("InputA", "KeyDown", config_input[0].key_down);
    write_int("InputA", "KeyLeftButton", config_input[0].key_left_button);
    write_int("InputA", "KeyRightButton", config_input[0].key_right_button);
    write_int("InputA", "Key0", config_input[0].key_0);
    write_int("InputA", "Key1", config_input[0].key_1);
    write_int("InputA", "Key2", config_input[0].key_2);
    write_int("InputA", "Key3", config_input[0].key_3);
    write_int("InputA", "Key4", config_input[0].key_4);
    write_int("InputA", "Key5", config_input[0].key_5);
    write_int("InputA", "Key6", config_input[0].key_6);
    write_int("InputA", "Key7", config_input[0].key_7);
    write_int("InputA", "Key8", config_input[0].key_8);
    write_int("InputA", "Key9", config_input[0].key_9);
    write_int("InputA", "KeyAsterisk", config_input[0].key_asterisk);
    write_int("InputA", "KeyHash", config_input[0].key_hash);

    write_bool("InputA", "Gamepad", config_input[0].gamepad);
    write_int("InputA", "GamepadDirectional", config_input[0].gamepad_directional);
    write_bool("InputA", "GamepadInvertX", config_input[0].gamepad_invert_x_axis);
    write_bool("InputA", "GamepadInvertY", config_input[0].gamepad_invert_y_axis);
    write_int("InputA", "GamepadLeft", config_input[0].gamepad_left_button);
    write_int("InputA", "GamepadRight", config_input[0].gamepad_right_button);
    write_int("InputA", "GamepadX", config_input[0].gamepad_x_axis);
    write_int("InputA", "GamepadY", config_input[0].gamepad_y_axis);
    write_int("InputA", "Gamepad1", config_input[0].gamepad_1);
    write_int("InputA", "Gamepad2", config_input[0].gamepad_2);
    write_int("InputA", "Gamepad3", config_input[0].gamepad_3);
    write_int("InputA", "Gamepad4", config_input[0].gamepad_4);
    write_int("InputA", "Gamepad5", config_input[0].gamepad_5);
    write_int("InputA", "Gamepad6", config_input[0].gamepad_6);
    write_int("InputA", "Gamepad7", config_input[0].gamepad_7);
    write_int("InputA", "Gamepad8", config_input[0].gamepad_8);
    write_int("InputA", "Gamepad9", config_input[0].gamepad_9);
    write_int("InputA", "Gamepad0", config_input[0].gamepad_0);
    write_int("InputA", "GamepadAsterisk", config_input[0].gamepad_asterisk);
    write_int("InputA", "GamepadHash", config_input[0].gamepad_hash);

    write_int("InputB", "KeyLeft", config_input[1].key_left);
    write_int("InputB", "KeyRight", config_input[1].key_right);
    write_int("InputB", "KeyUp", config_input[1].key_up);
    write_int("InputB", "KeyDown", config_input[1].key_down);
    write_int("InputB", "KeyLeftButton", config_input[1].key_left_button);
    write_int("InputB", "KeyRightButton", config_input[1].key_right_button);
    write_int("InputB", "Key0", config_input[1].key_0);
    write_int("InputB", "Key1", config_input[1].key_1);
    write_int("InputB", "Key2", config_input[1].key_2);
    write_int("InputB", "Key3", config_input[1].key_3);
    write_int("InputB", "Key4", config_input[1].key_4);
    write_int("InputB", "Key5", config_input[1].key_5);
    write_int("InputB", "Key6", config_input[1].key_6);
    write_int("InputB", "Key7", config_input[1].key_7);
    write_int("InputB", "Key8", config_input[1].key_8);
    write_int("InputB", "Key9", config_input[1].key_9);
    write_int("InputB", "KeyAsterisk", config_input[1].key_asterisk);
    write_int("InputB", "KeyHash", config_input[1].key_hash);

    write_bool("InputB", "Gamepad", config_input[1].gamepad);
    write_int("InputB", "GamepadDirectional", config_input[1].gamepad_directional);
    write_bool("InputB", "GamepadInvertX", config_input[1].gamepad_invert_x_axis);
    write_bool("InputB", "GamepadInvertY", config_input[1].gamepad_invert_y_axis);
    write_int("InputB", "GamepadLeft", config_input[1].gamepad_left_button);
    write_int("InputB", "GamepadRight", config_input[1].gamepad_right_button);
    write_int("InputB", "GamepadX", config_input[1].gamepad_x_axis);
    write_int("InputB", "GamepadY", config_input[1].gamepad_y_axis);
    write_int("InputB", "Gamepad1", config_input[1].gamepad_1);
    write_int("InputB", "Gamepad2", config_input[1].gamepad_2);
    write_int("InputB", "Gamepad3", config_input[1].gamepad_3);
    write_int("InputB", "Gamepad4", config_input[1].gamepad_4);
    write_int("InputB", "Gamepad5", config_input[1].gamepad_5);
    write_int("InputB", "Gamepad6", config_input[1].gamepad_6);
    write_int("InputB", "Gamepad7", config_input[1].gamepad_7);
    write_int("InputB", "Gamepad8", config_input[1].gamepad_8);
    write_int("InputB", "Gamepad9", config_input[1].gamepad_9);
    write_int("InputB", "Gamepad0", config_input[1].gamepad_0);
    write_int("InputB", "GamepadAsterisk", config_input[1].gamepad_asterisk);
    write_int("InputB", "GamepadHash", config_input[1].gamepad_hash);

    if (config_ini_file->write(config_ini_data, true))
    {
        Log("Settings saved");
    }
}

static bool check_portable(void)
{
    char* base_path;
    char portable_file_path[260];
    
    base_path = SDL_GetBasePath();
    
    strcpy(portable_file_path, base_path);
    strcat(portable_file_path, "portable.ini");

    FILE* file = fopen(portable_file_path, "r");
    
    if (IsValidPointer(file))
    {
        fclose(file);
        return true;
    }

    return false;
}

static int read_int(const char* group, const char* key, int default_value)
{
    int ret = 0;

    std::string value = config_ini_data[group][key];

    if(value.empty())
        ret = default_value;
    else
        ret = std::stoi(value);

    Log("Load setting: [%s][%s]=%d", group, key, ret);
    return ret;
}

static void write_int(const char* group, const char* key, int integer)
{
    std::string value = std::to_string(integer);
    config_ini_data[group][key] = value;
    Log("Save setting: [%s][%s]=%s", group, key, value.c_str());
}

static float read_float(const char* group, const char* key, float default_value)
{
    float ret = 0.0f;

    std::string value = config_ini_data[group][key];

    if(value.empty())
        ret = default_value;
    else
        ret = strtof(value.c_str(), NULL);

    Log("Load setting: [%s][%s]=%.2f", group, key, ret);
    return ret;
}

static void write_float(const char* group, const char* key, float value)
{
    std::string value_str = std::to_string(value);
    config_ini_data[group][key] = value_str;
    Log("Save setting: [%s][%s]=%s", group, key, value_str.c_str());
}

static bool read_bool(const char* group, const char* key, bool default_value)
{
    bool ret;

    std::string value = config_ini_data[group][key];

    if(value.empty())
        ret = default_value;
    else
        std::istringstream(value) >> std::boolalpha >> ret;

    Log("Load setting: [%s][%s]=%s", group, key, ret ? "true" : "false");
    return ret;
}

static void write_bool(const char* group, const char* key, bool boolean)
{
    std::stringstream converter;
    converter << std::boolalpha << boolean;
    std::string value;
    value = converter.str();
    config_ini_data[group][key] = value;
    Log("Save setting: [%s][%s]=%s", group, key, value.c_str());
}

static std::string read_string(const char* group, const char* key)
{
    std::string ret = config_ini_data[group][key];
    Log("Load setting: [%s][%s]=%s", group, key, ret.c_str());
    return ret;
}

static void write_string(const char* group, const char* key, std::string value)
{
    config_ini_data[group][key] = value;
    Log("Save setting: [%s][%s]=%s", group, key, value.c_str());
}
