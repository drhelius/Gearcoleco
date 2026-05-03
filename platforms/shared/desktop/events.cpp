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

#include <SDL3/SDL.h>
#include "gearcoleco.h"
#include "config.h"
#include "gui.h"
#include "gui_actions.h"
#include "emu.h"
#include "application.h"
#include "gamepad.h"

#define EVENTS_IMPORT
#include "events.h"

static bool input_updated = false;

struct KeyState
{
    GC_Keys key;
    bool pressed;
};

static KeyState input_last_state[GC_MAX_GAMEPADS][20] = { };
static bool input_initialized = false;

static bool events_check_hotkey(const SDL_Event* event, const config_Hotkey& hotkey, bool allow_repeat);
static bool events_match_hotkey_scancode(const SDL_Event* event, const config_Hotkey& hotkey);
static void input_force_key(int controller, int index, GC_Keys key, bool pressed);
static void input_poll_controller(int controller);
static void input_filter_opposing_directions(int controller, bool* left, bool* right, bool* up, bool* down);
static void input_send_key(int controller, int index, GC_Keys key, bool pressed);

void events_shortcuts(const SDL_Event* event)
{
    if (event->type == SDL_EVENT_KEY_UP)
    {
        if (events_match_hotkey_scancode(event, config_hotkeys[config_HotkeyIndex_Rewind]))
            gui_action_rewind_released();
        return;
    }

    if (event->type != SDL_EVENT_KEY_DOWN)
        return;

    if (events_check_hotkey(event, config_hotkeys[config_HotkeyIndex_Rewind], false))
    {
        gui_action_rewind_pressed();
        return;
    }

    // Check special case hotkeys first
    if (events_check_hotkey(event, config_hotkeys[config_HotkeyIndex_Quit], false))
    {
        application_trigger_quit();
        return;
    }

    if (events_check_hotkey(event, config_hotkeys[config_HotkeyIndex_Fullscreen], false))
    {
        config_emulator.fullscreen = !config_emulator.fullscreen;
        application_trigger_fullscreen(config_emulator.fullscreen);
        return;
    }

    if (events_check_hotkey(event, config_hotkeys[config_HotkeyIndex_CaptureMouse], false))
    {
        config_emulator.capture_mouse = !config_emulator.capture_mouse;
        return;
    }

    // Check slot selection hotkeys
    for (int i = 0; i < 5; i++)
    {
        if (events_check_hotkey(event, config_hotkeys[config_HotkeyIndex_SelectSlot1 + i], false))
        {
            config_emulator.save_slot = i;
            return;
        }
    }

    // Check all hotkeys mapped to gui shortcuts
    for (int i = 0; i < GUI_HOTKEY_MAP_COUNT; i++)
    {
        if (gui_hotkey_map[i].shortcut >= 0 && events_check_hotkey(event, config_hotkeys[gui_hotkey_map[i].config_index], gui_hotkey_map[i].allow_repeat))
        {
            gui_shortcut((gui_ShortCutEvent)gui_hotkey_map[i].shortcut);
            return;
        }
    }

    // Fixed hotkeys for debug copy/paste/select operations
    int key = event->key.scancode;
    SDL_Keymod mods = event->key.mod;

    if (event->key.repeat == 0 && key == SDL_SCANCODE_A && (mods & SDL_KMOD_CTRL))
    {
        gui_shortcut(gui_ShortcutDebugSelectAll);
        return;
    }

    if (event->key.repeat == 0 && key == SDL_SCANCODE_C && (mods & SDL_KMOD_CTRL))
    {
        gui_shortcut(gui_ShortcutDebugCopy);
        return;
    }

    if (event->key.repeat == 0 && key == SDL_SCANCODE_V && (mods & SDL_KMOD_CTRL))
    {
        gui_shortcut(gui_ShortcutDebugPaste);
        return;
    }

    // ESC to exit fullscreen
    if (event->key.repeat == 0 && key == SDL_SCANCODE_ESCAPE)
    {
        if (config_emulator.fullscreen && !config_emulator.always_show_menu)
        {
            config_emulator.fullscreen = false;
            application_trigger_fullscreen(false);
        }
    }
}

