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
#include <fstream>
#include <sstream>
#include <string.h>
#include <SDL3/SDL.h>
#include "gearcoleco.h"
#include "Adam.h"
#include "AdamMedia.h"
#include "F18A.h"
#include "sound_queue.h"
#include "config.h"
#include "rewind.h"
#include "runahead.h"
#include "events.h"
#include "gui_debug_trace_logger.h"
#include "no_bios.h"
#include "mcp/mcp_manager.h"
#include "utils.h"

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
u16* debug_f18a_nametable_buffer;
u16* debug_f18a_pattern_buffer;
u16* debug_f18a_sprite_buffers[GC_MAX_SPRITES];

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
static bool loading_softpatching;
static Cartridge::ForceConfiguration loading_config;
static GC_Machine loading_machine;
static int loading_adam_boot_mode;
static bool loading_adam_media_persistence;
static bool loading_adam_media_write_protected[GC_ADAM_MEDIA_SLOT_COUNT];
static char loading_bios_path[4096];
static char loading_adam_eos_path[4096];
static char loading_adam_smartwriter_path[4096];
static char loaded_content_path[4096];

struct AdamHostMediaRecord
{
    char source_path[4096];
    char working_path[4096];
    u32 base_crc;
};

static AdamHostMediaRecord adam_host_media[GC_ADAM_MEDIA_SLOT_COUNT];

static void save_ram(void);
static void load_ram(void);
static void reset_buffers(void);
static void apply_video_config(void);
static const char* get_mapper(Cartridge::CartridgeTypes type);
static const char* get_configurated_dir(int option, const char* path);
static void init_debug(void);
static void destroy_debug(void);
static void update_debug(void);
static void update_debug_background_buffer(void);
static void update_debug_tile_buffer(void);
static void update_debug_sprite_buffers(void);
static void update_debug_f18a_nametable_buffer(void);
static void update_debug_f18a_pattern_buffer(void);
static void update_debug_f18a_sprite_buffers(void);
static void debug_step_instruction(void);
static void reset_rewind_timing(void);
static int get_rewind_pop_budget(void);
static bool read_binary_file(const char* path, u8** data, size_t* size);
static bool read_binary_file_exact(const char* path, u8** data, size_t expected_size);
static u32 calculate_crc32(const u8* data, size_t size);
static bool prepare_adam_firmware_paths(void);
static bool load_adam_firmware_paths(void);
static bool load_adam_content(const char* file_path);
static bool load_adam_media_path(GC_AdamMediaSlot slot, GC_AdamMediaType type,
    const char* file_path, bool primary);
static bool resolve_adam_playlist(const char* playlist_path, char* media_path,
    size_t media_path_size, GC_AdamMediaType* type);
static GC_AdamMediaType adam_media_type_from_path(const char* path);
static GC_AdamMediaType detect_adam_media_path(const char* path);
static bool read_adam_media_path(const char* path, u8** data, size_t* size,
    GC_AdamMediaType* type);
static void clear_adam_host_media(GC_AdamMediaSlot slot);
static void clear_all_adam_host_media(void);
static void make_adam_working_path(const char* source_path, GC_AdamMediaType type,
    GC_AdamMediaSlot slot, u32 base_crc, char* path, size_t path_size);
static bool flush_all_adam_media(void);
static bool get_adam_state_path(int index, char* path, size_t path_size);

bool emu_init(void)
{
    emu_frame_buffer = new u8[EMU_FRAME_BUFFER_SIZE];
    audio_buffer = new s16[GC_AUDIO_BUFFER_SIZE];

    init_debug();
    reset_buffers();

    gearcoleco = new GearcolecoCore();
    gearcoleco->Init();

    mcp_manager = new McpManager();
    mcp_manager->Init(gearcoleco);

    sound_queue_init();
    rewind_init();
    runahead_init();

    audio_enabled = true;
    emu_audio_sync = true;
    emu_debug_disable_breakpoints = false;
    emu_debug_irq_breakpoints = false;
    emu_debug_command = Debug_Command_None;
    emu_debug_halt_step_frames_pending = 0;
    emu_debug_pc_changed = false;
    emu_debug_step_frames_pending = 0;
    emu_frame_counter = 0;
    emu_debug_tile_palette = 0;
    emu_debug_tile_color_mode = true;
    emu_debug_f18a_layer = 0;
    emu_debug_f18a_pattern_palette = 0;
    loaded_content_path[0] = '\0';
    clear_all_adam_host_media();

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
    if (!flush_all_adam_media())
        Error("Unable to flush one or more ADAM working copies during shutdown");
    SafeDelete(mcp_manager);
    rewind_destroy();
    runahead_destroy();
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
    GC_AdamMediaType media_type = detect_adam_media_path(loading_file_path);
    GC_Machine machine = loading_machine;

    if (machine == GC_MACHINE_AUTO)
        machine = (media_type == GC_ADAM_MEDIA_NONE && !ends_with_no_case(loading_file_path, ".m3u")) ?
            GC_MACHINE_COLECOVISION : GC_MACHINE_ADAM;

    if (machine == GC_MACHINE_ADAM)
        loading_result = load_adam_content(loading_file_path);
    else if (media_type != GC_ADAM_MEDIA_NONE || ends_with_no_case(loading_file_path, ".m3u"))
    {
        Error("ADAM media cannot be loaded while ColecoVision is selected");
        loading_result = false;
    }
    else
    {
        loading_result = gearcoleco->LoadROM(loading_file_path, &loading_config, loading_softpatching);
        if (loading_result)
        {
            clear_all_adam_host_media();
            strncpy_fit(loaded_content_path, loading_file_path, sizeof(loaded_content_path));
        }
    }
    loading_state.store(Loading_State_Finished);
}

bool emu_load_media_async(const char* file_path, Cartridge::ForceConfiguration config)
{
    if (loading_state.load() != Loading_State_None)
        return false;

    if (!IsValidPointer(file_path) || (file_path[0] == '\0'))
        return false;

    if (!flush_all_adam_media())
    {
        Error("Unable to flush dirty ADAM media; current content was kept loaded");
        return false;
    }

    gui_debug_trace_logger_reset();

    emu_debug_command = Debug_Command_None;
    reset_buffers();
    save_ram();

    strncpy(loading_file_path, file_path, sizeof(loading_file_path) - 1);
    loading_file_path[sizeof(loading_file_path) - 1] = '\0';
    loading_result = false;
    loading_softpatching = config_emulator.softpatching;
    loading_config = config;
    loading_machine = (GC_Machine)config_emulator.machine;
    loading_adam_boot_mode = config_emulator.adam_boot_mode;
    loading_adam_media_persistence = config_emulator.adam_media_persistence;
    memcpy(loading_adam_media_write_protected, config_emulator.adam_media_write_protected,
        sizeof(loading_adam_media_write_protected));
    if (!prepare_adam_firmware_paths())
        return false;
    gearcoleco->SetVideoChip((GC_VideoChip)config_video.video_chip);
    loading_state.store(Loading_State_Loading);
    if (loading_thread_active)
        loading_thread.join();
    loading_thread = std::thread(load_media_thread_func);
    loading_thread_active = true;
    return true;
}

bool emu_is_media_loading(void)
{
    return loading_state.load() == Loading_State_Loading;
}

