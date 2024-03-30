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
#include <string>
#include <stdexcept>

MemEditor::MemEditor()
{
    m_separator_column_width = 8.0f;
    m_selection_start = 0;
    m_selection_end = 0;
    m_bytes_per_row = 16;
    m_row_scroll_top = 0;
    m_row_scroll_bottom = 0;
    m_editing_address = -1;
    m_set_keyboard_here = false;
    m_uppercase_hex = true;
    m_gray_out_zeros = true;
    m_preview_data_type = 0;
    m_preview_endianess = 0;
}

MemEditor::~MemEditor()
{

}

void MemEditor::Draw(uint8_t* mem_data, int mem_size, int base_display_addr)
{
    int total_rows = (mem_size + (m_bytes_per_row - 1)) / m_bytes_per_row;
    int separator_count = (m_bytes_per_row - 1) / 4;
    int byte_column_count = 2 + m_bytes_per_row + separator_count + 2;
    int byte_cell_padding = 0;
    int character_cell_padding = 0;
    int max_chars_per_cell = 2;
    ImVec2 character_size = ImGui::CalcTextSize("0");
    char buf[16];
    ImVec4 addr_color = ImVec4(0.1f,0.9f,0.9f,1.0f);
    ImVec4 ascii_color = ImVec4(1.0f,0.502f,0.957f,1.0f);
    ImVec4 column_color = ImVec4(1.0f,0.90f,0.05f,1.0f);
    ImVec4 normal_color = ImVec4(1.0f,1.0f,1.0f,1.0f);
    ImVec4 gray_color = ImVec4(0.4f,0.4f,0.4f,1.0f);
    float height_separator = ImGui::GetStyle().ItemSpacing.y;
    float footer_height =  ImGui::GetFrameHeightWithSpacing() * 4;

    if (ImGui::BeginChild("##mem", ImVec2(ImGui::GetContentRegionAvail().x, -footer_height), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav))
    {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.5, 0));

        if (ImGui::BeginTable("##header", byte_column_count, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoKeepColumnsVisible))
        {
            ImGui::TableSetupColumn("ADDR   ");
            ImGui::TableSetupColumn("");

            for (int i = 0; i < m_bytes_per_row; i++) {
                if (IsColumnSeparator(i, m_bytes_per_row))
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, m_separator_column_width);

                sprintf(buf, "%02X", i);

                ImGui::TableSetupColumn(buf, ImGuiTableColumnFlags_WidthFixed, character_size.x * max_chars_per_cell + (6 + byte_cell_padding) * 1);
            }

            ImGui::TableSetupColumn("");
            ImGui::TableSetupColumn("ASCII", ImGuiTableColumnFlags_WidthFixed, (character_size.x + character_cell_padding * 1) * m_bytes_per_row);

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::TextColored(addr_color, "%s", ImGui::TableGetColumnName(0));

            for (int i = 1; i < (ImGui::TableGetColumnCount() - 1); i++) {
                ImGui::TableNextColumn();
                ImGui::TextColored(column_color, "%s", ImGui::TableGetColumnName(i));
            }

            ImGui::TableNextColumn();
            ImGui::TextColored(ascii_color, "%s", ImGui::TableGetColumnName(ImGui::TableGetColumnCount() - 1));

            ImGui::EndTable();
        }

        if (ImGui::BeginTable("##hex", byte_column_count, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoKeepColumnsVisible /*| ImGuiTableFlags_RowBg*/ | ImGuiTableFlags_ScrollY))
        {
            m_row_scroll_top = ImGui::GetScrollY() / character_size.y;
            m_row_scroll_bottom = m_row_scroll_top + (ImGui::GetWindowHeight() / character_size.y);

            //ImGui::TableSetupScrollFreeze(0, 1);

            ImGui::TableSetupColumn("ADDR");
            ImGui::TableSetupColumn("");

            for (int i = 0; i < m_bytes_per_row; i++) {
                if (IsColumnSeparator(i, m_bytes_per_row))
                    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, m_separator_column_width);

                sprintf(buf, "%02X", i);

                ImGui::TableSetupColumn(buf, ImGuiTableColumnFlags_WidthFixed, character_size.x * max_chars_per_cell + (6 + byte_cell_padding) * 1);
            }

            ImGui::TableSetupColumn("  ");
            ImGui::TableSetupColumn("ASCII", ImGuiTableColumnFlags_WidthFixed, (character_size.x + character_cell_padding * 1) * m_bytes_per_row);

            // ImGui::TableNextRow();
            // for (int i = 0; i < ImGui::TableGetColumnCount(); i++) {
            //     ImGui::TableNextColumn();
            //     ImGui::TextUnformatted(ImGui::TableGetColumnName(i));
            //     ImGui::Dummy(ImVec2(0, character_size.y / 2));
            // }

            ImGuiListClipper clipper;
            clipper.Begin(total_rows);

            while (clipper.Step())
            {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
                {
                    ImGui::TableNextRow();
                    int address = (row * m_bytes_per_row) + base_display_addr;

                    ImGui::TableNextColumn();
                    ImGui::Text("%04X:  ", address);
                    ImGui::TableNextColumn();

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

                        DrawSelectionFrame(x, row, byte_address, cell_start_pos, cell_size);

                        if (cell_hovered)
                        {
                            HandleSelection(byte_address, row);
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

                            bool gray_out = m_gray_out_zeros && mem_data[byte_address] == 0;
                            ImGui::TextColored(gray_out ? gray_color : normal_color, m_uppercase_hex ? "%02X" : "%02x", mem_data[byte_address]);

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

                    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));
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

                            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (character_cell_padding * 1) / 2);
                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                            ImGui::PushItemWidth(character_size.x);

                            int byte_address = address + x;
                            unsigned char c = mem_data[byte_address];

                            bool gray_out = m_gray_out_zeros && (c < 32 || c >= 128);
                            ImGui::TextColored(gray_out ? gray_color : normal_color, "%c", (c >= 32 && c < 128) ? c : '.');

                            ImGui::PopItemWidth();
                            ImGui::PopStyleVar();
                        }

                        ImGui::EndTable();
                    }

                    ImGui::PopStyleVar();
                }
            }

            ImGui::EndTable();

        }

        ImGui::PopStyleVar();

    }
    ImGui::EndChild();

    DrawOptions(mem_size, base_display_addr);
    DrawDataPreview(m_selection_start, mem_data, mem_size);
}

