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
#include "rewind.h"
#include "events.h"
#include "no_bios.h"
#include "mcp/mcp_manager.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#if defined(_WIN32)
#define STBIW_WINDOWS_UTF8
#endif
#include "stb_image_write.h"

static GearcolecoCore* gearcoleco;
static McpManager* mcp_manager;
static s16* audio_buffer;
static bool audio_enabled;
static int emu_debug_halt_step_frames_pending;
static const int kDebugHaltStepMaxFrames = 4;
static Uint64 rewind_last_counter = 0;
static double rewind_pop_accumulator = 0.0;

u16* debug_background_buffer;
u16* debug_tile_buffer;
u16* debug_sprite_buffers[GC_MAX_SPRITES];

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
static void update_debug_background_buffer(void);
static void update_debug_tile_buffer(void);
static void update_debug_sprite_buffers(void);
static void debug_step_instruction(void);
static void reset_rewind_timing(void);
static int get_rewind_pop_budget(void);

bool emu_init(void)
{
    int screen_size = GC_RESOLUTION_WIDTH_WITH_OVERSCAN * GC_RESOLUTION_HEIGHT_WITH_OVERSCAN;

    emu_frame_buffer = new u8[screen_size * 4];
    audio_buffer = new s16[GC_AUDIO_BUFFER_SIZE];

    init_debug();
    reset_buffers();

    gearcoleco = new GearcolecoCore();
    gearcoleco->Init();

    mcp_manager = new McpManager();
    mcp_manager->Init(gearcoleco);

    sound_queue_init();
    rewind_init();

    audio_enabled = true;
    emu_audio_sync = true;
    emu_debug_disable_breakpoints = false;
    emu_debug_irq_breakpoints = false;
    emu_debug_command = Debug_Command_None;
    emu_debug_halt_step_frames_pending = 0;
    emu_debug_pc_changed = false;
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
    SafeDelete(mcp_manager);
    rewind_destroy();
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

    emu_debug_command = Debug_Command_None;
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

    if (config_debug.debug && (config_debug.dis_look_ahead_count > 0))
        gearcoleco->GetProcessor()->DisassembleAhead(config_debug.dis_look_ahead_count);

    update_savestates_data();
    rewind_reset();

    return true;
}

void emu_render_current_frame(void)
{
    if (emu_is_empty())
        return;

    int size = gearcoleco->GetMemory()->IsBiosLoaded() ? GC_RESOLUTION_WIDTH_WITH_OVERSCAN * GC_RESOLUTION_HEIGHT_WITH_OVERSCAN : GC_RESOLUTION_WIDTH * GC_RESOLUTION_HEIGHT;
    u16* src_buffer = gearcoleco->GetMemory()->IsBiosLoaded() ? gearcoleco->GetVideo()->GetFrameBuffer() : kNoBiosImage;

    gearcoleco->GetVideo()->Render32bit(src_buffer, emu_frame_buffer, GC_PIXEL_RGBA8888, size, true);

    if (config_debug.debug)
        update_debug();
}

void emu_reset_rewind_timing(void)
{
    reset_rewind_timing();
}

