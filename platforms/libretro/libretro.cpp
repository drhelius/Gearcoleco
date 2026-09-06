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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include <stdio.h>
#include "libretro.h"
#include "libretro_core_options.h"

#include "../../src/gearcoleco.h"
#include "../../src/Mapper.h"
#include "../../src/Adam.h"
#include "../../src/AdamMedia.h"

#ifdef _WIN32
static const char slash = '\\';
#else
static const char slash = '/';
#endif

#define MAX_PADS 2
#define JOYPAD_BUTTONS 16
#define MAX_ADAM_DISK_IMAGES 64
#define RETRO_ADAM_SUBSYSTEM_ID 0xAD01
#define RETRO_ADAM_STATE_MAGIC 0x4D414447

#define RETRO_DEVICE_COLECOVISION RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_JOYPAD, 0)

static struct retro_log_callback logging;
retro_log_printf_t log_cb;
static char retro_system_directory[4096];
static char retro_save_directory[4096];
static char retro_game_path[4096];

static s16 audio_buf[GC_AUDIO_BUFFER_SIZE];
static int audio_sample_count = 0;
static int current_screen_width = 0;
static int current_screen_height = 0;
static float current_aspect_ratio = 0;
static bool allow_up_down = false;
static bool libretro_supports_bitmasks = false;
static int spinner_support = 0;
static int spinner_sensitivity = 1;
static float aspect_ratio = 0.0f;
static GC_VideoChip video_chip = GC_VIDEO_CHIP_AUTO;
static bool categories_supported = false;
static bool adam_cartridge_hardware = false;
static int adam_control_drive = -1;
static unsigned adam_primary_slot = 0;
static bool adam_computer_reset_latched = false;
static bool adam_writable_media = false;
static bool content_loaded = false;

static GearcolecoCore* core;
static u8* frame_buffer;
static Cartridge::ForceConfiguration config;
static const retro_vfs_interface* vfs_interface = NULL;

struct RetroAdamHostMedia
{
    char working_path[4096];
    u32 base_crc;
};

struct RetroAdamDiskImage
{
    char path[4096];
    char label[256];
    u8* data;
    size_t size;
};

struct RetroAdamDiskSet
{
    RetroAdamDiskImage images[MAX_ADAM_DISK_IMAGES];
    unsigned count;
    unsigned index;
    GC_AdamMediaType type;
    GC_AdamMediaSlot slot;
    bool ejected;
};

struct RetroAdamState
{
    u32 magic;
    u32 version;
    u32 count;
    u32 index;
    u8 type;
    u8 slot;
    u8 ejected;
    u8 reserved;
};

struct RetroAdamMediaBackup
{
    u8* data;
    size_t size;
    u32 base_crc;
    GC_AdamMediaType type;
    bool inserted;
    bool write_protected;
};

static RetroAdamHostMedia adam_host_media[GC_ADAM_MEDIA_SLOT_COUNT];
static RetroAdamDiskSet adam_disk_sets[GC_ADAM_MEDIA_SLOT_COUNT];
static RetroAdamDiskSet* adam_disk_set = &adam_disk_sets[0];
static unsigned adam_initial_image_index = 0;
static char adam_initial_image_path[4096];

static int joypad[MAX_PADS][JOYPAD_BUTTONS];
static int joypre[MAX_PADS][JOYPAD_BUTTONS];
static int joypad_ext[MAX_PADS][4];
static int joypre_ext[MAX_PADS][4];
static unsigned input_device[MAX_PADS] = {
    RETRO_DEVICE_COLECOVISION,
    RETRO_DEVICE_COLECOVISION
};
static bool mouse[2];
static bool mousepre[2];
static bool adam_retro_key_down[RETROK_LAST];
static int adam_key_references[GC_ADAM_KEY_COUNT];

static GC_Keys keymap[] = {
    Key_Up,
    Key_Down,
    Key_Left,
    Key_Right,
    Key_Right_Button,
    Key_Left_Button,
    Keypad_2,
    Keypad_1,
    Keypad_Asterisk,
    Keypad_Hash,
    Keypad_3,
    Keypad_4,
    Keypad_5,
    Keypad_6,
    Keypad_7,
    Keypad_8,
    Keypad_0,
    Keypad_9,
    Key_Blue,
    Key_Purple
};

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
    (void)level;
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
}

static retro_environment_t environ_cb;

static bool IsJoypadDevice(unsigned device)
{
    return ((device == RETRO_DEVICE_JOYPAD) || (device == RETRO_DEVICE_COLECOVISION));
}

static void clear_input_state(void);
static void clear_adam_keyboard_state(void);
static void reset_controller_devices(void);
static void apply_controller_device(unsigned port, unsigned device, bool log_device);
static bool read_file(const char* path, u8** data, size_t* size);
static bool read_game_info(const struct retro_game_info* info, u8** data, size_t* size);
static bool read_adam_game_info(const struct retro_game_info* info, u8** data, size_t* size,
    GC_AdamMediaType* type);
static bool ends_with_no_case(const char* text, const char* suffix);
static bool join_path(const char* directory, const char* name, char* path, size_t size);
static u32 calculate_crc32(const u8* data, size_t size);
static GC_AdamMediaType adam_media_type_from_path(const char* path);
static bool load_colecovision_firmware(void);
static bool load_adam_firmware(void);
static bool flush_adam_media(GC_AdamMediaSlot slot);
static bool flush_all_adam_media(void);
static void clear_adam_host_media(void);
static void clear_adam_disk_set(void);
static void clear_adam_disk_sets(void);
static void capture_adam_media(RetroAdamMediaBackup* backup);
static bool prepare_adam_media(const RetroAdamMediaBackup* backup);
static void clear_adam_media_backup(RetroAdamMediaBackup* backup);
static bool initialize_disk_set(const struct retro_game_info* info, GC_AdamMediaType type);
static bool load_disk_set_image(unsigned index);
static bool setup_loaded_game(void);
static void keyboard_event(bool down, unsigned keycode, uint32_t character, uint16_t key_modifiers);
static bool disk_set_eject_state(bool ejected);
static bool disk_get_eject_state(void);
static unsigned disk_get_image_index(void);
static bool disk_set_image_index(unsigned index);
static unsigned disk_get_num_images(void);
static bool disk_replace_image_index(unsigned index, const struct retro_game_info* info);
static bool disk_add_image_index(void);
static bool disk_set_initial_image(unsigned index, const char* path);
static bool disk_get_image_path(unsigned index, char* path, size_t len);
static bool disk_get_image_label(unsigned index, char* label, size_t len);

void retro_init(void)
{
    if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
        log_cb = logging.log;
    else
        log_cb = fallback_log;

    const char *dir = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) && dir) {
        snprintf(retro_system_directory, sizeof(retro_system_directory), "%s", dir);
    }
    else {
        retro_system_directory[0] = '\0';
    }

    const char* save_dir = NULL;
    if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_dir) && save_dir)
        snprintf(retro_save_directory, sizeof(retro_save_directory), "%s", save_dir);
    else
        retro_save_directory[0] = '\0';

    log_cb(RETRO_LOG_INFO, "%s (%s) libretro\n", GEARCOLECO_TITLE, EMULATOR_BUILD);

    struct retro_vfs_interface_info vfs_interface_info = {};
    vfs_interface_info.required_interface_version = 1;
    vfs_interface_info.iface = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_interface_info) &&
        vfs_interface_info.iface && vfs_interface_info.iface->open &&
        vfs_interface_info.iface->close && vfs_interface_info.iface->size &&
        vfs_interface_info.iface->read)
        vfs_interface = vfs_interface_info.iface;
    else
        vfs_interface = NULL;

    core = new GearcolecoCore();

#ifdef PS2
    core->Init(GC_PIXEL_BGR555);
#else
    core->Init(GC_PIXEL_RGB565);
#endif

    frame_buffer = new u8[GC_VIDEO_MAX_WIDTH * GC_VIDEO_MAX_HEIGHT * 2];

    audio_sample_count = 0;
    retro_game_path[0] = '\0';
    content_loaded = false;
    clear_adam_host_media();
    clear_adam_disk_sets();

    config.region = Cartridge::CartridgeUnknownRegion;
    config.type = Cartridge::CartridgeNotSupported;

    clear_input_state();

    for (int i = 0; i < MAX_PADS; i++)
        apply_controller_device(i, input_device[i], false);

    libretro_supports_bitmasks = environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL);
}

void retro_deinit(void)
{
    if (content_loaded)
    {
        if (!flush_all_adam_media())
            log_cb(RETRO_LOG_ERROR, "Unable to flush one or more ADAM working copies during deinit.\n");
        core->AdamReleaseAllKeys();
        core->UnloadContent();
    }
    clear_adam_disk_sets();
    clear_adam_host_media();
    SafeDeleteArray(frame_buffer);
    SafeDelete(core);
    vfs_interface = NULL;

    audio_sample_count = 0;
    current_screen_width = 0;
    current_screen_height = 0;
    current_aspect_ratio = 0.0f;
    aspect_ratio = 0.0f;
    video_chip = GC_VIDEO_CHIP_AUTO;
    adam_cartridge_hardware = false;
    adam_control_drive = -1;
    adam_primary_slot = 0;
    adam_computer_reset_latched = false;
    adam_writable_media = false;
    content_loaded = false;
    retro_game_path[0] = '\0';
    retro_save_directory[0] = '\0';
    libretro_supports_bitmasks = false;

    reset_controller_devices();
    clear_input_state();
}

unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
    if (port >= MAX_PADS)
    {
        if (log_cb)
            log_cb(RETRO_LOG_DEBUG, "retro_set_controller_port_device invalid port number: %u\n", port);
        return;
    }

    input_device[port] = device;

    apply_controller_device(port, device, true);
}

static void clear_adam_keyboard_state(void)
{
    memset(adam_retro_key_down, 0, sizeof(adam_retro_key_down));
    memset(adam_key_references, 0, sizeof(adam_key_references));
    if (core)
        core->AdamReleaseAllKeys();
}

static void clear_input_state(void)
{
    clear_adam_keyboard_state();
    for (int i = 0; i < MAX_PADS; i++)
    {
        for (int j = 0; j < JOYPAD_BUTTONS; j++)
        {
            joypad[i][j] = 0;
            joypre[i][j] = 0;
        }

        for (int j = 0; j < 4; j++)
        {
            joypad_ext[i][j] = 0;
            joypre_ext[i][j] = 0;
        }

        mouse[i] = false;
        mousepre[i] = false;
    }
}

static void reset_controller_devices(void)
{
    for (int i = 0; i < MAX_PADS; i++)
        input_device[i] = RETRO_DEVICE_COLECOVISION;
}

