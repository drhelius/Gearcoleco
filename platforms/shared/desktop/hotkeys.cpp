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

#include "config.h"
#include "emu.h"
#include "game_focus.h"
#include "gui.h"
#include "gui_debug_memory.h"

#define HOTKEYS_IMPORT
#include "hotkeys.h"

static const int mod_primary = HOTKEY_MOD_PRIMARY;
static const int mod_shift = SDL_KMOD_SHIFT;
static const int flags_focus = HOTKEY_FLAG_GAME_FOCUS | HOTKEY_FLAG_GAMEPAD;
static const int flags_debug = HOTKEY_FLAG_DEBUG | HOTKEY_FLAG_REPEAT;

const hotkey_Definition hotkey_definitions[] =
{
    { "GameFocus", "Game Focus", flags_focus, SDL_SCANCODE_F10, mod_primary },
    { "KeyboardMapper", "Keyboard Mapper", flags_focus, SDL_SCANCODE_F1, mod_primary },
    { "KeyMapping", "Key Mapping", HOTKEY_FLAG_GAME_FOCUS, SDL_SCANCODE_F2, mod_primary },
    { "Fullscreen", "Fullscreen", flags_focus, SDL_SCANCODE_RETURN, SDL_KMOD_ALT },
    { "ShowMainMenu", "Show Main Menu", HOTKEY_FLAG_GAMEPAD, SDL_SCANCODE_M, mod_primary | mod_shift },
    { "OpenROM", "Open ROM", 0, SDL_SCANCODE_O, mod_primary | mod_shift },
    { "ReloadROM", "Reload ROM / Reset ADAM", 0, SDL_SCANCODE_D, mod_primary | mod_shift },
#if defined(__APPLE__)
    { "Quit", "Quit", 0, SDL_SCANCODE_Q, mod_primary },
#else
    { "Quit", "Quit", 0, SDL_SCANCODE_F9, mod_primary },
#endif
    { "Reset", "Reset", HOTKEY_FLAG_GAMEPAD, SDL_SCANCODE_R, mod_primary | mod_shift },
    { "Pause", "Pause", HOTKEY_FLAG_GAMEPAD, SDL_SCANCODE_P, mod_primary | mod_shift },
    { "FFWD", "Fast Forward", HOTKEY_FLAG_GAMEPAD, SDL_SCANCODE_F, mod_primary | mod_shift },
    { "Rewind", "Rewind", HOTKEY_FLAG_HOLD | HOTKEY_FLAG_GAMEPAD, SDL_SCANCODE_B, mod_primary | mod_shift },
    { "SaveState", "Save State", HOTKEY_FLAG_GAMEPAD, SDL_SCANCODE_S, mod_primary | mod_shift },
    { "LoadState", "Load State", HOTKEY_FLAG_GAMEPAD, SDL_SCANCODE_L, mod_primary | mod_shift },
    { "SelectSlot1", "Save State Slot 1", HOTKEY_FLAG_GAMEPAD, SDL_SCANCODE_1, mod_primary },
    { "SelectSlot2", "Save State Slot 2", HOTKEY_FLAG_GAMEPAD, SDL_SCANCODE_2, mod_primary },
    { "SelectSlot3", "Save State Slot 3", HOTKEY_FLAG_GAMEPAD, SDL_SCANCODE_3, mod_primary },
    { "SelectSlot4", "Save State Slot 4", HOTKEY_FLAG_GAMEPAD, SDL_SCANCODE_4, mod_primary },
    { "SelectSlot5", "Save State Slot 5", HOTKEY_FLAG_GAMEPAD, SDL_SCANCODE_5, mod_primary },
    { "Screenshot", "Screenshot", HOTKEY_FLAG_GAMEPAD, SDL_SCANCODE_X, mod_primary | mod_shift },
    { "VideoRecording", "Video Recording", HOTKEY_FLAG_GAMEPAD, SDL_SCANCODE_V, mod_primary | mod_shift },
    { "Mute", "Mute All Audio", HOTKEY_FLAG_GAMEPAD, SDL_SCANCODE_U, mod_primary | mod_shift },
    { "DebugStepInto", "Step Into", flags_debug, SDL_SCANCODE_F11, 0 },
    { "DebugStepOver", "Step Over", flags_debug, SDL_SCANCODE_F10, 0 },
    { "DebugStepOut", "Step Out", flags_debug, SDL_SCANCODE_F11, mod_shift },
    { "DebugStepFrame", "Step Frame", flags_debug, SDL_SCANCODE_F6, 0 },
    { "DebugContinue", "Continue", flags_debug, SDL_SCANCODE_F5, 0 },
    { "DebugBreak", "Break", flags_debug, SDL_SCANCODE_F7, 0 },
    { "DebugRunToCursor", "Run to Cursor", flags_debug, SDL_SCANCODE_F8, 0 },
    { "DebugBreakpoint", "Toggle Breakpoint", flags_debug, SDL_SCANCODE_F9, 0 },
    { "DebugGoBack", "Go Back", flags_debug, SDL_SCANCODE_BACKSPACE, SDL_KMOD_CTRL }
};

