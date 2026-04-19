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

#define EMU_IMPORT
#include "emu.h"

#include <thread>
#include <atomic>
#include <string.h>
#include "gearcoleco.h"
#include "sound_queue.h"
#include "config.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#if defined(_WIN32)
#define STBIW_WINDOWS_UTF8
#endif
#include "stb_image_write.h"

static GearcolecoCore* gearcoleco;
static s16* audio_buffer;
static bool audio_enabled;
static bool debugging = false;
static bool debug_step = false;
static bool debug_next_frame = false;

u16* debug_background_buffer;
u16* debug_tile_buffer;
u16* debug_sprite_buffers[64];

enum Loading_State
{
    Loading_State_None = 0,
    Loading_State_Loading,
    Loading_State_Finished
};

static std::atomic<int> loading_state(Loading_State_None);
static std::thread loading_thread;
static bool loading_thread_active;
static bool loading_result;
static char loading_file_path[4096];
static Cartridge::ForceConfiguration loading_config;

static void save_ram(void);
static void load_ram(void);
static void reset_buffers(void);
static const char* get_mapper(Cartridge::CartridgeTypes type);
static const char* get_configurated_dir(int option, const char* path);
static void init_debug(void);
static void destroy_debug(void);
static void update_debug(void);

bool emu_init(void)
{
    int screen_size = GC_RESOLUTION_WIDTH_WITH_OVERSCAN * GC_RESOLUTION_HEIGHT_WITH_OVERSCAN;

    emu_frame_buffer = new u8[screen_size * 4];
    audio_buffer = new s16[GC_AUDIO_BUFFER_SIZE];

    init_debug();
    reset_buffers();

    gearcoleco = new GearcolecoCore();
    gearcoleco->Init();

    sound_queue_init();

    audio_enabled = true;
    emu_audio_sync = true;
    emu_debug_disable_breakpoints_cpu = false;
    emu_debug_disable_breakpoints_mem = false;
    emu_debug_step_frames_pending = 0;
    emu_debug_tile_palette = 0;
    emu_debug_tile_color_mode = true;

    for (int i = 0; i < 5; i++)
    {
        emu_savestates[i].rom_name[0] = 0;
        InitPointer(emu_savestates_screenshots[i].data);
        emu_savestates_screenshots[i].size = 0;
        emu_savestates_screenshots[i].width = 0;
        emu_savestates_screenshots[i].height = 0;
    }

    return true;
}

void emu_destroy(void)
{
    if (loading_thread_active)
    {
        loading_thread.join();
        loading_thread_active = false;
    }
    loading_state.store(Loading_State_None);

    save_ram();
    SafeDeleteArray(audio_buffer);
    sound_queue_destroy();
    SafeDelete(gearcoleco);
    SafeDeleteArray(emu_frame_buffer);
    destroy_debug();

    for (int i = 0; i < 5; i++)
        SafeDeleteArray(emu_savestates_screenshots[i].data);
}

static void load_media_thread_func(void)
{
    loading_result = gearcoleco->LoadROM(loading_file_path, &loading_config);
    loading_state.store(Loading_State_Finished);
}

void emu_load_media_async(const char* file_path, Cartridge::ForceConfiguration config)
{
    if (loading_state.load() != Loading_State_None)
        return;

    reset_buffers();
    save_ram();

    strncpy(loading_file_path, file_path, sizeof(loading_file_path) - 1);
    loading_file_path[sizeof(loading_file_path) - 1] = '\0';
    loading_result = false;
    loading_config = config;
    loading_state.store(Loading_State_Loading);
    if (loading_thread_active)
        loading_thread.join();
    loading_thread = std::thread(load_media_thread_func);
    loading_thread_active = true;
}

bool emu_is_media_loading(void)
{
    return loading_state.load() == Loading_State_Loading;
}

bool emu_finish_media_loading(void)
{
    if (loading_state.load() != Loading_State_Finished)
        return false;

    if (loading_thread_active)
    {
        loading_thread.join();
        loading_thread_active = false;
    }

    loading_state.store(Loading_State_None);

    if (!loading_result)
        return false;

    emu_audio_reset();
    load_ram();

    update_savestates_data();

    return true;
}