static void apply_controller_device(unsigned port, unsigned device, bool log_device)
{
    if (log_device && log_cb)
        log_cb(RETRO_LOG_DEBUG, "Plugging device %u into port %u.\n", device, port);

    struct retro_input_descriptor joypad[] = {

        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,                              "Joystick Up" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,                            "Joystick Down" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,                            "Joystick Left" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,                           "Joystick Right" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,                               "Yellow (Left)" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,                               "Red (Right)" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,                               "Keypad 1" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,                               "Keypad 2" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,                               "Keypad 3" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,                               "Keypad 4" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,                              "Keypad 5" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,                              "Keypad 6" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,                              "Keypad 7" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,                              "Keypad 8" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,                           "Keypad *" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,                          "Keypad #" },
        { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y,  "Keypad 9" },
        { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X,  "Keypad 0" },
        { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Purple" },
        { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Blue" },

        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,                              "Joystick Up" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,                            "Joystick Down" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,                            "Joystick Left" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,                           "Joystick Right" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,                               "Yellow (Left)" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,                               "Red (Right)" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,                               "Keypad 1" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,                               "Keypad 2" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L,                               "Keypad 3" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,                               "Keypad 4" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2,                              "Keypad 5" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2,                              "Keypad 6" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3,                              "Keypad 7" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,                              "Keypad 8" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,                           "Keypad *" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT,                          "Keypad #" },
        { 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y,  "Keypad 9" },
        { 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X,  "Keypad 0" },
        { 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "Purple" },
        { 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "Blue" },

        { 0, 0, 0, 0, NULL }
    };

    struct retro_input_descriptor desc[] = {
        { 0, 0, 0, 0, NULL }
    };

    if (!environ_cb)
        return;

    bool joypad_connected = false;
    for (int i = 0; i < MAX_PADS; i++)
    {
        if (IsJoypadDevice(input_device[i]))
        {
            joypad_connected = true;
            break;
        }
    }

    if (joypad_connected)
        environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, joypad);
    else
        environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);
}

void retro_get_system_info(struct retro_system_info *info)
{
    memset(info, 0, sizeof(*info));
    info->library_name     = GEARCOLECO_TITLE;
    info->library_version  = GEARCOLECO_VERSION;
    info->need_fullpath    = false;
    info->valid_extensions = "col|cv|bin|rom|zip|ddp|dsk|m3u";
}

static retro_video_refresh_t video_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

void retro_get_system_av_info(struct retro_system_av_info *info)
{
    GC_RuntimeInfo runtime_info;
    core->GetRuntimeInfo(runtime_info);

    current_screen_width = runtime_info.screen_width;
    current_screen_height = runtime_info.screen_height;

    info->geometry.base_width   = runtime_info.screen_width;
    info->geometry.base_height  = runtime_info.screen_height;
    info->geometry.max_width    = GC_VIDEO_MAX_WIDTH;
    info->geometry.max_height   = GC_VIDEO_MAX_HEIGHT;
    info->geometry.aspect_ratio = aspect_ratio;
    info->timing.fps            = runtime_info.fps;
    info->timing.sample_rate    = 44100.0;
}

void retro_set_environment(retro_environment_t cb)
{
    environ_cb = cb;

    bool support_no_game = true;
    environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &support_no_game);

    static const struct retro_disk_control_ext_callback disk_control = {
        disk_set_eject_state,
        disk_get_eject_state,
        disk_get_image_index,
        disk_set_image_index,
        disk_get_num_images,
        disk_replace_image_index,
        disk_add_image_index,
        disk_set_initial_image,
        disk_get_image_path,
        disk_get_image_label
    };
    if (!environ_cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE, (void*)&disk_control))
    {
        static const struct retro_disk_control_callback disk_control_legacy = {
            disk_set_eject_state,
            disk_get_eject_state,
            disk_get_image_index,
            disk_set_image_index,
            disk_get_num_images,
            disk_replace_image_index,
            disk_add_image_index
        };
        environ_cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE, (void*)&disk_control_legacy);
    }

    static const struct retro_subsystem_rom_info adam_roms[] = {
        { "ADAM cartridge", "col|cv|bin|rom", false, false, false, NULL, 0 },
        { "Disk 1", "dsk|zip|m3u", false, false, false, NULL, 0 },
        { "Disk 2", "dsk|zip|m3u", false, false, false, NULL, 0 },
        { "Data Pack 1", "ddp|zip|m3u", false, false, false, NULL, 0 },
        { "Data Pack 2", "ddp|zip|m3u", false, false, false, NULL, 0 }
    };
    static const struct retro_subsystem_info subsystems[] = {
        { "ADAM", "adam", adam_roms, 5, RETRO_ADAM_SUBSYSTEM_ID },
        { NULL, NULL, NULL, 0, 0 }
    };
    environ_cb(RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO, (void*)subsystems);

    static const struct retro_controller_description port_1[] = {
        { "Joypad Auto", RETRO_DEVICE_JOYPAD },
        { "Joypad Port Empty", RETRO_DEVICE_NONE },
        { "ColecoVision", RETRO_DEVICE_COLECOVISION },
    };

    static const struct retro_controller_description port_2[] = {
        { "Joypad Auto", RETRO_DEVICE_JOYPAD },
        { "Joypad Port Empty", RETRO_DEVICE_NONE },
        { "ColecoVision", RETRO_DEVICE_COLECOVISION },
    };

    static const struct retro_controller_info ports[] = {
        { port_1, 3 },
        { port_2, 3 },
        { NULL, 0 },
    };

    environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);

    libretro_set_core_options(environ_cb, &categories_supported);
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
    audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
    audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
    input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
    input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
    video_cb = cb;
}

static bool read_file(const char* path, u8** data, size_t* size)
{
    if (!path || !path[0] || !data || !size)
        return false;

    *data = NULL;
    *size = 0;

    if (vfs_interface)
    {
        if (!vfs_interface->open || !vfs_interface->size || !vfs_interface->read ||
            !vfs_interface->close)
        {
            return false;
        }
        retro_vfs_file_handle* file = vfs_interface->open(path, RETRO_VFS_FILE_ACCESS_READ,
            RETRO_VFS_FILE_ACCESS_HINT_NONE);
        if (!file)
            return false;

        s64 file_size = (s64)vfs_interface->size(file);
        if ((file_size <= 0) || (file_size > 0x7FFFFFFF))
        {
            vfs_interface->close(file);
            return false;
        }

        u8* buffer = new u8[(size_t)file_size];
        s64 total = 0;
        while (total < file_size)
        {
            s64 read = (s64)vfs_interface->read(file, buffer + total, file_size - total);
            if (read <= 0)
                break;
            total += read;
        }

        bool complete_read = total == file_size;
        bool closed = vfs_interface->close(file) == 0;
        bool loaded = complete_read && closed;
        if (!loaded)
        {
            SafeDeleteArray(buffer);
            return false;
        }

        *data = buffer;
        *size = (size_t)file_size;
        return true;
    }

    FILE* file = fopen_utf8(path, "rb");
    if (!file)
        return false;
    if ((fseek(file, 0, SEEK_END) != 0))
    {
        fclose(file);
        return false;
    }
    long file_size = ftell(file);
    if ((file_size <= 0) || (file_size > 0x7FFFFFFF) || (fseek(file, 0, SEEK_SET) != 0))
    {
        fclose(file);
        return false;
    }

    u8* buffer = new u8[(size_t)file_size];
    bool loaded = fread(buffer, 1, (size_t)file_size, file) == (size_t)file_size;
    fclose(file);
    if (!loaded)
    {
        SafeDeleteArray(buffer);
        return false;
    }

    *data = buffer;
    *size = (size_t)file_size;
    return true;
}

static bool read_game_info(const struct retro_game_info* info, u8** data, size_t* size)
{
    if (!info || !data || !size)
        return false;

    if (IsValidPointer(info->data) && (info->size > 0) && (info->size <= 0x7FFFFFFF))
    {
        *data = new u8[info->size];
        memcpy(*data, info->data, info->size);
        *size = info->size;
        return true;
    }

    if (!info->path || !info->path[0])
        return false;
    return read_file(info->path, data, size);
}

static bool is_zip_data(const u8* data, size_t size)
{
    return IsValidPointer(data) && (size >= 4) && (data[0] == 'P') && (data[1] == 'K') &&
        (((data[2] == 3) && (data[3] == 4)) || ((data[2] == 5) && (data[3] == 6)) ||
        ((data[2] == 7) && (data[3] == 8)));
}

