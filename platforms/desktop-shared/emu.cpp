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

#include "../../src/gearcoleco.h"
#include "../audio-shared/Sound_Queue.h"

#define EMU_IMPORT
#include "emu.h"

static GearcolecoCore* gearcoleco;
static Sound_Queue* sound_queue;
static s16* audio_buffer;
static bool audio_enabled;
static bool debugging = false;
static bool debug_step = false;
static bool debug_next_frame = false;

u16* frame_buffer;
u16* debug_background_buffer;
u16* debug_tile_buffer;
u16* debug_sprite_buffers[64];

static void save_ram(void);
static void load_ram(void);
static const char* get_mapper(Cartridge::CartridgeTypes type);
static void init_debug(void);
static void destroy_debug(void);
static void update_debug(void);
static void update_debug_background_buffer(void);
static void update_debug_tile_buffer(void);
static void update_debug_sprite_buffers(void);

void emu_init(void)
{
    int screen_size = GC_RESOLUTION_MAX_WIDTH * GC_RESOLUTION_MAX_HEIGHT;

    emu_frame_buffer = new u8[screen_size * 3];
    frame_buffer = new u16[screen_size];
    
    for (int i=0, j=0; i < screen_size; i++, j+=3)
    {
        frame_buffer[i] = 0;
        emu_frame_buffer[j] = 0;
        emu_frame_buffer[j+1] = 0;
        emu_frame_buffer[j+2] = 0;
    }

    init_debug();

    gearcoleco = new GearcolecoCore();
    gearcoleco->Init();

    sound_queue = new Sound_Queue();
    sound_queue->start(44100, 2);

    audio_buffer = new s16[GC_AUDIO_BUFFER_SIZE];

    for (int i = 0; i < GC_AUDIO_BUFFER_SIZE; i++)
        audio_buffer[i] = 0;

    audio_enabled = true;
    emu_audio_sync = true;
    emu_debug_disable_breakpoints_cpu = false;
    emu_debug_disable_breakpoints_mem = false;
    emu_debug_tile_palette = 0;
    emu_savefiles_dir_option = 0;
    emu_savestates_dir_option = 0;
    emu_savefiles_path[0] = 0;
    emu_savestates_path[0] = 0;
}

void emu_destroy(void)
{
    save_ram();
    SafeDeleteArray(audio_buffer);
    SafeDelete(sound_queue);
    SafeDelete(gearcoleco);
    SafeDeleteArray(emu_frame_buffer);
    SafeDeleteArray(frame_buffer);
    destroy_debug();
}

void emu_load_rom(const char* file_path, Cartridge::ForceConfiguration config)
{
    save_ram();
    gearcoleco->LoadROM(file_path, &config);
    load_ram();
    emu_debug_continue();
}

