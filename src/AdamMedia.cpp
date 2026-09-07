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
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#include <string.h>
#include "AdamMedia.h"
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include "miniz.h"
#undef MINIZ_NO_ZLIB_COMPATIBLE_NAMES

static u32 CalculateAdamMediaCRC32(const u8* data, size_t size)
{
    u32 crc = ~0U;

    while (size-- > 0)
    {
        crc ^= *data++;

        for (int bit = 0; bit < 8; bit++)
        {
            u32 mask = 0U - (crc & 1U);
            crc = (crc >> 1) ^ (0xEDB88320U & mask);
        }
    }

    return crc ^ ~0U;
}

static bool AdamMediaNameEndsWith(const char* name, const char* suffix)
{
    if (!IsValidPointer(name) || !IsValidPointer(suffix))
        return false;

    size_t name_length = strlen(name);
    size_t suffix_length = strlen(suffix);
    if (suffix_length > name_length)
        return false;

    const char* start = name + name_length - suffix_length;
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

bool AdamMedia::IsValidImageSize(GC_AdamMediaType type, size_t size)
{
    if (type == GC_ADAM_MEDIA_DATA_PACK)
        return size == kDataPackSize;

    if (type == GC_ADAM_MEDIA_DISK)
        return (size == kDisk160KSize) || (size == kDisk320KSize);

    return false;
}

bool AdamMedia::ExtractFromZip(const u8* archive_data, size_t archive_size, u8** media_data,
    size_t* media_size, GC_AdamMediaType* media_type, char* media_name, size_t media_name_size)
{
    if (!IsValidPointer(media_data) || !IsValidPointer(media_size) ||
        !IsValidPointer(media_type))
    {
        return false;
    }

    *media_data = NULL;
    *media_size = 0;
    *media_type = GC_ADAM_MEDIA_NONE;
    if (IsValidPointer(media_name) && (media_name_size > 0))
        media_name[0] = '\0';

    if (!IsValidPointer(archive_data) || (archive_size == 0))
        return false;

    mz_zip_archive archive;
    memset(&archive, 0, sizeof(archive));
    if (!mz_zip_reader_init_mem(&archive, archive_data, archive_size, 0))
        return false;

    int candidate = -1;
    GC_AdamMediaType candidate_type = GC_ADAM_MEDIA_NONE;
    size_t candidate_size = 0;
    char candidate_name[MZ_ZIP_MAX_ARCHIVE_FILENAME_SIZE];
    candidate_name[0] = '\0';

    mz_uint files = mz_zip_reader_get_num_files(&archive);
    for (mz_uint i = 0; i < files; i++)
    {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&archive, i, &file_stat))
        {
            mz_zip_reader_end(&archive);
            return false;
        }

        if (file_stat.m_is_directory)
            continue;

        GC_AdamMediaType type = GC_ADAM_MEDIA_NONE;
        if (AdamMediaNameEndsWith(file_stat.m_filename, ".ddp"))
            type = GC_ADAM_MEDIA_DATA_PACK;
        else if (AdamMediaNameEndsWith(file_stat.m_filename, ".dsk"))
            type = GC_ADAM_MEDIA_DISK;
        else
            continue;

        if (!file_stat.m_is_supported ||
            (file_stat.m_uncomp_size > (mz_uint64)(size_t)-1) ||
            !IsValidImageSize(type, (size_t)file_stat.m_uncomp_size) || (candidate >= 0))
        {
            mz_zip_reader_end(&archive);
            return false;
        }

        candidate = (int)i;
        candidate_type = type;
        candidate_size = (size_t)file_stat.m_uncomp_size;
        strncpy(candidate_name, file_stat.m_filename, sizeof(candidate_name) - 1);
        candidate_name[sizeof(candidate_name) - 1] = '\0';
    }

    if (candidate < 0)
    {
        mz_zip_reader_end(&archive);
        return false;
    }

    u8* extracted = new u8[candidate_size];
    bool success = mz_zip_reader_extract_to_mem(&archive, (mz_uint)candidate, extracted,
        candidate_size, 0) != 0;
    mz_zip_reader_end(&archive);
    if (!success)
    {
        SafeDeleteArray(extracted);
        return false;
    }

    *media_data = extracted;
    *media_size = candidate_size;
    *media_type = candidate_type;
    if (IsValidPointer(media_name) && (media_name_size > 0))
    {
        strncpy(media_name, candidate_name, media_name_size - 1);
        media_name[media_name_size - 1] = '\0';
    }
    return true;
}