static bool read_adam_game_info(const struct retro_game_info* info, u8** data, size_t* size,
    GC_AdamMediaType* type)
{
    if (!IsValidPointer(data) || !IsValidPointer(size) || !IsValidPointer(type))
        return false;

    *data = NULL;
    *size = 0;
    *type = GC_ADAM_MEDIA_NONE;

    u8* source = NULL;
    size_t source_size = 0;
    if (!read_game_info(info, &source, &source_size))
        return false;

    GC_AdamMediaType source_type = adam_media_type_from_path(info && info->path ?
        info->path : NULL);
    bool archive = (info && info->path && ends_with_no_case(info->path, ".zip")) ||
        is_zip_data(source, source_size);
    if (archive)
    {
        bool extracted = AdamMedia::ExtractFromZip(source, source_size, data, size, type);
        SafeDeleteArray(source);
        return extracted;
    }

    if (source_type == GC_ADAM_MEDIA_NONE)
    {
        if (info && info->path && info->path[0])
        {
            SafeDeleteArray(source);
            return false;
        }

        if (source_size == AdamMedia::kDataPackSize)
            source_type = GC_ADAM_MEDIA_DATA_PACK;
        else if ((source_size == AdamMedia::kDisk160KSize) ||
            (source_size == AdamMedia::kDisk320KSize))
            source_type = GC_ADAM_MEDIA_DISK;
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

static bool find_firmware(const char* const* names, int name_count, size_t expected_size,
    u8** data, char* located_path, size_t located_path_size)
{
    if (!retro_system_directory[0])
        return false;

    *data = NULL;
    located_path[0] = '\0';
    for (int directory = 0; directory < 2; directory++)
    {
        const char* search_directory = retro_system_directory;
        char subdirectory[4096];
        if (directory != 0)
        {
            if (!join_path(retro_system_directory, "gearcoleco", subdirectory,
                sizeof(subdirectory)))
                continue;
            search_directory = subdirectory;
        }

        for (int i = 0; i < name_count; i++)
        {
            char path[4096];
            if (!join_path(search_directory, names[i], path, sizeof(path)))
                continue;

            u8* candidate = NULL;
            size_t size = 0;
            if (!read_file(path, &candidate, &size))
                continue;
            if (size != expected_size)
            {
                log_cb(RETRO_LOG_WARN, "Ignoring firmware with incorrect size %zu: %s\n", size,
                    path);
                SafeDeleteArray(candidate);
                continue;
            }

            if (!*data)
            {
                *data = candidate;
                snprintf(located_path, located_path_size, "%s", path);
            }
            else
            {
                if (memcmp(*data, candidate, expected_size) != 0)
                {
                    log_cb(RETRO_LOG_ERROR, "Ambiguous firmware role: %s and %s contain different valid-sized images.\n",
                        located_path, path);
                    SafeDeleteArray(candidate);
                    SafeDeleteArray(*data);
                    located_path[0] = '\0';
                    return false;
                }
                SafeDeleteArray(candidate);
            }
        }
    }

    return *data != NULL;
}

static int firmware_alias_count(const Adam::FirmwareMetadata* metadata)
{
    int count = 0;
    while (metadata && (count < 4) && metadata->aliases[count])
        count++;
    return count;
}

static void show_firmware_message(const char* message)
{
    struct retro_message msg = {};
    msg.msg = message;
    msg.frames = 360;
    environ_cb(RETRO_ENVIRONMENT_SET_MESSAGE, &msg);
}

static bool load_colecovision_firmware(void)
{
    const Adam::FirmwareMetadata* metadata =
        Adam::GetFirmwareMetadata(GC_ADAM_FIRMWARE_OS7);
    u8* os7 = NULL;
    char path[4096];
    if (!find_firmware(metadata->aliases, firmware_alias_count(metadata), metadata->size,
        &os7, path, sizeof(path)))
    {
        show_firmware_message("OS-7 BIOS missing; see core log");
        log_cb(RETRO_LOG_ERROR, "OS-7 BIOS missing. Accepted names: colecovision.rom, "
            "coleco.rom, os7.u2; size 8192; known CRC32 3aa93ef3. Searched %s and "
            "%s%cgearcoleco.\n", retro_system_directory, retro_system_directory, slash);
        return false;
    }

    u32 crc = calculate_crc32(os7, metadata->size);
    if (crc != metadata->crc)
        log_cb(RETRO_LOG_WARN, "Unknown OS-7 revision CRC32 %08x: %s\n", crc, path);
    bool loaded = core->GetMemory()->LoadBiosFromBuffer(os7, metadata->size);
    if (loaded)
        core->LoadAdamFirmware(GC_ADAM_FIRMWARE_OS7, os7, metadata->size);
    SafeDeleteArray(os7);
    return loaded;
}

static bool load_adam_firmware(void)
{
    const Adam::FirmwareMetadata* os7_metadata =
        Adam::GetFirmwareMetadata(GC_ADAM_FIRMWARE_OS7);
    const Adam::FirmwareMetadata* eos_metadata =
        Adam::GetFirmwareMetadata(GC_ADAM_FIRMWARE_EOS);
    const Adam::FirmwareMetadata* writer_metadata =
        Adam::GetFirmwareMetadata(GC_ADAM_FIRMWARE_SMARTWRITER);
    u8* os7 = NULL;
    u8* eos = NULL;
    u8* writer = NULL;
    char os7_path[4096];
    char eos_path[4096];
    char writer_path[4096];

    bool os7_found = find_firmware(os7_metadata->aliases, firmware_alias_count(os7_metadata),
        os7_metadata->size, &os7, os7_path, sizeof(os7_path));
    bool eos_found = find_firmware(eos_metadata->aliases, firmware_alias_count(eos_metadata),
        eos_metadata->size, &eos, eos_path, sizeof(eos_path));
    bool writer_found = find_firmware(writer_metadata->aliases,
        firmware_alias_count(writer_metadata), writer_metadata->size, &writer, writer_path,
        sizeof(writer_path));

    if (!os7_found || !eos_found || !writer_found)
    {
        show_firmware_message("ADAM firmware incomplete; see core log");
        log_cb(RETRO_LOG_ERROR, "ADAM firmware incomplete. OS-7: colecovision.rom/coleco.rom/"
            "os7.u2, 8192 bytes, CRC32 3aa93ef3. EOS: eos.rom, 8192 bytes, CRC32 "
            "05a37a34. SmartWriter: writer.rom/wp.rom/wp_r80.rom, 32768 bytes, CRC32 "
            "58d86a2a. Searched %s and %s%cgearcoleco. Missing roles:%s%s%s\n",
            retro_system_directory, retro_system_directory, slash,
            os7_found ? "" : " OS-7", eos_found ? "" : " EOS",
            writer_found ? "" : " SmartWriter");
        SafeDeleteArray(os7);
        SafeDeleteArray(eos);
        SafeDeleteArray(writer);
        return false;
    }

    u32 os7_crc = calculate_crc32(os7, os7_metadata->size);
    u32 eos_crc = calculate_crc32(eos, eos_metadata->size);
    u32 writer_crc = calculate_crc32(writer, writer_metadata->size);
    if (os7_crc != os7_metadata->crc)
        log_cb(RETRO_LOG_WARN, "Unknown OS-7 revision CRC32 %08x: %s\n", os7_crc, os7_path);
    if (eos_crc != eos_metadata->crc)
        log_cb(RETRO_LOG_WARN, "Unknown EOS revision CRC32 %08x: %s\n", eos_crc, eos_path);
    if (writer_crc != writer_metadata->crc)
        log_cb(RETRO_LOG_WARN, "Unknown SmartWriter revision CRC32 %08x: %s\n", writer_crc,
            writer_path);

    bool loaded = core->LoadAdamFirmware(os7, os7_metadata->size, eos, eos_metadata->size,
        writer, writer_metadata->size);
    if (loaded)
        loaded = core->GetMemory()->LoadBiosFromBuffer(os7, os7_metadata->size);

    SafeDeleteArray(os7);
    SafeDeleteArray(eos);
    SafeDeleteArray(writer);
    return loaded;
}

static bool ends_with_no_case(const char* text, const char* suffix)
{
    if (!text || !suffix)
        return false;
    size_t text_length = strlen(text);
    size_t suffix_length = strlen(suffix);
    if (suffix_length > text_length)
        return false;

    const char* start = text + text_length - suffix_length;
    for (size_t i = 0; i < suffix_length; i++)
    {
        char a = start[i];
        char b = suffix[i];
        if ((a >= 'A') && (a <= 'Z'))
            a = (char)(a - 'A' + 'a');
        if ((b >= 'A') && (b <= 'Z'))
            b = (char)(b - 'A' + 'a');
        if (a != b)
            return false;
    }
    return true;
}

static GC_AdamMediaType adam_media_type_from_path(const char* path)
{
    if (ends_with_no_case(path, ".ddp"))
        return GC_ADAM_MEDIA_DATA_PACK;
    if (ends_with_no_case(path, ".dsk"))
        return GC_ADAM_MEDIA_DISK;
    return GC_ADAM_MEDIA_NONE;
}

static bool adam_media_size_valid(GC_AdamMediaType type, size_t size)
{
    return AdamMedia::IsValidImageSize(type, size);
}

static const char* path_filename(const char* path)
{
    const char* name = path ? path : "";
    const char* forward = strrchr(name, '/');
    const char* backward = strrchr(name, '\\');
    if (forward && (!backward || (forward > backward)))
        return forward + 1;
    if (backward)
        return backward + 1;
    return name;
}

static void path_directory(const char* path, char* directory, size_t size)
{
    if (!directory || (size == 0))
        return;
    directory[0] = '\0';
    if (!path)
        return;

    const char* forward = strrchr(path, '/');
    const char* backward = strrchr(path, '\\');
    const char* separator = forward;
    if (backward && (!separator || (backward > separator)))
        separator = backward;
    if (!separator)
        return;

    size_t length = (size_t)(separator - path);
    if (length >= size)
        length = size - 1;
    memcpy(directory, path, length);
    directory[length] = '\0';
}

static bool path_is_absolute(const char* path)
{
    if (!path || !path[0])
        return false;
    if ((path[0] == '/') || (path[0] == '\\'))
        return true;
    return path[1] == ':';
}

static bool join_path(const char* directory, const char* name, char* path, size_t size)
{
    if (!name || !path || (size == 0))
        return false;
    if (path_is_absolute(name) || !directory || !directory[0])
        return snprintf(path, size, "%s", name) > 0 && strlen(name) < size;
    return snprintf(path, size, "%s%c%s", directory, slash, name) > 0 &&
        (strlen(directory) + strlen(name) + 2 <= size);
}

static void make_image_label(const char* path, char* label, size_t size)
{
    if (!label || (size == 0))
        return;
    const char* name = path_filename(path);
    snprintf(label, size, "%s", name[0] ? name : "ADAM media");
    char* dot = strrchr(label, '.');
    if (dot)
        *dot = '\0';
}

static void clear_disk_image(RetroAdamDiskImage* image)
{
    if (!image)
        return;
    SafeDeleteArray(image->data);
    memset(image, 0, sizeof(*image));
}

static void clear_adam_disk_set(void)
{
    unsigned slot = (unsigned)(adam_disk_set - adam_disk_sets);
    for (int i = 0; i < MAX_ADAM_DISK_IMAGES; i++)
        clear_disk_image(&adam_disk_set->images[i]);
    memset(adam_disk_set, 0, sizeof(*adam_disk_set));
    adam_disk_set->slot = (GC_AdamMediaSlot)slot;
    adam_disk_set->type = slot <= GC_ADAM_MEDIA_DISK_2 ? GC_ADAM_MEDIA_DISK : GC_ADAM_MEDIA_DATA_PACK;
    adam_disk_set->ejected = true;
}

static void clear_adam_disk_sets(void)
{
    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
    {
        adam_disk_set = &adam_disk_sets[i];
        clear_adam_disk_set();
    }
    adam_primary_slot = 0;
    adam_disk_set = &adam_disk_sets[0];
}

static void clear_adam_host_media(void)
{
    memset(adam_host_media, 0, sizeof(adam_host_media));
}

static void capture_adam_media(RetroAdamMediaBackup* backup)
{
    memset(backup, 0, sizeof(RetroAdamMediaBackup) * GC_ADAM_MEDIA_SLOT_COUNT);
    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
    {
        AdamMedia* media = core->GetAdamMedia((GC_AdamMediaSlot)i);
        if (!media || !media->IsInserted())
            continue;

        backup[i].data = new u8[media->GetSize()];
        memcpy(backup[i].data, media->GetData(), media->GetSize());
        backup[i].size = media->GetSize();
        backup[i].base_crc = media->GetBaseCRC();
        backup[i].type = media->GetType();
        backup[i].inserted = true;
        backup[i].write_protected = media->IsWriteProtected();
    }
}

static bool prepare_adam_media(const RetroAdamMediaBackup* backup)
{
    bool prepared = true;
    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
    {
        if (backup[i].inserted)
        {
            if (!core->LoadAdamMediaFromBuffer((GC_AdamMediaSlot)i, backup[i].type,
                backup[i].data, backup[i].size, backup[i].write_protected,
                backup[i].base_crc))
            {
                prepared = false;
            }
        }
        else
            core->EjectAdamMedia((GC_AdamMediaSlot)i);
    }
    return prepared;
}

static void clear_adam_media_backup(RetroAdamMediaBackup* backup)
{
    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
        SafeDeleteArray(backup[i].data);
}

static bool copy_disk_image(RetroAdamDiskImage* image, const struct retro_game_info* info)
{
    if (!image || !info)
        return false;

    RetroAdamDiskImage temporary = {};
    if (info->path)
        snprintf(temporary.path, sizeof(temporary.path), "%s", info->path);
    make_image_label(temporary.path, temporary.label, sizeof(temporary.label));

    if (info->data && info->size > 0)
    {
        if (info->size > 0x7FFFFFFF)
            return false;
        temporary.data = new u8[info->size];
        memcpy(temporary.data, info->data, info->size);
        temporary.size = info->size;
    }
    else if (!temporary.path[0])
        return false;

    clear_disk_image(image);
    *image = temporary;
    return true;
}

static bool read_disk_image(const RetroAdamDiskImage* image, u8** data, size_t* size,
    GC_AdamMediaType* type)
{
    if (!image)
        return false;

    struct retro_game_info info = {};
    info.path = image->path[0] ? image->path : NULL;
    info.data = image->data;
    info.size = image->size;
    return read_adam_game_info(&info, data, size, type);
}

static bool initialize_disk_set(const struct retro_game_info* info, GC_AdamMediaType type)
{
    clear_adam_disk_set();
    if (!info)
        return false;

    bool playlist = info->path && ends_with_no_case(info->path, ".m3u");
    if (!playlist)
    {
        if ((type == GC_ADAM_MEDIA_NONE) || !copy_disk_image(&adam_disk_set->images[0], info))
            return false;
        RetroAdamDiskImage* image = &adam_disk_set->images[0];
        u8* validation = NULL;
        size_t validation_size = 0;
        GC_AdamMediaType validation_type = GC_ADAM_MEDIA_NONE;
        if (!read_disk_image(image, &validation, &validation_size, &validation_type))
        {
            clear_adam_disk_set();
            return false;
        }
        bool valid = (validation_type == type) && adam_media_size_valid(type, validation_size);
        SafeDeleteArray(validation);
        if (!valid)
        {
            clear_adam_disk_set();
            return false;
        }
        adam_disk_set->count = 1;
        adam_disk_set->index = 0;
        adam_disk_set->type = type;
        adam_disk_set->ejected = false;
        return true;
    }

    u8* text = NULL;
    size_t text_size = 0;
    if (!read_game_info(info, &text, &text_size))
        return false;

    char directory[4096];
    path_directory(info->path, directory, sizeof(directory));
    size_t position = 0;
    GC_AdamMediaType playlist_type = GC_ADAM_MEDIA_NONE;

    while ((position < text_size) && (adam_disk_set->count < MAX_ADAM_DISK_IMAGES))
    {
        size_t end = position;
        while ((end < text_size) && (text[end] != '\n'))
            end++;
        size_t next = end + 1;
        while ((position < end) && ((text[position] == ' ') || (text[position] == '\t') ||
            (text[position] == '\r') || ((position < 3) && ((text[position] == 0xEF) ||
            (text[position] == 0xBB) || (text[position] == 0xBF)))))
            position++;
        while ((end > position) && ((text[end - 1] == ' ') || (text[end - 1] == '\t') ||
            (text[end - 1] == '\r')))
            end--;

        if ((end > position) && (text[position] != '#'))
        {
            if ((end - position) >= 4096)
            {
                SafeDeleteArray(text);
                clear_adam_disk_set();
                return false;
            }

            char entry[4096];
            memcpy(entry, text + position, end - position);
            entry[end - position] = '\0';
            RetroAdamDiskImage* image = &adam_disk_set->images[adam_disk_set->count];
            if (!join_path(directory, entry, image->path, sizeof(image->path)))
            {
                SafeDeleteArray(text);
                clear_adam_disk_set();
                return false;
            }

            u8* validation = NULL;
            size_t validation_size = 0;
            GC_AdamMediaType entry_type = GC_ADAM_MEDIA_NONE;
            if (!read_disk_image(image, &validation, &validation_size, &entry_type) ||
                ((playlist_type != GC_ADAM_MEDIA_NONE) && (entry_type != playlist_type)) ||
                !adam_media_size_valid(entry_type, validation_size))
            {
                SafeDeleteArray(validation);
                SafeDeleteArray(text);
                clear_adam_disk_set();
                log_cb(RETRO_LOG_ERROR, "Invalid or mixed ADAM playlist entry: %s\n", image->path);
                return false;
            }
            SafeDeleteArray(validation);
            playlist_type = entry_type;
            make_image_label(image->path, image->label, sizeof(image->label));
            adam_disk_set->count++;
        }
        position = next;
    }

    SafeDeleteArray(text);
    if ((adam_disk_set->count == 0) || ((position < text_size) &&
        (adam_disk_set->count == MAX_ADAM_DISK_IMAGES)))
    {
        clear_adam_disk_set();
        return false;
    }

    adam_disk_set->type = playlist_type;
    if (type != GC_ADAM_MEDIA_NONE && playlist_type != type)
    {
        clear_adam_disk_set();
        return false;
    }
    adam_disk_set->index = 0;
    if ((adam_initial_image_index < adam_disk_set->count) && adam_initial_image_path[0] &&
        (strcmp(adam_disk_set->images[adam_initial_image_index].path, adam_initial_image_path) == 0))
        adam_disk_set->index = adam_initial_image_index;
    adam_initial_image_index = 0;
    adam_initial_image_path[0] = '\0';
    adam_disk_set->ejected = false;
    return true;
}

static void sanitize_content_name(const char* path, char* name, size_t size)
{
    if (!name || (size == 0))
        return;
    snprintf(name, size, "%s", path_filename(path));
    char* dot = strrchr(name, '.');
    if (dot)
        *dot = '\0';
    if (!name[0])
        snprintf(name, size, "%s", "adam-media");
    for (size_t i = 0; name[i]; i++)
    {
        bool valid = ((name[i] >= 'a') && (name[i] <= 'z')) ||
            ((name[i] >= 'A') && (name[i] <= 'Z')) ||
            ((name[i] >= '0') && (name[i] <= '9')) || (name[i] == '-') || (name[i] == '_');
        if (!valid)
            name[i] = '-';
    }
}

static bool make_working_path(const char* source_path, GC_AdamMediaType type,
    GC_AdamMediaSlot slot, u32 base_crc, char* path, size_t size)
{
    if (!retro_save_directory[0] || !path || (size == 0))
        return false;
    char name[256];
    sanitize_content_name(source_path && source_path[0] ? source_path : retro_game_path, name,
        sizeof(name));
    const char* media = type == GC_ADAM_MEDIA_DATA_PACK ? "ddp" : "dsk";
    int written = snprintf(path, size, "%s%c%s.%s.%08x.slot%d.gearcoleco.%s",
        retro_save_directory, slash, name, media, base_crc, (int)slot + 1, media);
    return (written > 0) && ((size_t)written < size);
}

static bool vfs_write_file(const char* path, const u8* data, size_t size)
{
    if (!vfs_interface || !vfs_interface->open || !vfs_interface->write ||
        !vfs_interface->flush || !vfs_interface->close || !vfs_interface->rename ||
        !vfs_interface->remove)
        return false;

    char temporary[4128];
    char backup[4128];
    snprintf(temporary, sizeof(temporary), "%s.tmp", path);
    snprintf(backup, sizeof(backup), "%s.bak", path);
    retro_vfs_file_handle* file = vfs_interface->open(temporary, RETRO_VFS_FILE_ACCESS_WRITE,
        RETRO_VFS_FILE_ACCESS_HINT_NONE);
    if (!file)
        return false;

    size_t total = 0;
    while (total < size)
    {
        s64 written = (s64)vfs_interface->write(file, data + total, size - total);
        if (written <= 0)
            break;
        total += (size_t)written;
    }
    bool complete_write = total == size;
    bool flushed = vfs_interface->flush(file) == 0;
    bool closed = vfs_interface->close(file) == 0;
    if (!complete_write || !flushed || !closed)
    {
        vfs_interface->remove(temporary);
        return false;
    }

    vfs_interface->remove(backup);
    bool had_original = vfs_interface->rename(path, backup) == 0;
    if (vfs_interface->rename(temporary, path) != 0)
    {
        if (had_original)
            vfs_interface->rename(backup, path);
        vfs_interface->remove(temporary);
        return false;
    }
    if (had_original)
        vfs_interface->remove(backup);
    return true;
}

static bool mount_adam_media(GC_AdamMediaSlot slot, GC_AdamMediaType type, const u8* source,
    size_t size, const char* source_path)
{
    if (!adam_media_size_valid(type, size))
        return false;

    u32 base_crc = calculate_crc32(source, size);
    char working_path[4096];
    working_path[0] = '\0';
    bool writable = adam_writable_media && vfs_interface && vfs_interface->write &&
        vfs_interface->flush && vfs_interface->rename && vfs_interface->remove &&
        make_working_path(source_path, type, slot, base_crc, working_path, sizeof(working_path));
    if (adam_writable_media && !writable)
        log_cb(RETRO_LOG_WARN, "ADAM writable media requested, but the save directory or required VFS write callbacks are unavailable; mounting write protected.\n");

    const u8* mounted = source;
    u8* working = NULL;
    size_t working_size = 0;
    if (writable && read_file(working_path, &working, &working_size))
    {
        if (working_size == size)
        {
            mounted = working;
            log_cb(RETRO_LOG_INFO, "Loading ADAM working copy: %s\n", working_path);
        }
        else
        {
            log_cb(RETRO_LOG_WARN, "Ignoring ADAM working copy with incorrect size: %s\n",
                working_path);
            SafeDeleteArray(working);
        }
    }

    bool loaded = core->LoadAdamMediaFromBuffer(slot, type, mounted, size, !writable, base_crc);
    if (loaded)
    {
        memset(&adam_host_media[slot], 0, sizeof(adam_host_media[slot]));
        if (writable)
            snprintf(adam_host_media[slot].working_path,
                sizeof(adam_host_media[slot].working_path), "%s", working_path);
        adam_host_media[slot].base_crc = base_crc;
    }
    SafeDeleteArray(working);
    return loaded;
}

static bool load_disk_set_image(unsigned index)
{
    if (index >= adam_disk_set->count)
        return false;
    RetroAdamDiskImage* image = &adam_disk_set->images[index];
    u8* data = NULL;
    size_t size = 0;
    GC_AdamMediaType type = GC_ADAM_MEDIA_NONE;
    if (!read_disk_image(image, &data, &size, &type))
        return false;
    if (type != adam_disk_set->type)
    {
        SafeDeleteArray(data);
        return false;
    }

    bool loaded = mount_adam_media(adam_disk_set->slot, adam_disk_set->type, data, size,
        image->path);
    SafeDeleteArray(data);
    return loaded;
}

static bool flush_adam_media(GC_AdamMediaSlot slot)
{
    AdamMedia* media = core->GetAdamMedia(slot);
    if (!media || !media->IsInserted() || !media->IsDirty())
        return true;
    if (!adam_host_media[slot].working_path[0])
        return false;
    if (!vfs_write_file(adam_host_media[slot].working_path, media->GetData(), media->GetSize()))
        return false;
    media->ClearDirty();
    log_cb(RETRO_LOG_INFO, "ADAM working copy saved: %s\n",
        adam_host_media[slot].working_path);
    return true;
}

static bool flush_all_adam_media(void)
{
    bool flushed = true;
    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
    {
        if (!flush_adam_media((GC_AdamMediaSlot)i))
            flushed = false;
    }
    return flushed;
}

static bool disk_set_eject_state(bool ejected)
{
    if (!content_loaded || core->GetMachine() != GC_MACHINE_ADAM)
        return false;
    if (adam_disk_set->ejected == ejected)
        return true;

    if (ejected)
    {
        if (!flush_adam_media(adam_disk_set->slot))
            return false;
        core->EjectAdamMedia(adam_disk_set->slot);
        memset(&adam_host_media[adam_disk_set->slot], 0,
            sizeof(adam_host_media[adam_disk_set->slot]));
        adam_disk_set->ejected = true;
        return true;
    }

    if ((adam_disk_set->index >= adam_disk_set->count) ||
        !load_disk_set_image(adam_disk_set->index))
    {
        return false;
    }
    adam_disk_set->ejected = false;
    return true;
}

static bool disk_get_eject_state(void)
{
    return adam_disk_set->ejected;
}

static unsigned disk_get_image_index(void)
{
    return adam_disk_set->index;
}

static bool disk_set_image_index(unsigned index)
{
    if (!content_loaded || core->GetMachine() != GC_MACHINE_ADAM)
        return false;
    if (!adam_disk_set->ejected || (index >= adam_disk_set->count))
        return false;
    adam_disk_set->index = index;
    return true;
}

static unsigned disk_get_num_images(void)
{
    return adam_disk_set->count;
}

static bool disk_replace_image_index(unsigned index, const struct retro_game_info* info)
{
    if (!content_loaded || core->GetMachine() != GC_MACHINE_ADAM)
        return false;
    if (!adam_disk_set->ejected || (index >= adam_disk_set->count))
        return false;

    if (!info)
    {
        clear_disk_image(&adam_disk_set->images[index]);
        for (unsigned i = index; i + 1 < adam_disk_set->count; i++)
        {
            adam_disk_set->images[i] = adam_disk_set->images[i + 1];
            memset(&adam_disk_set->images[i + 1], 0, sizeof(adam_disk_set->images[i + 1]));
        }
        adam_disk_set->count--;
        if (adam_disk_set->count == 0)
            adam_disk_set->index = 0;
        else if (adam_disk_set->index >= adam_disk_set->count)
            adam_disk_set->index = adam_disk_set->count - 1;
        return true;
    }

    u8* validation = NULL;
    size_t validation_size = 0;
    GC_AdamMediaType type = GC_ADAM_MEDIA_NONE;
    if (!read_adam_game_info(info, &validation, &validation_size, &type))
        return false;
    bool valid = (type == adam_disk_set->type) &&
        adam_media_size_valid(adam_disk_set->type, validation_size);
    SafeDeleteArray(validation);
    if (!valid)
        return false;
    return copy_disk_image(&adam_disk_set->images[index], info);
}

static bool disk_add_image_index(void)
{
    if (!content_loaded || core->GetMachine() != GC_MACHINE_ADAM)
        return false;
    if (!adam_disk_set->ejected || (adam_disk_set->count >= MAX_ADAM_DISK_IMAGES))
        return false;
    memset(&adam_disk_set->images[adam_disk_set->count], 0,
        sizeof(adam_disk_set->images[adam_disk_set->count]));
    adam_disk_set->count++;
    return true;
}

static bool disk_set_initial_image(unsigned index, const char* path)
{
    if (!path || !path[0] || (index >= MAX_ADAM_DISK_IMAGES))
        return false;
    adam_initial_image_index = index;
    snprintf(adam_initial_image_path, sizeof(adam_initial_image_path), "%s", path);
    return true;
}

static bool disk_get_image_path(unsigned index, char* path, size_t len)
{
    if (!path || (len == 0) || (index >= adam_disk_set->count) ||
        !adam_disk_set->images[index].path[0])
        return false;
    snprintf(path, len, "%s", adam_disk_set->images[index].path);
    return true;
}

static bool disk_get_image_label(unsigned index, char* label, size_t len)
{
    if (!label || (len == 0) || (index >= adam_disk_set->count))
        return false;
    snprintf(label, len, "%s", adam_disk_set->images[index].label[0] ?
        adam_disk_set->images[index].label : "ADAM media");
    return true;
}

static GC_AdamKey adam_key_from_retro_key(unsigned keycode)
{
    if (keycode == RETROK_UNKNOWN)
        return GC_ADAM_KEY_COUNT;
    if ((keycode >= RETROK_a) && (keycode <= RETROK_z))
        return (GC_AdamKey)(GC_ADAM_KEY_A + (keycode - RETROK_a));
    if ((keycode >= RETROK_0) && (keycode <= RETROK_9))
        return (GC_AdamKey)(GC_ADAM_KEY_0 + (keycode - RETROK_0));

    switch (keycode)
    {
        // Full ADAM typing requires frontend Game Focus; even letters are RetroArch hotkeys.
        // Keep Scroll Lock and F10-F12 free for frontend focus/menu controls.
        case RETROK_F1: return GC_ADAM_KEY_SMART_1;
        case RETROK_F2: return GC_ADAM_KEY_SMART_2;
        case RETROK_F3: return GC_ADAM_KEY_SMART_3;
        case RETROK_F4: return GC_ADAM_KEY_SMART_4;
        case RETROK_F5: return GC_ADAM_KEY_SMART_5;
        case RETROK_F6: return GC_ADAM_KEY_SMART_6;
        case RETROK_F7: return GC_ADAM_KEY_UNDO;
        case RETROK_F8: return GC_ADAM_KEY_WILD_CARD;
        case RETROK_HOME: return GC_ADAM_KEY_HOME;
        case RETROK_INSERT: return GC_ADAM_KEY_INSERT;
        case RETROK_DELETE: return GC_ADAM_KEY_DELETE;
        case RETROK_PAGEUP: return GC_ADAM_KEY_MOVE;
        case RETROK_PAGEDOWN: return GC_ADAM_KEY_STORE;
        case RETROK_END: return GC_ADAM_KEY_CLEAR;
        case RETROK_PRINT: return GC_ADAM_KEY_PRINT;
        case RETROK_ESCAPE: return GC_ADAM_KEY_ESCAPE;
        case RETROK_SPACE: return GC_ADAM_KEY_SPACE;
        case RETROK_MINUS: return GC_ADAM_KEY_MINUS;
        case RETROK_EQUALS:
        case RETROK_PLUS: return GC_ADAM_KEY_PLUS;
        case RETROK_BACKQUOTE:
        case RETROK_CARET: return GC_ADAM_KEY_CARET;
        case RETROK_SEMICOLON: return GC_ADAM_KEY_SEMICOLON;
        case RETROK_QUOTE: return GC_ADAM_KEY_QUOTE;
        case RETROK_LEFTBRACKET: return GC_ADAM_KEY_OPEN_BRACKET;
        case RETROK_RIGHTBRACKET: return GC_ADAM_KEY_CLOSE_BRACKET;
        case RETROK_BACKSLASH: return GC_ADAM_KEY_BACKSLASH;
        case RETROK_COMMA: return GC_ADAM_KEY_COMMA;
        case RETROK_PERIOD: return GC_ADAM_KEY_PERIOD;
        case RETROK_SLASH: return GC_ADAM_KEY_SLASH;
        case RETROK_RETURN:
        case RETROK_KP_ENTER: return GC_ADAM_KEY_RETURN;
        case RETROK_BACKSPACE: return GC_ADAM_KEY_BACKSPACE;
        case RETROK_TAB: return GC_ADAM_KEY_TAB;
        case RETROK_UP: return GC_ADAM_KEY_UP;
        case RETROK_RIGHT: return GC_ADAM_KEY_RIGHT;
        case RETROK_DOWN: return GC_ADAM_KEY_DOWN;
        case RETROK_LEFT: return GC_ADAM_KEY_LEFT;
        case RETROK_LSHIFT:
        case RETROK_RSHIFT: return GC_ADAM_KEY_SHIFT;
        case RETROK_LCTRL:
        case RETROK_RCTRL: return GC_ADAM_KEY_CONTROL;
        case RETROK_CAPSLOCK: return GC_ADAM_KEY_LOCK;
        default: return GC_ADAM_KEY_COUNT;
    }
}

static void keyboard_event(bool down, unsigned keycode, uint32_t character,
    uint16_t key_modifiers)
{
    UNUSED(character);
    UNUSED(key_modifiers);
    if (!content_loaded || (core->GetMachine() != GC_MACHINE_ADAM) ||
        (keycode >= RETROK_LAST))
        return;

    GC_AdamKey key = adam_key_from_retro_key(keycode);
    if (key >= GC_ADAM_KEY_COUNT)
        return;
    if (down)
    {
        if (adam_retro_key_down[keycode])
            return;
        adam_retro_key_down[keycode] = true;
        if (adam_key_references[key]++ == 0)
            core->AdamKeyPressed(key);
    }
    else
    {
        if (!adam_retro_key_down[keycode])
            return;
        adam_retro_key_down[keycode] = false;
        if ((adam_key_references[key] > 0) && (--adam_key_references[key] == 0))
            core->AdamKeyReleased(key);
    }
}

static void update_input(void)
{
    int16_t joypad_bits[MAX_PADS];

    input_poll_cb();

    if (libretro_supports_bitmasks)
    {
        for (int j = 0; j < MAX_PADS; j++)
        {
            if (IsJoypadDevice(input_device[j]))
                joypad_bits[j] = input_state_cb(j, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
            else
                joypad_bits[j] = 0;
        }
    }
    else
    {
        for (int j = 0; j < MAX_PADS; j++)
        {
            joypad_bits[j] = 0;
            if (IsJoypadDevice(input_device[j]))
            {
                for (int i = 0; i < (RETRO_DEVICE_ID_JOYPAD_R3+1); i++)
                    joypad_bits[j] |= input_state_cb(j, RETRO_DEVICE_JOYPAD, 0, i) ? (1 << i) : 0;
            }
        }
    }

    // Copy previous state
    for (int j = 0; j < MAX_PADS; j++)
    {
        for (int i = 0; i < JOYPAD_BUTTONS; i++)
            joypre[j][i] = joypad[j][i];
        for (int i = 0; i < 4; i++)
            joypre_ext[j][i] = joypad_ext[j][i];
    }

    // Get current state
    for (int j = 0; j < MAX_PADS; j++)
    {
        if (allow_up_down)
        {
            joypad[j][0] = joypad_bits[j] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)     ? 1 : 0;
            joypad[j][1] = joypad_bits[j] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)   ? 1 : 0;
            joypad[j][2] = joypad_bits[j] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)   ? 1 : 0;
            joypad[j][3] = joypad_bits[j] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)  ? 1 : 0;
        }
        else
        {
            bool raw_up = (joypad_bits[j] & (1 << RETRO_DEVICE_ID_JOYPAD_UP)) != 0;
            bool raw_down = (joypad_bits[j] & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN)) != 0;
            bool raw_left = (joypad_bits[j] & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT)) != 0;
            bool raw_right = (joypad_bits[j] & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT)) != 0;
            bool up = raw_up;
            bool down = raw_down;
            bool left = raw_left;
            bool right = raw_right;

            if (raw_up && raw_down)
            {
                if (joypre[j][0] == 1)
                {
                    up = true;
                    down = false;
                }
                else if (joypre[j][1] == 1)
                {
                    up = false;
                    down = true;
                }
                else
                {
                    up = true;
                    down = false;
                }
            }

            if (raw_left && raw_right)
            {
                if (joypre[j][2] == 1)
                {
                    left = true;
                    right = false;
                }
                else if (joypre[j][3] == 1)
                {
                    left = false;
                    right = true;
                }
                else
                {
                    left = true;
                    right = false;
                }
            }

            joypad[j][0] = up ? 1 : 0;
            joypad[j][1] = down ? 1 : 0;
            joypad[j][2] = left ? 1 : 0;
            joypad[j][3] = right ? 1 : 0;
        }
        joypad[j][4] = joypad_bits[j] & (1 << RETRO_DEVICE_ID_JOYPAD_A)      ? 1 : 0;
        joypad[j][5] = joypad_bits[j] & (1 << RETRO_DEVICE_ID_JOYPAD_B)      ? 1 : 0;
        joypad[j][6] = joypad_bits[j] & (1 << RETRO_DEVICE_ID_JOYPAD_X)      ? 1 : 0;
        joypad[j][7] = joypad_bits[j] & (1 << RETRO_DEVICE_ID_JOYPAD_Y)      ? 1 : 0;
        joypad[j][8] = joypad_bits[j] & (1 << RETRO_DEVICE_ID_JOYPAD_START)  ? 1 : 0;
        joypad[j][9] = joypad_bits[j] & (1 << RETRO_DEVICE_ID_JOYPAD_SELECT) ? 1 : 0;
        joypad[j][10] = joypad_bits[j] & (1 << RETRO_DEVICE_ID_JOYPAD_L)     ? 1 : 0;
        joypad[j][11] = joypad_bits[j] & (1 << RETRO_DEVICE_ID_JOYPAD_R)     ? 1 : 0;
        joypad[j][12] = joypad_bits[j] & (1 << RETRO_DEVICE_ID_JOYPAD_L2)    ? 1 : 0;
        joypad[j][13] = joypad_bits[j] & (1 << RETRO_DEVICE_ID_JOYPAD_R2)    ? 1 : 0;
        joypad[j][14] = joypad_bits[j] & (1 << RETRO_DEVICE_ID_JOYPAD_L3)    ? 1 : 0;
        joypad[j][15] = joypad_bits[j] & (1 << RETRO_DEVICE_ID_JOYPAD_R3)    ? 1 : 0;

        int analog_left_x = 0;
        int analog_left_y = 0;
        int analog_right_x = 0;
        int analog_right_y = 0;

        if (IsJoypadDevice(input_device[j]))
        {
            analog_left_x = input_state_cb( j, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
            analog_left_y = input_state_cb( j, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
            analog_right_x = input_state_cb( j, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
            analog_right_y = input_state_cb( j, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);
        }

        const int threshold = 4000;

        joypad_ext[j][0] = (analog_left_x > threshold || analog_left_x < -threshold) ? 1 : 0;
        joypad_ext[j][1] = (analog_left_y > threshold || analog_left_y < -threshold) ? 1 : 0;
        joypad_ext[j][2] = (analog_right_x > threshold || analog_right_x < -threshold) ? 1 : 0;
        joypad_ext[j][3] = (analog_right_y > threshold || analog_right_y < -threshold) ? 1 : 0;
    }

    for (int j = 0; j < MAX_PADS; j++)
    {
        for (int i = 0; i < JOYPAD_BUTTONS; i++)
        {
            if (joypad[j][i])
                core->KeyPressed(static_cast<GC_Controllers>(j), keymap[i]);
            else
                core->KeyReleased(static_cast<GC_Controllers>(j), keymap[i]);
        }

        for (int i = 0; i < 4; i++)
        {
            if (joypad_ext[j][i])
                core->KeyPressed(static_cast<GC_Controllers>(j), keymap[i + JOYPAD_BUTTONS]);
            else
                core->KeyReleased(static_cast<GC_Controllers>(j), keymap[i + JOYPAD_BUTTONS]);
        }
    }

    if (spinner_support > 0)
    {
        for (int i=0; i<2; i++)
        {
            mousepre[i] = mouse[i];
        }

        mouse[0] = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
        mouse[1] = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);

        int mouse_x = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
        int mouse_y = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);

        int sen = spinner_sensitivity - 1;
        if (sen < 0)
            sen = 0;
        float senf = (float)(sen / 2.0f) + 1.0f;
        float relx = (float)(mouse_x) * senf;

        switch (spinner_support)
        {
            // SAC
            case (1):
            {
                core->Spinner1((int)-relx);
                break;
            }
            // Wheel
            case (2):
            {
                core->Spinner1((int)relx);
                break;
            }
            // Roller
            case (3):
            {
                float rely = (float)(mouse_y) * senf;
                core->Spinner1((int)relx);
                core->Spinner2((int)rely);
                break;
            }
            default:
                break;
        }

        if (mouse[0])
            core->KeyPressed(Controller_1, Key_Left_Button);

        if (mouse[1])
            core->KeyPressed(Controller_1, Key_Right_Button);
    }
}