void emu_update(void)
{
    if (loading_state.load() != Loading_State_None)
        return;

    if (emu_is_empty())
        return;

    int sampleCount = 0;

    if (!debugging || debug_step || debug_next_frame)
    {
        bool breakpoints = (!emu_debug_disable_breakpoints_cpu && !emu_debug_disable_breakpoints_mem) || IsValidPointer(gearcoleco->GetMemory()->GetRunToBreakpoint());

        if (gearcoleco->RunToVBlank(emu_frame_buffer, audio_buffer, &sampleCount, debug_step, breakpoints))
        {
            debugging = true;
        }

        if (debug_next_frame && emu_debug_step_frames_pending > 0)
        {
            emu_debug_step_frames_pending--;
            if (emu_debug_step_frames_pending > 0)
                debug_next_frame = true;
            else
                debug_next_frame = false;
        }
        else
        {
            debug_next_frame = false;
        }
        debug_step = false;
    }

    if (config_debug.debug)
        update_debug();

    if ((sampleCount > 0) && !gearcoleco->IsPaused())
    {
        sound_queue_write(audio_buffer, sampleCount, emu_audio_sync);
    }
}

void emu_key_pressed(GC_Controllers controller, GC_Keys key)
{
    gearcoleco->KeyPressed(controller, key);
}

void emu_key_released(GC_Controllers controller, GC_Keys key)
{
    gearcoleco->KeyReleased(controller, key);
}

void emu_spinner1(int movement)
{
    gearcoleco->Spinner1(movement);
}

void emu_spinner2(int movement)
{
    gearcoleco->Spinner2(movement);
}

void emu_pause(void)
{
    gearcoleco->Pause(true);
}

void emu_resume(void)
{
    gearcoleco->Pause(false);
}

bool emu_is_paused(void)
{
    return gearcoleco->IsPaused();
}

bool emu_is_debug_idle(void)
{
    return config_debug.debug && debugging && !debug_step && !debug_next_frame;
}

bool emu_is_empty(void)
{
    if (loading_state.load() != Loading_State_None)
        return true;
    return !gearcoleco->GetCartridge()->IsReady();
}

bool emu_is_bios_loaded(void)
{
    return gearcoleco->GetMemory()->IsBiosLoaded();
}

void emu_reset(Cartridge::ForceConfiguration config)
{
    emu_audio_reset();
    save_ram();
    gearcoleco->ResetROM(&config);
    load_ram();
}

void emu_dissasemble_rom(void)
{
    gearcoleco->SaveDisassembledROM();
}

void emu_audio_mute(bool mute)
{
    audio_enabled = !mute;
    gearcoleco->GetAudio()->Mute(mute);
}

void emu_audio_reset(void)
{
    sound_queue_stop();
    sound_queue_start(GC_AUDIO_SAMPLE_RATE, 2, GC_AUDIO_QUEUE_SIZE, config_audio.buffer_count);
}

bool emu_is_audio_enabled(void)
{
    return audio_enabled;
}

bool emu_is_audio_open(void)
{
    return sound_queue_is_open();
}

void emu_palette(GC_Color* palette)
{
    gearcoleco->GetVideo()->SetCustomPalette(palette);
}

void emu_predefined_palette(int palette)
{
    gearcoleco->GetVideo()->SetPredefinedPalette(palette);
}

void emu_save_ram(const char* file_path)
{
    if (!emu_is_empty())
        gearcoleco->SaveRam(file_path, true);
}

void emu_load_ram(const char* file_path, Cartridge::ForceConfiguration config)
{
    if (!emu_is_empty())
    {
        save_ram();
        gearcoleco->ResetROM(&config);
        gearcoleco->LoadRam(file_path, true);
    }
}

void emu_save_state_slot(int index)
{
    if (!emu_is_empty())
    {
        const char* dir = get_configurated_dir(config_emulator.savestates_dir_option, config_emulator.savestates_path.c_str());
        gearcoleco->SaveState(dir, index, true);
        update_savestates_data();
    }
}

void emu_load_state_slot(int index)
{
    if (!emu_is_empty())
    {
        const char* dir = get_configurated_dir(config_emulator.savestates_dir_option, config_emulator.savestates_path.c_str());
        gearcoleco->LoadState(dir, index);
    }
}

void emu_save_state_file(const char* file_path)
{
    if (!emu_is_empty())
        gearcoleco->SaveState(file_path, -1);
}

void emu_load_state_file(const char* file_path)
{
    if (!emu_is_empty())
        gearcoleco->LoadState(file_path, -1);
}

void emu_get_runtime(GC_RuntimeInfo& runtime)
{
    gearcoleco->GetRuntimeInfo(runtime);
}

