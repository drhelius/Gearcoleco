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

static bool focus_on = false;
static bool focus_pending = false;
static bool focus_fullscreen = false;

static bool is_blocked_by_gui(void);
static bool must_leave(void);

bool game_focus_is_on(void)
{
    return focus_on;
}

void game_focus_set(bool on)
{
    focus_pending = false;

    if (on == focus_on)
        return;

    if (on && (emu_is_empty() || emu_key_mapper_is_open()))
        return;

    if (on && is_blocked_by_gui())
    {
        focus_pending = true;
        return;
    }

    focus_on = on;
    events_release_mouse();
    hotkeys_release_held();

    if (!on)
    {
        gui_notify(gui_NotificationInfo, ICON_MD_VIDEOGAME_ASSET, "Game Focus off", NULL, "game_focus", gui_NotificationDurationShort);
        return;
    }

    const char* hotkey = config_hotkeys[config_HotkeyIndex_GameFocus].str;
    char detail[96];
    snprintf(detail, sizeof(detail), "%s to leave", hotkey);
    gui_notify(gui_NotificationInfo, ICON_MD_VIDEOGAME_ASSET, "Game Focus on", hotkey[0] != 0 ? detail : NULL,
        "game_focus");
}

void game_focus_toggle(void)
{
    game_focus_set(!focus_on);
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
    return focus_on && emu_has_pointer();
}

void game_focus_update(void)
{
    bool fullscreen = config_emulator.fullscreen && !config_debug.debug && !emu_is_empty();

    if (fullscreen != focus_fullscreen)
    {
        focus_fullscreen = fullscreen;
        game_focus_set(fullscreen);
    }

    if (focus_pending && !is_blocked_by_gui())
        game_focus_set(true);

    if (focus_on && must_leave())
        game_focus_set(false);

    bool captured = game_focus_pointer_captured();

    if (captured && !SDL_GetWindowRelativeMouseMode(application_sdl_window))
        ImGui::GetIO().AddMousePosEvent(-FLT_MAX, -FLT_MAX);

    SDL_SetWindowRelativeMouseMode(application_sdl_window, captured);
}

static bool is_blocked_by_gui(void)
{
    return gui_in_use || gui_dialog_in_use || ImGui::IsPopupOpen(NULL, ImGuiPopupFlags_AnyPopup);
}

static bool must_leave(void)
{
    if (emu_is_empty() || emu_key_mapper_is_open() || is_blocked_by_gui())
        return true;

    return (SDL_GetWindowFlags(application_sdl_window) & SDL_WINDOW_INPUT_FOCUS) == 0;
}
