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

#ifndef HOTKEYS_H
#define HOTKEYS_H

#include <SDL3/SDL.h>
#include "config.h"

#ifdef HOTKEYS_IMPORT
    #define EXTERN
#else
    #define EXTERN extern
#endif

#define HOTKEY_FLAG_REPEAT 0x01
#define HOTKEY_FLAG_HOLD 0x02
#define HOTKEY_FLAG_GAME_FOCUS 0x04
#define HOTKEY_FLAG_GAMEPAD 0x08
#define HOTKEY_FLAG_DEBUG 0x10

#define HOTKEY_MOD_PRIMARY 0x10000

#if defined(__APPLE__)
#define HOTKEY_PRIMARY_KMOD SDL_KMOD_GUI
#define HOTKEY_PRIMARY_LABEL "Cmd+"
#else
#define HOTKEY_PRIMARY_KMOD SDL_KMOD_CTRL
#define HOTKEY_PRIMARY_LABEL "Ctrl+"
#endif

struct hotkey_Definition
{
    const char* name;
    const char* label;
    int flags;
    SDL_Scancode key;
    int mod;
};

extern const hotkey_Definition hotkey_definitions[config_HotkeyIndex_COUNT];

EXTERN bool hotkeys_process_event(const SDL_Event* event);
EXTERN void hotkeys_release_held(void);
EXTERN void hotkeys_reset_defaults(bool debug);
EXTERN SDL_Keymod hotkeys_default_mod(int hotkey);
EXTERN bool hotkeys_is_allowed(int hotkey, int mod);

#undef HOTKEYS_IMPORT
#undef EXTERN
#endif /* HOTKEYS_H */