void emu_get_info(char* info, int buffer_size)
{
    if (!emu_is_empty())
    {
        Cartridge* cart = gearcoleco->GetCartridge();
        GC_RuntimeInfo runtime;
        gearcoleco->GetRuntimeInfo(runtime);

        const char* filename = cart->GetFileName();
        const char* pal = cart->IsPAL() ? "PAL" : "NTSC";
        const char* checksum = cart->IsValidROM() ? "VALID" : "FAILED";
        int rom_banks = cart->GetROMBankCount();
        const char* mapper = get_mapper(cart->GetType());

        snprintf(info, buffer_size, "File Name: %s\nMapper: %s\nRefresh Rate: %s\nCartridge Header: %s\nROM Banks: %d\nScreen Resolution: %dx%d", filename, mapper, pal, checksum, rom_banks, runtime.screen_width, runtime.screen_height);
    }
    else
    {
        snprintf(info, buffer_size, "There is no ROM loaded!");
    }
}

GearcolecoCore* emu_get_core(void)
{
    return gearcoleco;
}

void emu_debug_step(void)
{
    debugging = debug_step = true;
    debug_next_frame = false;
    gearcoleco->Pause(false);
}

void emu_debug_continue(void)
{
    debugging = debug_step = debug_next_frame = false;
    gearcoleco->Pause(false);
}

void emu_debug_next_frame(void)
{
    debugging = debug_next_frame = true;
    debug_step = false;
    emu_debug_step_frames_pending++;
    gearcoleco->Pause(false);
}

void emu_debug_step_over(void)
{
    // TODO: implement step over
    emu_debug_step();
}

void emu_debug_step_into(void)
{
    // TODO: implement step into
    emu_debug_step();
}

void emu_debug_step_out(void)
{
    // TODO: implement step out
    emu_debug_step();
}

void emu_debug_step_frame(void)
{
    // TODO: implement step frame
    emu_debug_next_frame();
}

void emu_debug_break(void)
{
    // TODO: implement break
    debugging = debug_step = true;
    debug_next_frame = false;
}

void emu_mcp_start(void)
{
    // TODO: implement MCP start
}

void emu_mcp_stop(void)
{
    // TODO: implement MCP stop
}

void emu_mcp_set_transport(int mode, int port)
{
    // TODO: implement MCP set transport
    UNUSED(mode);
    UNUSED(port);
}

bool emu_mcp_is_running(void)
{
    // TODO: implement MCP is running
    return false;
}

int emu_mcp_get_transport_mode(void)
{
    // TODO: implement MCP get transport mode
    return 0;
}

void emu_load_bios(const char* file_path)
{
    gearcoleco->GetMemory()->LoadBios(file_path);
}

void emu_video_no_sprite_limit(bool enabled)
{
    gearcoleco->GetVideo()->SetNoSpriteLimit(enabled);
}

void emu_set_overscan(int overscan)
{
    switch (overscan)
    {
        case 0:
            gearcoleco->GetVideo()->SetOverscan(Video::OverscanDisabled);
            break;
        case 1:
            gearcoleco->GetVideo()->SetOverscan(Video::OverscanTopBottom);
            break;
        case 2:
            gearcoleco->GetVideo()->SetOverscan(Video::OverscanFull284);
            break;
        case 3:
            gearcoleco->GetVideo()->SetOverscan(Video::OverscanFull320);
            break;
        default:
            gearcoleco->GetVideo()->SetOverscan(Video::OverscanDisabled);
    }
}

void emu_save_screenshot(const char* file_path)
{
    if (!gearcoleco->GetCartridge()->IsReady())
        return;

    GC_RuntimeInfo runtime;
    emu_get_runtime(runtime);

    stbi_write_png(file_path, runtime.screen_width, runtime.screen_height, 4, emu_frame_buffer, runtime.screen_width * 4);

    Log("Screenshot saved to %s", file_path);
}

void emu_start_vgm_recording(const char* file_path)
{
    if (!gearcoleco->GetCartridge()->IsReady())
        return;

    if (gearcoleco->GetAudio()->IsVgmRecording())
        emu_stop_vgm_recording();

    GC_RuntimeInfo runtime;
    gearcoleco->GetRuntimeInfo(runtime);

    bool is_pal = (runtime.region == Region_PAL);
    int clock_rate = is_pal ? GC_MASTER_CLOCK_PAL : GC_MASTER_CLOCK_NTSC;

    if (gearcoleco->GetAudio()->StartVgmRecording(file_path, clock_rate, is_pal))
        Log("VGM recording started: %s", file_path);
}

void emu_stop_vgm_recording(void)
{
    if (gearcoleco->GetAudio()->IsVgmRecording())
    {
        gearcoleco->GetAudio()->StopVgmRecording();
        Log("VGM recording stopped");
    }
}

bool emu_is_vgm_recording(void)
{
    return gearcoleco->GetAudio()->IsVgmRecording();
}

