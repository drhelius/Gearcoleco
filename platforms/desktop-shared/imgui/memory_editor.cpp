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

GearMemoryEditor::GearMemoryEditor()
{
    m_editing_address = -1;
    m_set_keyboard_here = false;
    m_selection_start = 1;
    m_selection_end = 256;
    m_bytes_per_row = 16;
    m_separator_column_width = 8.0f;;
    m_uppercase_hex = true;
}

GearMemoryEditor::~GearMemoryEditor()
{

}

void GearMemoryEditor::Draw(uint8_t* mem_data, int mem_size, int base_display_addr)
{
    int total_rows = (mem_size + (m_bytes_per_row - 1)) / m_bytes_per_row;
    int separator_count = (m_bytes_per_row - 1) / 4;
    int byte_column_count = 2 + m_bytes_per_row + separator_count + 2;
    int byte_cell_padding = 0;
    int character_cell_padding = 0;
    int max_chars_per_cell = 2;
    ImVec2 character_size = ImGui::CalcTextSize("0");
    char buf[16];

    if (ImGui::BeginChild("##mem", ImVec2(ImGui::GetContentRegionAvail().x, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.5, 0));

        if (ImGui::BeginTable("##hex", byte_column_count, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoKeepColumnsVisible | ImGuiTableFlags_RowBg | /*ImGuiTableFlags_Borders |*/ ImGuiTableFlags_ScrollY))
        {

            ImGui::TableSetupScrollFreeze(0, 1);

            // Row address column
            ImGui::TableSetupColumn("ADDR");
            ImGui::TableSetupColumn("");

            // Byte columns
            for (int i = 0; i < m_bytes_per_row; i++) {
                if (IsColumnSeparator(i, m_bytes_per_row))
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, m_separator_column_width);

                sprintf(buf, "%02X", i);

                ImGui::TableSetupColumn(buf, ImGuiTableColumnFlags_WidthFixed, character_size.x * max_chars_per_cell + (6 + byte_cell_padding) * 1);
            }

            // ASCII column
            ImGui::TableSetupColumn("  ");
            ImGui::TableSetupColumn("ASCII", ImGuiTableColumnFlags_WidthFixed, (character_size.x + character_cell_padding * 1) * m_bytes_per_row);

            ImGui::TableNextRow();
            for (int i = 0; i < ImGui::TableGetColumnCount(); i++) {
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(ImGui::TableGetColumnName(i));
                ImGui::Dummy(ImVec2(0, character_size.y / 2));
            }

            ImGuiListClipper clipper;
            clipper.Begin(total_rows);

            while (clipper.Step())
            {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                {
                    ImGui::TableNextRow();
                    int address = (row * m_bytes_per_row) + base_display_addr;

                    // Draw address column
                    ImGui::TableNextColumn();
                    ImGui::Text("%04X:  ", address);
                    ImGui::TableNextColumn();

                    // Draw byte columns
                    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(2.75f, 0.0f));
                    for (int x = 0; x < m_bytes_per_row; x++)
                    {
                        int byte_address = address + x;

                        ImGui::TableNextColumn();
                        if (IsColumnSeparator(x, m_bytes_per_row))
                            ImGui::TableNextColumn();

                        ImVec2 cell_start_pos = ImGui::GetCursorScreenPos() - ImGui::GetStyle().CellPadding;
                        ImVec2 cell_size = (character_size * ImVec2(max_chars_per_cell, 1)) + (ImVec2(2, 2) * ImGui::GetStyle().CellPadding) + ImVec2(1 + byte_cell_padding, 0);
                        bool cell_hovered = ImGui::IsMouseHoveringRect(cell_start_pos, cell_start_pos + cell_size, false) && ImGui::IsWindowHovered();

                        if (IsColumnSeparator(x + 1, m_bytes_per_row) && (byte_address != m_selection_end))
                            cell_size.x += m_separator_column_width + 1;


                        DrawSelectionFrame(x, row, byte_address, cell_start_pos, cell_size);

                        // handle selection
                        if (cell_hovered)
                        {
                            HandleSelection(byte_address);
                        }

                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                        
                        if (m_editing_address == byte_address)
                        {
                            ImGui::PushItemWidth((character_size).x *2);
                            sprintf(buf, "%02X", mem_data[byte_address]);

                            if (m_set_keyboard_here)
                            {
                                ImGui::SetKeyboardFocusHere();
                                m_set_keyboard_here = false;
                            }

                            ImGui::InputText("##editing_input", buf, 3, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_AlwaysOverwrite);
                        }
                        else
                        {
                            ImGui::PushItemWidth((character_size).x);
                            ImGui::Text("%02X", mem_data[byte_address]);

                            if (cell_hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                            {
                                m_editing_address = byte_address;
                                m_set_keyboard_here = true;
                            }
                        }

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
                    if (ImGui::BeginTable("##ascii_column", m_bytes_per_row))
                    {
                        for (int x = 0; x < m_bytes_per_row; x++)
                        {
                            sprintf(buf, "##ascii_cell%d", x);
                            ImGui::TableSetupColumn(buf, ImGuiTableColumnFlags_WidthFixed, character_size.x + character_cell_padding * 1);
                        }

                        ImGui::TableNextRow();

                        for (int x = 0; x < m_bytes_per_row; x++)
                        {
                            ImGui::TableNextColumn();

                            //int byteAddress = (row * m_bytesPerRow) + x + base_display_addr;


                            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (character_cell_padding * 1) / 2);
                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                            ImGui::PushItemWidth(character_size.x);

                            int byte_address = address + x;
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

bool GearMemoryEditor::IsColumnSeparator(int current_column, int column_count)
{
    return (current_column > 0) && (current_column < column_count) && ((current_column % 4) == 0);
}

void GearMemoryEditor::DrawSelectionFrame(int x, int y, int address, const ImVec2 &cellPos, const ImVec2 &cellSize)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec4 frame_color = ImVec4(0.1f,0.9f,0.9f,1.0f);
    ImVec4 background_color = ImVec4(0.0f,0.3f,0.3f,1.0f);

    if (address < m_selection_start || address > m_selection_end)
        return;

    drawList->AddRectFilled(cellPos, cellPos + cellSize, ImColor(background_color));

    // Draw vertical line at the left of first byte and the start of the line
    if (x == 0 || address == m_selection_start)
        drawList->AddLine(cellPos - ImVec2(0, 1), cellPos + ImVec2(0, cellSize.y), ImColor(frame_color), 1);

    // Draw vertical line at the right of the last byte and the end of the line
    if (x == (m_bytes_per_row - 1) || (address) == m_selection_end)
        drawList->AddLine(cellPos + ImVec2(cellSize.x, -1), cellPos + cellSize - ImVec2(0, 1), ImColor(frame_color), 1);

    // Draw horizontal line at the top of the bytes
    if (y == 0 || (address - m_bytes_per_row) < m_selection_start)
        drawList->AddLine(cellPos - ImVec2(1, 0), cellPos + ImVec2(cellSize.x + 1, 0), ImColor(frame_color), 1);

    // Draw horizontal line at the bottom of the bytes
    if ((address + m_bytes_per_row) > m_selection_end)
        drawList->AddLine(cellPos + ImVec2(0, cellSize.y), cellPos + cellSize + ImVec2(1, 0), ImColor(frame_color), 1);
}

void GearMemoryEditor::HandleSelection(int address)
{
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        m_selection_end = address;

    }
    else if (ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseDown(ImGuiMouseButton_Right))
    {
        m_selection_start = address;
        m_selection_end = address;
    }
}