bool MemEditor::IsColumnSeparator(int current_column, int column_count)
{
    return (current_column > 0) && (current_column < column_count) && ((current_column % 4) == 0);
}

void MemEditor::DrawSelectionFrame(int x, int y, int address, ImVec2 cell_pos, ImVec2 cell_size)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec4 frame_color = ImVec4(0.1f,0.9f,0.9f,1.0f);
    ImVec4 background_color = ImVec4(0.0f,0.3f,0.3f,1.0f);
    int start = m_selection_start <= m_selection_end ? m_selection_start : m_selection_end;
    int end = m_selection_end >= m_selection_start ? m_selection_end : m_selection_start;

    if (address < start || address > end)
        return;

    if (IsColumnSeparator(x + 1, m_bytes_per_row) && (address != end))
    {
        cell_size.x += m_separator_column_width + 1;
    }

    drawList->AddRectFilled(cell_pos, cell_pos + cell_size, ImColor(background_color));

    if (x == 0 || address == start)
        drawList->AddLine(cell_pos - ImVec2(0, 1), cell_pos + ImVec2(0, cell_size.y), ImColor(frame_color), 1);

    if (x == (m_bytes_per_row - 1) || (address) == end)
        drawList->AddLine(cell_pos + ImVec2(cell_size.x, -1), cell_pos + cell_size - ImVec2(0, 1), ImColor(frame_color), 1);

    if (y == 0 || (address - m_bytes_per_row) < start)
        drawList->AddLine(cell_pos - ImVec2(1, 0), cell_pos + ImVec2(cell_size.x + 1, 0), ImColor(frame_color), 1);

    if ((address + m_bytes_per_row) >= end)
        drawList->AddLine(cell_pos + ImVec2(0, cell_size.y), cell_pos + cell_size + ImVec2(1, 0), ImColor(frame_color), 1);
}