static void check_variables(void)
{
    struct retro_variable var = { NULL, NULL };

    var.key = "gearcoleco_cartridge_hardware";
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
        adam_cartridge_hardware = strcmp(var.value, "ADAM") == 0;

    var.key = "gearcoleco_adam_disk_drive";
    var.value = NULL;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        const char* names[] = { "Disk 1", "Disk 2", "Data Pack 1", "Data Pack 2" };
        adam_control_drive = -1;
        for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
        {
            if (strcmp(var.value, names[i]) == 0)
                adam_control_drive = i;
        }
        if (content_loaded && core->GetMachine() == GC_MACHINE_ADAM)
            adam_disk_set = &adam_disk_sets[adam_control_drive >= 0 ? adam_control_drive : adam_primary_slot];
    }
    var.key = "gearcoleco_adam_computer_reset";
    var.value = NULL;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        bool reset = strcmp(var.value, "Reset") == 0;
        if (reset && !adam_computer_reset_latched && content_loaded && core->GetMachine() == GC_MACHINE_ADAM)
        {
            core->ResetAdamComputer();
            clear_input_state();
            struct retro_variable idle = { "gearcoleco_adam_computer_reset", "Idle" };
            if (environ_cb(RETRO_ENVIRONMENT_SET_VARIABLE, &idle))
                reset = false;
        }
        adam_computer_reset_latched = reset;
    }

    var.key = "gearcoleco_adam_writable_media";
    var.value = NULL;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
        adam_writable_media = strcmp(var.value, "Save-directory working copy") == 0;

    var.key = "gearcoleco_up_down_allowed";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        if (strcmp(var.value, "Enabled") == 0)
            allow_up_down = true;
        else
            allow_up_down = false;
    }

    var.key = "gearcoleco_timing";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        if (strcmp(var.value, "Auto") == 0)
            config.region = Cartridge::CartridgeUnknownRegion;
        else if (strcmp(var.value, "NTSC (60 Hz)") == 0)
            config.region = Cartridge::CartridgeNTSC;
        else if (strcmp(var.value, "PAL (50 Hz)") == 0)
            config.region = Cartridge::CartridgePAL;
        else
            config.region = Cartridge::CartridgeUnknownRegion;
    }

    var.key = "gearcoleco_mapper";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        if (strcmp(var.value, "Auto") == 0)
            config.type = Cartridge::CartridgeNotSupported;
        else if (strcmp(var.value, "Standard") == 0)
            config.type = Cartridge::CartridgeColecoVision;
        else if (strcmp(var.value, "MegaCart") == 0)
            config.type = Cartridge::CartridgeMegaCart;
        else if (strcmp(var.value, "Activision") == 0)
            config.type = Cartridge::CartridgeActivisionCart;
        else if (strcmp(var.value, "OCM") == 0)
            config.type = Cartridge::CartridgeOCM;
        else
            config.type = Cartridge::CartridgeNotSupported;
    }

    var.key = "gearcoleco_video_chip";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        if (strcmp(var.value, "TMS9918A") == 0)
            video_chip = GC_VIDEO_CHIP_TMS9918A;
        else if (strcmp(var.value, "F18A") == 0)
            video_chip = GC_VIDEO_CHIP_F18A;
        else
            video_chip = GC_VIDEO_CHIP_AUTO;
    }

    var.key = "gearcoleco_aspect_ratio";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        if (strcmp(var.value, "1:1 PAR") == 0)
            aspect_ratio = 0.0f;
        else if (strcmp(var.value, "4:3 DAR") == 0)
            aspect_ratio = 4.0f / 3.0f;
        else if (strcmp(var.value, "16:9 DAR") == 0)
            aspect_ratio = 16.0f / 9.0f;
            else if (strcmp(var.value, "16:10 DAR") == 0)
            aspect_ratio = 16.0f / 10.0f;
        else
            aspect_ratio = 0.0f;
    }

    var.key = "gearcoleco_overscan";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        if (strcmp(var.value, "Disabled") == 0)
            core->GetVideo()->SetOverscan(Video::OverscanDisabled);
        else if (strcmp(var.value, "Top+Bottom") == 0)
            core->GetVideo()->SetOverscan(Video::OverscanTopBottom);
        else if (strcmp(var.value, "Full (284 width)") == 0)
            core->GetVideo()->SetOverscan(Video::OverscanFull284);
        else if (strcmp(var.value, "Full (320 width)") == 0)
            core->GetVideo()->SetOverscan(Video::OverscanFull320);
        else
            core->GetVideo()->SetOverscan(Video::OverscanDisabled);
    }

    var.key = "gearcoleco_no_sprite_limit";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        if (strcmp(var.value, "Enabled") == 0)
            core->GetVideo()->SetNoSpriteLimit(true);
        else
            core->GetVideo()->SetNoSpriteLimit(false);
    }

    var.key = "gearcoleco_spinners";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        if (strcmp(var.value, "Disabled") == 0)
            spinner_support = 0;
        else if (strcmp(var.value, "Super Action Controller") == 0)
            spinner_support = 1;
        else if (strcmp(var.value, "Wheel Controller") == 0)
            spinner_support = 2;
        else if (strcmp(var.value, "Roller Controller") == 0)
            spinner_support = 3;
        else
            spinner_support = 0;
    }

    var.key = "gearcoleco_spinner_sensitivity";
    var.value = NULL;

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        spinner_sensitivity = atoi(var.value);
    }
}

