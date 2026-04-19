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

// TODO: This file contains no-op stubs for yet-to-be-ported debug functions.
// Replace each stub with the real implementation as they are ported.

#include "definitions.h"
#include "gui_debug.h"
#include "gui_debug_disassembler.h"
#include "gui_debug_memory.h"
#include "gui_debug_trace_logger.h"
#include "application_headless.h"

void gui_debug_init(void)
{
    // TODO: implement
}

void gui_debug_destroy(void)
{
    // TODO: implement
}

void gui_debug_reset(void)
{
    // TODO: implement
}

void gui_debug_windows(void)
{
    // TODO: implement
}

void gui_debug_save_settings(const char* file_path)
{
    // TODO: implement
    UNUSED(file_path);
}

void gui_debug_load_settings(const char* file_path)
{
    // TODO: implement
    UNUSED(file_path);
}

void gui_debug_auto_save_settings(void)
{
    // TODO: implement
}

void gui_debug_auto_load_settings(void)
{
    // TODO: implement
}

void gui_debug_reset_symbols(void)
{
    // TODO: implement
}

bool gui_debug_load_symbols_file(const char* file_path)
{
    // TODO: implement
    UNUSED(file_path);
    return false;
}

void gui_debug_toggle_breakpoint(void)
{
    // TODO: implement
}

void gui_debug_runtocursor(void)
{
    // TODO: implement
}

void gui_debug_go_back(void)
{
    // TODO: implement
}

void gui_debug_save_disassembler(const char* file_path, bool full)
{
    // TODO: implement
    UNUSED(file_path);
    UNUSED(full);
}

void gui_debug_memory_step_frame(void)
{
    // TODO: implement
}

void gui_debug_memory_copy(void)
{
    // TODO: implement
}

void gui_debug_memory_paste(void)
{
    // TODO: implement
}

void gui_debug_memory_select_all(void)
{
    // TODO: implement
}

void gui_debug_memory_save_dump(const char* file_path, bool binary)
{
    // TODO: implement
    UNUSED(file_path);
    UNUSED(binary);
}

void gui_debug_trace_logger_clear(void)
{
    // TODO: implement
}

void gui_debug_save_log(const char* file_path)
{
    // TODO: implement
    UNUSED(file_path);
}

int application_headless_init(const char* rom_file, const char* symbol_file, int mcp_mode, int mcp_tcp_port)
{
    // TODO: implement
    UNUSED(rom_file);
    UNUSED(symbol_file);
    UNUSED(mcp_mode);
    UNUSED(mcp_tcp_port);
    return 0;
}

void application_headless_destroy(void)
{
    // TODO: implement
}

void application_headless_mainloop(void)
{
    // TODO: implement
}