static void apply_video_config(void)
{
    emu_set_overscan(config_debug.debug ? 0 : config_video.overscan);
    emu_video_no_sprite_limit(config_video.sprite_limit);
    if (config_video.palette == 2)
        emu_palette(config_video.color);
    else
        emu_predefined_palette(config_video.palette);
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
    apply_video_config();
    load_ram();

    if (config_debug.debug && (config_debug.dis_look_ahead_count > 0))
        gearcoleco->GetProcessor()->DisassembleAhead(config_debug.dis_look_ahead_count);

    update_savestates_data();
    rewind_reset();
    runahead_reset();

    return true;
}

static bool read_binary_file(const char* path, u8** data, size_t* size)
{
    if (!IsValidPointer(path) || !IsValidPointer(data) || !IsValidPointer(size) || (path[0] == '\0'))
        return false;

    *data = NULL;
    *size = 0;

    std::ifstream file;
    open_ifstream_utf8(file, path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return false;

    std::streamoff file_size = file.tellg();
    if ((file_size <= 0) || (file_size > 0x7FFFFFFF))
    {
        file.close();
        return false;
    }

    u8* buffer = new u8[(size_t)file_size];
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(buffer), file_size);
    bool read = file.good() || file.eof();
    std::streamsize count = file.gcount();
    file.close();

    if (!read || (count != file_size))
    {
        SafeDeleteArray(buffer);
        return false;
    }

    *data = buffer;
    *size = (size_t)file_size;
    return true;
}

static bool read_binary_file_exact(const char* path, u8** data, size_t expected_size)
{
    size_t size = 0;
    if (!read_binary_file(path, data, &size))
        return false;

    if (size != expected_size)
    {
        SafeDeleteArray(*data);
        return false;
    }

    return true;
}

static u32 calculate_crc32(const u8* data, size_t size)
{
    u32 crc = 0xFFFFFFFFU;

    for (size_t i = 0; i < size; i++)
    {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++)
            crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320U : 0);
    }

    return crc ^ 0xFFFFFFFFU;
}

