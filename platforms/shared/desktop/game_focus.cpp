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

#include <float.h>
#include "imgui.h"
#include "fonts/IconsMaterialDesign.h"
#include "application.h"
#include "config.h"
#include "emu.h"
#include "events.h"
#include "gui.h"
#include "gui_notifications.h"
#include "hotkeys.h"

#define GAME_FOCUS_IMPORT
#include "game_focus.h"

enum Focus_State
{
    Focus_State_Off = 0,
    Focus_State_Mouse,
    Focus_State_Game
};

static Focus_State focus_state = Focus_State_Off;
static Focus_State focus_pending = Focus_State_Off;
static bool focus_fullscreen = false;

static void set_state(Focus_State state);
static void notify(Focus_State previous);
static bool is_blocked_by_gui(void);
static bool must_leave(void);

bool game_focus_is_on(void)
{
    return focus_state == Focus_State_Game;
}

bool game_focus_is_mouse_on(void)
{
    return focus_state == Focus_State_Mouse;
}

void game_focus_set(bool on)
{
    set_state(on ? Focus_State_Game : Focus_State_Off);
}

void game_focus_toggle(void)
{
    if (focus_state != Focus_State_Game)
        set_state(Focus_State_Game);
    else
        set_state(focus_fullscreen && emu_has_pointer() ? Focus_State_Mouse : Focus_State_Off);
}

void game_focus_set_mouse(bool on)
{
    if (focus_state != Focus_State_Game)
        set_state(on ? Focus_State_Mouse : Focus_State_Off);
}

void game_focus_toggle_mouse(void)
{
    game_focus_set_mouse(focus_state != Focus_State_Mouse);
}

bool game_focus_is_captured_mouse_event(const SDL_Event* event)
{
    if (!game_focus_pointer_captured())
        return false;

    if ((event->type != SDL_EVENT_MOUSE_MOTION) && (event->type != SDL_EVENT_MOUSE_WHEEL) &&
        (event->type != SDL_EVENT_MOUSE_BUTTON_DOWN))
        return false;

    return SDL_GetWindowFromEvent(event) == application_sdl_window;
}

bool game_focus_pointer_captured(void)
{
    return (focus_state != Focus_State_Off) && emu_has_pointer();
}

void game_focus_update(void)
{
    bool fullscreen = config_emulator.fullscreen && !config_debug.debug && !emu_is_empty();

    if (fullscreen != focus_fullscreen)
    {
        focus_fullscreen = fullscreen;

        if (!fullscreen)
            set_state(Focus_State_Off);
        else if (focus_state == Focus_State_Off)
            set_state(Focus_State_Mouse);
    }

    if ((focus_pending != Focus_State_Off) && !is_blocked_by_gui())
        set_state(focus_pending);

    if ((focus_state != Focus_State_Off) && must_leave())
        set_state(Focus_State_Off);

    bool captured = game_focus_pointer_captured();

    if (captured && !SDL_GetWindowRelativeMouseMode(application_sdl_window))
        ImGui::GetIO().AddMousePosEvent(-FLT_MAX, -FLT_MAX);

    SDL_SetWindowRelativeMouseMode(application_sdl_window, captured);
}

static void set_state(Focus_State state)
{
    focus_pending = Focus_State_Off;

    if ((state == Focus_State_Mouse) && !emu_has_pointer())
        state = Focus_State_Off;

    if (state == focus_state)
        return;

    if ((state != Focus_State_Off) && (emu_is_empty() || emu_key_mapper_is_open()))
        return;

    if ((state != Focus_State_Off) && is_blocked_by_gui())
    {
        focus_pending = state;
        return;
    }

    Focus_State previous = focus_state;
    focus_state = state;
    events_release_mouse();
    hotkeys_release_held();
    notify(previous);
}

static void notify(Focus_State previous)
{
    const char* focus_hotkey = config_hotkeys[config_HotkeyIndex_GameFocus].str;
    const char* mouse_hotkey = config_hotkeys[config_HotkeyIndex_MouseCapture].str;
    char detail[96];

    switch (focus_state)
    {
        case Focus_State_Game:
            snprintf(detail, sizeof(detail), "%s to leave", focus_hotkey);
            gui_notify(gui_NotificationInfo, ICON_MD_VIDEOGAME_ASSET, "Game Focus on",
                focus_hotkey[0] != 0 ? detail : NULL, "game_focus");
            break;
        case Focus_State_Mouse:
            snprintf(detail, sizeof(detail), "%s to release", mouse_hotkey);

            if (previous == Focus_State_Game)
                gui_notify(gui_NotificationInfo, ICON_MD_VIDEOGAME_ASSET, "Game Focus off",
                    mouse_hotkey[0] != 0 ? detail : NULL, "game_focus", gui_NotificationDurationShort);
            else
                gui_notify(gui_NotificationInfo, ICON_MD_MOUSE, "Mouse captured", mouse_hotkey[0] != 0 ? detail : NULL,
                    "game_focus");
            break;
        default:
            if (previous == Focus_State_Game)
                gui_notify(gui_NotificationInfo, ICON_MD_VIDEOGAME_ASSET, "Game Focus off", NULL, "game_focus",
                    gui_NotificationDurationShort);
            else
                gui_notify(gui_NotificationInfo, ICON_MD_MOUSE, "Mouse released", NULL, "game_focus",
                    gui_NotificationDurationShort);
            break;
    }
}

static bool is_blocked_by_gui(void)
{
    return gui_in_use || gui_dialog_in_use || ImGui::IsPopupOpen(NULL, ImGuiPopupFlags_AnyPopup);
}

static bool must_leave(void)
{
    if (emu_is_empty() || emu_key_mapper_is_open() || is_blocked_by_gui())
        return true;

    if ((focus_state == Focus_State_Mouse) && !emu_has_pointer())
        return true;

    return (SDL_GetWindowFlags(application_sdl_window) & SDL_WINDOW_INPUT_FOCUS) == 0;
}
