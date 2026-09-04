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

#include "gui_debug_adam.h"

#include "imgui.h"
#include "Adam.h"
#include "AdamNet.h"
#include "config.h"
#include "emu.h"
#include "gui.h"
#include "gui_filedialogs.h"
#include "gui_debug_constants.h"

void gui_debug_window_adam_printer(void)
{
    GearcolecoCore* core = emu_get_core();
    if (!IsValidPointer(core) || (core->GetMachine() != GC_MACHINE_ADAM))
        return;

    AdamNet* adam_net = core->GetAdam()->GetAdamNet();
    if (!IsValidPointer(adam_net))
        return;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(90, 90), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(560, 360), ImGuiCond_FirstUseEver);
    ImGui::Begin("ADAM Printer", &config_debug.show_adam_printer);

    ImGui::PushFont(gui_default_font);
    int size = adam_net->GetPrinterSize();
    ImGui::TextColored(cyan, "Captured:");
    ImGui::SameLine();
    ImGui::Text("%d / %d bytes", size, AdamNet::kPrinterSpoolSize);

    if (ImGui::Button("Save..."))
        gui_file_dialog_save_adam_printer();
    ImGui::SameLine();
    if (ImGui::Button("Clear"))
        adam_net->ClearPrinter();

    ImGui::Separator();
    ImGui::BeginChild("##adam_printer_output", ImVec2(0, 0), true,
        ImGuiWindowFlags_HorizontalScrollbar);

    char output[AdamNet::kPrinterSpoolSize + 1];
    const u8* data = adam_net->GetPrinterData();
    int position = 0;
    for (int i = 0; i < size && position < AdamNet::kPrinterSpoolSize; i++)
    {
        u8 value = data[i];
        if (value == '\r')
        {
            if ((i + 1 >= size) || (data[i + 1] != '\n'))
                output[position++] = '\n';
        }
        else if ((value == '\n') || (value == '\t') || ((value >= 0x20) && (value < 0x7F)))
            output[position++] = (char)value;
        else
            output[position++] = '.';
    }
    output[position] = '\0';
    ImGui::TextUnformatted(output);

    ImGui::EndChild();
    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleVar();
}
