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

#include "memory_editor.h"
#include "imgui.h"

GearMemoryEditor::GearMemoryEditor()
{

}

GearMemoryEditor::~GearMemoryEditor()
{

}

void GearMemoryEditor::Draw(uint8_t* mem_data, int mem_size, int base_display_addr)
{

    int columns = 8;

    ImGui::BeginChild("##mem", ImVec2(ImGui::GetContentRegionAvail().x, 0), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // ImDrawList* draw_list = ImGui::GetWindowDrawList();
    // ImVec2 line_start = ImGui::GetCursorScreenPos();
    // ImVec2 line_end = ImVec2(line_start.x + ImGui::GetWindowWidth(), line_start.y);

    float line_height = ImGui::GetTextLineHeight();
    int line_total_count = (mem_size + (columns - 1)) / columns;


    ImGuiListClipper clipper;
    clipper.Begin(line_total_count, line_height);


    ImVec2 window_pos = ImGui::GetWindowPos();

    draw_list->AddLine(ImVec2(window_pos.x + 50, window_pos.y), ImVec2(window_pos.x + 50, window_pos.y + 9999), ImGui::GetColorU32(ImGuiCol_Border));
    draw_list->AddLine(ImVec2(window_pos.x + 300, window_pos.y), ImVec2(window_pos.x + 300, window_pos.y + 9999), ImGui::GetColorU32(ImGuiCol_Border));

    ImGui::Columns(columns + 2, "mycolumns3", false);  // 3-ways, no border
    ImGui::Separator();

    while (clipper.Step())
    {
        for (int line_i = clipper.DisplayStart; line_i < clipper.DisplayEnd; line_i++)
        {
            int addr = line_i * columns;
            ImGui::Text("%04X ", addr + base_display_addr);
            ImGui::NextColumn();

            // Draw Hexadecimal
            for (int n = 0; n < columns && addr < mem_size; n++, addr++)
            {
                //ImGui::SameLine();
                //ImGui::SameLine(32 + n * 3 * ImGui::CalcTextSize("FF ").x);
                ImGui::Text("%02X", mem_data[addr]);
                ImGui::NextColumn();
            }

            // Draw ASCII
            addr = line_i * columns;
            //ImGui::SameLine(32 + 3 * 16 * ImGui::CalcTextSize("FF ").x + ImGui::CalcTextSize(" ").x);
            //ImGui::SameLine();
            for (int n = 0; n < columns && addr < mem_size; n++, addr++)
            {
                unsigned char c = mem_data[addr];
                //ImGui::SameLine();
                ImGui::Text("%c", (c >= 32 && c < 128) ? c : '.');
            }
            ImGui::NextColumn();
        }
    }

    ImGui::Columns(1);


    ImGui::EndChild();



}