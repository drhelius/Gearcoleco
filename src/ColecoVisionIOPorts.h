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

#ifndef COLECOVISIONIOPORTS_H
#define	COLECOVISIONIOPORTS_H

#include "IOPorts.h"

class Audio;
class Video;
class Input;
class Cartridge;
class Memory;

class ColecoVisionIOPorts : public IOPorts
{
public:
    ColecoVisionIOPorts(Audio* pAudio, Video* pVideo, Input* pInput, Cartridge* pCartridge, Memory* pMemory);
    ~ColecoVisionIOPorts();
    void Reset();
    u8 DoInput(u8 port);
    void DoOutput(u8 port, u8 value);
    void SaveState(std::ostream& stream);
    void LoadState(std::istream& stream);
private:
    Audio* m_pAudio;
    Video* m_pVideo;
    Input* m_pInput;
    Cartridge* m_pCartridge;
    Memory* m_pMemory;
};

#include "Video.h"
#include "Audio.h"
#include "Input.h"
#include "Cartridge.h"
#include "Memory.h"

inline u8 ColecoVisionIOPorts::DoInput(u8 port)
{
    Log("--> ** Attempting to read from port $%X", port);
    return 0xFF;
}

inline void ColecoVisionIOPorts::DoOutput(u8 port, u8 value)
{
    Log("--> ** Output to port $%X: %X", port, value);
}

#endif	/* COLECOVISIONIOPORTS_H */
