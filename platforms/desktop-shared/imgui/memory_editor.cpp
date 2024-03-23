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
    m_selected_address = -1;
}

GearMemoryEditor::~GearMemoryEditor()
{

}

void GearMemoryEditor::Draw(uint8_t* mem_data, int mem_size, int base_display_addr)
{

    int columns = 8;

    ImGui::BeginChild("##mem", ImVec2(ImGui::GetContentRegionAvail().x, 0), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav);

    int total_rows = (mem_size + (columns - 1)) / columns;
    int total_columns = columns + 2;
    static ImGuiTableFlags flags = ImGuiTableFlags_NoKeepColumnsVisible | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_SizingFixedFit;
    static ImVec2 cell_padding(0.0f, 0.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, cell_padding);

    if (ImGui::BeginTable("mem", total_columns, flags))
    {
        char buf[3];
        ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
        ImGuiListClipper clipper;
        clipper.Begin(total_rows);

        bool next = false;

        while (clipper.Step())
        {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
            {
                int addr = row * columns;

                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                ImGui::Text("%04X:  ", addr + base_display_addr);

                for (int n = 0; n < columns && addr < mem_size; n++, addr++)
                {
                    ImGui::TableNextColumn();
                    
                    sprintf(buf, "%02X", mem_data[addr]);

                    ImGui::PushID(addr);
                    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_AlwaysOverwrite;
                    ImGui::SetNextItemWidth(20);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

                    if (next)
                    {
                        ImGui::SetKeyboardFocusHere();
                        next = false;
                    }

                    if (ImGui::InputText("##cell", buf, IM_ARRAYSIZE(buf), flags))
                    {
                        unsigned int data_input_value = 0;
                        if (sscanf(buf, "%X", &data_input_value) == 1)
                        {
                            mem_data[addr] = (uint8_t)data_input_value;
                        }
                        next = true;
                    }
                    ImGui::PopStyleVar();
                    ImGui::PopID();
                }

                ImGui::TableNextColumn();
                addr = row * columns;
                for (int n = 0; n < columns && addr < mem_size; n++, addr++)
                {
                    unsigned char c = mem_data[addr];
                    if (n == 0)
                        ImGui::Text("   ");

                    ImGui::SameLine(0.0f, 0.0f);
                    ImGui::Text("%c", (c >= 32 && c < 128) ? c : '.');
                }
            }
        }

        ImGui::PopStyleColor();
        ImGui::EndTable();
    }

    ImGui::PopStyleVar();

    ImGui::EndChild();
}