static bool file_has_size(const char* path, size_t expected_size)
{
    std::ifstream file;
    open_ifstream_utf8(file, path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return false;

    std::streamoff size = file.tellg();
    file.close();
    return size == (std::streamoff)expected_size;
}

static bool prepare_adam_firmware_paths(void)
{
    strncpy_fit(loading_bios_path, config_emulator.bios_path.c_str(), sizeof(loading_bios_path));
    strncpy_fit(loading_adam_eos_path, config_emulator.adam_eos_path.c_str(),
        sizeof(loading_adam_eos_path));
    strncpy_fit(loading_adam_smartwriter_path, config_emulator.adam_smartwriter_path.c_str(),
        sizeof(loading_adam_smartwriter_path));

    char directory[4096];
    get_directory(loading_bios_path, directory, sizeof(directory));

    if ((loading_adam_eos_path[0] == '\0') && (loading_bios_path[0] != '\0'))
        join_path(directory, Adam::GetFirmwareMetadata(GC_ADAM_FIRMWARE_EOS)->aliases[0],
            loading_adam_eos_path, sizeof(loading_adam_eos_path));

    if ((loading_adam_smartwriter_path[0] == '\0') && (loading_bios_path[0] != '\0'))
    {
        const Adam::FirmwareMetadata* metadata =
            Adam::GetFirmwareMetadata(GC_ADAM_FIRMWARE_SMARTWRITER);
        for (int i = 0; i < 3; i++)
        {
            char candidate[4096];
            join_path(directory, metadata->aliases[i], candidate, sizeof(candidate));
            if (file_has_size(candidate, Adam::kSmartWriterROMSize))
            {
                strncpy_fit(loading_adam_smartwriter_path, candidate,
                    sizeof(loading_adam_smartwriter_path));
                break;
            }
        }

        if (loading_adam_smartwriter_path[0] == '\0')
            join_path(directory, metadata->aliases[0], loading_adam_smartwriter_path,
                sizeof(loading_adam_smartwriter_path));
    }

    return true;
}

static bool load_adam_firmware_paths(void)
{
    u8* os7 = NULL;
    u8* eos = NULL;
    u8* smartwriter = NULL;

    bool os7_loaded = read_binary_file_exact(loading_bios_path, &os7, Adam::kOS7ROMSize);
    bool eos_loaded = read_binary_file_exact(loading_adam_eos_path, &eos, Adam::kEOSROMSize);
    bool smartwriter_loaded = read_binary_file_exact(loading_adam_smartwriter_path, &smartwriter,
        Adam::kSmartWriterROMSize);

    if (!os7_loaded || !eos_loaded || !smartwriter_loaded)
    {
        Error("ADAM firmware is incomplete. Required: OS-7 colecovision.rom/coleco.rom/os7.u2 "
            "(8192 bytes), EOS eos.rom (8192 bytes), SmartWriter writer.rom/wp.rom/wp_r80.rom "
            "(32768 bytes). Searched: OS-7 '%s', EOS '%s', SmartWriter '%s'",
            loading_bios_path, loading_adam_eos_path, loading_adam_smartwriter_path);
        SafeDeleteArray(os7);
        SafeDeleteArray(eos);
        SafeDeleteArray(smartwriter);
        return false;
    }

    u32 os7_crc = calculate_crc32(os7, Adam::kOS7ROMSize);
    u32 eos_crc = calculate_crc32(eos, Adam::kEOSROMSize);
    u32 smartwriter_crc = calculate_crc32(smartwriter, Adam::kSmartWriterROMSize);
    if (os7_crc != Adam::GetFirmwareMetadata(GC_ADAM_FIRMWARE_OS7)->crc)
        Log("Warning: unknown OS-7 revision, CRC32 %08X", os7_crc);
    if (eos_crc != Adam::GetFirmwareMetadata(GC_ADAM_FIRMWARE_EOS)->crc)
        Log("Warning: unknown EOS revision, CRC32 %08X", eos_crc);
    if (smartwriter_crc != Adam::GetFirmwareMetadata(GC_ADAM_FIRMWARE_SMARTWRITER)->crc)
        Log("Warning: unknown SmartWriter revision, CRC32 %08X", smartwriter_crc);

    bool loaded = gearcoleco->LoadAdamFirmware(os7, Adam::kOS7ROMSize, eos,
        Adam::kEOSROMSize, smartwriter, Adam::kSmartWriterROMSize);
    if (loaded)
        loaded = gearcoleco->GetMemory()->LoadBiosFromBuffer(os7, Adam::kOS7ROMSize);

    SafeDeleteArray(os7);
    SafeDeleteArray(eos);
    SafeDeleteArray(smartwriter);
    return loaded;
}

static GC_AdamMediaType adam_media_type_from_path(const char* path)
{
    if (ends_with_no_case(path, ".ddp"))
        return GC_ADAM_MEDIA_DATA_PACK;
    if (ends_with_no_case(path, ".dsk"))
        return GC_ADAM_MEDIA_DISK;
    return GC_ADAM_MEDIA_NONE;
}

static bool read_adam_media_path(const char* path, u8** data, size_t* size,
    GC_AdamMediaType* type)
{
    if (!IsValidPointer(data) || !IsValidPointer(size) || !IsValidPointer(type))
        return false;

    *data = NULL;
    *size = 0;
    *type = GC_ADAM_MEDIA_NONE;

    u8* source = NULL;
    size_t source_size = 0;
    if (!read_binary_file(path, &source, &source_size))
        return false;

    GC_AdamMediaType source_type = adam_media_type_from_path(path);
    if (ends_with_no_case(path, ".zip"))
    {
        char media_name[512];
        bool extracted = AdamMedia::ExtractFromZip(source, source_size, data, size, type,
            media_name, sizeof(media_name));
        SafeDeleteArray(source);
        if (!extracted)
            return false;
        return true;
    }

    if (!AdamMedia::IsValidImageSize(source_type, source_size))
    {
        SafeDeleteArray(source);
        return false;
    }

    *data = source;
    *size = source_size;
    *type = source_type;
    return true;
}

static GC_AdamMediaType detect_adam_media_path(const char* path)
{
    GC_AdamMediaType type = adam_media_type_from_path(path);
    if ((type != GC_ADAM_MEDIA_NONE) || !ends_with_no_case(path, ".zip"))
        return type;

    u8* data = NULL;
    size_t size = 0;
    if (!read_adam_media_path(path, &data, &size, &type))
        return GC_ADAM_MEDIA_NONE;
    SafeDeleteArray(data);
    return type;
}

static bool resolve_adam_playlist(const char* playlist_path, char* media_path,
    size_t media_path_size, GC_AdamMediaType* type)
{
    u8* text = NULL;
    size_t text_size = 0;
    if (!read_binary_file(playlist_path, &text, &text_size))
    {
        Error("Unable to read ADAM playlist: %s", playlist_path);
        return false;
    }

    char directory[4096];
    get_directory(playlist_path, directory, sizeof(directory));
    media_path[0] = '\0';
    *type = GC_ADAM_MEDIA_NONE;
    int entries = 0;
    size_t position = 0;

    while (position < text_size)
    {
        size_t end = position;
        while ((end < text_size) && (text[end] != '\n'))
            end++;
        size_t next = end + 1;

        while ((position < end) && ((text[position] == ' ') || (text[position] == '\t') ||
            (text[position] == '\r') || ((position < 3) && (text[position] == 0xEF ||
            text[position] == 0xBB || text[position] == 0xBF))))
            position++;
        while ((end > position) && ((text[end - 1] == ' ') || (text[end - 1] == '\t') ||
            (text[end - 1] == '\r')))
            end--;

        if ((end > position) && (text[position] != '#'))
        {
            if (entries >= 64)
            {
                SafeDeleteArray(text);
                Error("ADAM playlist contains more than 64 entries: %s", playlist_path);
                return false;
            }
            if ((end - position) >= 4096)
            {
                SafeDeleteArray(text);
                Error("ADAM playlist entry is too long: %s", playlist_path);
                return false;
            }

            char entry[4096];
            memcpy(entry, text + position, end - position);
            entry[end - position] = '\0';

            char resolved[4096];
            if (!join_path(directory, entry, resolved, sizeof(resolved)))
            {
                SafeDeleteArray(text);
                return false;
            }

            u8* validation = NULL;
            size_t validation_size = 0;
            GC_AdamMediaType entry_type = GC_ADAM_MEDIA_NONE;
            bool valid = read_adam_media_path(resolved, &validation, &validation_size,
                &entry_type);
            SafeDeleteArray(validation);

            if (!valid || ((*type != GC_ADAM_MEDIA_NONE) && (*type != entry_type)))
            {
                SafeDeleteArray(text);
                Error("ADAM playlist contains missing, invalid, or mixed media: %s", resolved);
                return false;
            }

            if (entries == 0)
            {
                strncpy_fit(media_path, resolved, media_path_size);
                *type = entry_type;
            }
            entries++;
        }

        position = next;
    }

    SafeDeleteArray(text);
    if (entries == 0)
    {
        Error("ADAM playlist is empty: %s", playlist_path);
        return false;
    }

    if (entries > 1)
        Log("Desktop ADAM playlist loaded; media UI starts with entry 1 of %d", entries);
    return true;
}

static void clear_adam_host_media(GC_AdamMediaSlot slot)
{
    if ((slot < GC_ADAM_MEDIA_DISK_1) || (slot >= GC_ADAM_MEDIA_SLOT_COUNT))
        return;

    memset(&adam_host_media[slot], 0, sizeof(adam_host_media[slot]));
}

static void clear_all_adam_host_media(void)
{
    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
        clear_adam_host_media((GC_AdamMediaSlot)i);
}

static void make_adam_working_path(const char* source_path, GC_AdamMediaType type,
    GC_AdamMediaSlot slot, u32 base_crc, char* path, size_t path_size)
{
    char directory[4096];
    char name[1024];
    char filename[1400];
    get_directory(source_path, directory, sizeof(directory));
    get_filename_without_extension(source_path, name, sizeof(name));
    const char* extension = type == GC_ADAM_MEDIA_DATA_PACK ? "ddp" : "dsk";
    snprintf(filename, sizeof(filename), "%s.%08x.slot%d.gearcoleco.%s", name, base_crc,
        (int)slot + 1, extension);
    join_path(directory, filename, path, path_size);
}

static bool load_adam_media_path(GC_AdamMediaSlot slot, GC_AdamMediaType type,
    const char* file_path, bool primary)
{
    bool disk_slot = (slot == GC_ADAM_MEDIA_DISK_1) || (slot == GC_ADAM_MEDIA_DISK_2);
    if ((disk_slot && (type != GC_ADAM_MEDIA_DISK)) ||
        (!disk_slot && (type != GC_ADAM_MEDIA_DATA_PACK)))
    {
        Error("ADAM media type does not match slot %d", slot);
        return false;
    }

    u8* source = NULL;
    size_t size = 0;
    GC_AdamMediaType source_type = GC_ADAM_MEDIA_NONE;
    if (!read_adam_media_path(file_path, &source, &size, &source_type))
    {
        Error("Unable to read ADAM media: %s", file_path);
        return false;
    }

    if (source_type != type)
    {
        Error("ADAM media type does not match slot %d: %s", slot, file_path);
        SafeDeleteArray(source);
        return false;
    }

    if (ends_with_no_case(file_path, ".zip"))
        Log("ADAM media extracted from ZIP: %s", file_path);

    u32 base_crc = calculate_crc32(source, size);
    char working_path[4096];
    make_adam_working_path(file_path, type, slot, base_crc, working_path, sizeof(working_path));

    u8* mounted_data = source;
    u8* working = NULL;
    bool persistence = primary ? loading_adam_media_persistence :
        config_emulator.adam_media_persistence;
    bool configured_write_protected = primary ? loading_adam_media_write_protected[slot] :
        config_emulator.adam_media_write_protected[slot];
    if (persistence && read_binary_file_exact(working_path, &working, size))
    {
        mounted_data = working;
        Log("Loading ADAM working copy: %s", working_path);
    }

    bool write_protected = !persistence || configured_write_protected;
    bool loaded = gearcoleco->LoadAdamMediaFromBuffer(slot, type, mounted_data, size,
        write_protected, base_crc);

    if (loaded)
    {
        clear_adam_host_media(slot);
        strncpy_fit(adam_host_media[slot].source_path, file_path,
            sizeof(adam_host_media[slot].source_path));
        if (persistence)
            strncpy_fit(adam_host_media[slot].working_path, working_path,
                sizeof(adam_host_media[slot].working_path));
        adam_host_media[slot].base_crc = base_crc;
        if (primary)
            strncpy_fit(loaded_content_path, file_path, sizeof(loaded_content_path));
    }

    SafeDeleteArray(working);
    SafeDeleteArray(source);
    return loaded;
}

static bool load_adam_content(const char* file_path)
{
    char media_path[4096];
    strncpy_fit(media_path, file_path, sizeof(media_path));
    GC_AdamMediaType media_type = detect_adam_media_path(file_path);

    if (ends_with_no_case(file_path, ".m3u"))
    {
        if (!resolve_adam_playlist(file_path, media_path, sizeof(media_path), &media_type))
            return false;
    }

    if (media_type != GC_ADAM_MEDIA_NONE)
    {
        u8* validation = NULL;
        size_t validation_size = 0;
        GC_AdamMediaType validation_type = GC_ADAM_MEDIA_NONE;
        if (!read_adam_media_path(media_path, &validation, &validation_size, &validation_type))
            return false;
        bool valid = validation_type == media_type;
        SafeDeleteArray(validation);
        if (!valid)
        {
            Error("Invalid ADAM media size %zu: %s", validation_size, media_path);
            return false;
        }

        if (loading_adam_boot_mode == 2)
        {
            Error("ADAM cartridge boot was selected but no cartridge was loaded");
            return false;
        }

        if (!load_adam_firmware_paths())
            return false;

        gearcoleco->UnloadContent();
        clear_all_adam_host_media();
        GC_AdamMediaSlot slot = media_type == GC_ADAM_MEDIA_DATA_PACK ?
            GC_ADAM_MEDIA_DATA_PACK_1 : GC_ADAM_MEDIA_DISK_1;
        if (!load_adam_media_path(slot, media_type, media_path, true))
            return false;

        if (ends_with_no_case(file_path, ".m3u"))
            strncpy_fit(loaded_content_path, file_path, sizeof(loaded_content_path));
        return true;
    }

    if (!load_adam_firmware_paths())
        return false;

    if (!gearcoleco->LoadROM(file_path, &loading_config, loading_softpatching))
    {
        Error("Invalid ADAM cartridge: %s", file_path);
        return false;
    }

    clear_all_adam_host_media();
    GC_AdamBootMode boot_mode = loading_adam_boot_mode == 1 ? GC_ADAM_BOOT_COMPUTER :
        GC_ADAM_BOOT_CARTRIDGE;
    if (!gearcoleco->StartAdam(boot_mode))
        return false;

    strncpy_fit(loaded_content_path, file_path, sizeof(loaded_content_path));
    return true;
}

static bool write_adam_working_copy(GC_AdamMediaSlot slot)
{
    AdamMedia* media = gearcoleco->GetAdamMedia(slot);
    if (!IsValidPointer(media) || !media->IsInserted() || !media->IsDirty())
        return true;

    AdamHostMediaRecord* record = &adam_host_media[slot];
    if (record->working_path[0] == '\0')
        return false;

    std::string temporary_path(record->working_path);
    temporary_path += ".tmp";
    std::ofstream file;
    open_ofstream_utf8(file, temporary_path.c_str(), std::ios::out | std::ios::binary |
        std::ios::trunc);
    if (!file.is_open())
    {
        Error("Unable to open ADAM working copy for writing: %s", temporary_path.c_str());
        return false;
    }

    file.write(reinterpret_cast<const char*>(media->GetData()), media->GetSize());
    file.close();
    if (!file.good())
    {
        SDL_RemovePath(temporary_path.c_str());
        Error("Unable to write complete ADAM working copy: %s", temporary_path.c_str());
        return false;
    }

    if (!SDL_RenamePath(temporary_path.c_str(), record->working_path))
    {
        SDL_RemovePath(temporary_path.c_str());
        Error("Unable to replace ADAM working copy %s: %s", record->working_path,
            SDL_GetError());
        return false;
    }

    media->ClearDirty();
    Log("ADAM working copy saved: %s", record->working_path);
    return true;
}

static bool flush_all_adam_media(void)
{
    bool flushed = true;
    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
    {
        if (!write_adam_working_copy((GC_AdamMediaSlot)i))
            flushed = false;
    }
    return flushed;
}

bool emu_start_adam(void)
{
    if ((loading_state.load() != Loading_State_None) || !flush_all_adam_media())
        return false;

    prepare_adam_firmware_paths();
    if (!load_adam_firmware_paths())
        return false;

    save_ram();
    gearcoleco->UnloadContent();
    clear_all_adam_host_media();
    loaded_content_path[0] = '\0';
    reset_buffers();
    gearcoleco->SetVideoChip(GC_VIDEO_CHIP_TMS9918A);
    if (!gearcoleco->StartAdam(GC_ADAM_BOOT_COMPUTER))
        return false;

    emu_audio_reset();
    apply_video_config();
    update_savestates_data();
    rewind_reset();
    runahead_reset();
    return true;
}

bool emu_unload_content(void)
{
    if ((loading_state.load() != Loading_State_None) || !flush_all_adam_media())
        return false;

    save_ram();
    gearcoleco->UnloadContent();
    clear_all_adam_host_media();
    loaded_content_path[0] = '\0';
    reset_buffers();
    rewind_reset();
    runahead_reset();
    update_savestates_data();
    return true;
}

bool emu_load_adam_firmware(GC_AdamFirmware firmware, const char* file_path)
{
    if ((loading_state.load() != Loading_State_None) ||
        (firmware < GC_ADAM_FIRMWARE_OS7) || (firmware >= GC_ADAM_FIRMWARE_COUNT))
        return false;

    const Adam::FirmwareMetadata* metadata = Adam::GetFirmwareMetadata(firmware);
    u8* data = NULL;
    if (!read_binary_file_exact(file_path, &data, metadata->size))
    {
        Error("Invalid ADAM firmware role %d: %s", firmware, file_path ? file_path : "");
        return false;
    }

    bool loaded = gearcoleco->LoadAdamFirmware(firmware, data, metadata->size);
    if (loaded && (firmware == GC_ADAM_FIRMWARE_OS7))
        loaded = gearcoleco->GetMemory()->LoadBiosFromBuffer(data, metadata->size);
    SafeDeleteArray(data);
    return loaded;
}

bool emu_is_adam_firmware_loaded(GC_AdamFirmware firmware)
{
    return (loading_state.load() == Loading_State_None) &&
        (firmware >= GC_ADAM_FIRMWARE_OS7) && (firmware < GC_ADAM_FIRMWARE_COUNT) &&
        (gearcoleco->GetAdam()->GetFirmwareCRC(firmware) != 0);
}

u32 emu_get_adam_firmware_crc(GC_AdamFirmware firmware)
{
    if ((loading_state.load() != Loading_State_None) ||
        (firmware < GC_ADAM_FIRMWARE_OS7) || (firmware >= GC_ADAM_FIRMWARE_COUNT))
        return 0;
    return gearcoleco->GetAdam()->GetFirmwareCRC(firmware);
}

bool emu_insert_adam_media(GC_AdamMediaSlot slot, const char* file_path)
{
    if ((loading_state.load() != Loading_State_None) ||
        (gearcoleco->GetMachine() != GC_MACHINE_ADAM) ||
        (slot < GC_ADAM_MEDIA_DISK_1) || (slot >= GC_ADAM_MEDIA_SLOT_COUNT))
        return false;

    GC_AdamMediaType type = detect_adam_media_path(file_path);
    bool disk_slot = (slot == GC_ADAM_MEDIA_DISK_1) || (slot == GC_ADAM_MEDIA_DISK_2);
    if ((disk_slot && (type != GC_ADAM_MEDIA_DISK)) ||
        (!disk_slot && (type != GC_ADAM_MEDIA_DATA_PACK)))
        return false;

    if (!write_adam_working_copy(slot))
        return false;

    bool loaded = load_adam_media_path(slot, type, file_path, false);
    if (loaded)
    {
        rewind_reset();
        runahead_reset();
    }
    return loaded;
}

bool emu_save_adam_media(GC_AdamMediaSlot slot)
{
    if ((loading_state.load() != Loading_State_None) ||
        (slot < GC_ADAM_MEDIA_DISK_1) || (slot >= GC_ADAM_MEDIA_SLOT_COUNT))
        return false;
    bool saved = write_adam_working_copy(slot);
    if (saved)
    {
        rewind_reset();
        runahead_reset();
    }
    return saved;
}

bool emu_eject_adam_media(GC_AdamMediaSlot slot)
{
    if ((loading_state.load() != Loading_State_None) ||
        (slot < GC_ADAM_MEDIA_DISK_1) || (slot >= GC_ADAM_MEDIA_SLOT_COUNT) ||
        !write_adam_working_copy(slot))
        return false;

    gearcoleco->EjectAdamMedia(slot);
    clear_adam_host_media(slot);
    rewind_reset();
    runahead_reset();
    return true;
}

bool emu_set_adam_media_write_protected(GC_AdamMediaSlot slot, bool write_protected)
{
    if (loading_state.load() != Loading_State_None)
        return false;
    AdamMedia* media = gearcoleco->GetAdamMedia(slot);
    if (!IsValidPointer(media) || !media->IsInserted())
        return false;
    media->SetWriteProtected(write_protected);
    rewind_reset();
    runahead_reset();
    return true;
}

bool emu_get_adam_media_info(GC_AdamMediaSlot slot, Emu_AdamMediaInfo* info)
{
    if (!IsValidPointer(info))
        return false;
    memset(info, 0, sizeof(*info));
    if ((loading_state.load() != Loading_State_None) ||
        (slot < GC_ADAM_MEDIA_DISK_1) ||
        (slot >= GC_ADAM_MEDIA_SLOT_COUNT))
        return false;

    AdamMedia* media = gearcoleco->GetAdamMedia(slot);
    if (!IsValidPointer(media) || !media->IsInserted())
        return true;

    info->inserted = true;
    info->write_protected = media->IsWriteProtected();
    info->dirty = media->IsDirty();
    info->type = media->GetType();
    info->size = media->GetSize();
    info->base_crc = media->GetBaseCRC();
    strncpy_fit(info->path, adam_host_media[slot].source_path, sizeof(info->path));
    strncpy_fit(info->working_path, adam_host_media[slot].working_path,
        sizeof(info->working_path));
    return true;
}

void emu_adam_key_pressed(GC_AdamKey key)
{
    if (loading_state.load() == Loading_State_None)
        gearcoleco->AdamKeyPressed(key);
}

void emu_adam_key_released(GC_AdamKey key)
{
    if (loading_state.load() == Loading_State_None)
        gearcoleco->AdamKeyReleased(key);
}

void emu_adam_release_all_keys(void)
{
    if (loading_state.load() == Loading_State_None)
        gearcoleco->AdamReleaseAllKeys();
}

GC_Machine emu_get_machine(void)
{
    return loading_state.load() == Loading_State_None ? gearcoleco->GetMachine() : GC_MACHINE_AUTO;
}

const char* emu_get_content_path(void)
{
    return loaded_content_path;
}

const char* emu_get_content_name(void)
{
    if (loaded_content_path[0] != '\0')
        return get_filename(loaded_content_path);
    return gearcoleco->GetMachine() == GC_MACHINE_ADAM ? "ADAM" : "";
}

void emu_render_current_frame(void)
{
    if (emu_is_empty())
        return;

    GC_RuntimeInfo runtime;
    gearcoleco->GetRuntimeInfo(runtime);
    int size = runtime.screen_width * runtime.screen_height;
    u16* src_buffer = gearcoleco->GetVideo()->GetFrameBuffer();

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
    if (loading_state.load() != Loading_State_None)
        return;

    emu_mcp_pump_commands();

    if (emu_is_empty())
        return;

    int sampleCount = 0;
    bool frame_executed = false;
    bool frame_completed = false;

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
            Debug_Command debug_command = emu_debug_command;
            rewind_commit_seek();
            breakpoint_hit = gearcoleco->RunToVBlank(emu_frame_buffer, audio_buffer, &sampleCount, &debug_run);
            frame_executed = true;

            if (!breakpoint_hit && (debug_command == Debug_Command_StepFrame || debug_command == Debug_Command_Continue))
                frame_completed = true;
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

            int runahead = runahead_get_frames();
            if (runahead > 0)
                runahead_run(runahead, emu_frame_buffer, audio_buffer, &sampleCount);
            else
                gearcoleco->RunToVBlank(emu_frame_buffer, audio_buffer, &sampleCount);

            frame_executed = true;
            frame_completed = true;
        }
    }

    if (frame_executed)
    {
        if (frame_completed)
            emu_frame_counter++;
        rewind_push();
    }

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
    return !gearcoleco->IsReady();
}