void emu_update(void)
{
    emu_mcp_pump_commands();

    if (loading_state.load() != Loading_State_None)
        return;

    if (emu_is_empty())
        return;

    int sampleCount = 0;
    bool frame_executed = false;

    if (rewind_is_active())
    {
        int to_pop = get_rewind_pop_budget();

        for (int i = 0; i < to_pop; i++)
        {
            if (!rewind_pop())
                break;
        }

        int silence_count = GC_AUDIO_QUEUE_SIZE;
        memset(audio_buffer, 0, silence_count * sizeof(s16));
        sound_queue_write(audio_buffer, silence_count, false);
        return;
    }

    reset_rewind_timing();

    if (config_debug.debug)
    {
        bool breakpoint_hit = false;
        GearcolecoCore::GC_Debug_Run debug_run;
        debug_run.step_debugger = (emu_debug_command == Debug_Command_Step);
        debug_run.stop_on_breakpoint = !emu_debug_disable_breakpoints && (emu_debug_halt_step_frames_pending == 0);
        debug_run.stop_on_run_to_breakpoint = true;
        debug_run.stop_on_irq = emu_debug_irq_breakpoints && (emu_debug_halt_step_frames_pending == 0);

        if (emu_debug_command != Debug_Command_None)
        {
            rewind_commit_seek();
            breakpoint_hit = gearcoleco->RunToVBlank(emu_frame_buffer, audio_buffer, &sampleCount, &debug_run);
            frame_executed = true;
        }

        if (breakpoint_hit || emu_debug_command == Debug_Command_StepFrame || emu_debug_command == Debug_Command_Step)
        {
            emu_debug_pc_changed = true;

            if (config_debug.dis_look_ahead_count > 0)
                gearcoleco->GetProcessor()->DisassembleAhead(config_debug.dis_look_ahead_count);
        }

        if (emu_debug_halt_step_frames_pending > 0)
        {
            if (breakpoint_hit)
                emu_debug_halt_step_frames_pending = 0;
            else if (emu_debug_command == Debug_Command_Continue)
            {
                emu_debug_halt_step_frames_pending--;

                if (emu_debug_halt_step_frames_pending == 0)
                {
                    emu_debug_command = Debug_Command_None;
                    emu_debug_pc_changed = true;

                    if (config_debug.dis_look_ahead_count > 0)
                        gearcoleco->GetProcessor()->DisassembleAhead(config_debug.dis_look_ahead_count);
                }
            }
        }

        if (breakpoint_hit)
            emu_debug_command = Debug_Command_None;

        if (emu_debug_command == Debug_Command_StepFrame && emu_debug_step_frames_pending > 0)
        {
            emu_debug_step_frames_pending--;
            if (emu_debug_step_frames_pending > 0)
                emu_debug_command = Debug_Command_StepFrame;
            else
                emu_debug_command = Debug_Command_None;
        }
        else if (emu_debug_command != Debug_Command_Continue)
            emu_debug_command = Debug_Command_None;

        update_debug();
    }
    else
    {
        if (!gearcoleco->IsPaused())
        {
            rewind_commit_seek();
            gearcoleco->RunToVBlank(emu_frame_buffer, audio_buffer, &sampleCount);
            frame_executed = true;
        }
    }

    if (frame_executed)
        rewind_push();

    if ((sampleCount > 0) && !gearcoleco->IsPaused())
    {
        sound_queue_write(audio_buffer, sampleCount, emu_audio_sync);
    }
    else if (gearcoleco->IsPaused())
    {
        int silence_count = GC_AUDIO_QUEUE_SIZE;
        memset(audio_buffer, 0, silence_count * sizeof(s16));
        sound_queue_write(audio_buffer, silence_count, false);
    }
}

static void reset_rewind_timing(void)
{
    rewind_last_counter = 0;
    rewind_pop_accumulator = 0.0;
}

