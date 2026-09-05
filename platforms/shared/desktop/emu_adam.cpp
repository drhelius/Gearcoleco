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

#include "emu_adam.h"

#include <stdio.h>
#include <fstream>
#include <string>
#include <string.h>
#include <SDL3/SDL.h>
#include "emu.h"
#include "config.h"
#include "rewind.h"
#include "runahead.h"
#include "utils.h"
#include "Adam.h"
#include "AdamMedia.h"
#include "AdamNet.h"
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include "miniz.h"
#undef MINIZ_NO_ZLIB_COMPATIBLE_NAMES

struct AdamHostMediaRecord
{
    char source_path[4096];
    char working_path[4096];
    u32 base_crc;
    bool state_owned;
};

struct AdamDesktopPlaylist
{
    int count;
    int current;
    GC_AdamMediaSlot slot;
    EmuDesktopContentType type;
    char playlist_path[4096];
    char paths[64][4096];
    char names[64][512];
};

static bool loading_media_persistence;
static bool loading_media_write_protected[GC_ADAM_MEDIA_SLOT_COUNT];
static int loading_savefiles_dir_option;
static char loading_savefiles_path[4096];
static char loading_bios_path[4096];
static char loading_eos_path[4096];
static char loading_smartwriter_path[4096];
static AdamHostMediaRecord host_media[GC_ADAM_MEDIA_SLOT_COUNT];
static AdamDesktopPlaylist loading_playlist;
static AdamDesktopPlaylist playlist;

static bool read_binary_file(const char* path, u8** data, size_t* size);
static bool read_binary_file_exact(const char* path, u8** data, size_t expected_size);
static u32 calculate_crc32(const u8* data, size_t size);
static bool file_has_size(const char* path, size_t expected_size);
static void resolve_firmware_path(GC_AdamFirmware firmware, char* path, size_t path_size);
static GC_AdamMediaType media_type_from_path(const char* path);
static void init_content(EmuDesktopContent* content);
static void clear_playlist(AdamDesktopPlaylist* target);
static void commit_playlist(GC_AdamMediaSlot slot);
static bool classify_zip(const char* path, const u8* data, size_t size,
    GC_Machine machine, EmuDesktopContent* content);
static bool classify_playlist(const char* path, EmuDesktopContent* content);
static bool valid_cartridge(const u8* data, size_t size);
static void append_zip_candidate(std::string* candidates, const char* type,
    const char* name);
static void clear_host_media(GC_AdamMediaSlot slot);
static void make_working_path(const char* source_path, GC_AdamMediaType type,
    GC_AdamMediaSlot slot, u32 base_crc, bool primary, char* path, size_t path_size);
static bool mount_content(GC_AdamMediaSlot slot, const EmuDesktopContent* content,
    bool primary, bool discard_current_changes);
static bool write_working_copy(GC_AdamMediaSlot slot);
static bool check_working_path(const char* file_path);
static bool write_media_file(AdamMedia* media, const char* file_path);

void emu_adam_init(void)
{
    clear_playlist(&loading_playlist);
    clear_playlist(&playlist);
    emu_adam_clear_host_media();
}

