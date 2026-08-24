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

#define GUI_DEBUG_F18A_IMPORT
#include "gui_debug_f18a.h"
#include "F18A.h"
#include "F18AGPU.h"

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

void gui_debug_window_f18a_nametables(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(100, 80), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(558, 472), ImGuiCond_FirstUseEver);

    ImGui::Begin("F18A Name Tables", &config_debug.show_f18a_nametables);

    static int selected_tile_x = -1;
    static int selected_tile_y = -1;
    static int selected_layer = -1;
    static bool show_grid = true;

    if (ImGui::IsWindowAppearing())
    {
        selected_tile_x = -1;
        selected_tile_y = -1;
    }

    F18A* video = static_cast<F18A*>(emu_get_core()->GetVideo());
    u8* regs = video->GetRegisters();
    int mode = ((regs[0] & 0x04) << 1) | ((regs[0] & 0x02) << 1) |
        ((regs[1] & 0x08) >> 2) | ((regs[1] & 0x10) >> 4);
    int columns = mode == 9 ? 80 : (mode == 1 ? 40 : 32);
    int rows = IsSetBit(regs[49], 6) ? 30 : 24;

    ImGui::PushItemWidth(100.0f);
    ImGui::Combo("Layer", &emu_debug_f18a_layer, "Layer 1\0Layer 2\0\0");
    ImGui::PopItemWidth();

    bool layer2 = emu_debug_f18a_layer != 0;
    if (selected_layer != emu_debug_f18a_layer)
    {
        selected_tile_x = -1;
        selected_tile_y = -1;
        selected_layer = emu_debug_f18a_layer;
    }
    int hscroll = regs[layer2 ? 25 : 27];
    int vscroll = regs[layer2 ? 26 : 28];
    int ntba = regs[layer2 ? 10 : 2];
    int ctba = regs[layer2 ? 11 : 3];
    int ecm = (regs[49] >> 4) & 3;
    int tile_width = (mode == 1 || mode == 9) ? 6 : 8;
    int display_columns = video->GetScreenWidth() / tile_width;

    ImGui::SameLine();
    bool enabled = layer2 ? IsSetBit(regs[49], 7) : !IsSetBit(regs[50], 4);
    ImGui::TextColored(enabled ? green : gray, enabled ? "Enabled" : "Disabled");
    ImGui::SameLine();
    ImGui::Checkbox("Show Grid##grid_f18a_bg", &show_grid);

    ImGui::PushFont(gui_default_font);
    ImGui::TextColored(violet, " NAME BASE  ");ImGui::SameLine();ImGui::Text("$%02X", ntba);
    ImGui::SameLine();
    ImGui::TextColored(violet, " ATTR BASE  ");ImGui::SameLine();ImGui::Text("$%02X", ctba);
    ImGui::SameLine();
    ImGui::TextColored(violet, " ECM ");ImGui::SameLine();ImGui::Text("%d", ecm);
    ImGui::SameLine();
    ImGui::TextColored(violet, " SCROLL ");ImGui::SameLine();
    ImGui::Text("H:%d V:%d", hscroll, vscroll);
    ImGui::TextColored(violet, " SIZE       ");ImGui::SameLine();
    ImGui::Text("%dx%d tiles (%dx%d pixels)", columns, rows, video->GetScreenWidth(), video->GetScreenHeight());
    ImGui::SameLine();
    ImGui::TextColored(violet, " PAGE SIZE ");ImGui::SameLine();
    ImGui::Text("H:%s V:%s",
        IsSetBit(regs[29], layer2 ? 5 : 1) ? "2" : "1",
        IsSetBit(regs[29], layer2 ? 4 : 0) ? "2" : "1");

    float scale = video->GetScreenWidth() > 256 ? 1.0f : 1.5f;
    float width = video->GetScreenWidth() * scale;
    float height = video->GetScreenHeight() * scale;
    float spacing_x = tile_width * scale;
    float spacing_y = 8.0f * scale;
    float uv_width = (float)video->GetScreenWidth() / (float)GC_VIDEO_MAX_WIDTH;
    float uv_height = (float)video->GetScreenHeight() / (float)GC_VIDEO_MAX_HEIGHT;

    ImGui::Columns(2, "f18a_nametable_columns", false);
    ImGui::SetColumnOffset(1, width + 10.0f);

    ImVec2 position = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_debug_f18a_nametable,
        ImVec2(width, height), ImVec2(0, 0), ImVec2(uv_width, uv_height));
    bool image_hovered = ImGui::IsItemHovered();
    draw_context_menu_background();

    if (show_grid)
    {
        for (int x = 0; x <= display_columns; x++)
        {
            float grid_x = position.x + (x * spacing_x);
            draw_list->AddLine(ImVec2(grid_x, position.y),
                ImVec2(grid_x, position.y + height), ImColor(dark_gray), 1.0f);
        }
        for (int y = 0; y <= rows; y++)
        {
            float grid_y = position.y + (y * spacing_y);
            draw_list->AddLine(ImVec2(position.x, grid_y),
                ImVec2(position.x + width, grid_y), ImColor(dark_gray), 1.0f);
        }
    }

    int hovered_tile_x = -1;
    int hovered_tile_y = -1;
    float mouse_x = io.MousePos.x - position.x;
    float mouse_y = io.MousePos.y - position.y;
    if (image_hovered && (mouse_x >= 0.0f) && (mouse_x < width) && (mouse_y >= 0.0f) && (mouse_y < height))
    {
        hovered_tile_x = (int)(mouse_x / spacing_x);
        hovered_tile_y = (int)(mouse_y / spacing_y);

        if (ImGui::IsMouseClicked(0))
        {
            if ((selected_tile_x == hovered_tile_x) && (selected_tile_y == hovered_tile_y))
            {
                selected_tile_x = -1;
                selected_tile_y = -1;
            }
            else
            {
                selected_tile_x = hovered_tile_x;
                selected_tile_y = hovered_tile_y;
                F18ADebugTileInfo info;
                int pixel_x = (selected_tile_x * tile_width) + (tile_width / 2);
                int pixel_y = (selected_tile_y * 8) + 4;
                if (video->GetDebugTileInfo(pixel_x, pixel_y, layer2, info))
                    gui_debug_memory_goto(MEMORY_EDITOR_VRAM, info.pattern_address);
            }
        }

        if (!((hovered_tile_x == selected_tile_x) && (hovered_tile_y == selected_tile_y)))
        {
            draw_list->AddRect(
                ImVec2(position.x + (hovered_tile_x * spacing_x),
                    position.y + (hovered_tile_y * spacing_y)),
                ImVec2(position.x + ((hovered_tile_x + 1) * spacing_x),
                    position.y + ((hovered_tile_y + 1) * spacing_y)),
                ImColor(cyan), 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);
        }
    }

    if ((selected_tile_x >= 0) && (selected_tile_y >= 0))
    {
        float time = (float)(0.5 + 0.5 * sin(ImGui::GetTime() * 4.0));
        ImVec4 pulse_color = gui_debug_lerp_color(red, white, time);
        draw_list->AddRect(
            ImVec2(position.x + (selected_tile_x * spacing_x),
                position.y + (selected_tile_y * spacing_y)),
            ImVec2(position.x + ((selected_tile_x + 1) * spacing_x),
                position.y + ((selected_tile_y + 1) * spacing_y)),
            ImColor(pulse_color), 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);
    }

    int tile_x = hovered_tile_x >= 0 ? hovered_tile_x : selected_tile_x;
    int tile_y = hovered_tile_y >= 0 ? hovered_tile_y : selected_tile_y;

    if ((tile_x >= 0) && (tile_y >= 0))
    {
        ImGui::NextColumn();
        float uv_tile_width = (float)tile_width / (float)GC_VIDEO_MAX_WIDTH;
        float uv_tile_height = 8.0f / (float)GC_VIDEO_MAX_HEIGHT;
        ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_debug_f18a_nametable,
            ImVec2(tile_width * 16.0f, 128.0f),
            ImVec2(uv_tile_width * tile_x, uv_tile_height * tile_y),
            ImVec2(uv_tile_width * (tile_x + 1), uv_tile_height * (tile_y + 1)));

        F18ADebugTileInfo info;
        int pixel_x = (tile_x * tile_width) + (tile_width / 2);
        int pixel_y = (tile_y * 8) + 4;
        if (video->GetDebugTileInfo(pixel_x, pixel_y, layer2, info))
        {
            ImGui::TextColored(brown, "DETAILS:");
            ImGui::TextColored(cyan, " Display X:");ImGui::SameLine();ImGui::Text("%d", tile_x);
            ImGui::TextColored(cyan, " Display Y:");ImGui::SameLine();ImGui::Text("%d", tile_y);
            ImGui::TextColored(cyan, " Source Column:");ImGui::SameLine();ImGui::Text("%d", info.column);
            ImGui::TextColored(cyan, " Source Row:");ImGui::SameLine();ImGui::Text("%d", info.row);
            ImGui::TextColored(cyan, " Name Addr:");ImGui::SameLine();ImGui::Text("$%04X", info.name_address);
            ImGui::TextColored(cyan, " Pattern:");ImGui::SameLine();ImGui::Text("$%02X", info.name);
            ImGui::TextColored(cyan, " Pattern Line:");ImGui::SameLine();ImGui::Text("$%04X", info.pattern_address);
            ImGui::TextColored(cyan, " Attr Addr:");ImGui::SameLine();ImGui::Text("$%04X", info.attribute_address);
            ImGui::TextColored(cyan, " Attribute:");ImGui::SameLine();ImGui::Text("$%02X", info.attribute);
            ImGui::TextColored(cyan, " Palette Select:");ImGui::SameLine();ImGui::Text("%d", info.palette_select);
            ImGui::TextColored(cyan, " X Flip:");ImGui::SameLine();
            info.flip_x ? ImGui::TextColored(green, "YES") : ImGui::TextColored(gray, "NO");
            ImGui::TextColored(cyan, " Y Flip:");ImGui::SameLine();
            info.flip_y ? ImGui::TextColored(green, "YES") : ImGui::TextColored(gray, "NO");
            ImGui::TextColored(cyan, " Priority:");ImGui::SameLine();
            info.priority ? ImGui::TextColored(green, "YES") : ImGui::TextColored(gray, "NO");
        }
    }

    ImGui::Columns(1);
    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_f18a_patterns(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(120, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(550, 390), ImGuiCond_FirstUseEver);

    ImGui::Begin("F18A Pattern Table", &config_debug.show_f18a_patterns);

    static int selected_pattern = -1;
    static bool show_grid = true;

    if (ImGui::IsWindowAppearing())
        selected_pattern = -1;

    Video* video = emu_get_core()->GetVideo();
    u8* regs = video->GetRegisters();
    int mode = ((regs[0] & 0x04) << 1) | ((regs[0] & 0x02) << 1) |
        ((regs[1] & 0x08) >> 2) | ((regs[1] & 0x10) >> 4);
    int ecm = (regs[49] >> 4) & 3;
    int pattern_rows = mode == 4 ? 24 : 8;
    int pattern_count = pattern_rows * 32;
    int max_palette = ecm == 1 ? 31 : (ecm == 2 ? 15 : 7);
    if (emu_debug_f18a_pattern_palette > max_palette)
        emu_debug_f18a_pattern_palette = max_palette;
    if (selected_pattern >= pattern_count)
        selected_pattern = -1;

    ImGui::Checkbox("Show Grid##grid_f18a_patterns", &show_grid);

    ImGui::PushFont(gui_default_font);
    ImGui::TextColored(violet, " PATTERN BASE ");ImGui::SameLine();
    ImGui::Text("$%04X", mode == 4 ? ((regs[4] & 4) << 11) : (regs[4] << 11));
    ImGui::SameLine();
    ImGui::TextColored(violet, " ECM ");ImGui::SameLine();ImGui::Text("%d", ecm);
    ImGui::TextColored(violet, " PLANE OFFSET ");ImGui::SameLine();
    const int pattern_offsets[] = { 2048, 1024, 512, 256 };
    ImGui::Text("$%04X", pattern_offsets[(regs[29] >> 2) & 3]);
    ImGui::PopFont();

    if (ecm != 0)
    {
        ImGui::PushItemWidth(180.0f);
        ImGui::SliderInt("Palette Group", &emu_debug_f18a_pattern_palette, 0, max_palette);
        ImGui::PopItemWidth();
    }

    float scale = 1.5f;
    float width = 256.0f * scale;
    float height = (pattern_rows * 8.0f) * scale;
    float uv_height = (float)(pattern_rows * 8) / 256.0f;
    float spacing = 8.0f * scale;

    ImGui::Columns(2, "f18a_pattern_columns", false);
    ImGui::SetColumnOffset(1, width + 10.0f);

    ImVec2 position = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();

    ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_debug_f18a_patterns,
        ImVec2(width, height), ImVec2(0, 0), ImVec2(1.0f, uv_height));
    bool image_hovered = ImGui::IsItemHovered();
    draw_context_menu_tiles("##f18a_patterns_ctx");

    if (show_grid)
    {
        for (int x = 0; x <= 32; x++)
        {
            float grid_x = position.x + (x * spacing);
            draw_list->AddLine(ImVec2(grid_x, position.y),
                ImVec2(grid_x, position.y + height), ImColor(dark_gray), 1.0f);
        }
        for (int y = 0; y <= pattern_rows; y++)
        {
            float grid_y = position.y + (y * spacing);
            draw_list->AddLine(ImVec2(position.x, grid_y),
                ImVec2(position.x + width, grid_y), ImColor(dark_gray), 1.0f);
        }
    }

    int hovered_pattern = -1;
    float mouse_x = io.MousePos.x - position.x;
    float mouse_y = io.MousePos.y - position.y;

    if (image_hovered && (mouse_x >= 0.0f) && (mouse_x < width) && (mouse_y >= 0.0f) && (mouse_y < height))
    {
        int tile_x = (int)(mouse_x / spacing);
        int tile_y = (int)(mouse_y / spacing);
        hovered_pattern = (tile_y << 5) + tile_x;

        if (ImGui::IsMouseClicked(0))
        {
            selected_pattern = selected_pattern == hovered_pattern ? -1 : hovered_pattern;

            if (selected_pattern >= 0)
            {
                int pattern_base = mode == 4 ? ((regs[4] & 4) << 11) :
                    (regs[4] << 11);
                gui_debug_memory_goto(MEMORY_EDITOR_VRAM,
                    (pattern_base + (selected_pattern << 3)) & 0x3FFF);
            }
        }

        if (hovered_pattern != selected_pattern)
        {
            draw_list->AddRect(
                ImVec2(position.x + (tile_x * spacing), position.y + (tile_y * spacing)),
                ImVec2(position.x + ((tile_x + 1) * spacing),
                    position.y + ((tile_y + 1) * spacing)),
                ImColor(cyan), 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);
        }
    }

    if (selected_pattern >= 0)
    {
        int tile_x = selected_pattern & 31;
        int tile_y = selected_pattern >> 5;
        float time = (float)(0.5 + 0.5 * sin(ImGui::GetTime() * 4.0));
        ImVec4 pulse_color = gui_debug_lerp_color(red, white, time);
        draw_list->AddRect(
            ImVec2(position.x + (tile_x * spacing), position.y + (tile_y * spacing)),
            ImVec2(position.x + ((tile_x + 1) * spacing),
                position.y + ((tile_y + 1) * spacing)),
            ImColor(pulse_color), 2.0f, ImDrawFlags_RoundCornersAll, 2.0f);
    }

    int pattern = hovered_pattern >= 0 ? hovered_pattern : selected_pattern;
    if (pattern >= 0)
    {
        int tile_x = pattern & 31;
        int tile_y = pattern >> 5;
        ImGui::NextColumn();
        ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_debug_f18a_patterns,
            ImVec2(128.0f, 128.0f),
            ImVec2((float)tile_x / 32.0f, (float)tile_y / 32.0f),
            ImVec2((float)(tile_x + 1) / 32.0f, (float)(tile_y + 1) / 32.0f));

        int pattern_offsets[] = { 2048, 1024, 512, 256 };
        int plane_offset = pattern_offsets[(regs[29] >> 2) & 3];
        int pattern_base = mode == 4 ? ((regs[4] & 4) << 11) : (regs[4] << 11);
        int pattern_address = pattern_base + (pattern << 3);

        ImGui::PushFont(gui_default_font);
        ImGui::TextColored(brown, "DETAILS:");
        ImGui::TextColored(cyan, " Pattern:");ImGui::SameLine();ImGui::Text("$%03X", pattern);
        ImGui::TextColored(cyan, " Pattern Base:");ImGui::SameLine();ImGui::Text("$%04X", pattern_base);
        ImGui::TextColored(cyan, " Plane 0:");ImGui::SameLine();ImGui::Text("$%04X", pattern_address & 0x3FFF);

        if (ecm >= 2)
        {
            ImGui::TextColored(cyan, " Plane 1:");ImGui::SameLine();
            ImGui::Text("$%04X", (pattern_address + plane_offset) & 0x3FFF);
        }
        if (ecm >= 3)
        {
            ImGui::TextColored(cyan, " Plane 2:");ImGui::SameLine();
            ImGui::Text("$%04X", (pattern_address + (plane_offset << 1)) & 0x3FFF);
        }

        ImGui::TextColored(cyan, " ECM:");ImGui::SameLine();ImGui::Text("%d", ecm);
        ImGui::TextColored(cyan, " Plane Offset:");ImGui::SameLine();ImGui::Text("$%04X", plane_offset);

        if (ecm != 0)
        {
            ImGui::TextColored(cyan, " Palette Group:");ImGui::SameLine();
            ImGui::Text("%d", emu_debug_f18a_pattern_palette);
        }

        ImGui::PopFont();
    }

    ImGui::Columns(1);

    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_f18a_sprites(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(180, 120), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(626, 478), ImGuiCond_FirstUseEver);

    ImGui::Begin("F18A Sprites", &config_debug.show_f18a_sprites);

    Video* video = emu_get_core()->GetVideo();
    u8* regs = video->GetRegisters();
    u8* vram = video->GetVRAM();
    int sat = (regs[5] & 0x7F) << 7;
    int mode = ((regs[0] & 0x04) << 1) | ((regs[0] & 0x02) << 1) |
        ((regs[1] & 0x08) >> 2) | ((regs[1] & 0x10) >> 4);
    int ecm = regs[49] & 3;
    static int selected_sprite = -1;
    int hovered_sprite = -1;
    GC_RuntimeInfo runtime;
    emu_get_runtime(runtime);

    if (ImGui::IsWindowAppearing())
        selected_sprite = -1;

    ImGui::PushFont(gui_default_font);
    ImGui::TextColored(violet, " ECM ");ImGui::SameLine();ImGui::Text("%d", ecm);
    ImGui::SameLine();
    ImGui::TextColored(violet, " MAX ");ImGui::SameLine();ImGui::Text("%d", regs[30] & 0x1F);
    ImGui::SameLine();
    ImGui::TextColored(violet, " STOP ");ImGui::SameLine();ImGui::Text("%d", regs[51] & 0x3F);
    ImGui::PopFont();

    ImGui::Columns(2, "f18a_sprites_columns", false);
    ImGui::SetColumnOffset(1, 350.0f);
    ImGui::BeginChild("f18a_sprites", ImVec2(0, 0), true);

    ImGuiIO& io = ImGui::GetIO();
    bool child_hovered = ImGui::IsWindowHovered();
    ImGui::PushFont(gui_default_font);
    for (int s = 0; s < GC_MAX_SPRITES; s++)
    {
        ImGui::PushID(s);
        ImVec2 position = ImGui::GetCursorScreenPos();
        int size = emu_debug_f18a_sprite_sizes[s];
        float image_size = size * 4.0f;
        float uv = (float)size / 16.0f;
        ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_debug_f18a_sprites[s],
            ImVec2(image_size, image_size), ImVec2(0, 0), ImVec2(uv, uv));
        draw_context_menu_sprites(s);

        float mouse_x = io.MousePos.x - position.x;
        float mouse_y = io.MousePos.y - position.y;
        bool hovered = child_hovered && mouse_x >= 0 && mouse_x < image_size &&
            mouse_y >= 0 && mouse_y < image_size;

        if (hovered)
        {
            hovered_sprite = s;
            if (ImGui::IsMouseClicked(0))
                selected_sprite = selected_sprite == s ? -1 : s;
        }

        if (hovered && (selected_sprite != s))
        {
            ImGui::GetWindowDrawList()->AddRect(position,
                ImVec2(position.x + image_size, position.y + image_size), ImColor(cyan), 2.0f,
                ImDrawFlags_RoundCornersAll, 3.0f);
        }

        if (selected_sprite == s)
        {
            float time = (float)(0.5 + 0.5 * sin(ImGui::GetTime() * 4.0));
            ImVec4 pulse_color = gui_debug_lerp_color(red, white, time);
            ImGui::GetWindowDrawList()->AddRect(position,
                ImVec2(position.x + image_size, position.y + image_size),
                ImColor(pulse_color), 2.0f,
                ImDrawFlags_RoundCornersAll, 3.0f);
        }

        ImGui::PopID();
        if ((s & 3) != 3)
            ImGui::SameLine();
    }
    ImGui::PopFont();

    ImGui::EndChild();
    ImGui::NextColumn();

    float screen_scale = runtime.screen_width > 256 ? 0.5f : 1.0f;
    float texture_width = (float)runtime.screen_width / (float)SYSTEM_TEXTURE_WIDTH;
    float texture_height = (float)runtime.screen_height / (float)SYSTEM_TEXTURE_HEIGHT;
    ImVec2 screen_position = ImGui::GetCursorScreenPos();
    ImGui::Image((ImTextureID)(intptr_t)ogl_renderer_emu_texture,
        ImVec2(runtime.screen_width * screen_scale, runtime.screen_height * screen_scale),
        ImVec2(0, 0), ImVec2(texture_width, texture_height));

    int sprite = hovered_sprite >= 0 ? hovered_sprite : selected_sprite;

    if (sprite >= 0)
    {
        int offset = sat + (sprite << 2);
        u8 raw_y = vram[offset & 0x3FFF];
        u8 raw_x = vram[(offset + 1) & 0x3FFF];
        u8 name = vram[(offset + 2) & 0x3FFF];
        u8 tag = vram[(offset + 3) & 0x3FFF];
        int source_size = emu_debug_f18a_sprite_sizes[sprite];
        int magnification = IsSetBit(regs[1], 0) ? 2 : 1;
        int display_size = source_size * magnification;
        int top = IsSetBit(regs[49], 3) ? raw_y : ((raw_y + 1) & 0xFF);
        if (top >= 0xE0)
            top = -(0x100 - top);
        int sprite_x = raw_x - (IsSetBit(tag, 7) ? 32 : 0);
        int horizontal_scale = runtime.screen_width > 256 ? 2 : 1;
        int origin = (mode == 1 || mode == 9) ? 8 : 0;
        int output_x = (sprite_x - origin) * horizontal_scale;
        int output_width = display_size * horizontal_scale;

        float rect_min_x = screen_position.x + (output_x * screen_scale);
        float rect_max_x = screen_position.x + ((output_x + output_width) * screen_scale);
        float rect_min_y = screen_position.y + (top * screen_scale);
        float rect_max_y = screen_position.y + ((top + display_size) * screen_scale);
        rect_min_x = fminf(fmaxf(rect_min_x, screen_position.x),
            screen_position.x + (runtime.screen_width * screen_scale));
        rect_max_x = fminf(fmaxf(rect_max_x, screen_position.x),
            screen_position.x + (runtime.screen_width * screen_scale));
        rect_min_y = fminf(fmaxf(rect_min_y, screen_position.y),
            screen_position.y + (runtime.screen_height * screen_scale));
        rect_max_y = fminf(fmaxf(rect_max_y, screen_position.y),
            screen_position.y + (runtime.screen_height * screen_scale));

        if ((rect_min_x < rect_max_x) && (rect_min_y < rect_max_y))
        {
            float time = (float)(0.5 + 0.5 * sin(ImGui::GetTime() * 4.0));
            ImVec4 pulse_color = gui_debug_lerp_color(red, white, time);
            ImGui::GetWindowDrawList()->AddRect(ImVec2(rect_min_x, rect_min_y),
                ImVec2(rect_max_x, rect_max_y), ImColor(pulse_color), 2.0f,
                ImDrawFlags_RoundCornersAll, 2.0f);
        }

        int pattern = source_size == 16 ? (name & 0xFC) : name;
        int pattern_address = ((regs[6] & 7) << 11) + (pattern << 3);

        ImGui::PushFont(gui_default_font);
        ImGui::TextColored(brown, "DETAILS:");
        ImGui::TextColored(cyan, " Sprite:");ImGui::SameLine();ImGui::Text("%d", sprite);
        ImGui::TextColored(cyan, " X:");ImGui::SameLine();ImGui::Text("$%02X", raw_x);
        ImGui::SameLine();ImGui::TextColored(cyan, " Y:");ImGui::SameLine();ImGui::Text("$%02X", raw_y);
        ImGui::TextColored(cyan, " Pattern:");ImGui::SameLine();ImGui::Text("$%02X", name);
        ImGui::TextColored(cyan, " Pattern Addr:");ImGui::SameLine();
        ImGui::Text("$%04X", pattern_address & 0x3FFF);
        ImGui::TextColored(cyan, " Tag:");ImGui::SameLine();ImGui::Text("$%02X", tag);
        ImGui::TextColored(cyan, " Size:");ImGui::SameLine();
        ImGui::Text("%dx%d", source_size, source_size);
        ImGui::TextColored(cyan, " Magnification:");ImGui::SameLine();
        ImGui::Text("%dX", magnification);
        ImGui::TextColored(cyan, " X Flip:");ImGui::SameLine();
        video->IsF18AUnlocked() && IsSetBit(tag, 6) ?
            ImGui::TextColored(green, "YES") : ImGui::TextColored(gray, "NO");
        ImGui::TextColored(cyan, " Y Flip:");ImGui::SameLine();
        video->IsF18AUnlocked() && IsSetBit(tag, 5) ?
            ImGui::TextColored(green, "YES") : ImGui::TextColored(gray, "NO");
        ImGui::TextColored(cyan, " Early Clock:");ImGui::SameLine();
        IsSetBit(tag, 7) ? ImGui::TextColored(green, "YES") : ImGui::TextColored(gray, "NO");
        ImGui::TextColored(cyan, " Color:");ImGui::SameLine();ImGui::Text("%d", tag & 0x0F);
        ImGui::PopFont();
    }

    ImGui::Columns(1);
    ImGui::End();
    ImGui::PopStyleVar();
}

