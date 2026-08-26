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

#ifndef CARTRIDGE_H
#define	CARTRIDGE_H

#include <list>
#include "definitions.h"
#include "log.h"

class Cartridge
{
public:
    enum CartridgeTypes
    {
        CartridgeColecoVision,
        CartridgeMegaCart,
        CartridgeActivisionCart,
        CartridgeOCM,
        CartridgeNotSupported
    };

    enum CartridgeRegions
    {
        CartridgeNTSC,
        CartridgePAL,
        CartridgeUnknownRegion
    };

    struct ForceConfiguration
    {
        CartridgeTypes type;
        CartridgeRegions region;
    };

    u8* GetEEPROM() const;

public:
    Cartridge();
    ~Cartridge();
    void Init();
    void Reset();
    u32 GetCRC() const;
    bool IsPAL() const;
    bool IsF18ARequired() const;
    bool HasSRAM() const
    {
        return m_bSRAM;
    }
    bool IsValidROM() const;
    bool IsReady() const;
    bool IsInGameDatabase() const;
    const char* GetGameDatabaseName() const;
    CartridgeTypes GetType() const;
    void ForceConfig(ForceConfiguration config);
    int GetROMSize() const
    {
        return m_iROMSize;
    }
    int GetROMBankCount() const
    {
        return m_iROMBankCount;
    }
    const char* GetFilePath() const;
    const char* GetFileName() const;
    const char* GetFileDirectory() const;
    u8* GetROM() const
    {
        return m_pROM;
    }
    bool LoadFromFile(const char* path, bool softpatching = false);
    bool LoadFromBuffer(const u8* buffer, int size);
    bool IsSoftpatchApplied() const;
    const char* GetSoftpatchPath() const;

private:
    bool GatherMetadata(u32 crc);
    void GetInfoFromDB(u32 crc);
    bool LoadFromZipFile(const u8* buffer, int size, bool softpatching);
    bool LoadFromBufferWithSoftpatch(const u8* buffer, int size, bool softpatching);

private:
    u8* m_pROM;
    int m_iROMSize;
    CartridgeTypes m_Type;
    bool m_bValidROM;
    bool m_bReady;
    bool m_bInGameDatabase;
    const char* m_pGameDatabaseName;
    char m_szFilePath[512];
    char m_szFileName[512];
    char m_szFileDirectory[512];
    int m_iROMBankCount;
    bool m_bPAL;
    bool m_bF18ARequired;
    u32 m_iCRC;
    bool m_bSRAM;
    u8* m_pEEPROM;
    bool m_softpatch_applied;
    char m_softpatch_path[4096];
};

#endif	/* CARTRIDGE_H */