static bool read_binary_file(const char* path, u8** data, size_t* size)
{
    if (!IsValidPointer(path) || !IsValidPointer(data) || !IsValidPointer(size) ||
        (path[0] == '\0'))
    {
        return false;
    }

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

static void resolve_firmware_path(GC_AdamFirmware firmware, char* path, size_t path_size)
{
    if (!IsValidPointer(path) || (path_size == 0))
        return;

    path[0] = '\0';
    const std::string* configured = NULL;
    if (firmware == GC_ADAM_FIRMWARE_OS7)
        configured = &config_emulator.bios_path;
    else if (firmware == GC_ADAM_FIRMWARE_EOS)
        configured = &config_emulator.adam_eos_path;
    else if (firmware == GC_ADAM_FIRMWARE_SMARTWRITER)
        configured = &config_emulator.adam_smartwriter_path;
    else
        return;

    if (!configured->empty())
    {
        strncpy_fit(path, configured->c_str(), path_size);
        return;
    }
    if ((firmware == GC_ADAM_FIRMWARE_OS7) || config_emulator.bios_path.empty())
        return;

    char directory[4096];
    get_directory(config_emulator.bios_path.c_str(), directory, sizeof(directory));
    const Adam::FirmwareMetadata* metadata = Adam::GetFirmwareMetadata(firmware);
    for (int i = 0; i < 4 && metadata->aliases[i]; i++)
    {
        char candidate[4096];
        join_path(directory, metadata->aliases[i], candidate, sizeof(candidate));
        if (file_has_size(candidate, metadata->size))
        {
            strncpy_fit(path, candidate, path_size);
            return;
        }
    }
    join_path(directory, metadata->aliases[0], path, path_size);
}

void emu_adam_prepare_load(void)
{
    loading_media_persistence = config_emulator.adam_media_persistence;
    memcpy(loading_media_write_protected, config_emulator.adam_media_write_protected,
        sizeof(loading_media_write_protected));
    loading_savefiles_dir_option = config_emulator.savefiles_dir_option;
    strncpy_fit(loading_savefiles_path, config_emulator.savefiles_path.c_str(),
        sizeof(loading_savefiles_path));
    resolve_firmware_path(GC_ADAM_FIRMWARE_OS7, loading_bios_path,
        sizeof(loading_bios_path));
    resolve_firmware_path(GC_ADAM_FIRMWARE_EOS, loading_eos_path,
        sizeof(loading_eos_path));
    resolve_firmware_path(GC_ADAM_FIRMWARE_SMARTWRITER, loading_smartwriter_path,
        sizeof(loading_smartwriter_path));
}

bool emu_adam_load_firmware(void)
{
    u8* os7 = NULL;
    u8* eos = NULL;
    u8* smartwriter = NULL;
    bool os7_loaded = read_binary_file_exact(loading_bios_path, &os7, Adam::kOS7ROMSize);
    bool eos_loaded = read_binary_file_exact(loading_eos_path, &eos, Adam::kEOSROMSize);
    bool smartwriter_loaded = read_binary_file_exact(loading_smartwriter_path, &smartwriter,
        Adam::kSmartWriterROMSize);
    if (!os7_loaded || !eos_loaded || !smartwriter_loaded)
    {
        Error("ADAM firmware is incomplete. Required: OS-7 colecovision.rom/coleco.rom/os7.u2 "
            "(8192 bytes), EOS eos.rom (8192 bytes), SmartWriter writer.rom/wp.rom/wp_r80.rom "
            "(32768 bytes). Searched: OS-7 '%s', EOS '%s', SmartWriter '%s'",
            loading_bios_path, loading_eos_path, loading_smartwriter_path);
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

    GearcolecoCore* core = emu_get_core();
    bool loaded = core->LoadAdamFirmware(os7, Adam::kOS7ROMSize, eos,
        Adam::kEOSROMSize, smartwriter, Adam::kSmartWriterROMSize);
    if (loaded)
        loaded = core->GetMemory()->LoadBiosFromBuffer(os7, Adam::kOS7ROMSize);
    SafeDeleteArray(os7);
    SafeDeleteArray(eos);
    SafeDeleteArray(smartwriter);
    return loaded;
}

static GC_AdamMediaType media_type_from_path(const char* path)
{
    if (ends_with_no_case(path, ".ddp"))
        return GC_ADAM_MEDIA_DATA_PACK;
    if (ends_with_no_case(path, ".dsk"))
        return GC_ADAM_MEDIA_DISK;
    return GC_ADAM_MEDIA_NONE;
}

static void init_content(EmuDesktopContent* content)
{
    memset(content, 0, sizeof(*content));
    content->type = EmuDesktopContentInvalid;
}

void emu_adam_destroy_content(EmuDesktopContent* content)
{
    SafeDeleteArray(content->data);
    init_content(content);
}

static void clear_playlist(AdamDesktopPlaylist* target)
{
    memset(target, 0, sizeof(*target));
    target->slot = GC_ADAM_MEDIA_SLOT_COUNT;
    target->type = EmuDesktopContentInvalid;
}

static void commit_playlist(GC_AdamMediaSlot slot)
{
    playlist = loading_playlist;
    playlist.slot = slot;
    playlist.current = 0;
    clear_playlist(&loading_playlist);
}

static bool valid_cartridge(const u8* data, size_t size)
{
    if (!IsValidPointer(data) || (size == 0) || (size > 0x7FFFFFFF))
        return false;
    return Cartridge::IsValidROMBuffer(data, (int)size);
}

static void append_zip_candidate(std::string* candidates, const char* type,
    const char* name)
{
    if (!candidates->empty())
        candidates->append(", ");
    candidates->append(type);
    candidates->append(" '");
    candidates->append(name);
    candidates->append("'");
}

static bool classify_zip(const char* path, const u8* data, size_t size,
    GC_Machine machine, EmuDesktopContent* content)
{
    mz_zip_archive archive;
    memset(&archive, 0, sizeof(archive));
    if (!mz_zip_reader_init_mem(&archive, data, size, 0))
    {
        Error("Invalid ZIP archive: %s", path);
        return false;
    }

    u8* cartridge_data = NULL;
    size_t cartridge_size = 0;
    char cartridge_name[512] = "";
    int cartridge_count = 0;
    u8* adam_data = NULL;
    size_t adam_size = 0;
    char adam_name[512] = "";
    GC_AdamMediaType adam_type = GC_ADAM_MEDIA_NONE;
    int adam_count = 0;
    std::string candidates;

    mz_uint files = mz_zip_reader_get_num_files(&archive);
    for (mz_uint i = 0; i < files; i++)
    {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&archive, i, &file_stat))
        {
            mz_zip_reader_end(&archive);
            SafeDeleteArray(cartridge_data);
            SafeDeleteArray(adam_data);
            Error("Unable to inspect ZIP entry %u: %s", i, path);
            return false;
        }
        if (file_stat.m_is_directory || !file_stat.m_is_supported ||
            (file_stat.m_uncomp_size == 0) ||
            (file_stat.m_uncomp_size > (mz_uint64)(size_t)-1))
        {
            continue;
        }

        bool cartridge = ends_with_no_case(file_stat.m_filename, ".col") ||
            ends_with_no_case(file_stat.m_filename, ".cv") ||
            ends_with_no_case(file_stat.m_filename, ".rom") ||
            ends_with_no_case(file_stat.m_filename, ".bin");
        GC_AdamMediaType media_type = media_type_from_path(file_stat.m_filename);
        if (!cartridge && (media_type == GC_ADAM_MEDIA_NONE))
            continue;

        size_t extracted_size = (size_t)file_stat.m_uncomp_size;
        u8* extracted = new u8[extracted_size];
        if (!mz_zip_reader_extract_to_mem(&archive, i, extracted, extracted_size, 0))
        {
            SafeDeleteArray(extracted);
            mz_zip_reader_end(&archive);
            SafeDeleteArray(cartridge_data);
            SafeDeleteArray(adam_data);
            Error("Unable to extract ZIP entry '%s': %s", file_stat.m_filename, path);
            return false;
        }

        if (cartridge && valid_cartridge(extracted, extracted_size))
        {
            cartridge_count++;
            append_zip_candidate(&candidates, "cartridge", file_stat.m_filename);
            if (!IsValidPointer(cartridge_data))
            {
                cartridge_data = extracted;
                cartridge_size = extracted_size;
                strncpy_fit(cartridge_name, file_stat.m_filename, sizeof(cartridge_name));
                extracted = NULL;
            }
        }
        else if ((media_type != GC_ADAM_MEDIA_NONE) &&
            AdamMedia::IsValidImageSize(media_type, extracted_size))
        {
            adam_count++;
            append_zip_candidate(&candidates,
                media_type == GC_ADAM_MEDIA_DISK ? "ADAM disk" : "ADAM data pack",
                file_stat.m_filename);
            if (!IsValidPointer(adam_data))
            {
                adam_data = extracted;
                adam_size = extracted_size;
                adam_type = media_type;
                strncpy_fit(adam_name, file_stat.m_filename, sizeof(adam_name));
                extracted = NULL;
            }
        }
        SafeDeleteArray(extracted);
    }
    mz_zip_reader_end(&archive);

    bool choose_adam = false;
    int compatible_count = 0;
    if (machine == GC_MACHINE_COLECOVISION)
        compatible_count = cartridge_count;
    else if (machine == GC_MACHINE_ADAM)
    {
        choose_adam = adam_count > 0;
        compatible_count = choose_adam ? adam_count : cartridge_count;
    }
    else
    {
        compatible_count = cartridge_count + adam_count;
        choose_adam = (adam_count == 1) && (cartridge_count == 0);
    }

    if (compatible_count != 1)
    {
        if (compatible_count > 1)
            Error("Ambiguous ZIP content in %s. Candidates: %s", path,
                candidates.empty() ? "none" : candidates.c_str());
        else
            Error("ZIP contains no compatible content for the selected machine: %s. "
                "Recognized candidates: %s", path,
                candidates.empty() ? "none" : candidates.c_str());
        SafeDeleteArray(cartridge_data);
        SafeDeleteArray(adam_data);
        return false;
    }

    content->archive = true;
    strncpy_fit(content->source_path, path, sizeof(content->source_path));
    if (choose_adam)
    {
        content->type = adam_type == GC_ADAM_MEDIA_DISK ? EmuDesktopContentAdamDisk :
            EmuDesktopContentAdamDataPack;
        content->data = adam_data;
        content->size = adam_size;
        strncpy_fit(content->entry_name, adam_name, sizeof(content->entry_name));
        SafeDeleteArray(cartridge_data);
    }
    else
    {
        content->type = EmuDesktopContentCartridge;
        content->data = cartridge_data;
        content->size = cartridge_size;
        strncpy_fit(content->entry_name, cartridge_name, sizeof(content->entry_name));
        SafeDeleteArray(adam_data);
    }
    return true;
}

