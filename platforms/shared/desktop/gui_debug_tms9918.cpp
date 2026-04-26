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

#define GUI_DEBUG_TMS9918_IMPORT
#include "gui_debug_tms9918.h"

#include <math.h>
#include <cmath>
#include "imgui.h"
#include "gearcoleco.h"
#include "gui_debug_constants.h"
#include "gui_debug_memory.h"
#include "gui_filedialogs.h"
#include "gui.h"
#include "config.h"
#include "emu.h"
#include "ogl_renderer.h"
#include "utils.h"

static void draw_context_menu_sprites(int index);
static void draw_context_menu_background(void);
static void draw_context_menu_tiles(const char* popup_id);

void gui_debug_window_vram_nametable(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(896, 31), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(580, 490), ImGuiCond_FirstUseEver);

    ImGui::Begin("Name Table", &config_debug.show_video_nametable);

    static int selected_bg_tile_x = -1;
    static int selected_bg_tile_y = -1;

    if (ImGui::IsWindowAppearing())
    {
        selected_bg_tile_x = -1;
        selected_bg_tile_y = -1;
    }

    GearcolecoCore* core = emu_get_core();
    Video* video = core->GetVideo();
    u8* regs = video->GetRegisters();
    u8* vram = video->GetVRAM();
    int mode = video->GetMode();

    bool window_hovered = ImGui::IsWindowHovered();
    static bool show_grid = true;
    int lines = 24;
    int cols = (mode == 1) ? 40 : 32;
    float scale = 1.5f;
    float tile_width = (mode == 1) ? 6.0f : 8.0f;
    float size_h = tile_width * cols * scale;
    float size_v = 8.0f * lines * scale;
    float spacing_h = tile_width * scale;
    float spacing_v = 8.0f * scale;
    float uv_h = tile_width / 256.0f;
    float uv_v = 1.0f / 32.0f;

    ImGui::Checkbox("Show Grid##grid_bg", &show_grid);

    ImGui::PushFont(gui_default_font);

    ImGui::Columns(2, "bg", false);
    ImGui::SetColumnOffset(1, size_h + 10.0f);

    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();

    ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_debug_vram_background, ImVec2(size_h, size_v), ImVec2(0.0f, 0.0f), ImVec2(uv_h * cols, uv_v * lines));

    draw_context_menu_background();

    if (show_grid)
    {
        float x = p.x;
        for (int n = 0; n <= cols; n++)
        {
            draw_list->AddLine(ImVec2(x, p.y), ImVec2(x, p.y + size_v), ImColor(dark_gray), 1.0f);
            x += spacing_h;
        }

        float y = p.y;
        for (int n = 0; n <= lines; n++)
        {
            draw_list->AddLine(ImVec2(p.x, y), ImVec2(p.x + size_h, y), ImColor(dark_gray), 1.0f);
            y += spacing_v;
        }
    }

    float mouse_x = io.MousePos.x - p.x;
    float mouse_y = io.MousePos.y - p.y;

    int hovered_bg_tile_x = -1;
    int hovered_bg_tile_y = -1;

    if (window_hovered && (mouse_x >= 0.0f) && (mouse_x < size_h) && (mouse_y >= 0.0f) && (mouse_y < size_v))
    {
        hovered_bg_tile_x = (int)(mouse_x / spacing_h);
        hovered_bg_tile_y = (int)(mouse_y / spacing_v);

        if (ImGui::IsMouseClicked(0))
        {
            if (selected_bg_tile_x == hovered_bg_tile_x && selected_bg_tile_y == hovered_bg_tile_y)
            {
                selected_bg_tile_x = -1;
                selected_bg_tile_y = -1;
            }
            else
            {
                selected_bg_tile_x = hovered_bg_tile_x;
                selected_bg_tile_y = hovered_bg_tile_y;

                int name_table_addr = regs[2] << 10;
                int pattern_table_addr = regs[4] << 11;
                int region = (hovered_bg_tile_y & 0x18) << 5;
                int tile_number = (hovered_bg_tile_y * cols) + hovered_bg_tile_x;
                int name_tile_addr = (name_table_addr + tile_number) & 0x3FFF;
                int name_tile = vram[name_tile_addr];
                if (mode == 2 || mode == 4)
                    pattern_table_addr &= 0x2000;
                if (mode == 2)
                    name_tile += region;
                int goto_addr = (pattern_table_addr + (name_tile << 3)) & 0x3FFF;
                gui_debug_memory_goto(MEMORY_EDITOR_VRAM, goto_addr);
            }
        }

        if (!(hovered_bg_tile_x == selected_bg_tile_x && hovered_bg_tile_y == selected_bg_tile_y))
            draw_list->AddRect(ImVec2(p.x + (hovered_bg_tile_x * spacing_h), p.y + (hovered_bg_tile_y * spacing_v)), ImVec2(p.x + ((hovered_bg_tile_x + 1) * spacing_h), p.y + ((hovered_bg_tile_y + 1) * spacing_v)), ImColor(cyan), 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);
    }

    if (selected_bg_tile_x >= 0 && selected_bg_tile_y >= 0)
    {
        float t = (float)(0.5 + 0.5 * sin(ImGui::GetTime() * 4.0));
        ImVec4 pulse_color = ImVec4(red.x + (white.x - red.x) * t, red.y + (white.y - red.y) * t, red.z + (white.z - red.z) * t, 1.0f);
        draw_list->AddRect(ImVec2(p.x + (selected_bg_tile_x * spacing_h), p.y + (selected_bg_tile_y * spacing_v)), ImVec2(p.x + ((selected_bg_tile_x + 1) * spacing_h), p.y + ((selected_bg_tile_y + 1) * spacing_v)), ImColor(pulse_color), 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);
    }

    int tile_x = (hovered_bg_tile_x >= 0) ? hovered_bg_tile_x : selected_bg_tile_x;
    int tile_y = (hovered_bg_tile_y >= 0) ? hovered_bg_tile_y : selected_bg_tile_y;

    if (tile_x >= 0 && tile_y >= 0)
    {
        ImGui::NextColumn();

        ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_debug_vram_background, ImVec2(tile_width * 16.0f, 128.0f), ImVec2(uv_h * tile_x, uv_v * tile_y), ImVec2(uv_h * (tile_x + 1), uv_v * (tile_y + 1)));

        ImGui::TextColored(green, "INFO:");

        ImGui::TextColored(cyan, " X:"); ImGui::SameLine();
        ImGui::Text("$%02X", tile_x); ImGui::SameLine();
        ImGui::TextColored(cyan, "   Y:"); ImGui::SameLine();
        ImGui::Text("$%02X", tile_y);

        int name_table_addr = regs[2] << 10;
        int color_table_addr = regs[3] << 6;
        if (mode == 2)
            color_table_addr &= 0x2000;
        int pattern_table_addr = regs[4] << 11;
        int region = (tile_y & 0x18) << 5;

        int tile_number = (tile_y * cols) + tile_x;
        int name_tile_addr = (name_table_addr + tile_number) & 0x3FFF;
        int name_tile = vram[name_tile_addr];

        if (mode == 2)
        {
            pattern_table_addr &= 0x2000;
            name_tile += region;
        }
        else if (mode == 4)
        {
            pattern_table_addr &= 0x2000;
        }

        int tile_addr = (pattern_table_addr + (name_tile << 3)) & 0x3FFF;

        int color_mask = ((regs[3] & 0x7F) << 3) | 0x07;

        int color_tile_addr = 0;

        if (mode == 2)
            color_tile_addr = color_table_addr + ((name_tile & color_mask) << 3);
        else if (mode == 0)
            color_tile_addr = color_table_addr + (name_tile >> 3);

        ImGui::TextColored(cyan, " Name Addr:"); ImGui::SameLine();
        ImGui::Text(" $%04X", name_tile_addr);
        ImGui::TextColored(cyan, " Tile Number:"); ImGui::SameLine();
        ImGui::Text("$%03X", name_tile);
        ImGui::TextColored(cyan, " Tile Addr:"); ImGui::SameLine();
        ImGui::Text(" $%04X", tile_addr);
        ImGui::TextColored(cyan, " Color Addr:"); ImGui::SameLine();
        ImGui::Text("$%04X", color_tile_addr);
    }

    ImGui::Columns(1);

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_vram_tiles(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(580, 530), ImGuiCond_FirstUseEver);

    ImGui::Begin("Pattern Table", &config_debug.show_video_tiles);

    static int selected_tile_x = -1;
    static int selected_tile_y = -1;

    if (ImGui::IsWindowAppearing())
    {
        selected_tile_x = -1;
        selected_tile_y = -1;
    }

    GearcolecoCore* core = emu_get_core();
    Video* video = core->GetVideo();
    u8* regs = video->GetRegisters();
    int mode = video->GetMode();

    bool window_hovered = ImGui::IsWindowHovered();
    static bool show_grid = true;
    bool split_mode2 = (mode == 2);
    int section_count = split_mode2 ? 3 : 1;
    int lines_per_section = split_mode2 ? 8 : 32;
    int visible_lines = section_count * lines_per_section;
    float scale = 1.5f;
    float width = 8.0f * 32.0f * scale;
    float section_height = 8.0f * lines_per_section * scale;
    float spacing = 8.0f * scale;
    float section_margin = 10.0f;
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();

    if (selected_tile_y >= visible_lines)
    {
        selected_tile_x = -1;
        selected_tile_y = -1;
    }

    ImGui::Checkbox("Show Grid##grid_tiles", &show_grid); ImGui::SameLine();
    ImGui::Checkbox("Color", &emu_debug_tile_color_mode);

    ImGui::Columns(2, "tiles", false);
    ImGui::SetColumnOffset(1, width + 10.0f);

    int hovered_tile_x = -1;
    int hovered_tile_y = -1;
    int hovered_section = -1;

    for (int section = 0; section < section_count; section++)
    {
        ImVec2 p_tiles = ImGui::GetCursorScreenPos();
        float tex_y_start = (float)(section * lines_per_section) / 32.0f;
        float tex_y_end = (float)((section + 1) * lines_per_section) / 32.0f;

        ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_debug_vram_tiles, ImVec2(width, section_height), ImVec2(0.0f, tex_y_start), ImVec2(1.0f, tex_y_end));

        char popup_id[32];
        snprintf(popup_id, sizeof(popup_id), "##tiles_ctx_%d", section);
        draw_context_menu_tiles(popup_id);

        if (show_grid)
        {
            float x = p_tiles.x;
            for (int grid_col = 0; grid_col <= 32; grid_col++)
            {
                draw_list->AddLine(ImVec2(x, p_tiles.y), ImVec2(x, p_tiles.y + section_height), ImColor(dark_gray), 1.0f);
                x += spacing;
            }

            float y = p_tiles.y;
            for (int grid_row = 0; grid_row <= lines_per_section; grid_row++)
            {
                draw_list->AddLine(ImVec2(p_tiles.x, y), ImVec2(p_tiles.x + width, y), ImColor(dark_gray), 1.0f);
                y += spacing;
            }
        }

        float mouse_x = io.MousePos.x - p_tiles.x;
        float mouse_y = io.MousePos.y - p_tiles.y;

        if (window_hovered && (mouse_x >= 0.0f) && (mouse_x < width) && (mouse_y >= 0.0f) && (mouse_y < section_height))
        {
            hovered_tile_x = (int)(mouse_x / spacing);
            hovered_tile_y = (int)(mouse_y / spacing) + (section * lines_per_section);
            hovered_section = section;
        }

        int section_first_line = section * lines_per_section;
        int section_last_line = section_first_line + lines_per_section;

        if ((selected_tile_x >= 0) && (selected_tile_y >= section_first_line) && (selected_tile_y < section_last_line))
        {
            int selected_local_y = selected_tile_y - section_first_line;
            float t = (float)(0.5 + 0.5 * sin(ImGui::GetTime() * 4.0));
            ImVec4 pulse_color = ImVec4(red.x + (white.x - red.x) * t, red.y + (white.y - red.y) * t, red.z + (white.z - red.z) * t, 1.0f);
            draw_list->AddRect(ImVec2(p_tiles.x + (selected_tile_x * spacing), p_tiles.y + (selected_local_y * spacing)), ImVec2(p_tiles.x + ((selected_tile_x + 1) * spacing), p_tiles.y + ((selected_local_y + 1) * spacing)), ImColor(pulse_color), 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);
        }

        if ((hovered_section == section) && !(hovered_tile_x == selected_tile_x && hovered_tile_y == selected_tile_y))
        {
            int hovered_local_y = hovered_tile_y - section_first_line;
            draw_list->AddRect(ImVec2(p_tiles.x + (hovered_tile_x * spacing), p_tiles.y + (hovered_local_y * spacing)), ImVec2(p_tiles.x + ((hovered_tile_x + 1) * spacing), p_tiles.y + ((hovered_local_y + 1) * spacing)), ImColor(cyan), 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);
        }

        if (section < section_count - 1)
            ImGui::Dummy(ImVec2(0.0f, section_margin));
    }

    if (hovered_tile_x >= 0 && ImGui::IsMouseClicked(0))
    {
        if (selected_tile_x == hovered_tile_x && selected_tile_y == hovered_tile_y)
        {
            selected_tile_x = -1;
            selected_tile_y = -1;
        }
        else
        {
            selected_tile_x = hovered_tile_x;
            selected_tile_y = hovered_tile_y;

            int tile = (hovered_tile_y << 5) + hovered_tile_x;
            int pattern_table_addr = (regs[4] & (mode == 2 ? 0x04 : 0x07)) << 11;
            int goto_addr = (pattern_table_addr + (tile << 3)) & 0x3FFF;
            gui_debug_memory_goto(MEMORY_EDITOR_VRAM, goto_addr);
        }
    }

    int tile_x = (hovered_tile_x >= 0) ? hovered_tile_x : selected_tile_x;
    int tile_y = (hovered_tile_y >= 0) ? hovered_tile_y : selected_tile_y;

    if (tile_x >= 0 && tile_y >= 0)
    {
        ImGui::NextColumn();

        ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_debug_vram_tiles, ImVec2(128.0f, 128.0f), ImVec2((1.0f / 32.0f) * tile_x, (1.0f / 32.0f) * tile_y), ImVec2((1.0f / 32.0f) * (tile_x + 1), (1.0f / 32.0f) * (tile_y + 1)));

        ImGui::PushFont(gui_default_font);

        ImGui::TextColored(brown, "DETAILS:");

        int tile = (tile_y << 5) + tile_x;
        int pattern_table_addr = (regs[4] & (mode == 2 ? 0x04 : 0x07)) << 11;
        int section = split_mode2 ? (tile_y / lines_per_section) : 0;
        int table_addr = (pattern_table_addr + (split_mode2 ? (section * 0x800) : 0)) & 0x3FFF;
        int tile_in_section = ((tile_y % lines_per_section) << 5) + tile_x;
        int tile_addr = split_mode2 ? ((table_addr + (tile_in_section << 3)) & 0x3FFF) : ((pattern_table_addr + (tile << 3)) & 0x3FFF);

        ImGui::TextColored(cyan, " Table Addr:"); ImGui::SameLine();
        ImGui::Text("$%04X", table_addr);

        ImGui::TextColored(cyan, " Tile Number:"); ImGui::SameLine();
        ImGui::Text("$%03X", tile);
        ImGui::TextColored(cyan, " Tile Addr:"); ImGui::SameLine();
        ImGui::Text("$%04X", tile_addr);

        ImGui::PopFont();
    }

    ImGui::Columns(1);

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_vram_sprites(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(200, 200), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(480, 336), ImGuiCond_FirstUseEver);

    ImGui::Begin("Sprites", &config_debug.show_video_sprites);

    static int selected_sprite = -1;

    if (ImGui::IsWindowAppearing())
        selected_sprite = -1;

    float scale = 4.0f;
    float size_8 = 8.0f * scale;
    float size_16 = 16.0f * scale;

    GearcolecoCore* core = emu_get_core();
    Video* video = core->GetVideo();
    u8* regs = video->GetRegisters();
    u8* vram = video->GetVRAM();
    GC_RuntimeInfo runtime;
    emu_get_runtime(runtime);
    bool sprites_16 = IsSetBit(regs[1], 1);

    float spr_width = sprites_16 ? size_16 : size_8;
    float spr_height = sprites_16 ? size_16 : size_8;

    ImVec2 spr_pos[GC_MAX_SPRITES];

    ImGuiIO& io = ImGui::GetIO();

    ImGui::PushFont(gui_default_font);

    ImGui::Columns(2, "spr", false);
    ImGui::SetColumnOffset(1, sprites_16 ? 330.0f : 200.0f);

    ImGui::BeginChild("sprites", ImVec2(0, 0.0f), true);

    bool child_hovered = ImGui::IsWindowHovered();

    int hovered_sprite = -1;

    for (int s = 0; s < GC_MAX_SPRITES; s++)
    {
        spr_pos[s] = ImGui::GetCursorScreenPos();

        ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_debug_vram_sprites[s], ImVec2(spr_width, spr_height), ImVec2(0.0f, 0.0f), ImVec2((1.0f / 16.0f) * (spr_width / scale), (1.0f / 16.0f) * (spr_height / scale)));

        draw_context_menu_sprites(s);

        float mx = io.MousePos.x - spr_pos[s].x;
        float my = io.MousePos.y - spr_pos[s].y;

        bool is_hovered = child_hovered && (mx >= 0.0f) && (mx < spr_width) && (my >= 0.0f) && (my < spr_height);

        if (is_hovered)
        {
            hovered_sprite = s;
            if (ImGui::IsMouseClicked(0))
                selected_sprite = (selected_sprite == s) ? -1 : s;
        }

        if (is_hovered && selected_sprite != s)
        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddRect(ImVec2(spr_pos[s].x, spr_pos[s].y), ImVec2(spr_pos[s].x + spr_width, spr_pos[s].y + spr_height), ImColor(cyan), 2.0f, ImDrawFlags_RoundCornersAll, 3.0f);
        }

        if (selected_sprite == s)
        {
            float t = (float)(0.5 + 0.5 * sin(ImGui::GetTime() * 4.0));
            ImVec4 pulse_color = ImVec4(red.x + (white.x - red.x) * t, red.y + (white.y - red.y) * t, red.z + (white.z - red.z) * t, 1.0f);
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddRect(ImVec2(spr_pos[s].x, spr_pos[s].y), ImVec2(spr_pos[s].x + spr_width, spr_pos[s].y + spr_height), ImColor(pulse_color), 2.0f, ImDrawFlags_RoundCornersAll, 3.0f);
        }

        if (s % 4 < 3)
            ImGui::SameLine();
    }

    ImGui::EndChild();

    ImGui::NextColumn();

    ImVec2 p_screen = ImGui::GetCursorScreenPos();

    float screen_scale = 1.0f;
    float tex_h = (float)runtime.screen_width / (float)(SYSTEM_TEXTURE_WIDTH);
    float tex_v = (float)runtime.screen_height / (float)(SYSTEM_TEXTURE_HEIGHT);

    ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_texture, ImVec2(runtime.screen_width * screen_scale, runtime.screen_height * screen_scale), ImVec2(0, 0), ImVec2(tex_h, tex_v));

    int display_sprite = (hovered_sprite >= 0) ? hovered_sprite : selected_sprite;

    if (display_sprite >= 0)
    {
        int s = display_sprite;
        u16 sprite_attribute_addr = (regs[5] & 0x7F) << 7;
        u16 sprite_pattern_addr = (regs[6] & 0x07) << 11;
        int sprite_attribute_offset = sprite_attribute_addr + (s << 2);
        int tile = vram[sprite_attribute_offset + 2];
        int sprite_tile_addr = sprite_pattern_addr + (tile << 3);
        int sprite_shift = (vram[sprite_attribute_offset + 3] & 0x80) ? 32 : 0;
        int x = vram[sprite_attribute_offset + 1];
        int y = vram[sprite_attribute_offset];

        int final_y = (y + 1) & 0xFF;

        if (final_y >= 0xE0)
            final_y = -(0x100 - final_y);

        float real_x = (float)(x - sprite_shift);
        float real_y = (float)final_y;

        float max_width = sprites_16 ? 16.0f : 8.0f;
        float max_height = sprites_16 ? 16.0f : 8.0f;

        if (IsSetBit(regs[1], 0))
        {
            max_width *= 2.0f;
            max_height *= 2.0f;
        }

        float rectx_min = p_screen.x + (real_x * screen_scale);
        float rectx_max = p_screen.x + ((real_x + max_width) * screen_scale);
        float recty_min = p_screen.y + (real_y * screen_scale);
        float recty_max = p_screen.y + ((real_y + max_height) * screen_scale);

        rectx_min = fminf(fmaxf(rectx_min, p_screen.x), p_screen.x + (runtime.screen_width * screen_scale));
        rectx_max = fminf(fmaxf(rectx_max, p_screen.x), p_screen.x + (runtime.screen_width * screen_scale));
        recty_min = fminf(fmaxf(recty_min, p_screen.y), p_screen.y + (runtime.screen_height * screen_scale));
        recty_max = fminf(fmaxf(recty_max, p_screen.y), p_screen.y + (runtime.screen_height * screen_scale));

        float t = (float)(0.5 + 0.5 * sin(ImGui::GetTime() * 4.0));
        ImVec4 pulse_color = ImVec4(
            red.x + (white.x - red.x) * t,
            red.y + (white.y - red.y) * t,
            red.z + (white.z - red.z) * t,
            1.0f);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRect(ImVec2(rectx_min, recty_min), ImVec2(rectx_max, recty_max), ImColor(pulse_color), 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);

        ImGui::TextColored(brown, "DETAILS:");
        ImGui::TextColored(cyan, " X:"); ImGui::SameLine();
        ImGui::Text("$%02X", x); ImGui::SameLine();
        ImGui::TextColored(cyan, "  Y:"); ImGui::SameLine();
        ImGui::Text("$%02X", y); ImGui::SameLine();

        ImGui::TextColored(cyan, "  Tile:"); ImGui::SameLine();
        ImGui::Text("$%02X", tile);

        ImGui::TextColored(cyan, " Tile Addr:"); ImGui::SameLine();
        ImGui::Text("$%04X", sprite_tile_addr);

        int color = vram[sprite_attribute_offset + 3] & 0x0F;
        ImGui::TextColored(cyan, " Color:"); ImGui::SameLine();
        ImGui::Text("%d", color);

        ImGui::TextColored(cyan, " Early Clock:"); ImGui::SameLine();
        sprite_shift > 0 ? ImGui::TextColored(green, "ON ") : ImGui::TextColored(gray, "OFF");
    }

    ImGui::Columns(1);

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_vram_palettes(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(350, 350), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 210), ImGuiCond_FirstUseEver);

    ImGui::Begin("Palettes", &config_debug.show_video_palettes);

    const u8* selected_palette = kPalette_888_coleco;
    const char* palette_name = "COLECO PALETTE";

    if (config_video.palette == 1)
    {
        selected_palette = kPalette_888_tms9918;
        palette_name = "TMS9918 PALETTE";
    }
    else if (config_video.palette == 2)
    {
        palette_name = "CUSTOM PALETTE";
    }

    ImGui::PushFont(gui_default_font);
    ImGui::TextColored(brown, "%s", palette_name);
    ImGui::PopFont();

    ImGui::Separator();
    ImGui::NewLine();

    float swatch_size = 30.0f;
    float swatch_spacing = 4.0f;

    const char* color_names[] = {
        "Transparent", "Black", "Medium Green", "Light Green",
        "Dark Blue", "Light Blue", "Dark Red", "Cyan",
        "Medium Red", "Light Red", "Dark Yellow", "Light Yellow",
        "Dark Green", "Magenta", "Gray", "White"
    };

    for (int row = 0; row < 2; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            int i = row * 8 + col;

            u8 r = 0;
            u8 g = 0;
            u8 b = 0;

            if (config_video.palette == 2)
            {
                r = config_video.color[i].red;
                g = config_video.color[i].green;
                b = config_video.color[i].blue;
            }
            else
            {
                r = selected_palette[i * 3 + 0];
                g = selected_palette[i * 3 + 1];
                b = selected_palette[i * 3 + 2];
            }

            ImVec4 color_vec = ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);

            ImGui::PushID(i);

            ImVec2 sw_p = ImGui::GetCursorScreenPos();

            ImGui::GetWindowDrawList()->AddRectFilled(
                sw_p, ImVec2(sw_p.x + swatch_size, sw_p.y + swatch_size), ImColor(color_vec));
            ImGui::GetWindowDrawList()->AddRect(
                sw_p, ImVec2(sw_p.x + swatch_size, sw_p.y + swatch_size), ImColor(gray));

            ImGui::InvisibleButton("##swatch", ImVec2(swatch_size, swatch_size));

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("#%d: %s", i, color_names[i]);
                ImGui::Text("RGB: (%d, %d, %d)", r, g, b);
                ImGui::Text("Hex: #%02X%02X%02X", r, g, b);
                ImGui::EndTooltip();
            }

            ImGui::PushFont(gui_default_font);
            ImVec2 text_size = ImGui::CalcTextSize("00");
            float text_x = sw_p.x + (swatch_size - text_size.x) * 0.5f;
            char color_label[4];
            snprintf(color_label, sizeof(color_label), "%d", i);
            ImGui::GetWindowDrawList()->AddText(
                ImVec2(text_x, sw_p.y + swatch_size + 2), ImColor(gray),
                color_label);
            ImGui::PopFont();

            ImGui::PopID();

            if (col < 7)
                ImGui::SameLine(0, swatch_spacing);
        }

        ImGui::Dummy(ImVec2(0, 18));
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_vram_regs(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(400, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(460, 370), ImGuiCond_FirstUseEver);

    ImGui::Begin("VDP Registers", &config_debug.show_video_regs);

    Video* video = emu_get_core()->GetVideo();
    u8* regs = video->GetRegisters();

    ImGui::PushFont(gui_default_font);

    ImGui::TextColored(brown, "STATE");
    ImGui::Separator();

    ImGui::TextColored(violet, " PAL (50Hz)       ");ImGui::SameLine();
    video->IsPAL() ? ImGui::TextColored(green, "YES ") : ImGui::TextColored(gray, "NO  ");
    ImGui::TextColored(violet, " LATCH FIRST BYTE ");ImGui::SameLine();
    video->GetLatch() ? ImGui::TextColored(green, "YES ") : ImGui::TextColored(gray, "NO  ");
    ImGui::TextColored(violet, " INTERNAL BUFFER  ");ImGui::SameLine();
    ImGui::Text("$%02X  ", video->GetBufferReg()); ImGui::SameLine(0, 0);
    ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(video->GetBufferReg()));
    ImGui::TextColored(violet, " INTERNAL STATUS  ");ImGui::SameLine();
    ImGui::Text("$%02X  ", video->GetStatusReg()); ImGui::SameLine(0, 0);
    ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(video->GetStatusReg()));
    ImGui::TextColored(violet, " INTERNAL ADDRESS ");ImGui::SameLine();
    ImGui::Text("$%04X", video->GetAddressReg());
    ImGui::TextColored(violet, " RENDER LINE      ");ImGui::SameLine();
    ImGui::Text("%d", video->GetRenderLine());
    ImGui::TextColored(violet, " CYCLE COUNTER    ");ImGui::SameLine();
    ImGui::Text("%d", video->GetCycleCounter());

    ImGui::NewLine();
    ImGui::TextColored(brown, "REGISTERS");
    ImGui::Separator();

    const char* reg_desc[] = {
        "CONTROL 0   ", "CONTROL 1   ", "PATTERN NAME",
        "COLOR TABLE ", "PATTERN GEN ", "SPRITE ATTR ",
        "SPRITE GEN  ", "COLORS      "
    };

    for (int i = 0; i < 8; i++)
    {
        ImGui::TextColored(cyan, " $%01X ", i);ImGui::SameLine();
        ImGui::TextColored(violet, "%s ", reg_desc[i]);ImGui::SameLine();
        ImGui::Text("$%02X  ", regs[i]); ImGui::SameLine(0, 0);
        ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(regs[i]));
    }

    ImGui::NewLine();
    ImGui::TextColored(brown, "DECODED");
    ImGui::Separator();

    int name_table_addr = regs[2] << 10;
    int color_table_addr = regs[3] << 6;
    int pattern_table_addr = regs[4] << 11;
    int sprite_attr_addr = (regs[5] & 0x7F) << 7;
    int sprite_gen_addr = (regs[6] & 0x07) << 11;
    int backdrop = regs[7] & 0x0F;
    int text_color = (regs[7] >> 4) & 0x0F;

    ImGui::TextColored(violet, " NAME TABLE       ");ImGui::SameLine();ImGui::Text("$%04X", name_table_addr);
    ImGui::TextColored(violet, " COLOR TABLE      ");ImGui::SameLine();ImGui::Text("$%04X", color_table_addr);
    ImGui::TextColored(violet, " PATTERN TABLE    ");ImGui::SameLine();ImGui::Text("$%04X", pattern_table_addr);
    ImGui::TextColored(violet, " SPRITE ATTR      ");ImGui::SameLine();ImGui::Text("$%04X", sprite_attr_addr);
    ImGui::TextColored(violet, " SPRITE GEN       ");ImGui::SameLine();ImGui::Text("$%04X", sprite_gen_addr);
    ImGui::TextColored(violet, " BACKDROP COLOR   ");ImGui::SameLine();ImGui::Text("%d", backdrop);
    ImGui::TextColored(violet, " TEXT COLOR       ");ImGui::SameLine();ImGui::Text("%d", text_color);
    ImGui::TextColored(violet, " SPRITE SIZE      ");ImGui::SameLine();ImGui::Text("%s", IsSetBit(regs[1], 1) ? "16X16" : "8X8");
    ImGui::TextColored(violet, " SPRITE MAG       ");ImGui::SameLine();ImGui::Text("%s", IsSetBit(regs[1], 0) ? "2X" : "1X");
    ImGui::TextColored(violet, " DISPLAY ENABLE   ");ImGui::SameLine();
    IsSetBit(regs[1], 6) ? ImGui::TextColored(green, "ON") : ImGui::TextColored(red, "OFF");
    ImGui::TextColored(violet, " NMI ENABLE       ");ImGui::SameLine();
    IsSetBit(regs[1], 5) ? ImGui::TextColored(green, "ON") : ImGui::TextColored(red, "OFF");

    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

static void draw_context_menu_sprites(int index)
{
    ImGui::PopFont();

    char ctx_id[16];
    snprintf(ctx_id, sizeof(ctx_id), "##spr_ctx_%02d", index);

    if (ImGui::BeginPopupContextItem(ctx_id))
    {
        if (ImGui::Selectable("Save Sprite As..."))
            gui_file_dialog_save_sprite(index);
        if (ImGui::Selectable("Save All Sprites To Folder..."))
            gui_file_dialog_save_all_sprites();

        ImGui::EndPopup();
    }

    ImGui::PushFont(gui_default_font);
}

static void draw_context_menu_background(void)
{
    ImGui::PopFont();

    if (ImGui::BeginPopupContextItem("##bg_ctx"))
    {
        if (ImGui::Selectable("Save Name Table As..."))
            gui_file_dialog_save_background();

        ImGui::EndPopup();
    }

    ImGui::PushFont(gui_default_font);
}

static void draw_context_menu_tiles(const char* popup_id)
{
    if (ImGui::BeginPopupContextItem(popup_id))
    {
        if (ImGui::Selectable("Save Pattern Table As..."))
            gui_file_dialog_save_tiles();

        ImGui::EndPopup();
    }
}