void retro_run(void)
{
    bool updated = false;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
    {
        check_variables();
    }

    update_input();

    core->RunToVBlank(frame_buffer, audio_buf, &audio_sample_count);

    GC_RuntimeInfo runtime_info;
    core->GetRuntimeInfo(runtime_info);

    if ((runtime_info.screen_width != current_screen_width) ||
        (runtime_info.screen_height != current_screen_height) ||
        (aspect_ratio != current_aspect_ratio))
    {
        current_screen_width = runtime_info.screen_width;
        current_screen_height = runtime_info.screen_height;
        current_aspect_ratio = aspect_ratio;

        retro_system_av_info info;
        info.geometry.base_width   = runtime_info.screen_width;
        info.geometry.base_height  = runtime_info.screen_height;
        info.geometry.max_width    = GC_VIDEO_MAX_WIDTH;
        info.geometry.max_height   = GC_VIDEO_MAX_HEIGHT;
        info.geometry.aspect_ratio = current_aspect_ratio;

        environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &info.geometry);
    }

    video_cb((uint8_t*)frame_buffer, runtime_info.screen_width, runtime_info.screen_height, runtime_info.screen_width * sizeof(u8) * 2);

    if (audio_sample_count > 0)
        audio_batch_cb(audio_buf, audio_sample_count / 2);

    audio_sample_count = 0;
}