bool emu_adam_classify_content(const char* path, GC_Machine machine,
    EmuDesktopContent* content)
{
    if (!IsValidPointer(path) || !IsValidPointer(content))
        return false;
    init_content(content);
    if (ends_with_no_case(path, ".m3u"))
        return classify_playlist(path, content);

    u8* data = NULL;
    size_t size = 0;
    if (!read_binary_file(path, &data, &size))
    {
        Error("Unable to read content: %s", path);
        return false;
    }
    if (ends_with_no_case(path, ".zip"))
    {
        bool classified = classify_zip(path, data, size, machine, content);
        SafeDeleteArray(data);
        return classified;
    }

    GC_AdamMediaType media_type = media_type_from_path(path);
    if (media_type != GC_ADAM_MEDIA_NONE)
    {
        if (!AdamMedia::IsValidImageSize(media_type, size))
        {
            Error("Invalid ADAM media size %zu: %s", size, path);
            SafeDeleteArray(data);
            return false;
        }
        content->type = media_type == GC_ADAM_MEDIA_DISK ? EmuDesktopContentAdamDisk :
            EmuDesktopContentAdamDataPack;
    }
    else
    {
        if (size > 0x7FFFFFFF)
        {
            Error("Cartridge image is too large: %s", path);
            SafeDeleteArray(data);
            return false;
        }
        content->type = EmuDesktopContentCartridge;
    }

    content->data = data;
    content->size = size;
    strncpy_fit(content->source_path, path, sizeof(content->source_path));
    strncpy_fit(content->entry_name, get_filename(path), sizeof(content->entry_name));
    return true;
}