void MemEditor::HandleSelection(int address, int row)
{
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        m_selection_end = address;

        if (m_selection_start != m_selection_end)
        {
            if (row > (m_row_scroll_bottom - 3))
            {
                ImGui::SetScrollY(ImGui::GetScrollY() + 5);
            }
            else if (row < (m_row_scroll_top + 4))
            {
                ImGui::SetScrollY(ImGui::GetScrollY() - 5);
            }
        }
    }
    else if (ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseDown(ImGuiMouseButton_Right))
    {
        m_selection_start = address;
        m_selection_end = address;
    }
    else if (m_selection_start > m_selection_end)
    {
        int tmp = m_selection_start;
        m_selection_start = m_selection_end;
        m_selection_end = tmp;
    }
}

void MemEditor::JumpToAddress(int address)
{
    ImVec2 character_size = ImGui::CalcTextSize("0");
    ImGui::SetScrollY((address / m_bytes_per_row) * character_size.y);
}

void MemEditor::DrawOptions(int mem_size, int base_display_addr)
{
    ImVec4 color = ImVec4(0.1f,0.9f,0.9f,1.0f);

    ImGui::TextColored(color, "REGION:");
    ImGui::SameLine();
    ImGui::Text("%04X-%04X", base_display_addr, base_display_addr + mem_size);
    ImGui::SameLine();
    ImGui::TextColored(color, " SELECTION:");
    ImGui::SameLine();
    ImGui::Text("%04X-%04X",m_selection_start, m_selection_end);

    if (ImGui::Button("Options"))
        ImGui::OpenPopup("context");

    if (ImGui::BeginPopup("context"))
    {
        ImGui::Text("Columns:   ");
        ImGui::SameLine();
        ImGui::PushItemWidth(120.0f);
        ImGui::SliderInt("##columns", &m_bytes_per_row, 4, 16);
        ImGui::Text("Preview as:");
        ImGui::SameLine();
        ImGui::PushItemWidth(120.0f);
        ImGui::Combo("##preview_type", &m_preview_data_type, "Uint8\0Int8\0Uint16\0Int16\0UInt32\0Int32\0\0");
        ImGui::Text("Preview as:");
        ImGui::SameLine();
        ImGui::PushItemWidth(120.0f);
        ImGui::Combo("##preview_endianess", &m_preview_endianess, "Little Endian\0Big Endian\0\0");
        ImGui::Checkbox("Uppercase hex", &m_uppercase_hex);
        ImGui::Checkbox("Gray out zeros", &m_gray_out_zeros);

        ImGui::EndPopup();
    }

    ImGui::SameLine();
    ImGui::Text("Go to:");
    ImGui::SameLine();
    ImGui::PushItemWidth(45);
    char goto_address[5] = "";
    if (ImGui::InputTextWithHint("##gotoaddr", "XXXX", goto_address, IM_ARRAYSIZE(goto_address), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
    {
        try
        {
            JumpToAddress((int)std::stoul(goto_address, 0, 16));
        }
        catch(const std::invalid_argument&)
        {
        }
        goto_address[0] = 0;
    }
}

void MemEditor::DrawDataPreview(int address, uint8_t* mem_data, int mem_size)
{
    if (address < 0 || address >= mem_size)
        return;

    ImVec4 color = ImVec4(0.992f,0.592f,0.122f,1.0f);

    ImGui::TextColored(color, "Dec:");
    ImGui::SameLine();
    ImGui::Text("%d", mem_data[address]);

    ImGui::TextColored(color, "Hex:");
    ImGui::SameLine();
    ImGui::Text("%02X", mem_data[address]);

    ImGui::TextColored(color, "Bin:");
    ImGui::SameLine();
    ImGui::Text("%02X", mem_data[address]);
}