bool emu_is_bios_loaded(void)
{
    return gearcoleco->GetMemory()->IsBiosLoaded();
}

void emu_reset(Cartridge::ForceConfiguration config)
{
    gui_debug_trace_logger_reset();
    emu_debug_command = Debug_Command_None;
    emu_debug_halt_step_frames_pending = 0;
    emu_debug_step_frames_pending = 0;
    emu_debug_pc_changed = true;
    emu_frame_counter = 0;
    reset_buffers();
    reset_rewind_timing();
    emu_audio_reset();
    save_ram();
    gearcoleco->SetVideoChip((GC_VideoChip)config_video.video_chip);
    gearcoleco->ResetROM(&config);
    apply_video_config();
    load_ram();
    rewind_reset();
    runahead_reset();
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
        gui_debug_trace_logger_reset();
        save_ram();
        gearcoleco->ResetROM(&config);
        gearcoleco->LoadRam(file_path, true);
        rewind_reset();
        runahead_reset();
    }
}

void emu_save_state_slot(int index)
{
    if (!emu_is_empty())
    {
        if (gearcoleco->GetMachine() == GC_MACHINE_ADAM)
        {
            char state_path[4096];
            if (get_adam_state_path(index, state_path, sizeof(state_path)))
                gearcoleco->SaveState(state_path, -1, true);
            update_savestates_data();
            return;
        }

        const char* dir = get_configurated_dir(config_emulator.savestates_dir_option, config_emulator.savestates_path.c_str());
        gearcoleco->SaveState(dir, index, true);
        update_savestates_data();
    }
}

