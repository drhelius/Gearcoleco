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

#ifndef GEAR_MEMORY_EDITOR_H
#define	GEAR_MEMORY_EDITOR_H

#include <stdint.h>
#include <stdio.h>
#include "imgui.h"

class GearMemoryEditor
{
public:
    GearMemoryEditor();
    ~GearMemoryEditor();

    void Draw(uint8_t* mem_data, int mem_size, int base_display_addr = 0x0000);

private:
    bool IsColumnSeparator(int current_column, int column_count);
    void DrawSelectionFrame(int x, int y, int address, const ImVec2 &cellPos, const ImVec2 &cellSize);
    void HandleSelection(int address);

private:
    int m_bytes_per_row;
    float m_separator_column_width;
    bool m_uppercase_hex;
    int m_editing_address;
    bool m_set_keyboard_here;
    int m_selection_start;
    int m_selection_end;
};

#endif	/* GEAR_MEMORY_EDITOR_H */