void events_handle_emu_event(const SDL_Event* event)
{
    if (gui_in_use)
        return;

    switch (event->type)
    {
        case SDL_EVENT_MOUSE_MOTION:
        {
            if (config_emulator.spinner > 0)
            {
                int sen = config_emulator.spinner_sensitivity - 1;
                if (sen < 0)
                    sen = 0;
                float senf = (float)(sen / 2.0f) + 1.0f;

                switch (config_emulator.spinner)
                {
                    case 1: // SAC
                    {
                        if (event->motion.xrel != 0.0f)
                        {
                            int movement = (int)(-(event->motion.xrel) * senf);
                            emu_spinner1(movement);
                        }
                        break;
                    }
                    case 2: // Wheel
                    {
                        if (event->motion.xrel != 0.0f)
                        {
                            int movement = (int)(event->motion.xrel * senf);
                            emu_spinner1(movement);
                        }
                        break;
                    }
                    case 3: // Roller
                    {
                        if (event->motion.xrel != 0.0f)
                        {
                            int movement = (int)(event->motion.xrel * senf);
                            emu_spinner1(movement);
                        }
                        if (event->motion.yrel != 0.0f)
                        {
                            int movement = (int)(event->motion.yrel * senf);
                            emu_spinner2(movement);
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
            break;
        }
        case SDL_EVENT_MOUSE_WHEEL:
        {
            // Mouse wheel only used for SAC mode (spinner2)
            if (config_emulator.spinner == 1 && (event->wheel.y != 0.0f))
            {
                int sen = (config_emulator.spinner_sensitivity - 1) * 20;
                if (sen < 0)
                    sen = 0;
                float senf = (float)(sen / 2.0f) + 1.0f;
                int movement = (int)(event->wheel.y * senf);
                emu_spinner2(movement);
            }
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            // Roller mouse buttons
            if ((config_emulator.spinner == 3) && !config_debug.debug && !gui_main_menu_hovered)
            {
                if (event->button.button == SDL_BUTTON_LEFT)
                    emu_key_pressed(Controller_1, Key_Left_Button);
                else if (event->button.button == SDL_BUTTON_RIGHT)
                    emu_key_pressed(Controller_1, Key_Right_Button);
            }
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            // Roller mouse buttons
            if ((config_emulator.spinner == 3) && !config_debug.debug)
            {
                if (event->button.button == SDL_BUTTON_LEFT)
                    emu_key_released(Controller_1, Key_Left_Button);
                else if (event->button.button == SDL_BUTTON_RIGHT)
                    emu_key_released(Controller_1, Key_Right_Button);
            }
            break;
        }
    }
}

void events_emu(void)
{
    if (input_updated || gui_in_use)
        return;
    input_updated = true;

    SDL_PumpEvents();

    for (int controller = 0; controller < 2; controller++)
    {
        input_poll_controller(controller);
        gamepad_check_shortcuts(controller);
    }
}

void events_sync_input(void)
{
    SDL_PumpEvents();

    if (!input_initialized)
    {
        input_initialized = true;
        memset(input_last_state, 0, sizeof(input_last_state));
    }

    static const GC_Keys keys[20] = {
        Key_Left, Key_Right, Key_Up, Key_Down,
        Key_Left_Button, Key_Right_Button, Key_Blue, Key_Purple,
        Keypad_0, Keypad_1, Keypad_2, Keypad_3,
        Keypad_4, Keypad_5, Keypad_6, Keypad_7,
        Keypad_8, Keypad_9, Keypad_Asterisk, Keypad_Hash
    };

    for (int controller = 0; controller < 2; controller++)
    {
        for (int i = 0; i < 20; i++)
            input_force_key(controller, i, keys[i], false);

        input_poll_controller(controller);
    }
}

void events_reset_input(void)
{
    input_updated = false;
}

bool events_input_updated(void)
{
    return input_updated;
}

static void input_force_key(int controller, int index, GC_Keys key, bool pressed)
{
    input_last_state[controller][index].pressed = pressed;

    if (pressed)
        emu_key_pressed((GC_Controllers)controller, key);
    else
        emu_key_released((GC_Controllers)controller, key);
}

static void input_send_key(int controller, int index, GC_Keys key, bool pressed)
{
    if (pressed != input_last_state[controller][index].pressed)
    {
        input_force_key(controller, index, key, pressed);
    }
}

static void input_filter_opposing_directions(int controller, bool* left, bool* right, bool* up, bool* down)
{
    if (config_input[controller].allow_up_down)
        return;

    if (*up && *down)
    {
        if (!input_last_state[controller][2].pressed)
            *up = false;
        if (!input_last_state[controller][3].pressed)
            *down = false;
    }

    if (*left && *right)
    {
        if (!input_last_state[controller][0].pressed)
            *left = false;
        if (!input_last_state[controller][1].pressed)
            *right = false;
    }
}

static void input_poll_controller(int controller)
{
    if (controller < 0 || controller >= 2)
        return;

    if (!input_initialized)
    {
        input_initialized = true;
        memset(input_last_state, 0, sizeof(input_last_state));
    }

    SDL_Keymod mods = SDL_GetModState();
    if (mods & SDL_KMOD_CTRL)
        return;

    const bool* keyboard_state = SDL_GetKeyboardState(NULL);
    SDL_Gamepad* gamepad_ctrl = gamepad_controller[controller];
    bool gp = IsValidPointer(gamepad_ctrl) && config_input[controller].gamepad;

    struct { SDL_Scancode key; GC_Keys gc_key; int gp_btn_field; } button_map[] = {
        { config_input[controller].key_left_button, Key_Left_Button, config_input[controller].gamepad_left_button },
        { config_input[controller].key_right_button, Key_Right_Button, config_input[controller].gamepad_right_button },
        { config_input[controller].key_blue, Key_Blue, config_input[controller].gamepad_blue },
        { config_input[controller].key_purple, Key_Purple, config_input[controller].gamepad_purple },
        { config_input[controller].key_0, Keypad_0, config_input[controller].gamepad_0 },
        { config_input[controller].key_1, Keypad_1, config_input[controller].gamepad_1 },
        { config_input[controller].key_2, Keypad_2, config_input[controller].gamepad_2 },
        { config_input[controller].key_3, Keypad_3, config_input[controller].gamepad_3 },
        { config_input[controller].key_4, Keypad_4, config_input[controller].gamepad_4 },
        { config_input[controller].key_5, Keypad_5, config_input[controller].gamepad_5 },
        { config_input[controller].key_6, Keypad_6, config_input[controller].gamepad_6 },
        { config_input[controller].key_7, Keypad_7, config_input[controller].gamepad_7 },
        { config_input[controller].key_8, Keypad_8, config_input[controller].gamepad_8 },
        { config_input[controller].key_9, Keypad_9, config_input[controller].gamepad_9 },
        { config_input[controller].key_asterisk, Keypad_Asterisk, config_input[controller].gamepad_asterisk },
        { config_input[controller].key_hash, Keypad_Hash, config_input[controller].gamepad_hash },
    };

    // Directional keys (index 0-3)
    bool dir_left = keyboard_state[config_input[controller].key_left];
    bool dir_right = keyboard_state[config_input[controller].key_right];
    bool dir_up = keyboard_state[config_input[controller].key_up];
    bool dir_down = keyboard_state[config_input[controller].key_down];

    if (gp)
    {
        if (config_input[controller].gamepad_directional == 0)
        {
            dir_left  |= SDL_GetGamepadButton(gamepad_ctrl, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
            dir_right |= SDL_GetGamepadButton(gamepad_ctrl, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
            dir_up    |= SDL_GetGamepadButton(gamepad_ctrl, SDL_GAMEPAD_BUTTON_DPAD_UP);
            dir_down  |= SDL_GetGamepadButton(gamepad_ctrl, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
        }
        else
        {
            const Sint16 STICK_DEAD_ZONE = 8000;
            Sint16 rawx = SDL_GetGamepadAxis(gamepad_ctrl, (SDL_GamepadAxis)config_input[controller].gamepad_x_axis);
            Sint16 rawy = SDL_GetGamepadAxis(gamepad_ctrl, (SDL_GamepadAxis)config_input[controller].gamepad_y_axis);
            Sint16 x = config_input[controller].gamepad_invert_x_axis ? -rawx : rawx;
            Sint16 y = config_input[controller].gamepad_invert_y_axis ? -rawy : rawy;

            if (x < -STICK_DEAD_ZONE) dir_left = true;
            else if (x > STICK_DEAD_ZONE) dir_right = true;
            if (y < -STICK_DEAD_ZONE) dir_up = true;
            else if (y > STICK_DEAD_ZONE) dir_down = true;
        }
    }

    input_filter_opposing_directions(controller, &dir_left, &dir_right, &dir_up, &dir_down);

    input_send_key(controller, 0, Key_Left, dir_left);
    input_send_key(controller, 1, Key_Right, dir_right);
    input_send_key(controller, 2, Key_Up, dir_up);
    input_send_key(controller, 3, Key_Down, dir_down);

    // Button + keypad keys (index 4-19)
    for (int i = 0; i < 16; i++)
    {
        bool pressed = keyboard_state[button_map[i].key];
        if (gp)
            pressed |= gamepad_get_button(gamepad_ctrl, button_map[i].gp_btn_field);
        // Left/Right buttons also map from directional for non-keypad
        input_send_key(controller, 4 + i, button_map[i].gc_key, pressed);
    }
}

static bool events_check_hotkey(const SDL_Event* event, const config_Hotkey& hotkey, bool allow_repeat)
{
    if (event->type != SDL_EVENT_KEY_DOWN)
        return false;

    if (!allow_repeat && event->key.repeat != 0)
        return false;

    if (event->key.scancode != hotkey.key)
        return false;

    SDL_Keymod mods = event->key.mod;
    SDL_Keymod expected = hotkey.mod;

    SDL_Keymod mods_normalized = (SDL_Keymod)0;
    if (mods & (SDL_KMOD_LCTRL | SDL_KMOD_RCTRL)) mods_normalized = (SDL_Keymod)(mods_normalized | SDL_KMOD_CTRL);
    if (mods & (SDL_KMOD_LSHIFT | SDL_KMOD_RSHIFT)) mods_normalized = (SDL_Keymod)(mods_normalized | SDL_KMOD_SHIFT);
    if (mods & (SDL_KMOD_LALT | SDL_KMOD_RALT)) mods_normalized = (SDL_Keymod)(mods_normalized | SDL_KMOD_ALT);
    if (mods & (SDL_KMOD_LGUI | SDL_KMOD_RGUI)) mods_normalized = (SDL_Keymod)(mods_normalized | SDL_KMOD_GUI);

    SDL_Keymod expected_normalized = (SDL_Keymod)0;
    if (expected & (SDL_KMOD_LCTRL | SDL_KMOD_RCTRL | SDL_KMOD_CTRL)) expected_normalized = (SDL_Keymod)(expected_normalized | SDL_KMOD_CTRL);
    if (expected & (SDL_KMOD_LSHIFT | SDL_KMOD_RSHIFT | SDL_KMOD_SHIFT)) expected_normalized = (SDL_Keymod)(expected_normalized | SDL_KMOD_SHIFT);
    if (expected & (SDL_KMOD_LALT | SDL_KMOD_RALT | SDL_KMOD_ALT)) expected_normalized = (SDL_Keymod)(expected_normalized | SDL_KMOD_ALT);
    if (expected & (SDL_KMOD_LGUI | SDL_KMOD_RGUI | SDL_KMOD_GUI)) expected_normalized = (SDL_Keymod)(expected_normalized | SDL_KMOD_GUI);

    return mods_normalized == expected_normalized;
}

static bool events_match_hotkey_scancode(const SDL_Event* event, const config_Hotkey& hotkey)
{
    return event->key.scancode == hotkey.key;
}