void gui_debug_window_f18a_palette(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(350, 200), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(286, 528), ImGuiCond_FirstUseEver);

    ImGui::Begin("F18A Palette RAM", &config_debug.show_f18a_palette);

    const u16* palette = emu_get_core()->GetVideo()->GetF18APalette();
    float swatch_size = 30.0f;
    float swatch_spacing = 4.0f;

    ImGui::PushFont(gui_default_font);
    ImGui::TextColored(brown, "F18A V1 PALETTE RAM");
    ImGui::PopFont();
    ImGui::Separator();
    ImGui::NewLine();

    for (int row = 0; row < 8; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            int index = row * 8 + col;
            u16 color = palette[index];
            u8 r = k4bitTo8bit[(color >> 8) & 0x0F];
            u8 g = k4bitTo8bit[(color >> 4) & 0x0F];
            u8 b = k4bitTo8bit[color & 0x0F];

            ImGui::PushID(index);
            ImVec2 position = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(position,
                ImVec2(position.x + swatch_size, position.y + swatch_size),
                ImColor(r, g, b));
            ImGui::GetWindowDrawList()->AddRect(position,
                ImVec2(position.x + swatch_size, position.y + swatch_size), ImColor(gray));
            ImGui::InvisibleButton("##f18a_palette", ImVec2(swatch_size, swatch_size));

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("#%d: $%03X", index, color);
                ImGui::Text("RGB: (%d, %d, %d)", r, g, b);
                ImGui::Text("Hex: #%02X%02X%02X", r, g, b);
                ImGui::EndTooltip();
            }

            char label[4];
            snprintf(label, sizeof(label), "%d", index);
            ImGui::PushFont(gui_default_font);
            ImVec2 text_size = ImGui::CalcTextSize(label);
            ImGui::GetWindowDrawList()->AddText(
                ImVec2(position.x + (swatch_size - text_size.x) * 0.5f,
                    position.y + swatch_size + 2), ImColor(gray), label);
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