static bool classify_playlist(const char* path, EmuDesktopContent* content)
{
    clear_playlist(&loading_playlist);
    strncpy_fit(loading_playlist.playlist_path, path,
        sizeof(loading_playlist.playlist_path));

    u8* text = NULL;
    size_t text_size = 0;
    if (!read_binary_file(path, &text, &text_size))
    {
        clear_playlist(&loading_playlist);
        Error("Unable to read ADAM playlist: %s", path);
        return false;
    }

    char directory[4096];
    get_directory(path, directory, sizeof(directory));
    EmuDesktopContent selected;
    init_content(&selected);
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
                emu_adam_destroy_content(&selected);
                clear_playlist(&loading_playlist);
                SafeDeleteArray(text);
                Error("ADAM playlist contains more than 64 entries: %s", path);
                return false;
            }
            if ((end - position) >= 4096)
            {
                emu_adam_destroy_content(&selected);
                clear_playlist(&loading_playlist);
                SafeDeleteArray(text);
                Error("ADAM playlist entry is too long: %s", path);
                return false;
            }

            char entry[4096];
            memcpy(entry, text + position, end - position);
            entry[end - position] = '\0';
            char resolved[4096];
            if (!join_path(directory, entry, resolved, sizeof(resolved)))
            {
                emu_adam_destroy_content(&selected);
                clear_playlist(&loading_playlist);
                SafeDeleteArray(text);
                return false;
            }

            EmuDesktopContent entry_content;
            init_content(&entry_content);
            bool valid = emu_adam_classify_content(resolved, GC_MACHINE_ADAM,
                &entry_content);
            bool adam_media = entry_content.type == EmuDesktopContentAdamDisk ||
                entry_content.type == EmuDesktopContentAdamDataPack;
            if (!valid || !adam_media || entry_content.playlist ||
                ((selected.type != EmuDesktopContentInvalid) &&
                (selected.type != entry_content.type)))
            {
                emu_adam_destroy_content(&entry_content);
                emu_adam_destroy_content(&selected);
                clear_playlist(&loading_playlist);
                SafeDeleteArray(text);
                Error("ADAM playlist contains missing, invalid, or mixed media: %s", resolved);
                return false;
            }

            if (entries == 0)
            {
                selected = entry_content;
                entry_content.data = NULL;
            }
            strncpy_fit(loading_playlist.paths[entries], resolved,
                sizeof(loading_playlist.paths[entries]));
            strncpy_fit(loading_playlist.names[entries], get_filename(resolved),
                sizeof(loading_playlist.names[entries]));
            emu_adam_destroy_content(&entry_content);
            entries++;
        }
        position = next;
    }

    SafeDeleteArray(text);
    if (entries == 0)
    {
        emu_adam_destroy_content(&selected);
        clear_playlist(&loading_playlist);
        Error("ADAM playlist is empty: %s", path);
        return false;
    }

    loading_playlist.count = entries;
    loading_playlist.type = selected.type;
    Log("Desktop ADAM playlist loaded with %d entr%s", entries,
        entries == 1 ? "y" : "ies");
    selected.playlist = true;
    *content = selected;
    return true;
}

static void clear_host_media(GC_AdamMediaSlot slot)
{
    if ((slot < GC_ADAM_MEDIA_DISK_1) || (slot >= GC_ADAM_MEDIA_SLOT_COUNT))
        return;
    memset(&host_media[slot], 0, sizeof(host_media[slot]));
}

void emu_adam_clear_host_media(void)
{
    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
        clear_host_media((GC_AdamMediaSlot)i);
    clear_playlist(&playlist);
}

static void make_working_path(const char* source_path, GC_AdamMediaType type,
    GC_AdamMediaSlot slot, u32 base_crc, bool primary, char* path, size_t path_size)
{
    char directory[4096];
    int directory_option = primary ? loading_savefiles_dir_option :
        config_emulator.savefiles_dir_option;
    const char* custom_directory = primary ? loading_savefiles_path :
        config_emulator.savefiles_path.c_str();
    switch ((Directory_Location)directory_option)
    {
        case Directory_Location_ROM:
            get_directory(source_path, directory, sizeof(directory));
            break;
        case Directory_Location_Custom:
            strncpy_fit(directory, custom_directory, sizeof(directory));
            break;
        default:
        case Directory_Location_Default:
            strncpy_fit(directory, config_root_path, sizeof(directory));
            break;
    }

    char name[1024];
    char filename[1400];
    get_filename_without_extension(source_path, name, sizeof(name));
    const char* extension = type == GC_ADAM_MEDIA_DATA_PACK ? "ddp" : "dsk";
    snprintf(filename, sizeof(filename), "%s.%08x.slot%d.gearcoleco.%s", name, base_crc,
        (int)slot + 1, extension);
    join_path(directory, filename, path, path_size);
}

