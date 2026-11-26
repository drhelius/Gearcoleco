/*
 * Gearcoleco - ColecoVision Emulator
 * Copyright (C) 2026  Ignacio Sanchez

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

#ifndef GAME_FOCUS_H
#define GAME_FOCUS_H

#include <SDL3/SDL.h>

#ifdef GAME_FOCUS_IMPORT
    #define EXTERN
#else
    #define EXTERN extern
#endif

EXTERN bool game_focus_is_on(void);
EXTERN bool game_focus_is_mouse_on(void);
EXTERN void game_focus_set(bool on);
EXTERN void game_focus_toggle(void);
EXTERN void game_focus_set_mouse(bool on);
EXTERN void game_focus_toggle_mouse(void);
EXTERN bool game_focus_is_captured_mouse_event(const SDL_Event* event);
EXTERN bool game_focus_pointer_captured(void);
EXTERN void game_focus_update(void);

#undef GAME_FOCUS_IMPORT
#undef EXTERN
#endif /* GAME_FOCUS_H */