void retro_reset(void)
{
    check_variables();
    core->SetVideoChip(video_chip);
    if (core->GetMachine() == GC_MACHINE_COLECOVISION)
        load_colecovision_firmware();
    core->ResetROMPreservingRAM(&config);
    check_variables();
}

static bool load_rom(const struct retro_game_info* info)
{
    if (!info)
        return false;

    if (IsValidPointer(info->data) && (info->size > 0))
        return core->LoadROMFromBuffer(reinterpret_cast<const u8*>(info->data), info->size, &config);

    if (!info->path || !info->path[0])
        return false;

    if (!vfs_interface)
        return core->LoadROM(info->path, &config);
    if (!vfs_interface->open || !vfs_interface->size || !vfs_interface->read ||
        !vfs_interface->close)
    {
        return false;
    }

    retro_vfs_file_handle* file = vfs_interface->open(info->path, RETRO_VFS_FILE_ACCESS_READ,
        RETRO_VFS_FILE_ACCESS_HINT_NONE);
    if (!file)
        return false;

    s64 size = (s64)vfs_interface->size(file);
    if ((size <= 0) || (size > 0x7FFFFFFF))
    {
        vfs_interface->close(file);
        return false;
    }

    u8* buffer = new u8[(int)size];
    s64 total = 0;

    while (total < size)
    {
        s64 read = (s64)vfs_interface->read(file, buffer + total, size - total);
        if (read <= 0)
            break;

        total += read;
    }

    bool complete_read = total == size;
    bool closed = vfs_interface->close(file) == 0;
    bool loaded = complete_read && closed;
    if (loaded)
        loaded = core->LoadROMFromBuffer(buffer, (int)size, &config);

    SafeDeleteArray(buffer);
    return loaded;
}