AdamMedia::AdamMedia()
{
    InitPointer(m_pData);
    m_Size = 0;
    m_Type = GC_ADAM_MEDIA_NONE;
    m_BaseCRC = 0;
    m_Generation = 0;
    m_Position = 0;
    m_Inserted = false;
    m_WriteProtected = false;
    m_Dirty = false;
}

AdamMedia::~AdamMedia()
{
    SafeDeleteArray(m_pData);
}

GC_AdamMediaError AdamMedia::Insert(GC_AdamMediaType type, const u8* data, size_t size,
    bool write_protected, u32 base_crc)
{
    if (!IsValidPointer(data) || (type == GC_ADAM_MEDIA_NONE))
        return GC_ADAM_MEDIA_ERROR_INVALID_ARGUMENT;

    if (!IsValidImageSize(type, size))
        return GC_ADAM_MEDIA_ERROR_INVALID_SIZE;

    u8* image = new u8[size];
    memcpy(image, data, size);

    SafeDeleteArray(m_pData);
    m_pData = image;
    m_Size = size;
    m_Type = type;
    m_BaseCRC = base_crc ? base_crc : CalculateAdamMediaCRC32(data, size);
    m_Generation++;
    m_Position = 0;
    m_Inserted = true;
    m_WriteProtected = write_protected;
    m_Dirty = false;
    return GC_ADAM_MEDIA_ERROR_NONE;
}

void AdamMedia::Eject()
{
    SafeDeleteArray(m_pData);
    m_Size = 0;
    m_Type = GC_ADAM_MEDIA_NONE;
    m_BaseCRC = 0;
    m_Generation++;
    m_Position = 0;
    m_Inserted = false;
    m_WriteProtected = false;
    m_Dirty = false;
}

size_t AdamMedia::GetByteOffset(u32 block, size_t byte_in_block) const
{
    if (m_Type == GC_ADAM_MEDIA_DATA_PACK)
        return (static_cast<size_t>(block) * kBlockSize) + byte_in_block;

    static const u8 disk_interleave[8] = { 0, 5, 2, 7, 4, 1, 6, 3 };
    size_t logical_sector = (static_cast<size_t>(block) * 2) + (byte_in_block / kSectorSize);
    size_t physical_sector = (logical_sector & ~static_cast<size_t>(7)) |
        disk_interleave[logical_sector & 7];
    return (physical_sector * kSectorSize) + (byte_in_block & (kSectorSize - 1));
}

GC_AdamMediaError AdamMedia::ReadBlock(u32 block, u8* data, size_t length)
{
    if (!m_Inserted || !IsValidPointer(m_pData))
        return GC_ADAM_MEDIA_ERROR_NO_MEDIA;
    if (!IsValidPointer(data) && (length > 0))
        return GC_ADAM_MEDIA_ERROR_INVALID_ARGUMENT;
    if ((length > kBlockSize) || (block >= GetBlockCount()))
        return GC_ADAM_MEDIA_ERROR_OUT_OF_RANGE;

    for (size_t i = 0; i < length; i++)
    {
        size_t offset = GetByteOffset(block, i);
        if (offset >= m_Size)
            return GC_ADAM_MEDIA_ERROR_OUT_OF_RANGE;
        data[i] = m_pData[offset];
    }

    m_Position = block;
    return GC_ADAM_MEDIA_ERROR_NONE;
}

GC_AdamMediaError AdamMedia::WriteBlock(u32 block, const u8* data, size_t length)
{
    if (!m_Inserted || !IsValidPointer(m_pData))
        return GC_ADAM_MEDIA_ERROR_NO_MEDIA;
    if (m_WriteProtected)
        return GC_ADAM_MEDIA_ERROR_WRITE_PROTECTED;
    if (!IsValidPointer(data) && (length > 0))
        return GC_ADAM_MEDIA_ERROR_INVALID_ARGUMENT;
    if ((length > kBlockSize) || (block >= GetBlockCount()))
        return GC_ADAM_MEDIA_ERROR_OUT_OF_RANGE;

    for (size_t i = 0; i < length; i++)
    {
        size_t offset = GetByteOffset(block, i);
        if (offset >= m_Size)
            return GC_ADAM_MEDIA_ERROR_OUT_OF_RANGE;
        m_pData[offset] = data[i];
    }

    if (length > 0)
        m_Dirty = true;
    m_Position = block;
    return GC_ADAM_MEDIA_ERROR_NONE;
}