void emu_update(void)
{
    if (!emu_is_empty())
    {
        int sampleCount = 0;

        if (!debugging || debug_step || debug_next_frame)
        {
            bool breakpoints = (!emu_debug_disable_breakpoints_cpu && !emu_debug_disable_breakpoints_mem) || IsValidPointer(gearcoleco->GetMemory()->GetRunToBreakpoint());

            if (gearcoleco->RunToVBlank(emu_frame_buffer, audio_buffer, &sampleCount, debug_step, breakpoints))
            {
                debugging = true;
            }

            debug_next_frame = false;
            debug_step = false;
        }

        update_debug();

        if ((sampleCount > 0) && !gearcoleco->IsPaused())
        {
            sound_queue->write(audio_buffer, sampleCount, emu_audio_sync);
        }
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

bool emu_is_empty(void)
{
    return !gearcoleco->GetCartridge()->IsReady();
}

bool emu_is_bios_loaded(void)
{
    return gearcoleco->GetMemory()->IsBiosLoaded();
}

void emu_reset(Cartridge::ForceConfiguration config)
{
    save_ram();
    gearcoleco->ResetROM(&config);
    load_ram();
}

void emu_dissasemble_rom(void)
{
    gearcoleco->SaveDisassembledROM();
}

void emu_audio_volume(float volume)
{
    audio_enabled = (volume > 0.0f);
    gearcoleco->SetSoundVolume(volume);
}

void emu_audio_reset(void)
{
    sound_queue->stop();
    sound_queue->start(44100, 2);
}

bool emu_is_audio_enabled(void)
{
    return audio_enabled;
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
        if ((emu_savestates_dir_option == 0) && (strcmp(emu_savestates_path, "")))
            gearcoleco->SaveState(emu_savestates_path, index);
        else
            gearcoleco->SaveState(index);
    }
}

void emu_load_state_slot(int index)
{
    if (!emu_is_empty())
    {
        if ((emu_savestates_dir_option == 0) && (strcmp(emu_savestates_path, "")))
            gearcoleco->LoadState(emu_savestates_path, index);
        else
            gearcoleco->LoadState(index);
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

void emu_get_info(char* info)
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

        sprintf(info, "File Name: %s\nMapper: %s\nRefresh Rate: %s\nCartridge Header: %s\nROM Banks: %d\nScreen Resolution: %dx%d", filename, mapper, pal, checksum, rom_banks, runtime.screen_width, runtime.screen_height);
    }
    else
    {
        sprintf(info, "No data!");
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
    gearcoleco->Pause(false);
}

void emu_load_bios(const char* file_path)
{
    gearcoleco->GetMemory()->LoadBios(file_path);
}

void emu_video_no_sprite_limit(bool enabled)
{
    gearcoleco->GetVideo()->SetNoSpriteLimit(enabled);
}

static void save_ram(void)
{
#ifdef DEBUG_GEARCOLECO
    emu_dissasemble_rom();
#endif

    if ((emu_savefiles_dir_option == 0) && (strcmp(emu_savefiles_path, "")))
        gearcoleco->SaveRam(emu_savefiles_path);
    else
        gearcoleco->SaveRam();
}

static void load_ram(void)
{
    if ((emu_savefiles_dir_option == 0) && (strcmp(emu_savefiles_path, "")))
        gearcoleco->LoadRam(emu_savefiles_path);
    else
        gearcoleco->LoadRam();
}

static const char* get_mapper(Cartridge::CartridgeTypes type)
{
    switch (type)
    {
    case Cartridge::CartridgeColecoVision:
        return "ColecoVision";
        break;
    case Cartridge::CartridgeMegaCart:
        return "MegaCart";
        break;
    case Cartridge::CartridgeActivisionCart:
        return "Activision";
        break;
    case Cartridge::CartridgeNotSupported:
        return "Not Supported";
        break;
    default:
        return "Undefined";
        break;
    }
}

static void init_debug(void)
{
    emu_debug_background_buffer = new u8[256 * 256 * 3];
    emu_debug_tile_buffer = new u8[32 * 32 * 64 * 3];
    debug_background_buffer = new u16[256 * 256];    
    debug_tile_buffer = new u16[32 * 32 * 64];

    for (int i=0,j=0; i < (32 * 32 * 64); i++,j+=3)
    {
        debug_tile_buffer[i] = 0;
        emu_debug_tile_buffer[j] = 0;
        emu_debug_tile_buffer[j+1] = 0;
        emu_debug_tile_buffer[j+2] = 0;
    }

    for (int s = 0; s < 64; s++)
    {
        emu_debug_sprite_buffers[s] = new u8[16 * 16 * 3];
        debug_sprite_buffers[s] = new u16[16 * 16];

        for (int i=0,j=0; i < (16 * 16); i++,j+=3)
        {
            debug_sprite_buffers[s][i] = 0;
            emu_debug_sprite_buffers[s][j] = 0;
            emu_debug_sprite_buffers[s][j+1] = 0;
            emu_debug_sprite_buffers[s][j+2] = 0;
        }
    }

    for (int i=0,j=0; i < (256 * 256); i++,j+=3)
    {
        debug_background_buffer[i] = 0;
        emu_debug_background_buffer[j] = 0;
        emu_debug_background_buffer[j+1] = 0;
        emu_debug_background_buffer[j+2] = 0;
    }
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

    update_debug_background_buffer();
    update_debug_tile_buffer();
    update_debug_sprite_buffers();

    Video* video = gearcoleco->GetVideo();

    video->Render24bit(debug_background_buffer, emu_debug_background_buffer, GC_PIXEL_RGB888, 256 * 256);
    video->Render24bit(debug_tile_buffer, emu_debug_tile_buffer, GC_PIXEL_RGB888, 32 * 32 * 64);

    for (int s = 0; s < 64; s++)
    {
        video->Render24bit(debug_sprite_buffers[s], emu_debug_sprite_buffers[s], GC_PIXEL_RGB888, 16 * 16);
    }
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
                int line_offset = line * GC_RESOLUTION_MAX_WIDTH;
                int tile_y = line >> 3;
                int tile_y_offset = line & 7;

                for (int tile_x = 0; tile_x < 40; tile_x++)
                {
                    int tile_number = (tile_y * 40) + tile_x;
                    int name_tile_addr = name_table_addr + tile_number;
                    int name_tile = vram[name_tile_addr];
                    u8 pattern_line = vram[pattern_table_addr + (name_tile << 3) + tile_y_offset];

                    int screen_offset = line_offset + (tile_x * 6);

                    for (int tile_pixel = 0; tile_pixel < 6; tile_pixel++)
                    {
                        int pixel = screen_offset + tile_pixel;
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
        int line_offset = line * GC_RESOLUTION_MAX_WIDTH;
        int tile_y = line >> 3;
        int tile_y_offset = line & 7;
        region = (tile_y & 0x18) << 5;

        for (int tile_x = 0; tile_x < 32; tile_x++)
        {
            int tile_number = (tile_y << 5) + tile_x;
            int name_tile_addr = name_table_addr + tile_number;
            int name_tile = vram[name_tile_addr];
            u8 pattern_line = 0;
            u8 color_line = 0;

            if (mode == 4)
            {
                int offset_color = pattern_table_addr + (name_tile << 3) + ((tile_y & 0x03) << 1) + (line & 0x04 ? 1 : 0);
                color_line = vram[offset_color];

                int left_color = color_line >> 4;
                int right_color = color_line & 0x0F;
                left_color = (left_color > 0) ? left_color : backdrop_color;
                right_color = (right_color > 0) ? right_color : backdrop_color;

                int screen_offset = line_offset + (tile_x << 3);

                for (int tile_pixel = 0; tile_pixel < 4; tile_pixel++)
                {
                    int pixel = screen_offset + tile_pixel;
                    debug_background_buffer[pixel] = left_color;
                }

                for (int tile_pixel = 4; tile_pixel < 8; tile_pixel++)
                {
                    int pixel = screen_offset + tile_pixel;
                    debug_background_buffer[pixel] = right_color;
                }

                continue;
            }
            else if (mode == 0)
            {
                pattern_line = vram[pattern_table_addr + (name_tile << 3) + tile_y_offset];
                color_line = vram[color_table_addr + (name_tile >> 3)];
            }
            else if (mode == 2)
            {
                name_tile += region;
                pattern_line = vram[pattern_table_addr + ((name_tile & region_mask) << 3) + tile_y_offset];
                color_line = vram[color_table_addr + ((name_tile & color_mask) << 3) + tile_y_offset];
            }

            int fg_color = color_line >> 4;
            int bg_color = color_line & 0x0F;
            fg_color = (fg_color > 0) ? fg_color : backdrop_color;
            bg_color = (bg_color > 0) ? bg_color : backdrop_color;

            int screen_offset = line_offset + (tile_x << 3);

            for (int tile_pixel = 0; tile_pixel < 8; tile_pixel++)
            {
                int pixel = screen_offset + tile_pixel;
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

            int tile_data_addr = (pattern_table_addr + (tile_number * 8) + (1 * offset_y)) & 0x3FFF;
            bool color = IsSetBit(vram[tile_data_addr], offset_x);

            u16 black = 0;

            u16 white = 15;

            debug_tile_buffer[pixel] = color ? white : black;
        }
    }
}

static void update_debug_sprite_buffers(void)
{
    GearcolecoCore* core = emu_get_core();
    Video* video = core->GetVideo();
    u8* regs = video->GetRegisters();
    u8* vram = video->GetVRAM();
    GC_RuntimeInfo runtime;
    emu_get_runtime(runtime);

    int sprite_size = IsSetBit(regs[1], 1) ? 16 : 8;
    u16 sprite_attribute_addr = (regs[5] & 0x7F) << 7;
    u16 sprite_pattern_addr = (regs[6] & 0x07) << 11;

    for (int s = 0; s < 32; s++)
    {
        int sprite_attribute_offset = sprite_attribute_addr + (s << 2);
        int sprite_color = vram[sprite_attribute_offset + 3] & 0x0F;
        int sprite_tile = vram[sprite_attribute_offset + 2];
        sprite_tile &= (sprite_size == 16) ? 0xFC : 0xFF;

        for (int pixel_y = 0; pixel_y < sprite_size; pixel_y++)
        {
            int sprite_line_addr = sprite_pattern_addr + (sprite_tile << 3) + pixel_y;

            for (int pixel_x = 0; pixel_x < 16; pixel_x++)
            {
                if ((sprite_size == 8) && (pixel_x == 8))
                    break;

                int pixel = (pixel_y * 16) + pixel_x;

                bool sprite_pixel = false;

                if (pixel_x < 8)
                    sprite_pixel = IsSetBit(vram[sprite_line_addr], 7 - pixel_x);
                else
                    sprite_pixel = IsSetBit(vram[sprite_line_addr + 16], 15 - pixel_x);

                debug_sprite_buffers[s][pixel] = sprite_pixel ? sprite_color : 0;
            }
        }
    }
}