static bool mount_content(GC_AdamMediaSlot slot, const EmuDesktopContent* content,
    bool primary, bool discard_current_changes)
{
    GC_AdamMediaType type = content->type == EmuDesktopContentAdamDisk ?
        GC_ADAM_MEDIA_DISK : GC_ADAM_MEDIA_DATA_PACK;
    bool disk_slot = (slot == GC_ADAM_MEDIA_DISK_1) || (slot == GC_ADAM_MEDIA_DISK_2);
    if ((disk_slot && (type != GC_ADAM_MEDIA_DISK)) ||
        (!disk_slot && (type != GC_ADAM_MEDIA_DATA_PACK)))
    {
        Error("ADAM media type does not match slot %d", slot);
        return false;
    }
    if (content->archive)
        Log("ADAM media extracted from ZIP: %s (%s)", content->source_path,
            content->entry_name);

    u32 base_crc = calculate_crc32(content->data, content->size);
    char working_path[4096];
    make_working_path(content->source_path, type, slot, base_crc, primary, working_path,
        sizeof(working_path));

    const u8* mounted_data = content->data;
    u8* working = NULL;
    bool persistence = primary ? loading_media_persistence :
        config_emulator.adam_media_persistence;
    bool configured_write_protected = primary ? loading_media_write_protected[slot] :
        config_emulator.adam_media_write_protected[slot];
    if (persistence && !check_working_path(working_path))
        return false;
    if (persistence && read_binary_file_exact(working_path, &working, content->size))
    {
        mounted_data = working;
        Log("Loading ADAM working copy: %s", working_path);
    }
    if (!primary && !discard_current_changes && !write_working_copy(slot))
    {
        SafeDeleteArray(working);
        return false;
    }

    bool write_protected = !persistence || configured_write_protected;
    bool loaded = emu_get_core()->LoadAdamMediaFromBuffer(slot, type, mounted_data,
        content->size, write_protected, base_crc);
    if (loaded)
    {
        clear_host_media(slot);
        strncpy_fit(host_media[slot].source_path, content->source_path,
            sizeof(host_media[slot].source_path));
        if (persistence)
            strncpy_fit(host_media[slot].working_path, working_path,
                sizeof(host_media[slot].working_path));
        host_media[slot].base_crc = base_crc;
    }
    SafeDeleteArray(working);
    return loaded;
}

bool emu_adam_load_content(const EmuDesktopContent* content, const char* requested_path,
    int boot_mode, Cartridge::ForceConfiguration* config, bool softpatching)
{
    GearcolecoCore* core = emu_get_core();
    if ((content->type == EmuDesktopContentAdamDisk) ||
        (content->type == EmuDesktopContentAdamDataPack))
    {
        if (boot_mode == 2)
        {
            Error("ADAM cartridge boot was selected but no cartridge was loaded");
            return false;
        }
        if (!emu_adam_load_firmware())
            return false;

        core->UnloadContent();
        emu_adam_clear_host_media();
        GC_AdamMediaSlot slot = content->type == EmuDesktopContentAdamDataPack ?
            GC_ADAM_MEDIA_DATA_PACK_1 : GC_ADAM_MEDIA_DISK_1;
        if (!mount_content(slot, content, true, false))
            return false;
        if (content->playlist)
            commit_playlist(slot);
        return true;
    }

    if (!emu_adam_load_firmware())
        return false;
    if (!core->LoadROMFromBuffer(content->data, (int)content->size, config,
        content->source_path, softpatching))
    {
        Error("Invalid ADAM cartridge: %s", requested_path);
        return false;
    }

    emu_adam_clear_host_media();
    GC_AdamBootMode selected_boot = boot_mode == 1 ? GC_ADAM_BOOT_COMPUTER :
        GC_ADAM_BOOT_CARTRIDGE;
    return core->StartAdam(selected_boot);
}

static bool write_working_copy(GC_AdamMediaSlot slot)
{
    AdamMedia* media = emu_get_core()->GetAdamMedia(slot);
    if (!IsValidPointer(media) || !media->IsInserted() || !media->IsDirty())
        return true;
    AdamHostMediaRecord* record = &host_media[slot];
    if (record->working_path[0] == '\0')
        return false;
    return write_media_file(media, record->working_path);
}

static bool check_working_path(const char* file_path)
{
    if (!IsValidPointer(file_path) || (file_path[0] == '\0'))
        return false;

    std::string probe_path(file_path);
    probe_path += ".write-test";
    std::ofstream probe;
    open_ofstream_utf8(probe, probe_path.c_str(), std::ios::out | std::ios::binary |
        std::ios::trunc);
    if (!probe.is_open())
    {
        Error("ADAM working-copy destination is not writable: %s", file_path);
        return false;
    }
    probe.close();
    bool writable = probe.good() && SDL_RemovePath(probe_path.c_str());
    if (!writable)
        Error("ADAM working-copy destination is not writable: %s", file_path);
    return writable;
}

