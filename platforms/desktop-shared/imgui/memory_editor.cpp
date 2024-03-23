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

void GearMemoryEditor::DrawAdvanced(uint8_t* mem_data, int mem_size, int base_display_addr)
{
    int m_bytesPerRow = 16;
    int bytesPerCell = 1;
    int columnCount = m_bytesPerRow / bytesPerCell;
    int separatorCount = (columnCount - 1) / 4;
    int byteColumnCount = 2 + columnCount + separatorCount + 2;
    float SeparatorColumWidth = 8.0f;
    bool m_upperCaseHex = true;
    ImVec2 CharacterSize = ImGui::CalcTextSize("0");
    int m_byteCellPadding = 0;
    int m_characterCellPadding = 0;
    int total_rows = (mem_size + (columnCount - 1)) / columnCount;
    int maxCharsPerCell = 2;

    char buf[16];

    if (ImGui::BeginChild("##mem", ImVec2(ImGui::GetContentRegionAvail().x, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.5, 0));

        if (ImGui::BeginTable("##hex", byteColumnCount, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoKeepColumnsVisible | ImGuiTableFlags_RowBg | /*ImGuiTableFlags_Borders |*/ ImGuiTableFlags_ScrollY))
        {

            ImGui::TableSetupScrollFreeze(0, 1);

            // Row address column
            ImGui::TableSetupColumn("ADDR");
            ImGui::TableSetupColumn("");

            // Byte columns
            for (int i = 0; i < columnCount; i++) {
                if (IsColumnSeparator(i, columnCount))
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, SeparatorColumWidth);

                sprintf(buf, "%02X", i * bytesPerCell);

                ImGui::TableSetupColumn(buf, ImGuiTableColumnFlags_WidthFixed, CharacterSize.x * maxCharsPerCell + (6 + m_byteCellPadding) * 1);
            }

            // ASCII column
            ImGui::TableSetupColumn("  ");
            ImGui::TableSetupColumn("ASCII", ImGuiTableColumnFlags_WidthFixed, (CharacterSize.x + m_characterCellPadding * 1) * m_bytesPerRow);

            ImGui::TableNextRow();
            for (int i = 0; i < ImGui::TableGetColumnCount(); i++) {
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(ImGui::TableGetColumnName(i));
                ImGui::Dummy(ImVec2(0, CharacterSize.y / 2));
            }

            ImGuiListClipper clipper;
            clipper.Begin(total_rows);

            while (clipper.Step())
            {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                {
                    ImGui::TableNextRow();
                    int address = (row * columnCount) + base_display_addr;

                    // Draw address column
                    ImGui::TableNextColumn();
                    ImGui::Text("%04X:  ", address);
                    ImGui::TableNextColumn();

                    // Draw byte columns
                    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2.75f, 0.0f));
                    for (int x = 0; x < columnCount; x++)
                    {
                        int byte_address = address + (x * bytesPerCell);

                        ImGui::TableNextColumn();
                        if (IsColumnSeparator(x, columnCount))
                            ImGui::TableNextColumn();

                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                        ImGui::PushItemWidth((CharacterSize).x);
                        ImGui::Text("%02X", mem_data[byte_address]);
                        ImGui::PopItemWidth();
                        ImGui::PopStyleVar();

                    }
                    ImGui::PopStyleVar();

                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();

                    // Draw ASCII column
                    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));
                    // for (int x = 0; x < columnCount; x++)
                    // {
                    //     int byte_address = address + (x * bytesPerCell);
                    //     unsigned char c = mem_data[byte_address];
                    //     ImGui::SameLine(0.0f, 0.0f);
                    //     ImGui::Text("%c", (c >= 32 && c < 128) ? c : '.');
                    // }
                    if (ImGui::BeginTable("##ascii_column", m_bytesPerRow))
                    {
                        for (int x = 0; x < m_bytesPerRow; x++)
                        {
                            sprintf(buf, "##ascii_cell%d", x);
                            ImGui::TableSetupColumn(buf, ImGuiTableColumnFlags_WidthFixed, CharacterSize.x + m_characterCellPadding * 1);
                        }

                        ImGui::TableNextRow();

                        for (int x = 0; x < m_bytesPerRow; x++)
                        {
                            ImGui::TableNextColumn();

                            //int byteAddress = (row * m_bytesPerRow) + x + base_display_addr;


                            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (m_characterCellPadding * 1) / 2);
                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                            ImGui::PushItemWidth(CharacterSize.x);

                            int byte_address = address + (x * bytesPerCell);
                            unsigned char c = mem_data[byte_address];

                            ImGui::Text("%c", (c >= 32 && c < 128) ? c : '.');


                            ImGui::PopItemWidth();
                            ImGui::PopStyleVar();
                            
                        }

                        ImGui::EndTable();
                    }
                    ImGui::PopStyleVar();
                }
            }

            ImGui::EndTable();
            ImGui::PopStyleVar();
        }

    }
    ImGui::EndChild();
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

bool GearMemoryEditor::IsColumnSeparator(int current_column, int column_count)
{
    return (current_column > 0) && (current_column < column_count) && ((current_column % 4) == 0);
}