static void save_ram(void)
{
#ifdef DEBUG_GEARCOLECO
    emu_dissasemble_rom();
#endif
    const char* dir = get_configurated_dir(config_emulator.savefiles_dir_option, config_emulator.savefiles_path.c_str());
    gearcoleco->SaveRam(dir);
}

static void load_ram(void)
{
    const char* dir = get_configurated_dir(config_emulator.savefiles_dir_option, config_emulator.savefiles_path.c_str());
    gearcoleco->LoadRam(dir);
}

static void reset_buffers(void)
{
    int screen_size = GC_RESOLUTION_WIDTH_WITH_OVERSCAN * GC_RESOLUTION_HEIGHT_WITH_OVERSCAN;
    for (int i = 0; i < screen_size * 4; i++)
        emu_frame_buffer[i] = 0;

    for (int i = 0; i < GC_AUDIO_BUFFER_SIZE; i++)
        audio_buffer[i] = 0;
}

static const char* get_mapper(Cartridge::CartridgeTypes type)
{
    switch (type)
    {
    case Cartridge::CartridgeColecoVision:
        return "ColecoVision";
    case Cartridge::CartridgeMegaCart:
        return "MegaCart";
    case Cartridge::CartridgeActivisionCart:
        return "Activision";
    case Cartridge::CartridgeOCM:
        return "OCM";
    case Cartridge::CartridgeNotSupported:
        return "Not Supported";
    default:
        return "Undefined";
    }
}

static const char* get_configurated_dir(int location, const char* path)
{
    switch ((Directory_Location)location)
    {
        default:
        case Directory_Location_Default:
            return config_root_path;
        case Directory_Location_ROM:
            return NULL;
        case Directory_Location_Custom:
            return path;
    }
}

static void init_debug(void)
{
    emu_debug_background_buffer = new u8[256 * 256 * 4];
    emu_debug_tile_buffer = new u8[32 * 32 * 64 * 4];
    debug_background_buffer = new u16[256 * 256];
    debug_tile_buffer = new u16[32 * 32 * 64];

    memset(debug_tile_buffer, 0, 32 * 32 * 64 * sizeof(u16));
    memset(emu_debug_tile_buffer, 0, 32 * 32 * 64 * 4);

    for (int s = 0; s < 64; s++)
    {
        emu_debug_sprite_buffers[s] = new u8[16 * 16 * 4];
        debug_sprite_buffers[s] = new u16[16 * 16];
        memset(debug_sprite_buffers[s], 0, 16 * 16 * sizeof(u16));
        memset(emu_debug_sprite_buffers[s], 0, 16 * 16 * 4);
    }

    memset(debug_background_buffer, 0, 256 * 256 * sizeof(u16));
    memset(emu_debug_background_buffer, 0, 256 * 256 * 4);
}

static void destroy_debug(void)
{
    SafeDeleteArray(emu_debug_background_buffer);
    SafeDeleteArray(emu_debug_tile_buffer);
    SafeDeleteArray(debug_background_buffer);
    SafeDeleteArray(debug_tile_buffer);

    for (int s = 0; s < 64; s++)
    {
        SafeDeleteArray(emu_debug_sprite_buffers[s]);
        SafeDeleteArray(debug_sprite_buffers[s]);
    }
}

static void update_debug(void)
{
    Video* video = gearcoleco->GetVideo();

    video->Render32bit(debug_background_buffer, emu_debug_background_buffer, GC_PIXEL_RGBA8888, 256 * 256);
    video->Render32bit(debug_tile_buffer, emu_debug_tile_buffer, GC_PIXEL_RGBA8888, 32 * 32 * 64);

    for (int s = 0; s < 64; s++)
        video->Render32bit(debug_sprite_buffers[s], emu_debug_sprite_buffers[s], GC_PIXEL_RGBA8888, 16 * 16);
}

void update_savestates_data(void)
{
    if (emu_is_empty())
        return;

    for (int i = 0; i < 5; i++)
    {
        emu_savestates[i].rom_name[0] = 0;
        SafeDeleteArray(emu_savestates_screenshots[i].data);

        const char* dir = get_configurated_dir(config_emulator.savestates_dir_option, config_emulator.savestates_path.c_str());

        if (!gearcoleco->GetSaveStateHeader(i + 1, dir, &emu_savestates[i]))
            continue;

        if (emu_savestates[i].screenshot_size > 0)
        {
            emu_savestates_screenshots[i].data = new u8[emu_savestates[i].screenshot_size];
            emu_savestates_screenshots[i].size = emu_savestates[i].screenshot_size;
            gearcoleco->GetSaveStateScreenshot(i + 1, dir, &emu_savestates_screenshots[i]);
        }
    }
}