static bool write_media_file(AdamMedia* media, const char* file_path)
{
    if (!IsValidPointer(media) || !media->IsInserted() || !IsValidPointer(file_path) ||
        (file_path[0] == '\0'))
    {
        return false;
    }

    std::string temporary_path(file_path);
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
    if (!SDL_RenamePath(temporary_path.c_str(), file_path))
    {
        SDL_RemovePath(temporary_path.c_str());
        Error("Unable to replace ADAM working copy %s: %s", file_path, SDL_GetError());
        return false;
    }
    media->ClearDirty();
    Log("ADAM working copy saved: %s", file_path);
    return true;
}

bool emu_flush_adam_media(void)
{
    if (emu_is_busy())
        return false;
    bool flushed = true;
    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
    {
        if (!write_working_copy((GC_AdamMediaSlot)i))
            flushed = false;
    }
    return flushed;
}

bool emu_load_adam_firmware(GC_AdamFirmware firmware, const char* file_path)
{
    GearcolecoCore* core = emu_get_core();
    if (emu_is_busy() || (!emu_is_empty() && (core->GetMachine() == GC_MACHINE_ADAM)) ||
        (firmware < GC_ADAM_FIRMWARE_OS7) || (firmware >= GC_ADAM_FIRMWARE_COUNT))
    {
        return false;
    }

    const Adam::FirmwareMetadata* metadata = Adam::GetFirmwareMetadata(firmware);
    u8* data = NULL;
    if (!read_binary_file_exact(file_path, &data, metadata->size))
    {
        Error("Invalid ADAM firmware role %d: %s", firmware, file_path ? file_path : "");
        return false;
    }
    bool loaded = core->LoadAdamFirmware(firmware, data, metadata->size);
    if (loaded && (firmware == GC_ADAM_FIRMWARE_OS7))
        loaded = core->GetMemory()->LoadBiosFromBuffer(data, metadata->size);
    SafeDeleteArray(data);
    return loaded;
}

bool emu_is_adam_firmware_loaded(GC_AdamFirmware firmware)
{
    return !emu_is_busy() && (firmware >= GC_ADAM_FIRMWARE_OS7) &&
        (firmware < GC_ADAM_FIRMWARE_COUNT) &&
        (emu_get_core()->GetAdam()->GetFirmwareCRC(firmware) != 0);
}

u32 emu_get_adam_firmware_crc(GC_AdamFirmware firmware)
{
    if (emu_is_busy() || (firmware < GC_ADAM_FIRMWARE_OS7) ||
        (firmware >= GC_ADAM_FIRMWARE_COUNT))
    {
        return 0;
    }
    return emu_get_core()->GetAdam()->GetFirmwareCRC(firmware);
}

void emu_get_adam_firmware_path(GC_AdamFirmware firmware, char* path, size_t path_size)
{
    resolve_firmware_path(firmware, path, path_size);
}

bool emu_inspect_adam_firmware(GC_AdamFirmware firmware, const char* file_path,
    size_t* actual_size, u32* crc)
{
    if (!IsValidPointer(actual_size) || !IsValidPointer(crc) ||
        (firmware < GC_ADAM_FIRMWARE_OS7) || (firmware >= GC_ADAM_FIRMWARE_COUNT))
    {
        return false;
    }
    *actual_size = 0;
    *crc = 0;
    u8* data = NULL;
    if (!read_binary_file(file_path, &data, actual_size))
        return false;
    *crc = calculate_crc32(data, *actual_size);
    SafeDeleteArray(data);
    return *actual_size == (size_t)Adam::GetFirmwareMetadata(firmware)->size;
}

bool emu_are_adam_firmware_paths_valid(void)
{
    for (int i = 0; i < GC_ADAM_FIRMWARE_COUNT; i++)
    {
        char path[4096];
        size_t actual_size = 0;
        u32 crc = 0;
        GC_AdamFirmware firmware = (GC_AdamFirmware)i;
        resolve_firmware_path(firmware, path, sizeof(path));
        if (!emu_inspect_adam_firmware(firmware, path, &actual_size, &crc))
            return false;
    }
    return true;
}

bool emu_insert_adam_media(GC_AdamMediaSlot slot, const char* file_path)
{
    return emu_replace_adam_media(slot, file_path, false);
}