void AdamMedia::SetWriteProtected(bool write_protected)
{
    m_WriteProtected = write_protected;
}

bool AdamMedia::IsInserted() const
{
    return m_Inserted;
}

bool AdamMedia::IsWriteProtected() const
{
    return m_WriteProtected;
}

bool AdamMedia::IsDirty() const
{
    return m_Dirty;
}

void AdamMedia::ClearDirty()
{
    m_Dirty = false;
}

GC_AdamMediaType AdamMedia::GetType() const
{
    return m_Type;
}

size_t AdamMedia::GetSize() const
{
    return m_Size;
}

u32 AdamMedia::GetBlockCount() const
{
    return static_cast<u32>(m_Size / kBlockSize);
}

u32 AdamMedia::GetBaseCRC() const
{
    return m_BaseCRC;
}

u32 AdamMedia::GetGeneration() const
{
    return m_Generation;
}

u32 AdamMedia::GetPosition() const
{
    return m_Position;
}

const u8* AdamMedia::GetData() const
{
    return m_pData;
}

u8* AdamMedia::GetData()
{
    return m_pData;
}

void AdamMedia::SaveState(std::ostream& stream) const
{
    u8 type = (u8)m_Type;
    u8 flags = 0;
    if (m_Inserted)
        flags |= 0x01;
    if (m_WriteProtected)
        flags |= 0x02;
    if (m_Dirty)
        flags |= 0x04;
    u32 size = static_cast<u32>(m_Size);

    stream.write(reinterpret_cast<const char*>(&type), sizeof(type));
    stream.write(reinterpret_cast<const char*>(&flags), sizeof(flags));
    stream.write(reinterpret_cast<const char*>(&size), sizeof(size));
    stream.write(reinterpret_cast<const char*>(&m_BaseCRC), sizeof(m_BaseCRC));
    stream.write(reinterpret_cast<const char*>(&m_Generation), sizeof(m_Generation));
    stream.write(reinterpret_cast<const char*>(&m_Position), sizeof(m_Position));

    if (m_Inserted && (size > 0))
        stream.write(reinterpret_cast<const char*>(m_pData), size);
}

bool AdamMedia::LoadState(std::istream& stream, bool load_image)
{
    u8 type = 0;
    u8 flags = 0;
    u32 size = 0;
    u32 base_crc = 0;
    u32 generation = 0;
    u32 position = 0;

    stream.read(reinterpret_cast<char*>(&type), sizeof(type));
    stream.read(reinterpret_cast<char*>(&flags), sizeof(flags));
    stream.read(reinterpret_cast<char*>(&size), sizeof(size));
    stream.read(reinterpret_cast<char*>(&base_crc), sizeof(base_crc));
    stream.read(reinterpret_cast<char*>(&generation), sizeof(generation));
    stream.read(reinterpret_cast<char*>(&position), sizeof(position));

    if (!stream.good() || (flags & ~0x07) || (type > GC_ADAM_MEDIA_DISK))
        return false;

    bool inserted = (flags & 0x01) != 0;
    GC_AdamMediaType media_type = (GC_AdamMediaType)type;

    if (inserted)
    {
        if (!IsValidImageSize(media_type, size) || (position >= (size / kBlockSize)))
            return false;
    }
    else if ((media_type != GC_ADAM_MEDIA_NONE) || (size != 0))
    {
        return false;
    }

    if (inserted && m_Inserted && ((m_Type != media_type) ||
        (m_Size != size) || (m_BaseCRC != base_crc)))
        return false;

    // Validate the image extent without allocating or copying its contents.
    std::streampos image_start = stream.tellg();
    if (inserted)
    {
        stream.seekg(size - 1, std::ios::cur);
        char last_byte;
        stream.read(&last_byte, 1);
        if (!stream.good())
            return false;
    }

    if (load_image)
    {
        if (size != m_Size)
        {
            SafeDeleteArray(m_pData);
            if (inserted)
                m_pData = new u8[size];
        }
        if (inserted)
        {
            stream.seekg(image_start);
            stream.read(reinterpret_cast<char*>(m_pData), size);
            if (!stream.good())
                return false;
        }
    }

    m_Size = size;
    m_Type = media_type;
    m_BaseCRC = base_crc;
    m_Generation = generation;
    m_Position = position;
    m_Inserted = inserted;
    m_WriteProtected = (flags & 0x02) != 0;
    m_Dirty = (flags & 0x04) != 0;
    return true;
}
