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

#ifndef LIBRETRO_CORE_OPTIONS_H__
#define LIBRETRO_CORE_OPTIONS_H__

#include <stdlib.h>
#include <string.h>

#include "libretro.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 ********************************
 * Core Option Definitions
 ********************************
 */

struct retro_core_option_v2_category option_cats_us[] = {
    {
        "system",
        "System",
        "Configure refresh rate and other system-level settings."
    },
    {
        "video",
        "Video",
        "Configure aspect ratio, overscan and sprite settings."
    },
    {
        "input",
        "Input",
        "Configure controller behavior, spinner support and sensitivity settings."
    },
    { NULL, NULL, NULL },
};

struct retro_core_option_v2_definition option_defs_us[] = {

    /* System */

    {
        "gearcoleco_timing",
        "Refresh Rate (restart)",
        NULL,
        "Select which refresh rate will be used in emulation. 'Auto' automatically selects the best refresh rate based on the loaded content. 'NTSC (60 Hz)' forces 60 Hz. 'PAL (50 Hz)' forces 50 Hz.",
        NULL,
        "system",
        {
            { "Auto",         NULL },
            { "NTSC (60 Hz)", NULL },
            { "PAL (50 Hz)",  NULL },
            { NULL, NULL },
        },
        "Auto"
    },

    /* Video */

    {
        "gearcoleco_aspect_ratio",
        "Aspect Ratio",
        NULL,
        "Select which aspect ratio will be presented by the core. '1:1 PAR' selects an aspect ratio that produces square pixels.",
        NULL,
        "video",
        {
            { "1:1 PAR",  NULL },
            { "4:3 DAR",  NULL },
            { "16:9 DAR", NULL },
            { "16:10 DAR", NULL },
            { NULL, NULL },
        },
        "1:1 PAR"
    },
    {
        "gearcoleco_overscan",
        "Overscan",
        NULL,
        "Select which overscan (borders) will be used in emulation. 'Disabled' shows no overscan. 'Top+Bottom' enables vertical overscan only. 'Full (284 width)' and 'Full (320 width)' enable full overscan with different horizontal widths.",
        NULL,
        "video",
        {
            { "Disabled",         NULL },
            { "Top+Bottom",       NULL },
            { "Full (284 width)", NULL },
            { "Full (320 width)", NULL },
            { NULL, NULL },
        },
        "Disabled"
    },
    {
        "gearcoleco_no_sprite_limit",
        "No Sprite Limit",
        NULL,
        "Remove the per-line sprite limit. This reduces flickering but may cause glitches in certain games. It's best to keep this option disabled.",
        NULL,
        "video",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },

    /* Input */

    {
        "gearcoleco_up_down_allowed",
        "Allow Up+Down / Left+Right",
        NULL,
        "Allow pressing, quickly alternating, or holding both left and right (or up and down) directions at the same time. This may cause movement based glitches in certain games.",
        NULL,
        "input",
        {
            { "Disabled", NULL },
            { "Enabled",  NULL },
            { NULL, NULL },
        },
        "Disabled"
    },
    {
        "gearcoleco_spinners",
        "Spinner Support",
        NULL,
        "Select which spinner controller will be emulated. Spinners are controlled by mouse movement. Mouse buttons are mapped to Left (Yellow) and Right (Red) buttons.",
        NULL,
        "input",
        {
            { "Disabled",                 NULL },
            { "Super Action Controller",  NULL },
            { "Wheel Controller",         NULL },
            { "Roller Controller",        NULL },
            { NULL, NULL },
        },
        "Disabled"
    },
    {
        "gearcoleco_spinner_sensitivity",
        "Spinner Sensitivity",
        NULL,
        "Set the sensitivity of the spinner controller. 1 is the lowest sensitivity and 10 is the highest.",
        NULL,
        "input",
        {
            { "1",  NULL },
            { "2",  NULL },
            { "3",  NULL },
            { "4",  NULL },
            { "5",  NULL },
            { "6",  NULL },
            { "7",  NULL },
            { "8",  NULL },
            { "9",  NULL },
            { "10", NULL },
            { NULL, NULL },
        },
        "1"
    },

    { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};

struct retro_core_options_v2 options_us = {
    option_cats_us,
    option_defs_us
};

/*
 ********************************
 * Functions
 ********************************
 */

static void libretro_set_core_options(retro_environment_t environ_cb,
        bool *categories_supported)
{
    unsigned version = 0;

    if (!environ_cb || !categories_supported)
        return;

    *categories_supported = false;

    if (!environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version))
        version = 0;

    if (version >= 2)
    {
        *categories_supported = environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2,
                &options_us);
    }
    else
    {
        size_t i, j;
        size_t option_index  = 0;
        size_t num_options   = 0;
        struct retro_core_option_definition *option_v1_defs_us = NULL;
        struct retro_variable *variables   = NULL;
        char **values_buf                  = NULL;

        while (true)
        {
            if (option_defs_us[num_options].key)
                num_options++;
            else
                break;
        }

        if (version >= 1)
        {
            option_v1_defs_us = (struct retro_core_option_definition *)
                    calloc(num_options + 1, sizeof(struct retro_core_option_definition));

            for (i = 0; i < num_options; i++)
            {
                struct retro_core_option_v2_definition *option_def_us = &option_defs_us[i];
                struct retro_core_option_value *option_values         = option_def_us->values;
                struct retro_core_option_definition *option_v1_def_us = &option_v1_defs_us[i];
                struct retro_core_option_value *option_v1_values      = option_v1_def_us->values;

                option_v1_def_us->key           = option_def_us->key;
                option_v1_def_us->desc          = option_def_us->desc;
                option_v1_def_us->info          = option_def_us->info;
                option_v1_def_us->default_value = option_def_us->default_value;

                while (option_values->value)
                {
                    option_v1_values->value = option_values->value;
                    option_v1_values->label = option_values->label;

                    option_values++;
                    option_v1_values++;
                }
            }

            environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, option_v1_defs_us);
        }
        else
        {
            variables  = (struct retro_variable *)calloc(num_options + 1,
                    sizeof(struct retro_variable));
            values_buf = (char **)calloc(num_options, sizeof(char *));

            if (!variables || !values_buf)
                goto error;

            for (i = 0; i < num_options; i++)
            {
                const char *key                        = option_defs_us[i].key;
                const char *desc                       = option_defs_us[i].desc;
                const char *default_value              = option_defs_us[i].default_value;
                struct retro_core_option_value *values  = option_defs_us[i].values;
                size_t buf_len                         = 3;
                size_t default_index                   = 0;

                values_buf[i] = NULL;

                if (desc)
                {
                    size_t num_values = 0;

                    while (true)
                    {
                        if (values[num_values].value)
                        {
                            if (default_value)
                                if (strcmp(values[num_values].value, default_value) == 0)
                                    default_index = num_values;

                            buf_len += strlen(values[num_values].value);
                            num_values++;
                        }
                        else
                            break;
                    }

                    if (num_values > 0)
                    {
                        buf_len += num_values - 1;
                        buf_len += strlen(desc);

                        values_buf[i] = (char *)calloc(buf_len, sizeof(char));
                        if (!values_buf[i])
                            goto error;

                        strcpy(values_buf[i], desc);
                        strcat(values_buf[i], "; ");

                        strcat(values_buf[i], values[default_index].value);

                        for (j = 0; j < num_values; j++)
                        {
                            if (j != default_index)
                            {
                                strcat(values_buf[i], "|");
                                strcat(values_buf[i], values[j].value);
                            }
                        }
                    }
                }

                variables[option_index].key   = key;
                variables[option_index].value = values_buf[i];
                option_index++;
            }

            environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);
        }

error:
        if (option_v1_defs_us)
        {
            free(option_v1_defs_us);
            option_v1_defs_us = NULL;
        }

        if (values_buf)
        {
            for (i = 0; i < num_options; i++)
            {
                if (values_buf[i])
                {
                    free(values_buf[i]);
                    values_buf[i] = NULL;
                }
            }

            free(values_buf);
            values_buf = NULL;
        }

        if (variables)
        {
            free(variables);
            variables = NULL;
        }
    }
}

#ifdef __cplusplus
}
#endif

#endif
