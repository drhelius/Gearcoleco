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

#include <string>
#include <algorithm>
#include <ctype.h>
#include "Cartridge.h"
#include "miniz/miniz.h"
#include "game_db.h"
#include "common.h"

Cartridge::Cartridge()
{
    InitPointer(m_pROM);
    InitPointer(m_pEEPROM);
    m_iROMSize = 0;
    m_Type = CartridgeNotSupported;
    m_bValidROM = false;
    m_bReady = false;
    m_szFilePath[0] = 0;
    m_szFileName[0] = 0;
    m_iROMBankCount = 0;
    m_bPAL = false;
    m_bSRAM = false;
    m_iCRC = 0;
}

Cartridge::~Cartridge()
{
    SafeDeleteArray(m_pROM);
    SafeDeleteArray(m_pEEPROM);
}

void Cartridge::Init()
{
    m_pEEPROM = new u8[0x400];
    Reset();
}

void Cartridge::Reset()
{
    SafeDeleteArray(m_pROM);
    m_iROMSize = 0;
    m_Type = CartridgeNotSupported;
    m_bValidROM = false;
    m_bReady = false;
    m_szFilePath[0] = 0;
    m_szFileName[0] = 0;
    m_iROMBankCount = 0;
    m_bPAL = false;
    m_bSRAM = false;
    m_iCRC = 0;
    for (int j = 0; j < 0x400; j++)
        m_pEEPROM[j] = 0xFF;
}

u32 Cartridge::GetCRC() const
{
    return m_iCRC;
}

bool Cartridge::IsPAL() const
{
    return m_bPAL;
}

bool Cartridge::HasSRAM() const
{
    return m_bSRAM;
}

bool Cartridge::IsValidROM() const
{
    return m_bValidROM;
}

bool Cartridge::IsReady() const
{
    return m_bReady;
}

Cartridge::CartridgeTypes Cartridge::GetType() const
{
    return m_Type;
}

void Cartridge::ForceConfig(Cartridge::ForceConfiguration config)
{
    m_iCRC = CalculateCRC32(0, m_pROM, m_iROMSize);
    GatherMetadata(m_iCRC);

    if (config.region == CartridgePAL)
    {
        Log("Forcing Region: PAL");
        m_bPAL = true;
    }
    else if (config.region == CartridgeNTSC)
    {
        Log("Forcing Region: NTSC");
        m_bPAL = false;
    }

    switch (config.type)
    {
        case Cartridge::CartridgeColecoVision:
            m_Type = config.type;
            Log("Forcing Mapper: Colecovision");
            break;
        case Cartridge::CartridgeMegaCart:
            m_Type = config.type;
            Log("Forcing Mapper: MegaCart");
            break;
        case Cartridge::CartridgeActivisionCart:
            m_Type = config.type;
            Log("Forcing Mapper: Activision");
            break;
        case Cartridge::CartridgeOCM:
            m_Type = config.type;
            Log("Forcing Mapper: OCM");
            break;
        default:
            break;
    }
}

int Cartridge::GetROMSize() const
{
    return m_iROMSize;
}

int Cartridge::GetROMBankCount() const
{
    return m_iROMBankCount;
}

const char* Cartridge::GetFilePath() const
{
    return m_szFilePath;
}

const char* Cartridge::GetFileName() const
{
    return m_szFileName;
}

u8* Cartridge::GetROM() const
{
    return m_pROM;
}

u8* Cartridge::GetEEPROM() const
{
    return m_pEEPROM;
}

bool Cartridge::LoadFromZipFile(const u8* buffer, int size)
{
    using namespace std;

    mz_zip_archive zip_archive;
    mz_bool status;
    memset(&zip_archive, 0, sizeof (zip_archive));

    status = mz_zip_reader_init_mem(&zip_archive, (void*) buffer, size, 0);
    if (!status)
    {
        Log("mz_zip_reader_init_mem() failed!");
        return false;
    }

    for (unsigned int i = 0; i < mz_zip_reader_get_num_files(&zip_archive); i++)
    {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
        {
            Log("mz_zip_reader_file_stat() failed!");
            mz_zip_reader_end(&zip_archive);
            return false;
        }

        Debug("ZIP Content - Filename: \"%s\", Comment: \"%s\", Uncompressed size: %u, Compressed size: %u", file_stat.m_filename, file_stat.m_comment, (unsigned int) file_stat.m_uncomp_size, (unsigned int) file_stat.m_comp_size);

        string fn((const char*) file_stat.m_filename);
        transform(fn.begin(), fn.end(), fn.begin(), (int(*)(int)) tolower);
        string extension = fn.substr(fn.find_last_of(".") + 1);

        if ((extension == "col") || (extension == "cv") || (extension == "rom") || (extension == "bin"))
        {
            void *p;
            size_t uncomp_size;

            p = mz_zip_reader_extract_file_to_heap(&zip_archive, file_stat.m_filename, &uncomp_size, 0);
            if (!p)
            {
                Log("mz_zip_reader_extract_file_to_heap() failed!");
                mz_zip_reader_end(&zip_archive);
                return false;
            }

            bool ok = LoadFromBuffer((const u8*) p, (int)uncomp_size);

            free(p);
            mz_zip_reader_end(&zip_archive);

            return ok;
        }
    }
    return false;
}