void emu_load_state_slot(int index)
{
    if (!emu_is_empty())
    {
        if (gearcoleco->GetMachine() == GC_MACHINE_ADAM)
        {
            char state_path[4096];
            if (get_adam_state_path(index, state_path, sizeof(state_path)) &&
                gearcoleco->LoadState(state_path, -1))
            {
                events_sync_input();
                rewind_reset();
                runahead_reset();
            }
            return;
        }

        const char* dir = get_configurated_dir(config_emulator.savestates_dir_option, config_emulator.savestates_path.c_str());
        if (gearcoleco->LoadState(dir, index))
        {
            events_sync_input();
            rewind_reset();
            runahead_reset();
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
            runahead_reset();
        }
    }
}

void emu_get_runtime(GC_RuntimeInfo& runtime)
{
    gearcoleco->GetRuntimeInfo(runtime);
}

double emu_get_frame_rate(void)
{
    if (!IsValidPointer(gearcoleco))
        return 60.0;

    GC_RuntimeInfo runtime;
    emu_get_runtime(runtime);

    return runtime.fps;
}

void emu_get_info(char* info, int buffer_size)
{
    if (!emu_is_empty())
    {
        if (gearcoleco->GetMachine() == GC_MACHINE_ADAM)
        {
            int inserted = 0;
            int dirty = 0;
            for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
            {
                AdamMedia* media = gearcoleco->GetAdamMedia((GC_AdamMediaSlot)i);
                if (IsValidPointer(media) && media->IsInserted())
                {
                    inserted++;
                    if (media->IsDirty())
                        dirty++;
                }
            }

            GC_RuntimeInfo runtime;
            gearcoleco->GetRuntimeInfo(runtime);
            snprintf(info, buffer_size,
                "Machine: ADAM\nBoot Mode: %s\nContent: %s\nMounted Media: %d\nDirty Media: %d\nScreen Resolution: %dx%d",
                gearcoleco->GetAdamBootMode() == GC_ADAM_BOOT_CARTRIDGE ? "Cartridge" : "Computer",
                emu_get_content_name(), inserted, dirty, runtime.screen_width, runtime.screen_height);
            return;
        }

        Cartridge* cart = gearcoleco->GetCartridge();
        GC_RuntimeInfo runtime;
        gearcoleco->GetRuntimeInfo(runtime);

        const char* filename = cart->GetFileName();
        const char* is_in_database = cart->IsInGameDatabase() ? "YES" : "NO";
        const char* pal = cart->IsPAL() ? "PAL" : "NTSC";
        const char* checksum = cart->IsValidROM() ? "VALID" : "FAILED";
        int rom_banks = cart->GetROMBankCount();
        const char* mapper = get_mapper(cart->GetType());

        snprintf(info, buffer_size, "File Name: %s\nInternal DB: %s\nMapper: %s\nRefresh Rate: %s\nCartridge Header: %s\nROM Banks: %d\nScreen Resolution: %dx%d", filename, is_in_database, mapper, pal, checksum, rom_banks, runtime.screen_width, runtime.screen_height);
    }
    else
    {
        snprintf(info, buffer_size, "There is no content loaded!");
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
    emu_debug_step_frames(1);
}

void emu_debug_step_frames(int frames)
{
    if (frames < 1)
        frames = 1;

    gearcoleco->Pause(false);
    emu_debug_step_frames_pending += frames;
    emu_debug_command = Debug_Command_StepFrame;
}

void emu_debug_break(void)
{
    gearcoleco->Pause(false);
    if (emu_debug_command == Debug_Command_Continue || emu_debug_command == Debug_Command_StepFrame)
    {
        emu_debug_step_frames_pending = 0;
        emu_debug_command = Debug_Command_Step;
    }
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

void emu_set_disassembler_syntax(int syntax)
{
#if !defined(GEARCOLECO_DISABLE_DISASSEMBLER)
    if (IsValidPointer(gearcoleco))
        gearcoleco->GetProcessor()->SetDisassemblerSyntax((GC_Disassembler_Syntax)syntax);
#else
    UNUSED(syntax);
#endif
}

void emu_mcp_start(void)
{
    mcp_manager->Start();
}

void emu_mcp_stop(void)
{
    mcp_manager->Stop();
}

void emu_mcp_set_transport(int mode, int port, const char* address)
{
    mcp_manager->SetTransportMode((McpTransportMode)mode, port, address);
}

bool emu_mcp_is_running(void)
{
    return mcp_manager && mcp_manager->IsRunning();
}

int emu_mcp_get_transport_mode(void)
{
    return mcp_manager ? mcp_manager->GetTransportMode() : -1;
}

const char* emu_mcp_get_http_address(void)
{
    return mcp_manager ? mcp_manager->GetTcpAddress() : "";
}

int emu_mcp_get_http_port(void)
{
    return mcp_manager ? mcp_manager->GetTcpPort() : 0;
}

void emu_mcp_pump_commands(void)
{
    mcp_manager->PumpCommands(gearcoleco);
}

bool emu_load_bios(const char* file_path)
{
    return emu_load_adam_firmware(GC_ADAM_FIRMWARE_OS7, file_path);
}

void emu_video_no_sprite_limit(bool enabled)
{
    gearcoleco->GetVideo()->SetNoSpriteLimit(enabled);
}

void emu_set_video_chip(int video_chip)
{
    gearcoleco->SetVideoChip((GC_VideoChip)video_chip);
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
    if (!gearcoleco->IsReady())
        return;

    GC_RuntimeInfo runtime;
    emu_get_runtime(runtime);

    stbi_write_png(file_path, runtime.screen_width, runtime.screen_height, 4, emu_frame_buffer, runtime.screen_width * 4);

    Log("Screenshot saved to %s", file_path);
}

void emu_save_sprite(const char* file_path, int index)
{
    if (!gearcoleco->IsReady())
        return;

    Video* video = gearcoleco->GetVideo();
    int sprite_size;
    u8* buffer;
    if (video->IsF18AHardware())
    {
        update_debug_f18a_sprite_buffers();
        sprite_size = emu_debug_f18a_sprite_sizes[index];
        video->Render32bit(debug_f18a_sprite_buffers[index], emu_debug_f18a_sprite_buffers[index], GC_PIXEL_RGBA8888, 16 * 16);
        buffer = emu_debug_f18a_sprite_buffers[index];
    }
    else
    {
        update_debug_sprite_buffers();
        sprite_size = IsSetBit(video->GetRegisters()[1], 1) ? 16 : 8;
        video->Render32bit(debug_sprite_buffers[index], emu_debug_sprite_buffers[index], GC_PIXEL_RGBA8888, 16 * 16);
        buffer = emu_debug_sprite_buffers[index];
    }

    stbi_write_png(file_path, sprite_size, sprite_size, 4, buffer, 16 * 4);

    Log("Sprite saved to %s", file_path);
}

void emu_save_background(const char* file_path)
{
    if (!gearcoleco->IsReady())
        return;

    Video* video = gearcoleco->GetVideo();
    if (video->IsF18AHardware())
    {
        update_debug_f18a_nametable_buffer();
        video->Render32bit(debug_f18a_nametable_buffer, emu_debug_f18a_nametable_buffer,
            GC_PIXEL_RGBA8888, GC_VIDEO_MAX_WIDTH * GC_VIDEO_MAX_HEIGHT);
        stbi_write_png(file_path, video->GetScreenWidth(), video->GetScreenHeight(), 4,
            emu_debug_f18a_nametable_buffer, GC_VIDEO_MAX_WIDTH * 4);
    }
    else
    {
        update_debug_background_buffer();
        video->Render32bit(debug_background_buffer, emu_debug_background_buffer, GC_PIXEL_RGBA8888, 256 * 256);
        stbi_write_png(file_path, 256, 192, 4, emu_debug_background_buffer, 256 * 4);
    }

    Log("Background saved to %s", file_path);
}

void emu_save_tiles(const char* file_path)
{
    if (!gearcoleco->IsReady())
        return;

    Video* video = gearcoleco->GetVideo();
    if (video->IsF18AHardware())
    {
        update_debug_f18a_pattern_buffer();
        video->Render32bit(debug_f18a_pattern_buffer, emu_debug_f18a_pattern_buffer, GC_PIXEL_RGBA8888, 256 * 256);
        u8* regs = video->GetRegisters();
        int mode = ((regs[0] & 0x04) << 1) | ((regs[0] & 0x02) << 1) | ((regs[1] & 0x08) >> 2) | ((regs[1] & 0x10) >> 4);
        int height = mode == 4 ? 192 : 64;
        stbi_write_png(file_path, 256, height, 4, emu_debug_f18a_pattern_buffer, 256 * 4);
    }
    else
    {
        update_debug_tile_buffer();
        video->Render32bit(debug_tile_buffer, emu_debug_tile_buffer, GC_PIXEL_RGBA8888, 256 * 256);
        stbi_write_png(file_path, 256, 256, 4, emu_debug_tile_buffer, 256 * 4);
    }

    Log("Pattern table saved to %s", file_path);
}

int emu_get_screenshot_png(unsigned char** out_buffer)
{
    if (!gearcoleco->IsReady())
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
    if (!gearcoleco->IsReady())
        return 0;

    if (sprite_index < 0 || sprite_index >= GC_MAX_SPRITES)
        return 0;

    Video* video = gearcoleco->GetVideo();
    int sprite_size;
    u8* buffer;
    if (video->IsF18AHardware())
    {
        update_debug_f18a_sprite_buffers();
        sprite_size = emu_debug_f18a_sprite_sizes[sprite_index];
        video->Render32bit(debug_f18a_sprite_buffers[sprite_index],
            emu_debug_f18a_sprite_buffers[sprite_index], GC_PIXEL_RGBA8888, 16 * 16);
        buffer = emu_debug_f18a_sprite_buffers[sprite_index];
    }
    else
    {
        update_debug_sprite_buffers();
        sprite_size = IsSetBit(video->GetRegisters()[1], 1) ? 16 : 8;
        video->Render32bit(debug_sprite_buffers[sprite_index],
            emu_debug_sprite_buffers[sprite_index], GC_PIXEL_RGBA8888, 16 * 16);
        buffer = emu_debug_sprite_buffers[sprite_index];
    }

    if (!buffer)
        return 0;

    int len = 0;
    *out_buffer = stbi_write_png_to_mem(buffer, 16 * 4, sprite_size, sprite_size, 4, &len);

    return len;
}

void emu_start_vgm_recording(const char* file_path)
{
    if (!gearcoleco->IsReady())
        return;

    if (gearcoleco->GetAudio()->IsVgmRecording())
        emu_stop_vgm_recording();

    GC_RuntimeInfo runtime;
    gearcoleco->GetRuntimeInfo(runtime);

    bool is_pal = (runtime.region == Region_PAL);
    int clock_rate = is_pal ? GC_MASTER_CLOCK_PAL : GC_MASTER_CLOCK_NTSC;
    VgmMetadata metadata;
    Cartridge* cartridge = gearcoleco->GetCartridge();
    metadata.game_name = gearcoleco->GetMachine() == GC_MACHINE_ADAM ? emu_get_content_name() :
        (cartridge->IsInGameDatabase() ? cartridge->GetGameDatabaseName() : cartridge->GetFileName());
    metadata.system_name = gearcoleco->GetMachine() == GC_MACHINE_ADAM ? "Coleco ADAM" : "ColecoVision";
    metadata.comment = "Created with " GEARCOLECO_TITLE " " GEARCOLECO_VERSION;

    if (gearcoleco->GetAudio()->StartVgmRecording(file_path, clock_rate, is_pal, metadata))
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
    for (int i = 0; i < EMU_FRAME_BUFFER_SIZE; i++)
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

static bool get_adam_state_path(int index, char* path, size_t path_size)
{
    if (!IsValidPointer(path) || (path_size == 0) || (index < 1))
        return false;

    char directory[4096];
    if (config_emulator.savestates_dir_option == Directory_Location_ROM)
    {
        if (loaded_content_path[0] != '\0')
            get_directory(loaded_content_path, directory, sizeof(directory));
        else
            strncpy_fit(directory, config_root_path, sizeof(directory));
    }
    else
    {
        const char* configured = get_configurated_dir(config_emulator.savestates_dir_option,
            config_emulator.savestates_path.c_str());
        strncpy_fit(directory, configured ? configured : config_root_path, sizeof(directory));
    }

    char name[1024];
    if (loaded_content_path[0] != '\0')
        get_filename_without_extension(loaded_content_path, name, sizeof(name));
    else
        strncpy_fit(name, "ADAM", sizeof(name));

    char filename[1100];
    snprintf(filename, sizeof(filename), "%s.state%d", name, index);
    return join_path(directory, filename, path, path_size);
}

static void init_debug(void)
{
    emu_debug_background_buffer = new u8[256 * 256 * 4];
    emu_debug_tile_buffer = new u8[32 * 32 * 64 * 4];
    emu_debug_f18a_nametable_buffer = new u8[GC_VIDEO_MAX_WIDTH * GC_VIDEO_MAX_HEIGHT * 4];
    emu_debug_f18a_pattern_buffer = new u8[256 * 256 * 4];
    debug_background_buffer = new u16[256 * 256];
    debug_tile_buffer = new u16[32 * 32 * 64];
    debug_f18a_nametable_buffer = new u16[GC_VIDEO_MAX_WIDTH * GC_VIDEO_MAX_HEIGHT];
    debug_f18a_pattern_buffer = new u16[256 * 256];

    memset(debug_tile_buffer, 0, 32 * 32 * 64 * sizeof(u16));
    memset(emu_debug_tile_buffer, 0, 32 * 32 * 64 * 4);
    memset(debug_f18a_nametable_buffer, 0, GC_VIDEO_MAX_WIDTH * GC_VIDEO_MAX_HEIGHT * sizeof(u16));
    memset(emu_debug_f18a_nametable_buffer, 0, GC_VIDEO_MAX_WIDTH * GC_VIDEO_MAX_HEIGHT * 4);
    memset(debug_f18a_pattern_buffer, 0, 256 * 256 * sizeof(u16));
    memset(emu_debug_f18a_pattern_buffer, 0, 256 * 256 * 4);

    for (int s = 0; s < GC_MAX_SPRITES; s++)
    {
        emu_debug_sprite_buffers[s] = new u8[16 * 16 * 4];
        debug_sprite_buffers[s] = new u16[16 * 16];
        emu_debug_f18a_sprite_buffers[s] = new u8[16 * 16 * 4];
        debug_f18a_sprite_buffers[s] = new u16[16 * 16];
        emu_debug_f18a_sprite_sizes[s] = 8;
        memset(debug_sprite_buffers[s], 0, 16 * 16 * sizeof(u16));
        memset(emu_debug_sprite_buffers[s], 0, 16 * 16 * 4);
        memset(debug_f18a_sprite_buffers[s], 0, 16 * 16 * sizeof(u16));
        memset(emu_debug_f18a_sprite_buffers[s], 0, 16 * 16 * 4);
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
    SafeDeleteArray(emu_debug_f18a_nametable_buffer);
    SafeDeleteArray(emu_debug_f18a_pattern_buffer);
    SafeDeleteArray(debug_f18a_nametable_buffer);
    SafeDeleteArray(debug_f18a_pattern_buffer);

    for (int s = 0; s < GC_MAX_SPRITES; s++)
    {
        SafeDeleteArray(emu_debug_sprite_buffers[s]);
        SafeDeleteArray(debug_sprite_buffers[s]);
        SafeDeleteArray(emu_debug_f18a_sprite_buffers[s]);
        SafeDeleteArray(debug_f18a_sprite_buffers[s]);
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

    if (video->IsF18AHardware())
    {
        if (config_debug.show_f18a_nametables)
        {
            update_debug_f18a_nametable_buffer();
            video->Render32bit(debug_f18a_nametable_buffer, emu_debug_f18a_nametable_buffer,
                GC_PIXEL_RGBA8888, GC_VIDEO_MAX_WIDTH * GC_VIDEO_MAX_HEIGHT);
        }
        if (config_debug.show_f18a_patterns)
        {
            update_debug_f18a_pattern_buffer();
            video->Render32bit(debug_f18a_pattern_buffer, emu_debug_f18a_pattern_buffer,
                GC_PIXEL_RGBA8888, 256 * 256);
        }
        if (config_debug.show_f18a_sprites)
        {
            update_debug_f18a_sprite_buffers();
            for (int s = 0; s < GC_MAX_SPRITES; s++)
            {
                video->Render32bit(debug_f18a_sprite_buffers[s],
                    emu_debug_f18a_sprite_buffers[s], GC_PIXEL_RGBA8888, 16 * 16);
            }
        }
    }
    else
    {
        if (config_debug.show_tms9918a_nametable)
        {
            update_debug_background_buffer();
            video->Render32bit(debug_background_buffer, emu_debug_background_buffer,
                GC_PIXEL_RGBA8888, 256 * 256);
        }
        if (config_debug.show_tms9918a_patterns)
        {
            update_debug_tile_buffer();
            video->Render32bit(debug_tile_buffer, emu_debug_tile_buffer,
                GC_PIXEL_RGBA8888, 32 * 32 * 64);
        }
        if (config_debug.show_tms9918a_sprites)
        {
            update_debug_sprite_buffers();
            for (int s = 0; s < GC_MAX_SPRITES; s++)
            {
                video->Render32bit(debug_sprite_buffers[s], emu_debug_sprite_buffers[s],
                    GC_PIXEL_RGBA8888, 16 * 16);
            }
        }
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
        case 3:
        {
            if (mode == 3)
                pattern_table_addr &= 0x2000;

            int fg_color = (regs[7] >> 4) & 0x0F;
            int bg_color = backdrop_color;
            fg_color = (fg_color > 0) ? fg_color : backdrop_color;

            for (int line = 0; line < 192; line++)
            {
                int line_offset = line * GC_RESOLUTION_WIDTH;
                int tile_y = line >> 3;
                int tile_y_offset = line & 7;
                int line_region = (tile_y & 0x18) << 5;

                for (int tile_x = 0; tile_x < 40; tile_x++)
                {
                    int tile_number = (tile_y * 40) + tile_x;
                    int name_tile_addr = name_table_addr + tile_number;
                    int name_tile = vram[name_tile_addr & 0x3FFF];

                    if (mode == 3)
                        name_tile = (name_tile + line_region) & region_mask;

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
            break;
        }
        case 5:
        case 7:
        {
            int fg_color = (regs[7] >> 4) & 0x0F;
            int bg_color = backdrop_color;
            fg_color = (fg_color > 0) ? fg_color : backdrop_color;

            for (int line = 0; line < 192; line++)
            {
                int line_offset = line * GC_RESOLUTION_WIDTH;

                for (int x = 0; x < GC_RESOLUTION_WIDTH; x++)
                    debug_background_buffer[line_offset + x] = bg_color;

                for (int tile_x = 0; tile_x < 40; tile_x++)
                {
                    int screen_offset = line_offset + (tile_x * 6);

                    for (int tile_pixel = 0; tile_pixel < 4; tile_pixel++)
                        debug_background_buffer[screen_offset + tile_pixel] = fg_color;
                }
            }
            return;
        }
        case 6:
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

            if ((mode == 4) || (mode == 6))
            {
                if (mode == 6)
                    name_tile = (name_tile + region) & region_mask;

                int offset_color = pattern_table_addr + (name_tile << 3) + ((line >> 2) & 0x07);
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

    bool split_pattern_table = (mode == 2) || (mode == 3) || (mode == 6);
    int pattern_table_addr = (regs[4] & (split_pattern_table ? 0x04 : 0x07)) << 11;

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

                if ((mode == 1) || (mode == 3))
                {
                    fg_color = (regs[7] >> 4) & 0x0F;
                    bg_color = backdrop_color;
                    fg_color = (fg_color > 0) ? fg_color : backdrop_color;

                    int tile_data_addr = (pattern_table_addr + (tile_number * 8) + offset_y) & 0x3FFF;
                    pattern_line = vram[tile_data_addr];
                }
                else if ((mode == 4) || (mode == 6))
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

static void update_debug_f18a_nametable_buffer(void)
{
    F18A* video = static_cast<F18A*>(gearcoleco->GetVideo());
    video->RenderDebugNameTable(debug_f18a_nametable_buffer, emu_debug_f18a_layer != 0);
}

static void update_debug_f18a_pattern_buffer(void)
{
    F18A* video = static_cast<F18A*>(gearcoleco->GetVideo());
    video->RenderDebugPatternTable(debug_f18a_pattern_buffer, emu_debug_f18a_pattern_palette);
}

static void update_debug_f18a_sprite_buffers(void)
{
    F18A* video = static_cast<F18A*>(gearcoleco->GetVideo());
    for (int s = 0; s < GC_MAX_SPRITES; s++)
    {
        emu_debug_f18a_sprite_sizes[s] = video->RenderDebugSprite(debug_f18a_sprite_buffers[s], s);
    }
}

void update_savestates_data(void)
{
    emu_savestates_generation++;

    if (emu_is_empty())
        return;

    for (int i = 0; i < 5; i++)
    {
        emu_savestates[i].rom_name[0] = 0;
        SafeDeleteArray(emu_savestates_screenshots[i].data);

        const char* dir = get_configurated_dir(config_emulator.savestates_dir_option, config_emulator.savestates_path.c_str());
        char adam_state_path[4096];
        bool adam = gearcoleco->GetMachine() == GC_MACHINE_ADAM;
        if (adam && !get_adam_state_path(i + 1, adam_state_path, sizeof(adam_state_path)))
            continue;

        if (!(adam ? gearcoleco->GetSaveStateHeader(-1, adam_state_path, &emu_savestates[i]) :
            gearcoleco->GetSaveStateHeader(i + 1, dir, &emu_savestates[i])))
            continue;

        if (emu_savestates[i].screenshot_size > 0)
        {
            emu_savestates_screenshots[i].data = new u8[emu_savestates[i].screenshot_size];
            emu_savestates_screenshots[i].size = emu_savestates[i].screenshot_size;
            if (adam)
                gearcoleco->GetSaveStateScreenshot(-1, adam_state_path,
                    &emu_savestates_screenshots[i]);
            else
                gearcoleco->GetSaveStateScreenshot(i + 1, dir,
                    &emu_savestates_screenshots[i]);
        }
    }
}
