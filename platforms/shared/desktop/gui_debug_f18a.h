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

#ifndef GUI_DEBUG_F18A_H
#define GUI_DEBUG_F18A_H

#ifdef GUI_DEBUG_F18A_IMPORT
    #define EXTERN
#else
    #define EXTERN extern
#endif

EXTERN void gui_debug_window_f18a_nametables(void);
EXTERN void gui_debug_window_f18a_patterns(void);
EXTERN void gui_debug_window_f18a_sprites(void);
EXTERN void gui_debug_window_f18a_palette(void);
EXTERN void gui_debug_window_f18a_regs(void);
EXTERN void gui_debug_window_f18a_extended_regs(void);

#undef GUI_DEBUG_F18A_IMPORT
#undef EXTERN
#endif /* GUI_DEBUG_F18A_H */