static bool set_pixel_format(void)
{
    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
    if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
    {
        log_cb(RETRO_LOG_ERROR, "RGB565 is not supported.\n");
        return false;
    }
    return true;
}

static bool setup_loaded_game(void)
{
    if (core->GetMachine() == GC_MACHINE_ADAM)
    {
        // RetroArch treats keyboard registration as a request for Auto Game Focus (Detect).
        // Register on ADAM load, so ordinary ColecoVision startup does not request it.
        struct retro_keyboard_callback keyboard = { keyboard_event };
        environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &keyboard);
        struct retro_memory_descriptor desc = {};
        desc.ptr = core->GetAdam()->GetMainRAM();
        desc.start = 0x0000;
        desc.len = Adam::kMainRAMSize;
        struct retro_memory_map map = { &desc, 1 };
        environ_cb(RETRO_ENVIRONMENT_SET_MEMORY_MAPS, &map);

        bool achievements = false;
        environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS, &achievements);
    }
    else
    {
        struct retro_memory_descriptor descs[6];
        memset(descs, 0, sizeof(descs));

        descs[0].ptr   = core->GetMemory()->GetBios();
        descs[0].start = 0x0000;
        descs[0].len   = 0x2000;
        descs[1].ptr   = NULL;
        descs[1].start = 0x2000;
        descs[1].len   = 0x4000;
        descs[2].ptr        = core->GetMemory()->GetRam();
        descs[2].start      = 0x6000;
        descs[2].select     = 0xE000;
        descs[2].disconnect = 0x1C00;
        descs[2].len        = 0x0400;
        descs[3].ptr   = core->GetCartridge()->GetROM();
        descs[3].start = 0x8000;
        descs[3].len   = core->GetCartridge()->GetROMSize();
        descs[4].ptr   = core->GetMemory()->GetSGMRam();
        descs[4].start = 0x010000;
        descs[4].len   = 0x2000;
        descs[5].ptr   = core->GetMemory()->GetSGMRam() + 0x2000;
        descs[5].start = 0x012000;
        descs[5].len   = 0x6000;

        struct retro_memory_map map = {
            descs, (unsigned)(sizeof(descs) / sizeof(descs[0]))
        };
        environ_cb(RETRO_ENVIRONMENT_SET_MEMORY_MAPS, &map);

        bool achievements = true;
        environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS, &achievements);
    }

    clear_input_state();
    content_loaded = true;
    adam_disk_set = &adam_disk_sets[adam_control_drive >= 0 ? adam_control_drive : adam_primary_slot];
    return true;
}

static GC_AdamMediaType infer_media_type(const struct retro_game_info* info)
{
    GC_AdamMediaType type = adam_media_type_from_path(info && info->path ? info->path : NULL);
    if (type != GC_ADAM_MEDIA_NONE)
        return type;

    if (info && ((info->data && info->size) ||
        (info->path && ends_with_no_case(info->path, ".zip"))))
    {
        u8* validation = NULL;
        size_t validation_size = 0;
        if (read_adam_game_info(info, &validation, &validation_size, &type))
            SafeDeleteArray(validation);
    }
    return type;
}

bool retro_load_game(const struct retro_game_info *info)
{
    if (content_loaded)
        retro_unload_game();

    if (!set_pixel_format())
        return false;

    check_variables();
    core->SetVideoChip(video_chip);
    clear_adam_disk_sets();
    clear_adam_host_media();
    retro_game_path[0] = '\0';

    if (!info)
    {
        if (!load_adam_firmware())
            return false;
        core->UnloadContent();
        if (!core->StartAdam(GC_ADAM_BOOT_COMPUTER))
            return false;
        return setup_loaded_game();
    }

    snprintf(retro_game_path, sizeof(retro_game_path), "%s", info->path ? info->path : "");
    bool playlist = info->path && ends_with_no_case(info->path, ".m3u");
    GC_AdamMediaType media_type = infer_media_type(info);
    GC_Machine machine = (playlist || media_type != GC_ADAM_MEDIA_NONE || adam_cartridge_hardware) ?
        GC_MACHINE_ADAM : GC_MACHINE_COLECOVISION;

    if (machine == GC_MACHINE_COLECOVISION)
    {
        if (playlist || (media_type != GC_ADAM_MEDIA_NONE))
        {
            log_cb(RETRO_LOG_ERROR, "ADAM media cannot be loaded while ColecoVision is selected.\n");
            return false;
        }
        if (!load_colecovision_firmware() || !load_rom(info))
        {
            log_cb(RETRO_LOG_ERROR, "Invalid or corrupted ColecoVision ROM.\n");
            return false;
        }
        return setup_loaded_game();
    }

    if (playlist || (media_type != GC_ADAM_MEDIA_NONE))
    {
        adam_disk_set = &adam_disk_sets[media_type == GC_ADAM_MEDIA_DATA_PACK ? 2 : 0];
        if (!initialize_disk_set(info, media_type))
        {
            log_cb(RETRO_LOG_ERROR, "Invalid or corrupted ADAM media.\n");
            return false;
        }
        // A playlist's media type is only known after parsing it.
        if (adam_disk_set->type == GC_ADAM_MEDIA_DATA_PACK && adam_disk_set->slot == GC_ADAM_MEDIA_DISK_1)
        {
            adam_disk_sets[2] = adam_disk_sets[0];
            memset(&adam_disk_sets[0], 0, sizeof(adam_disk_sets[0]));
            clear_adam_disk_set();
            adam_disk_set = &adam_disk_sets[2];
            adam_disk_set->slot = GC_ADAM_MEDIA_DATA_PACK_1;
        }
        adam_primary_slot = (unsigned)adam_disk_set->slot;
        if (!load_adam_firmware())
        {
            clear_adam_disk_sets();
            return false;
        }
        core->UnloadContent();
        if (!load_disk_set_image(adam_disk_set->index))
        {
            log_cb(RETRO_LOG_ERROR, "Invalid or corrupted ADAM media.\n");
            clear_adam_disk_sets();
            return false;
        }
    }
    else
    {
        if (!load_adam_firmware())
            return false;
        if (!load_rom(info))
        {
            log_cb(RETRO_LOG_ERROR, "Invalid or corrupted ADAM cartridge.\n");
            return false;
        }
        if (!core->StartAdam(GC_ADAM_BOOT_CARTRIDGE))
            return false;
    }

    return setup_loaded_game();
}

void retro_unload_game(void)
{
    if (!content_loaded)
        return;
    if (!flush_all_adam_media())
        log_cb(RETRO_LOG_ERROR, "Unable to flush one or more ADAM working copies during unload.\n");
    core->AdamReleaseAllKeys();
    core->UnloadContent();
    clear_adam_disk_sets();
    clear_adam_host_media();
    clear_input_state();
    retro_game_path[0] = '\0';
    content_loaded = false;
}