bool Cartridge::LoadFromFile(const char* path)
{
    using namespace std;

    Log("Loading %s...", path);

    Reset();

    strcpy(m_szFilePath, path);

    std::string pathstr(path);
    std::string filename;

    size_t pos = pathstr.find_last_of("\\");
    if (pos != std::string::npos)
    {
        filename.assign(pathstr.begin() + pos + 1, pathstr.end());
    }
    else
    {
        pos = pathstr.find_last_of("/");
        if (pos != std::string::npos)
        {
            filename.assign(pathstr.begin() + pos + 1, pathstr.end());
        }
        else
        {
            filename = pathstr;
        }
    }

    strcpy(m_szFileName, filename.c_str());

    ifstream file;
    open_ifstream_utf8(file, path, ios::in | ios::binary | ios::ate);

    if (file.is_open())
    {
        int size = static_cast<int> (file.tellg());
        char* memblock = new char[size];
        file.seekg(0, ios::beg);
        file.read(memblock, size);
        file.close();

        string fn(path);
        transform(fn.begin(), fn.end(), fn.begin(), (int(*)(int)) tolower);
        string extension = fn.substr(fn.find_last_of(".") + 1);

        if (extension == "zip")
        {
            Debug("Loading from ZIP...");
            m_bReady = LoadFromZipFile(reinterpret_cast<u8*> (memblock), size);
        }
        else
        {
            m_bReady = LoadFromBuffer(reinterpret_cast<u8*> (memblock), size);
        }

        if (m_bReady)
        {
            Debug("ROM loaded", path);
        }
        else
        {
            Log("There was a problem loading the memory for file %s...", path);
        }

        SafeDeleteArray(memblock);
    }
    else
    {
        Log("There was a problem loading the file %s...", path);
        m_bReady = false;
    }

    if (!m_bReady)
    {
        Reset();
    }

    return m_bReady;
}

bool Cartridge::LoadFromBuffer(const u8* buffer, int size)
{
    if (IsValidPointer(buffer))
    {
        Log("Loading from buffer... Size: %d", size);

        // Unkown size
        if ((size % 1024) != 0)
        {
            Log("Invalid size found. %d bytes", size);
            //return false;
        }

        m_iROMSize = size;
        m_pROM = new u8[m_iROMSize];
        memcpy(m_pROM, buffer, m_iROMSize);

        m_bReady = true;

        m_iCRC = CalculateCRC32(0, m_pROM, m_iROMSize);

        GatherMetadata(m_iCRC);

        return true;
    }
    else
        return false;
}

bool Cartridge::GatherMetadata(u32 crc)
{
    m_bPAL = false;
    m_bSRAM = false;

    Log("ROM Size: %d KB", m_iROMSize / 1024);

    m_iROMBankCount = (m_iROMSize / 0x4000) + (m_iROMSize % 0x4000 ? 1 : 0);

    Log("ROM Bank Count: %d", m_iROMBankCount);

    m_Type = Cartridge::CartridgeNotSupported;

    int headerOffset = 0;
    u16 header = m_pROM[headerOffset + 1] | (m_pROM[headerOffset + 0] << 8);
    m_bValidROM = (header == 0xAA55) || (header == 0x55AA);

    if (header == 0x6699)
    {
        Log("Cartridge is a Colec Adam expansion ROM. Header: %X", header);
    }

    if (m_bValidROM && (m_iROMSize <= 0x8000))
    {
        m_Type = Cartridge::CartridgeColecoVision;
        Log("Cartridge is Colecovision. ROM size: %d bytes.", m_iROMSize);
    }
    else if (m_bValidROM && (m_iROMSize > 0x8000))
    {
        m_Type = Cartridge::CartridgeActivisionCart;
        Log("Cartridge is Activision Cart. ROM size: %d bytes. Banks %d.", m_iROMSize, m_iROMBankCount);
    }
    else if (!m_bValidROM && (m_iROMSize > 0x8000))
    {
        headerOffset = m_iROMSize - 0x4000;
        header = m_pROM[headerOffset + 1] | (m_pROM[headerOffset + 0] << 8);
        m_bValidROM = (header == 0xAA55) || (header == 0x55AA);

        if (m_bValidROM)
        {
            m_Type = Cartridge::CartridgeMegaCart;
            Log("Cartridge is Mega Cart. ROM size: %d bytes. Banks %d.", m_iROMSize, m_iROMBankCount);
        }
    }
    else
    {
        m_Type = Cartridge::CartridgeNotSupported;
        Log("ROM is NOT Valid. No header found.");
    }

    GetInfoFromDB(crc);

    switch (m_Type)
    {
        case Cartridge::CartridgeColecoVision:
            Log("ColecoVision mapper found");
            break;
        case Cartridge::CartridgeMegaCart:
            Log("MegaCart mapper found");
            break;
        case Cartridge::CartridgeActivisionCart:
            Log("Activision mapper found");
            break;
        case Cartridge::CartridgeOCM:
            Log("OCM mapper found");
            break;
        case Cartridge::CartridgeNotSupported:
            Log("Cartridge not supported!!");
            break;
        default:
            Log("ERROR with cartridge type!!");
            break;
    }

    return (m_Type != CartridgeNotSupported);
}

void Cartridge::GetInfoFromDB(u32 crc)
{
    int i = 0;
    bool found = false;

    while(!found && (kGameDatabase[i].title != 0))
    {
        u32 db_crc = kGameDatabase[i].crc;

        if (db_crc == crc)
        {
            found = true;

            Log("ROM found in database: %s. CRC: %X", kGameDatabase[i].title, crc);

            if (kGameDatabase[i].mode & GC_GameDBMode_SRAM)
            {
                Log("Cartridge with SRAM");
                m_bSRAM = true;
            }

            if (kGameDatabase[i].mode & GC_GameDBMode_OCM)
            {
                Log("Cartridge with OCM mapper");
                m_Type = CartridgeOCM;
                for (int j = 0; j < 0x400; j++)
                    m_pEEPROM[j] = 0xFF;
            }
        }
        else
            i++;
    }

    if (!found)
    {
        Debug("ROM not found in database. CRC: %X", crc);
    }
}
