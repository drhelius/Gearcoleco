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

#ifndef ADAM_MEDIA_H
#define ADAM_MEDIA_H

#include "definitions.h"
#include <iostream>

class AdamMedia
{
public:
    static const size_t kDataPackSize = 0x40000;
    static const size_t kDisk160KSize = 0x28000;
    static const size_t kDisk320KSize = 0x50000;
    static const size_t kBlockSize = 0x400;
    static const size_t kSectorSize = 0x200;

    AdamMedia();
    ~AdamMedia();
    GC_AdamMediaError Insert(GC_AdamMediaType type, const u8* data, size_t size, bool write_protected, u32 base_crc = 0);
    void Eject();
    GC_AdamMediaError ReadBlock(u32 block, u8* data, size_t length);
    GC_AdamMediaError WriteBlock(u32 block, const u8* data, size_t length);
    void SetWriteProtected(bool write_protected);
    bool IsInserted() const;
    bool IsWriteProtected() const;
    bool IsDirty() const;
    void ClearDirty();
    void MarkDirty();
    GC_AdamMediaType GetType() const;
    size_t GetSize() const;
    u32 GetBlockCount() const;
    u32 GetBaseCRC() const;
    u32 GetGeneration() const;
    u32 GetPosition() const;
    const u8* GetData() const;
    u8* GetData();
    void SaveState(std::ostream& stream) const;
    bool LoadState(std::istream& stream, bool load_image = true);
    static bool IsValidImageSize(GC_AdamMediaType type, size_t size);
    static bool ExtractFromZip(const u8* archive_data, size_t archive_size, u8** media_data,
        size_t* media_size, GC_AdamMediaType* media_type, char* media_name = NULL, size_t media_name_size = 0);

private:
    static u32 CalculateAdamMediaCRC32(const u8* data, size_t size);
    static bool AdamMediaNameEndsWith(const char* name, const char* suffix);
    size_t GetByteOffset(u32 block, size_t byte_in_block) const;

private:
    u8* m_pData;
    size_t m_Size;
    GC_AdamMediaType m_Type;
    u32 m_BaseCRC;
    u32 m_Generation;
    u32 m_Position;
    bool m_Inserted;
    bool m_WriteProtected;
    bool m_Dirty;
};

#endif /* ADAM_MEDIA_H */
