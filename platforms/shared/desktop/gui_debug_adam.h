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

#ifndef GUI_DEBUG_ADAM_H
#define GUI_DEBUG_ADAM_H

struct GC_AdamDebugState;

void gui_debug_window_adam_system(const GC_AdamDebugState* state);
void gui_debug_window_adam_net(const GC_AdamDebugState* state);
void gui_debug_window_adam_media_printer(const GC_AdamDebugState* state);

#endif /* GUI_DEBUG_ADAM_H */