bool emu_replace_adam_media(GC_AdamMediaSlot slot, const char* file_path,
    bool discard_current_changes)
{
    if (emu_is_busy() || (emu_get_core()->GetMachine() != GC_MACHINE_ADAM) ||
        (slot < GC_ADAM_MEDIA_DISK_1) || (slot >= GC_ADAM_MEDIA_SLOT_COUNT))
    {
        return false;
    }

    clear_playlist(&loading_playlist);
    EmuDesktopContent content;
    init_content(&content);
    if (!emu_adam_classify_content(file_path, GC_MACHINE_ADAM, &content))
        return false;
    bool disk = content.type == EmuDesktopContentAdamDisk;
    bool data_pack = content.type == EmuDesktopContentAdamDataPack;
    bool disk_slot = (slot == GC_ADAM_MEDIA_DISK_1) || (slot == GC_ADAM_MEDIA_DISK_2);
    if ((disk_slot && !disk) || (!disk_slot && !data_pack))
    {
        emu_adam_destroy_content(&content);
        return false;
    }

    bool loaded = mount_content(slot, &content, false, discard_current_changes);
    if (loaded)
    {
        if (content.playlist)
            commit_playlist(slot);
        else if (playlist.slot == slot)
            clear_playlist(&playlist);
    }
    emu_adam_destroy_content(&content);
    if (loaded)
    {
        rewind_reset();
        runahead_reset();
    }
    return loaded;
}

int emu_get_adam_playlist_count(GC_AdamMediaSlot slot)
{
    return !emu_is_busy() && (playlist.slot == slot) ? playlist.count : 0;
}

int emu_get_adam_playlist_index(GC_AdamMediaSlot slot)
{
    return !emu_is_busy() && (playlist.slot == slot) ? playlist.current : -1;
}

const char* emu_get_adam_playlist_name(GC_AdamMediaSlot slot, int index)
{
    if (emu_is_busy() || (playlist.slot != slot) || (index < 0) ||
        (index >= playlist.count))
    {
        return "";
    }
    return playlist.names[index];
}

const char* emu_get_adam_playlist_path(GC_AdamMediaSlot slot)
{
    return !emu_is_busy() && (playlist.slot == slot) ? playlist.playlist_path : "";
}

bool emu_select_adam_playlist_entry(GC_AdamMediaSlot slot, int index,
    bool discard_current_changes)
{
    if (emu_is_busy() || (emu_get_core()->GetMachine() != GC_MACHINE_ADAM) ||
        (playlist.slot != slot) || (index < 0) || (index >= playlist.count))
    {
        return false;
    }
    if (index == playlist.current)
        return true;

    EmuDesktopContent content;
    init_content(&content);
    if (!emu_adam_classify_content(playlist.paths[index], GC_MACHINE_ADAM, &content) ||
        (content.type != playlist.type))
    {
        emu_adam_destroy_content(&content);
        return false;
    }
    bool loaded = mount_content(slot, &content, false, discard_current_changes);
    emu_adam_destroy_content(&content);
    if (loaded)
    {
        playlist.current = index;
        rewind_reset();
        runahead_reset();
    }
    return loaded;
}

bool emu_save_adam_media(GC_AdamMediaSlot slot)
{
    if (emu_is_busy() || (slot < GC_ADAM_MEDIA_DISK_1) ||
        (slot >= GC_ADAM_MEDIA_SLOT_COUNT))
    {
        return false;
    }
    bool saved = write_working_copy(slot);
    if (saved)
    {
        rewind_reset();
        runahead_reset();
    }
    return saved;
}

bool emu_save_adam_media_as(GC_AdamMediaSlot slot, const char* file_path)
{
    if (emu_is_busy() || !IsValidPointer(file_path) ||
        (slot < GC_ADAM_MEDIA_DISK_1) || (slot >= GC_ADAM_MEDIA_SLOT_COUNT))
    {
        return false;
    }
    AdamMedia* media = emu_get_core()->GetAdamMedia(slot);
    if (!write_media_file(media, file_path))
        return false;

    clear_host_media(slot);
    AdamHostMediaRecord* record = &host_media[slot];
    strncpy_fit(record->source_path, file_path, sizeof(record->source_path));
    strncpy_fit(record->working_path, file_path, sizeof(record->working_path));
    record->base_crc = media->GetBaseCRC();
    rewind_reset();
    runahead_reset();
    return true;
}

bool emu_discard_adam_media_changes(GC_AdamMediaSlot slot)
{
    if (emu_is_busy() || (slot < GC_ADAM_MEDIA_DISK_1) ||
        (slot >= GC_ADAM_MEDIA_SLOT_COUNT))
    {
        return false;
    }
    AdamMedia* media = emu_get_core()->GetAdamMedia(slot);
    if (!IsValidPointer(media) || !media->IsInserted())
        return false;
    media->ClearDirty();
    rewind_reset();
    runahead_reset();
    return true;
}

bool emu_discard_all_adam_media_changes(void)
{
    if (emu_is_busy())
        return false;
    bool discarded = false;
    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
    {
        AdamMedia* media = emu_get_core()->GetAdamMedia((GC_AdamMediaSlot)i);
        if (IsValidPointer(media) && media->IsInserted() && media->IsDirty())
        {
            media->ClearDirty();
            discarded = true;
        }
    }
    if (discarded)
    {
        rewind_reset();
        runahead_reset();
    }
    return true;
}

bool emu_eject_adam_media(GC_AdamMediaSlot slot)
{
    if (emu_is_busy() || (slot < GC_ADAM_MEDIA_DISK_1) ||
        (slot >= GC_ADAM_MEDIA_SLOT_COUNT) || !write_working_copy(slot))
    {
        return false;
    }
    emu_get_core()->EjectAdamMedia(slot);
    clear_host_media(slot);
    if (playlist.slot == slot)
        clear_playlist(&playlist);
    rewind_reset();
    runahead_reset();
    return true;
}