void gui_debug_window_f18a_regs(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(339, 69), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(262, 628), ImGuiCond_FirstUseEver);

    ImGui::Begin("F18A Registers", &config_debug.show_f18a_regs);

    Video* video = emu_get_core()->GetVideo();
    u8* regs = video->GetRegisters();

    ImGui::PushFont(gui_default_font);

    ImGui::TextColored(brown, "STATE");
    ImGui::Separator();

    ImGui::TextColored(violet, " VIDEO CHIP       ");ImGui::SameLine();
    ImGui::Text("F18A V1.9");
    {
        ImGui::TextColored(violet, " F18A UNLOCKED    ");ImGui::SameLine();
        video->IsF18AUnlocked() ? ImGui::TextColored(green, "YES ") : ImGui::TextColored(gray, "NO  ");
        ImGui::TextColored(violet, " LOGICAL GEOMETRY ");ImGui::SameLine();
        ImGui::Text("%dx%d", video->GetScreenWidth(), video->GetScreenHeight());
    }

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

void gui_debug_window_f18a_extended_regs(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(620, 69), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(298, 714), ImGuiCond_FirstUseEver);

    ImGui::Begin("F18A Extended Registers", &config_debug.show_f18a_extended_regs);

    Video* video = emu_get_core()->GetVideo();
    u8* regs = video->GetRegisters();

    ImGui::PushFont(gui_default_font);

    ImGui::TextColored(brown, "REGISTERS");
    ImGui::Separator();

    const int reg_indexes[] = { 10, 11, 15, 19, 24, 25, 26, 27, 28, 29, 30, 31,
        32, 33, 34, 35, 36, 47, 48, 49, 50, 51, 54, 55, 56, 57 };
    const char* reg_names[] = { "T2 NAME", "T2 COLOR", "STATUS SEL", "LINE IRQ",
        "PALETTE SEL", "T2 HSCROLL", "T2 VSCROLL", "T1 HSCROLL", "T1 VSCROLL",
        "PAGE SIZE", "SPRITE MAX", "BITMAP CTRL", "BITMAP BASE", "BITMAP X",
        "BITMAP Y", "BITMAP WIDTH", "BITMAP HEIGHT", "DATA PORT", "VRAM INC",
        "ECM MODE", "F18A CTRL", "SPRITE STOP", "GPU PC MSB", "GPU PC LSB",
        "GPU CTRL", "UNLOCK" };

    for (int i = 0; i < (int)(sizeof(reg_indexes) / sizeof(reg_indexes[0])); i++)
    {
        int index = reg_indexes[i];
        ImGui::TextColored(cyan, " $%02X ", index);ImGui::SameLine();
        ImGui::TextColored(violet, "%-13s", reg_names[i]);ImGui::SameLine();
        ImGui::Text("$%02X", regs[index]);
    }

    ImGui::NewLine();
    ImGui::TextColored(brown, "F18A STATUS");
    ImGui::Separator();

    for (int i = 0; i < 16; i++)
    {
        ImGui::TextColored(cyan, " SR%-2d", i);ImGui::SameLine();
        ImGui::Text("$%02X", video->GetF18AStatusRegister(i));
        if ((i & 3) != 3)
            ImGui::SameLine();
    }

    F18AGPU* gpu = video->GetF18AGPU();
    ImGui::NewLine();
    ImGui::TextColored(brown, "F18A GPU");
    ImGui::Separator();
    ImGui::TextColored(violet, " PC               ");ImGui::SameLine();ImGui::Text("$%04X", gpu->GetPC());
    ImGui::TextColored(violet, " STATUS           ");ImGui::SameLine();ImGui::Text("$%04X", gpu->GetStatus());
    ImGui::TextColored(violet, " RUNNING          ");ImGui::SameLine();
    gpu->IsRunning() ? ImGui::TextColored(green, "YES") : ImGui::TextColored(gray, "NO");
    ImGui::TextColored(violet, " CYCLE BALANCE    ");ImGui::SameLine();
    ImGui::Text("%lld", (long long)gpu->GetCycleBalance());

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