static int get_rewind_pop_budget(void)
{
    Uint64 now = SDL_GetPerformanceCounter();

    if (rewind_last_counter == 0)
    {
        rewind_last_counter = now;
        return 0;
    }

    double elapsed = (double)(now - rewind_last_counter) / (double)SDL_GetPerformanceFrequency();
    rewind_last_counter = now;

    if (elapsed < 0.0)
        elapsed = 0.0;
    else if (elapsed > 0.25)
        elapsed = 0.25;

    int frames_per_snapshot = rewind_get_frames_per_snapshot();
    if (frames_per_snapshot < 1)
        frames_per_snapshot = 1;

    double snapshots_per_second = (60.0 * (double)config_rewind.speed) / (double)frames_per_snapshot;
    rewind_pop_accumulator += elapsed * snapshots_per_second;

    int to_pop = (int)rewind_pop_accumulator;
    if (to_pop > 0)
        rewind_pop_accumulator -= (double)to_pop;

    return to_pop;
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
    return config_debug.debug && (emu_debug_command == Debug_Command_None);
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
    rewind_reset();
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

void emu_audio_set_master_volume(float volume)
{
    gearcoleco->GetAudio()->SetMasterVolume(volume);
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
        rewind_reset();
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
        if (gearcoleco->LoadState(dir, index))
        {
            events_sync_input();
            rewind_reset();
        }
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
    {
        if (gearcoleco->LoadState(file_path, -1))
        {
            events_sync_input();
            rewind_reset();
        }
    }
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

void emu_debug_step_over(void)
{
    Processor* processor = emu_get_core()->GetProcessor();
    Processor::ProcessorState* proc_state = processor->GetState();
    Memory* memory = emu_get_core()->GetMemory();
    u16 pc = proc_state->PC->GetValue();
    GC_Disassembler_Record* record = memory->GetDisassemblerRecord(pc);

    if (IsValidPointer(record) && record->subroutine)
    {
        u16 return_address = pc + record->size;
        processor->AddRunToBreakpoint(return_address);
        emu_debug_command = Debug_Command_Continue;
    }
    else
    {
        debug_step_instruction();
        return;
    }

    gearcoleco->Pause(false);
}

void emu_debug_step_into(void)
{
    debug_step_instruction();
}

void emu_debug_step_out(void)
{
    Processor* processor = emu_get_core()->GetProcessor();
    std::stack<Processor::GC_CallStackEntry>* call_stack = processor->GetDisassemblerCallStack();

    if (call_stack->size() > 0)
    {
        Processor::GC_CallStackEntry entry = call_stack->top();
        u16 return_address = entry.back;
        processor->AddRunToBreakpoint(return_address);
        emu_debug_command = Debug_Command_Continue;
    }
    else
    {
        debug_step_instruction();
        return;
    }

    gearcoleco->Pause(false);
}

void emu_debug_step_frame(void)
{
    gearcoleco->Pause(false);
    emu_debug_step_frames_pending++;
    emu_debug_command = Debug_Command_StepFrame;
}

void emu_debug_break(void)
{
    gearcoleco->Pause(false);
    if (emu_debug_command == Debug_Command_Continue)
        emu_debug_command = Debug_Command_Step;
}

void emu_debug_continue(void)
{
    gearcoleco->Pause(false);
    emu_debug_halt_step_frames_pending = 0;
    emu_debug_command = Debug_Command_Continue;
}

bool emu_debug_halt_step_active(void)
{
    return emu_debug_halt_step_frames_pending > 0;
}

void emu_mcp_start(void)
{
    mcp_manager->Start();
}

void emu_mcp_stop(void)
{
    mcp_manager->Stop();
}

void emu_mcp_set_transport(int mode, int port)
{
    mcp_manager->SetTransportMode((McpTransportMode)mode, port);
}

bool emu_mcp_is_running(void)
{
    return mcp_manager && mcp_manager->IsRunning();
}

int emu_mcp_get_transport_mode(void)
{
    return mcp_manager ? mcp_manager->GetTransportMode() : -1;
}

void emu_mcp_pump_commands(void)
{
    mcp_manager->PumpCommands(gearcoleco);
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

void emu_save_sprite(const char* file_path, int index)
{
    if (!gearcoleco->GetCartridge()->IsReady())
        return;

    update_debug();

    int sprite_size = IsSetBit(gearcoleco->GetVideo()->GetRegisters()[1], 1) ? 16 : 8;

    stbi_write_png(file_path, sprite_size, sprite_size, 4, emu_debug_sprite_buffers[index], 16 * 4);

    Log("Sprite saved to %s", file_path);
}

void emu_save_background(const char* file_path)
{
    if (!gearcoleco->GetCartridge()->IsReady())
        return;

    update_debug();

    stbi_write_png(file_path, 256, 192, 4, emu_debug_background_buffer, 256 * 4);

    Log("Background saved to %s", file_path);
}

void emu_save_tiles(const char* file_path)
{
    if (!gearcoleco->GetCartridge()->IsReady())
        return;

    update_debug();

    stbi_write_png(file_path, 256, 256, 4, emu_debug_tile_buffer, 256 * 4);

    Log("Pattern table saved to %s", file_path);
}

int emu_get_screenshot_png(unsigned char** out_buffer)
{
    if (!gearcoleco->GetCartridge()->IsReady())
        return 0;

    GC_RuntimeInfo runtime;
    emu_get_runtime(runtime);

    int stride = runtime.screen_width * 4;
    int len = 0;

    *out_buffer = stbi_write_png_to_mem(emu_frame_buffer, stride,
                                         runtime.screen_width, runtime.screen_height,
                                         4, &len);

    return len;
}

int emu_get_sprite_png(int sprite_index, unsigned char** out_buffer)
{
    if (!gearcoleco->GetCartridge()->IsReady())
        return 0;

    if (sprite_index < 0 || sprite_index >= GC_MAX_SPRITES)
        return 0;

    update_debug();

    int sprite_size = IsSetBit(gearcoleco->GetVideo()->GetRegisters()[1], 1) ? 16 : 8;

    u8* buffer = emu_debug_sprite_buffers[sprite_index];

    if (!buffer)
        return 0;

    int len = 0;
    *out_buffer = stbi_write_png_to_mem(buffer, 16 * 4, sprite_size, sprite_size, 4, &len);

    return len;
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

    for (int s = 0; s < GC_MAX_SPRITES; s++)
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

    for (int s = 0; s < GC_MAX_SPRITES; s++)
    {
        SafeDeleteArray(emu_debug_sprite_buffers[s]);
        SafeDeleteArray(debug_sprite_buffers[s]);
    }
}

static void debug_step_instruction(void)
{
    Processor* processor = emu_get_core()->GetProcessor();
    u16 pc = processor->GetState()->PC->GetValue();

    emu_debug_halt_step_frames_pending = 0;

    if (processor->Halted() || (emu_get_core()->GetMemory()->DebugRetrieve(pc) == 0x76))
    {
        processor->AddRunToBreakpoint(pc + 1);
        emu_debug_halt_step_frames_pending = kDebugHaltStepMaxFrames;
        emu_debug_command = Debug_Command_Continue;
    }
    else
        emu_debug_command = Debug_Command_Step;

    gearcoleco->Pause(false);
}

static void update_debug(void)
{
    Video* video = gearcoleco->GetVideo();

    update_debug_background_buffer();
    update_debug_tile_buffer();
    update_debug_sprite_buffers();

    video->Render32bit(debug_background_buffer, emu_debug_background_buffer, GC_PIXEL_RGBA8888, 256 * 256);
    video->Render32bit(debug_tile_buffer, emu_debug_tile_buffer, GC_PIXEL_RGBA8888, 32 * 32 * 64);

    for (int s = 0; s < GC_MAX_SPRITES; s++)
        video->Render32bit(debug_sprite_buffers[s], emu_debug_sprite_buffers[s], GC_PIXEL_RGBA8888, 16 * 16);
}

static void update_debug_background_buffer(void)
{
    Video* video = gearcoleco->GetVideo();
    u8* vram = video->GetVRAM();
    u8* regs = video->GetRegisters();
    int mode = video->GetMode();

    int name_table_addr = regs[2] << 10;
    int color_table_addr = regs[3] << 6;
    int pattern_table_addr = regs[4] << 11;
    int region_mask = ((regs[4] & 0x03) << 8) | 0xFF;
    int color_mask = ((regs[3] & 0x7F) << 3) | 0x07;
    int backdrop_color = regs[7] & 0x0F;
    backdrop_color = (backdrop_color > 0) ? backdrop_color : 1;
    int region = 0;

    switch (mode)
    {
        case 1:
        {
            int fg_color = (regs[7] >> 4) & 0x0F;
            int bg_color = backdrop_color;
            fg_color = (fg_color > 0) ? fg_color : backdrop_color;

            for (int line = 0; line < 192; line++)
            {
                int line_offset = line * GC_RESOLUTION_WIDTH;
                int tile_y = line >> 3;
                int tile_y_offset = line & 7;

                for (int tile_x = 0; tile_x < 40; tile_x++)
                {
                    int tile_number = (tile_y * 40) + tile_x;
                    int name_tile_addr = name_table_addr + tile_number;
                    int name_tile = vram[name_tile_addr & 0x3FFF];
                    u8 pattern_line = vram[(pattern_table_addr + (name_tile << 3) + tile_y_offset) & 0x3FFF];

                    int screen_offset = line_offset + (tile_x * 6);

                    for (int tile_pixel = 0; tile_pixel < 6; tile_pixel++)
                    {
                        int pixel = screen_offset + tile_pixel;
                        if (pixel < 256 * 256)
                            debug_background_buffer[pixel] = IsSetBit(pattern_line, 7 - tile_pixel) ? fg_color : bg_color;
                    }
                }
            }
            return;
        }
        case 2:
        {
            pattern_table_addr &= 0x2000;
            color_table_addr &= 0x2000;
            break;
        }
        case 4:
        {
            pattern_table_addr &= 0x2000;
            break;
        }
    }

    for (int line = 0; line < 192; line++)
    {
        int line_offset = line * GC_RESOLUTION_WIDTH;
        int tile_y = line >> 3;
        int tile_y_offset = line & 7;
        region = (tile_y & 0x18) << 5;

        for (int tile_x = 0; tile_x < 32; tile_x++)
        {
            int tile_number = (tile_y << 5) + tile_x;
            int name_tile_addr = name_table_addr + tile_number;
            int name_tile = vram[name_tile_addr & 0x3FFF];
            u8 pattern_line = 0;
            u8 color_line = 0;

            if (mode == 4)
            {
                int offset_color = pattern_table_addr + (name_tile << 3) + ((tile_y & 0x03) << 1) + (line & 0x04 ? 1 : 0);
                color_line = vram[offset_color & 0x3FFF];

                int left_color = color_line >> 4;
                int right_color = color_line & 0x0F;
                left_color = (left_color > 0) ? left_color : backdrop_color;
                right_color = (right_color > 0) ? right_color : backdrop_color;

                int screen_offset = line_offset + (tile_x << 3);

                for (int tile_pixel = 0; tile_pixel < 4; tile_pixel++)
                {
                    int pixel = screen_offset + tile_pixel;
                    if (pixel < 256 * 256)
                        debug_background_buffer[pixel] = left_color;
                }

                for (int tile_pixel = 4; tile_pixel < 8; tile_pixel++)
                {
                    int pixel = screen_offset + tile_pixel;
                    if (pixel < 256 * 256)
                        debug_background_buffer[pixel] = right_color;
                }

                continue;
            }
            else if (mode == 0)
            {
                pattern_line = vram[(pattern_table_addr + (name_tile << 3) + tile_y_offset) & 0x3FFF];
                color_line = vram[(color_table_addr + (name_tile >> 3)) & 0x3FFF];
            }
            else if (mode == 2)
            {
                name_tile += region;
                pattern_line = vram[(pattern_table_addr + ((name_tile & region_mask) << 3) + tile_y_offset) & 0x3FFF];
                color_line = vram[(color_table_addr + ((name_tile & color_mask) << 3) + tile_y_offset) & 0x3FFF];
            }

            int fg_color = color_line >> 4;
            int bg_color = color_line & 0x0F;
            fg_color = (fg_color > 0) ? fg_color : backdrop_color;
            bg_color = (bg_color > 0) ? bg_color : backdrop_color;

            int screen_offset = line_offset + (tile_x << 3);

            for (int tile_pixel = 0; tile_pixel < 8; tile_pixel++)
            {
                int pixel = screen_offset + tile_pixel;
                if (pixel < 256 * 256)
                    debug_background_buffer[pixel] = IsSetBit(pattern_line, 7 - tile_pixel) ? fg_color : bg_color;
            }
        }
    }
}

static void update_debug_tile_buffer(void)
{
    Video* video = gearcoleco->GetVideo();
    u8* vram = video->GetVRAM();
    u8* regs = video->GetRegisters();
    int mode = video->GetMode();

    int pattern_table_addr = (regs[4] & ((mode == 2) ? 0x04 : 0x07)) << 11;

    if (emu_debug_tile_color_mode)
    {
        int color_table_addr = regs[3] << 6;
        int backdrop_color = regs[7] & 0x0F;
        backdrop_color = (backdrop_color > 0) ? backdrop_color : 1;

        if (mode == 2)
        {
            pattern_table_addr &= 0x2000;
            color_table_addr &= 0x2000;
        }
        else if (mode == 4)
        {
            pattern_table_addr &= 0x2000;
        }

        for (int y = 0; y < 256; y++)
        {
            int width_y = (y * 256);
            int tile_y = y / 8;
            int offset_y = y & 0x7;

            for (int x = 0; x < 256; x++)
            {
                int tile_x = x / 8;
                int offset_x = 7 - (x & 0x7);
                int pixel = width_y + x;

                int tile_number = (tile_y * 32) + tile_x;
                u8 pattern_line = 0;
                u8 color_line = 0;
                int fg_color = 15;
                int bg_color = 0;

                if (mode == 1)
                {
                    fg_color = (regs[7] >> 4) & 0x0F;
                    bg_color = backdrop_color;
                    fg_color = (fg_color > 0) ? fg_color : backdrop_color;

                    int tile_data_addr = (pattern_table_addr + (tile_number * 8) + offset_y) & 0x3FFF;
                    pattern_line = vram[tile_data_addr];
                }
                else if (mode == 4)
                {
                    int offset_color = pattern_table_addr + (tile_number << 3) + ((tile_y & 0x03) << 1) + (offset_y & 0x04 ? 1 : 0);
                    color_line = vram[offset_color & 0x3FFF];

                    int left_color = color_line >> 4;
                    int right_color = color_line & 0x0F;
                    left_color = (left_color > 0) ? left_color : backdrop_color;
                    right_color = (right_color > 0) ? right_color : backdrop_color;

                    if ((x & 0x07) < 4)
                        debug_tile_buffer[pixel] = left_color;
                    else
                        debug_tile_buffer[pixel] = right_color;
                    continue;
                }
                else if (mode == 0)
                {
                    int tile_data_addr = (pattern_table_addr + (tile_number * 8) + offset_y) & 0x3FFF;
                    pattern_line = vram[tile_data_addr];
                    color_line = vram[(color_table_addr + (tile_number >> 3)) & 0x3FFF];

                    fg_color = color_line >> 4;
                    bg_color = color_line & 0x0F;
                    fg_color = (fg_color > 0) ? fg_color : backdrop_color;
                    bg_color = (bg_color > 0) ? bg_color : backdrop_color;
                }
                else if (mode == 2)
                {
                    int tile_data_addr = (pattern_table_addr + (tile_number * 8) + offset_y) & 0x3FFF;
                    pattern_line = vram[tile_data_addr];
                    color_line = vram[(color_table_addr + (tile_number * 8) + offset_y) & 0x3FFF];

                    fg_color = color_line >> 4;
                    bg_color = color_line & 0x0F;
                    fg_color = (fg_color > 0) ? fg_color : backdrop_color;
                    bg_color = (bg_color > 0) ? bg_color : backdrop_color;
                }
                else
                {
                    int tile_data_addr = (pattern_table_addr + (tile_number * 8) + offset_y) & 0x3FFF;
                    pattern_line = vram[tile_data_addr];
                    fg_color = 15;
                    bg_color = 0;
                }

                bool color_bit = IsSetBit(pattern_line, offset_x);
                debug_tile_buffer[pixel] = color_bit ? fg_color : bg_color;
            }
        }
    }
    else
    {
        for (int y = 0; y < 256; y++)
        {
            int width_y = (y * 256);
            int tile_y = y / 8;
            int offset_y = y & 0x7;

            for (int x = 0; x < 256; x++)
            {
                int tile_x = x / 8;
                int offset_x = 7 - (x & 0x7);
                int pixel = width_y + x;

                int tile_number = (tile_y * 32) + tile_x;

                int tile_data_addr = (pattern_table_addr + (tile_number * 8) + offset_y) & 0x3FFF;
                bool color = IsSetBit(vram[tile_data_addr], offset_x);

                debug_tile_buffer[pixel] = color ? 15 : 0;
            }
        }
    }
}

static void update_debug_sprite_buffers(void)
{
    Video* video = gearcoleco->GetVideo();
    u8* regs = video->GetRegisters();
    u8* vram = video->GetVRAM();

    int sprite_size = IsSetBit(regs[1], 1) ? 16 : 8;
    u16 sprite_attribute_addr = (regs[5] & 0x7F) << 7;
    u16 sprite_pattern_addr = (regs[6] & 0x07) << 11;

    for (int s = 0; s < GC_MAX_SPRITES; s++)
    {
        int sprite_attribute_offset = sprite_attribute_addr + (s << 2);
        int sprite_color = vram[sprite_attribute_offset + 3] & 0x0F;
        int sprite_tile = vram[sprite_attribute_offset + 2];
        sprite_tile &= (sprite_size == 16) ? 0xFC : 0xFF;

        for (int pixel_y = 0; pixel_y < sprite_size; pixel_y++)
        {
            int sprite_line_addr = (sprite_pattern_addr + (sprite_tile << 3) + pixel_y) & 0x3FFF;

            for (int pixel_x = 0; pixel_x < 16; pixel_x++)
            {
                if ((sprite_size == 8) && (pixel_x == 8))
                    break;

                int pixel = (pixel_y * 16) + pixel_x;

                bool sprite_pixel = false;

                if (pixel_x < 8)
                    sprite_pixel = IsSetBit(vram[sprite_line_addr], 7 - pixel_x);
                else
                    sprite_pixel = IsSetBit(vram[(sprite_line_addr + 16) & 0x3FFF], 15 - pixel_x);

                debug_sprite_buffers[s][pixel] = sprite_pixel ? sprite_color : 0;
            }
        }
    }
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