bool emu_set_adam_media_write_protected(GC_AdamMediaSlot slot, bool write_protected)
{
    if (emu_is_busy())
        return false;
    AdamMedia* media = emu_get_core()->GetAdamMedia(slot);
    if (!IsValidPointer(media) || !media->IsInserted())
        return false;
    if (host_media[slot].state_owned && !write_protected)
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
    if (emu_is_busy() || (slot < GC_ADAM_MEDIA_DISK_1) ||
        (slot >= GC_ADAM_MEDIA_SLOT_COUNT))
    {
        return false;
    }
    AdamMedia* media = emu_get_core()->GetAdamMedia(slot);
    if (!IsValidPointer(media) || !media->IsInserted())
        return true;

    info->inserted = true;
    info->write_protected = media->IsWriteProtected();
    info->dirty = media->IsDirty();
    info->state_owned = host_media[slot].state_owned;
    info->type = media->GetType();
    info->size = media->GetSize();
    info->base_crc = media->GetBaseCRC();
    strncpy_fit(info->path, host_media[slot].source_path, sizeof(info->path));
    strncpy_fit(info->working_path, host_media[slot].working_path,
        sizeof(info->working_path));
    return true;
}

void emu_reconcile_adam_media_after_state_load(void)
{
    if (emu_get_core()->GetMachine() != GC_MACHINE_ADAM)
        return;
    for (int i = 0; i < GC_ADAM_MEDIA_SLOT_COUNT; i++)
    {
        AdamMedia* media = emu_get_core()->GetAdamMedia((GC_AdamMediaSlot)i);
        AdamHostMediaRecord* record = &host_media[i];
        if (!IsValidPointer(media) || !media->IsInserted())
        {
            clear_host_media((GC_AdamMediaSlot)i);
            if (playlist.slot == (GC_AdamMediaSlot)i)
                clear_playlist(&playlist);
            continue;
        }
        if ((record->source_path[0] != '\0') &&
            (record->base_crc == media->GetBaseCRC()))
        {
            continue;
        }

        clear_host_media((GC_AdamMediaSlot)i);
        if (playlist.slot == (GC_AdamMediaSlot)i)
            clear_playlist(&playlist);
        record->state_owned = true;
        media->SetWriteProtected(true);
        media->ClearDirty();
    }
}

bool emu_save_adam_printer(const char* file_path)
{
    if (!IsValidPointer(file_path) || (file_path[0] == '\0') ||
        (emu_get_core()->GetMachine() != GC_MACHINE_ADAM))
    {
        return false;
    }
    AdamNet* adam_net = emu_get_core()->GetAdam()->GetAdamNet();
    if (!IsValidPointer(adam_net))
        return false;

    std::ofstream file;
    open_ofstream_utf8(file, file_path, std::ios::out | std::ios::binary |
        std::ios::trunc);
    if (!file.is_open())
        return false;
    int size = adam_net->GetPrinterSize();
    if (size > 0)
        file.write(reinterpret_cast<const char*>(adam_net->GetPrinterData()), size);
    file.close();
    return file.good();
}

void emu_clear_adam_printer(void)
{
    if (!emu_is_busy() && (emu_get_core()->GetMachine() == GC_MACHINE_ADAM))
        emu_get_core()->GetAdam()->GetAdamNet()->ClearPrinter();
}

void emu_adam_key_pressed(GC_AdamKey key)
{
    if (!emu_is_busy())
        emu_get_core()->AdamKeyPressed(key);
}

void emu_adam_key_released(GC_AdamKey key)
{
    if (!emu_is_busy())
        emu_get_core()->AdamKeyReleased(key);
}

void emu_adam_release_all_keys(void)
{
    if (!emu_is_busy())
        emu_get_core()->AdamReleaseAllKeys();
}

bool emu_adam_get_state_path(int index, char* path, size_t path_size)
{
    if (!IsValidPointer(path) || (path_size == 0) || (index < 1))
        return false;

    const char* content_path = emu_get_content_path();
    char directory[4096];
    if (config_emulator.savestates_dir_option == Directory_Location_ROM)
    {
        if (content_path[0] != '\0')
            get_directory(content_path, directory, sizeof(directory));
        else
            strncpy_fit(directory, config_root_path, sizeof(directory));
    }
    else
    {
        const char* configured = config_emulator.savestates_dir_option ==
            Directory_Location_Custom ? config_emulator.savestates_path.c_str() :
            config_root_path;
        strncpy_fit(directory, configured, sizeof(directory));
    }

    char name[1024];
    if (content_path[0] != '\0')
        get_filename_without_extension(content_path, name, sizeof(name));
    else
        strncpy_fit(name, "ADAM", sizeof(name));
    char filename[1100];
    snprintf(filename, sizeof(filename), "%s.state%d", name, index);
    return join_path(directory, filename, path, path_size);
}