unsigned retro_get_region(void)
{
    return core->GetMachine() == GC_MACHINE_ADAM ? RETRO_REGION_NTSC :
        (core->GetCartridge()->IsPAL() ? RETRO_REGION_PAL : RETRO_REGION_NTSC);
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
    if (type != RETRO_ADAM_SUBSYSTEM_ID || !info || num != 5)
        return false;
    if (content_loaded)
        retro_unload_game();
    if (!set_pixel_format())
        return false;
    check_variables();
    clear_adam_disk_sets();
    clear_adam_host_media();
    bool cartridge = (info[0].data && info[0].size) || (info[0].path && info[0].path[0]);
    bool have_media = false;
    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
    {
        const struct retro_game_info* media = &info[i + 1];
        if (!(media->data && media->size) && !(media->path && media->path[0]))
            continue;
        adam_disk_set = &adam_disk_sets[i];
        if (!initialize_disk_set(media, i < 2 ? GC_ADAM_MEDIA_DISK : GC_ADAM_MEDIA_DATA_PACK))
        {
            clear_adam_disk_sets();
            return false;
        }
        if (!have_media)
            adam_primary_slot = i;
        have_media = true;
    }
    if (!load_adam_firmware())
    {
        clear_adam_disk_sets();
        return false;
    }
    core->UnloadContent();
    if (cartridge && !load_rom(&info[0]))
    {
        clear_adam_disk_sets();
        return false;
    }
    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
    {
        adam_disk_set = &adam_disk_sets[i];
        if (adam_disk_set->count && !load_disk_set_image(adam_disk_set->index))
        {
            core->UnloadContent();
            clear_adam_disk_sets();
            clear_adam_host_media();
            return false;
        }
    }
    if (!core->StartAdam(cartridge && !have_media ? GC_ADAM_BOOT_CARTRIDGE : GC_ADAM_BOOT_COMPUTER))
        return false;
    const struct retro_game_info* primary = cartridge ? &info[0] : (have_media ? &info[adam_primary_slot + 1] : NULL);
    snprintf(retro_game_path, sizeof(retro_game_path), "%s", primary && primary->path ? primary->path : "");
    return setup_loaded_game();
}

size_t retro_serialize_size(void)
{
    return IsValidPointer(core) ? core->GetLibretroSaveStateSize() :
        GC_LIBRETRO_SAVESTATE_SIZE_COLECOVISION;
}

static RetroAdamState disk_state(const RetroAdamDiskSet* set, u32 version)
{
    RetroAdamState state = {};
    state.magic = RETRO_ADAM_STATE_MAGIC;
    state.version = version;
    state.count = set->count;
    state.index = set->index;
    state.type = (u8)set->type;
    state.slot = (u8)set->slot;
    state.ejected = set->ejected ? 1 : 0;
    return state;
}

bool retro_serialize(void *data, size_t size)
{
    size_t required_size = retro_serialize_size();
    if (!content_loaded || !data || size < required_size ||
        !core->SaveState(reinterpret_cast<u8*>(data), size))
        return false;
    if (core->GetMachine() != GC_MACHINE_ADAM)
        return true;
    RetroAdamState states[GC_ADAM_MEDIA_SLOT_COUNT + 1];
    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
        states[i] = disk_state(&adam_disk_sets[i], 2);
    // Keep the identifying footer in its original location for version detection.
    states[GC_ADAM_MEDIA_SLOT_COUNT] = disk_state(adam_disk_set, 2);
    if (size < sizeof(GC_SaveState_Header_Libretro) + sizeof(states))
        return false;
    size_t offset = size - sizeof(GC_SaveState_Header_Libretro) - sizeof(states);
    memcpy(reinterpret_cast<u8*>(data) + offset, states, sizeof(states));
    return true;
}

bool retro_unserialize(const void *data, size_t size)
{
    if (!content_loaded || !data || !IsValidPointer(core))
        return false;
    bool colecovision = core->GetMachine() == GC_MACHINE_COLECOVISION;
    bool legacy_colecovision_size = colecovision && size == GC_LIBRETRO_SAVESTATE_SIZE_ADAM;
    if ((size != retro_serialize_size() && !legacy_colecovision_size) ||
        size < sizeof(GC_SaveState_Header_Libretro))
        return false;
    GC_SaveState_Header_Libretro header = {};
    memcpy(&header, reinterpret_cast<const u8*>(data) + size - sizeof(header), sizeof(header));
    bool current_header = header.magic == GC_SAVESTATE_MAGIC &&
        header.version >= GC_SAVESTATE_MIN_VERSION && header.version <= GC_SAVESTATE_VERSION;
    if (header.magic != GC_SAVESTATE_MAGIC || (!current_header && !colecovision))
        return false;

    RetroAdamState states[GC_ADAM_MEDIA_SLOT_COUNT];
    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
        states[i] = disk_state(&adam_disk_sets[i], 1);
    bool adam_state = current_header && !colecovision;
    if (adam_state)
    {
        RetroAdamState footer;
        if (size < sizeof(header) + sizeof(footer))
            return false;
        size_t offset = size - sizeof(header) - sizeof(footer);
        memcpy(&footer, reinterpret_cast<const u8*>(data) + offset, sizeof(footer));
        if (footer.magic != RETRO_ADAM_STATE_MAGIC || footer.slot >= GC_ADAM_MEDIA_SLOT_COUNT)
            return false;
        if (footer.version == 2)
        {
            if (offset < sizeof(states))
                return false;
            memcpy(states, reinterpret_cast<const u8*>(data) + offset - sizeof(states), sizeof(states));
            if (memcmp(&footer, &states[footer.slot], sizeof(footer)))
                return false;
        }
        else if (footer.version == 1)
        {
            // Version 1 recorded one controlled drive. Other mounts are still checked by the core.
            if (footer.count == 0 && footer.type == GC_ADAM_MEDIA_NONE)
                footer.type = (u8)adam_disk_sets[footer.slot].type;
            states[footer.slot] = footer;
        }
        else
            return false;
        for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
        {
            const RetroAdamState* state = &states[i];
            bool index_valid = state->ejected ? state->index <= state->count :
                (state->count > 0 && state->index < state->count);
            if (state->magic != RETRO_ADAM_STATE_MAGIC || state->version != footer.version ||
                state->reserved || state->ejected > 1 || !index_valid || state->slot != i ||
                state->count != adam_disk_sets[i].count || state->type != adam_disk_sets[i].type)
                return false;
        }
    }

    u8* selected_data[GC_ADAM_MEDIA_SLOT_COUNT] = {};
    size_t selected_size[GC_ADAM_MEDIA_SLOT_COUNT] = {};
    u32 selected_crc[GC_ADAM_MEDIA_SLOT_COUNT] = {};
    bool prepared = true;
    if (adam_state)
    {
        for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
        {
            if (states[i].ejected)
                continue;
            GC_AdamMediaType type = GC_ADAM_MEDIA_NONE;
            if (!read_disk_image(&adam_disk_sets[i].images[states[i].index], &selected_data[i],
                &selected_size[i], &type) || type != adam_disk_sets[i].type)
            {
                prepared = false;
                break;
            }
            selected_crc[i] = calculate_crc32(selected_data[i], selected_size[i]);
        }
    }
    if (!prepared)
    {
        for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
            SafeDeleteArray(selected_data[i]);
        return false;
    }

    RetroAdamHostMedia previous_host_media[GC_ADAM_MEDIA_SLOT_COUNT];
    memcpy(previous_host_media, adam_host_media, sizeof(previous_host_media));
    RetroAdamMediaBackup previous_media[GC_ADAM_MEDIA_SLOT_COUNT] = {};
    size_t backup_size = retro_serialize_size();
    u8* backup = new u8[backup_size];
    if (!core->SaveState(backup, backup_size))
    {
        for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
            SafeDeleteArray(selected_data[i]);
        SafeDeleteArray(backup);
        return false;
    }
    if (adam_state)
    {
        capture_adam_media(previous_media);
        for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
        {
            if (states[i].ejected)
            {
                core->EjectAdamMedia((GC_AdamMediaSlot)i);
                memset(&adam_host_media[i], 0, sizeof(adam_host_media[i]));
            }
            else if (!mount_adam_media((GC_AdamMediaSlot)i, adam_disk_sets[i].type,
                selected_data[i], selected_size[i], adam_disk_sets[i].images[states[i].index].path))
            {
                prepared = false;
                break;
            }
        }
    }
    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
        SafeDeleteArray(selected_data[i]);
    bool loaded = prepared && core->LoadState(reinterpret_cast<const u8*>(data), size);
    if (loaded && adam_state)
    {
        for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
        {
            AdamMedia* media = core->GetAdamMedia((GC_AdamMediaSlot)i);
            bool matches = states[i].ejected ? !media->IsInserted() :
                (media->IsInserted() && media->GetType() == adam_disk_sets[i].type &&
                media->GetSize() == selected_size[i] && media->GetBaseCRC() == selected_crc[i]);
            if (!matches)
            {
                loaded = false;
                break;
            }
        }
    }
    if (loaded)
    {
        if (adam_state)
        {
            for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
            {
                adam_disk_sets[i].index = states[i].index;
                adam_disk_sets[i].ejected = states[i].ejected != 0;
            }
        }
        clear_adam_media_backup(previous_media);
        SafeDeleteArray(backup);
        clear_input_state();
        return true;
    }
    if (adam_state && !prepare_adam_media(previous_media))
        log_cb(RETRO_LOG_ERROR, "Failed to prepare media while restoring rejected save state.\n");
    if (!core->LoadState(backup, backup_size))
        log_cb(RETRO_LOG_ERROR, "Failed to restore core after rejected save state.\n");
    clear_adam_media_backup(previous_media);
    SafeDeleteArray(backup);
    memcpy(adam_host_media, previous_host_media, sizeof(adam_host_media));
    return false;
}

void *retro_get_memory_data(unsigned id)
{
    switch (id)
    {
        case RETRO_MEMORY_SAVE_RAM:
        {
            Mapper* mapper = core->GetMemory()->GetMapper();
            if (mapper)
                return mapper->GetSaveData();
            return NULL;
        }
        case RETRO_MEMORY_SYSTEM_RAM:
            return core->GetMachine() == GC_MACHINE_ADAM ? core->GetAdam()->GetMainRAM() :
                core->GetMemory()->GetRam();
    }

    return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
    switch (id)
    {
        case RETRO_MEMORY_SAVE_RAM:
        {
            Mapper* mapper = core->GetMemory()->GetMapper();
            if (mapper)
                return mapper->GetSaveDataSize();
            return 0;
        }
        case RETRO_MEMORY_SYSTEM_RAM:
            return core->GetMachine() == GC_MACHINE_ADAM ? Adam::kMainRAMSize : 0x400;
    }

    return 0;
}

void retro_cheat_reset(void)
{
    // TODO
}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
    UNUSED(index);
    UNUSED(enabled);
    UNUSED(code);
    // TODO
}