static bool hotkey_held[config_HotkeyIndex_COUNT] = { };

static bool is_available(int hotkey);
static bool matches(const SDL_Event* event, const config_Hotkey& hotkey, bool allow_repeat);
static SDL_Keymod normalize_mod(int mod);
static bool process_debug_edit(const SDL_Event* event);

bool hotkeys_process_event(const SDL_Event* event)
{
    if (event->type == SDL_EVENT_KEY_UP)
    {
        for (int i = 0; i < config_HotkeyIndex_COUNT; i++)
        {
            if (hotkey_held[i] && (event->key.scancode == config_hotkeys[i].key))
            {
                hotkey_held[i] = false;
                gui_hotkey((config_HotkeyIndex)i, false);
                return true;
            }
        }

        return false;
    }

    if (event->type != SDL_EVENT_KEY_DOWN)
        return false;

    for (int i = 0; i < config_HotkeyIndex_COUNT; i++)
    {
        const hotkey_Definition& definition = hotkey_definitions[i];

        if (!matches(event, config_hotkeys[i], (definition.flags & HOTKEY_FLAG_REPEAT) != 0) || !is_available(i))
            continue;

        if ((definition.flags & HOTKEY_FLAG_HOLD) != 0)
            hotkey_held[i] = true;

        emu_host_hotkey_used();
        gui_hotkey((config_HotkeyIndex)i, true);
        return true;
    }

    return process_debug_edit(event);
}

void hotkeys_release_held(void)
{
    for (int i = 0; i < config_HotkeyIndex_COUNT; i++)
    {
        if (!hotkey_held[i])
            continue;

        hotkey_held[i] = false;
        gui_hotkey((config_HotkeyIndex)i, false);
    }
}

void hotkeys_reset_defaults(bool debug)
{
    for (int i = 0; i < config_HotkeyIndex_COUNT; i++)
    {
        if (((hotkey_definitions[i].flags & HOTKEY_FLAG_DEBUG) != 0) != debug)
            continue;

        config_hotkeys[i].key = hotkey_definitions[i].key;
        config_hotkeys[i].mod = hotkeys_default_mod(i);
        config_update_hotkey_string(&config_hotkeys[i]);
    }
}

SDL_Keymod hotkeys_default_mod(int hotkey)
{
    int mod = hotkey_definitions[hotkey].mod;
    int resolved = mod & ~HOTKEY_MOD_PRIMARY;

    if ((mod & HOTKEY_MOD_PRIMARY) != 0)
    {
#if defined(__APPLE__)
        resolved |= SDL_KMOD_GUI;
#else
        resolved |= SDL_KMOD_CTRL;
#endif
    }

    return (SDL_Keymod)resolved;
}

static bool is_available(int hotkey)
{
    const hotkey_Definition& definition = hotkey_definitions[hotkey];

    if (((definition.flags & HOTKEY_FLAG_DEBUG) != 0) && !config_debug.debug)
        return false;

    return !game_focus_is_on() || ((definition.flags & HOTKEY_FLAG_GAME_FOCUS) != 0);
}

static bool matches(const SDL_Event* event, const config_Hotkey& hotkey, bool allow_repeat)
{
    if ((hotkey.key == SDL_SCANCODE_UNKNOWN) || (event->key.scancode != hotkey.key))
        return false;

    if (!allow_repeat && event->key.repeat)
        return false;

    return normalize_mod(event->key.mod) == normalize_mod(hotkey.mod);
}

static SDL_Keymod normalize_mod(int mod)
{
    int normalized = 0;

    if ((mod & SDL_KMOD_CTRL) != 0)
        normalized |= SDL_KMOD_CTRL;

    if ((mod & SDL_KMOD_SHIFT) != 0)
        normalized |= SDL_KMOD_SHIFT;

    if ((mod & SDL_KMOD_ALT) != 0)
        normalized |= SDL_KMOD_ALT;

    if ((mod & SDL_KMOD_GUI) != 0)
        normalized |= SDL_KMOD_GUI;

    return (SDL_Keymod)normalized;
}

static bool process_debug_edit(const SDL_Event* event)
{
    if (!config_debug.debug || game_focus_is_on() || event->key.repeat || ((event->key.mod & SDL_KMOD_CTRL) == 0))
        return false;

    switch (event->key.scancode)
    {
        case SDL_SCANCODE_A:
            gui_debug_memory_select_all();
            return true;
        case SDL_SCANCODE_C:
            gui_debug_memory_copy();
            return true;
        case SDL_SCANCODE_V:
            gui_debug_memory_paste();
            return true;
        default:
            return false;
    }
}
