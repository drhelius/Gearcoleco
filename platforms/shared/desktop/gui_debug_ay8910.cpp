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

#define GUI_DEBUG_AY8910_IMPORT
#include "gui_debug_ay8910.h"

#include "imgui.h"
#include "implot.h"
#include "fonts/IconsMaterialDesign.h"
#include "gearcoleco.h"
#include "gui.h"
#include "gui_debug_constants.h"
#include "config.h"
#include "emu.h"
#include "utils.h"

static const char* k_env_shape_names[16] = {
    "\\___", "\\___", "\\___", "\\___",
    "/___", "/___", "/___", "/___",
    "\\\\\\\\", "\\___ ", "\\/\\/", "\\---",
    "////", "/---", "/\\/\\", "/___"
};

static bool exclusive_channel[3] = { false, false, false };
static float* wave_buffer = NULL;

static void draw_channel_scope(Audio* audio, AY8910* ay, int channel);
static void draw_frequency(float freq_hz);

void gui_debug_ay8910_init(void)
{
    wave_buffer = new float[GC_AUDIO_BUFFER_SIZE];
}

void gui_debug_ay8910_destroy(void)
{
    SafeDeleteArray(wave_buffer);
}

void gui_debug_window_ay8910(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(500, 45), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(360, 430), ImGuiCond_FirstUseEver);
    ImGui::Begin("AY-3-8910 (SGM)", &config_debug.show_ay8910);

    GearcolecoCore* core = emu_get_core();
    Audio* audio = core->GetAudio();
    AY8910* ay = audio->GetAY8910();

    const u8* regs = ay->GetRegisters();
    const u16* tone_periods = ay->GetTonePeriods();
    const u8* amplitudes = ay->GetAmplitudes();
    const bool* tone_disable = ay->GetToneDisable();
    const bool* noise_disable = ay->GetNoiseDisable();
    const bool* envelope_mode = ay->GetEnvelopeMode();
    int clock_rate = ay->GetClockRate();

    if (ImGui::BeginTabBar("##ay_tabs", ImGuiTabBarFlags_None))
    {
        for (int c = 0; c < 3; c++)
        {
            char tab_name[32];
            snprintf(tab_name, sizeof(tab_name), "%c", 'A' + c);

            if (ImGui::BeginTabItem(tab_name))
            {
                ImGui::PushFont(gui_default_font);

                draw_channel_scope(audio, ay, c);

                ImGui::Separator();

                u16 period = tone_periods[c];
                u8 amp = amplitudes[c];
                bool tone_enabled = !tone_disable[c];
                bool noise_enabled = !noise_disable[c];
                bool env = envelope_mode[c];

                ImGui::TextColored(violet, "TONE        "); ImGui::SameLine();
                ImGui::TextColored(tone_enabled ? green : gray, "%s", tone_enabled ? "ON " : "OFF");

                ImGui::TextColored(violet, "NOISE       "); ImGui::SameLine();
                ImGui::TextColored(noise_enabled ? green : gray, "%s", noise_enabled ? "ON " : "OFF");

                ImGui::TextColored(violet, "PERIOD      "); ImGui::SameLine();
                ImGui::TextColored(white, "$%03X (%d)", period, period);

                ImGui::TextColored(violet, "FREQ REG LO "); ImGui::SameLine();
                ImGui::TextColored(white, "$%02X", regs[c * 2]);

                ImGui::TextColored(violet, "FREQ REG HI "); ImGui::SameLine();
                ImGui::TextColored(white, "$%02X", regs[c * 2 + 1] & 0x0F);

                ImGui::TextColored(violet, "OUTPUT HZ   "); ImGui::SameLine();
                if (period > 0 && clock_rate > 0)
                    draw_frequency((float)clock_rate / (16.0f * period));
                else
                    ImGui::TextColored(gray, "DC");

                ImGui::Separator();

                ImGui::TextColored(violet, "AMPLITUDE   "); ImGui::SameLine();
                if (env)
                {
                    ImGui::TextColored(orange, "ENV (%d)", ay->GetEnvelopeVolume());
                }
                else
                {
                    ImGui::TextColored(amp == 0 ? gray : white, "$%X", amp); ImGui::SameLine();
                    if (amp == 0)
                        ImGui::TextColored(gray, " (OFF)");
                    else if (amp == 15)
                        ImGui::TextColored(green, " (MAX)");
                    else
                        ImGui::TextColored(gray, " (%d/15)", amp);
                }

                ImGui::TextColored(violet, "ENV MODE    "); ImGui::SameLine();
                ImGui::TextColored(env ? green : gray, "%s", env ? "ON" : "OFF");

                ImGui::TextColored(violet, "AMP REG     "); ImGui::SameLine();
                ImGui::TextColored(white, "$%02X", regs[8 + c]);

                ImGui::PopFont();
                ImGui::EndTabItem();
            }
        }

        if (ImGui::BeginTabItem("Noise"))
        {
            ImGui::PushFont(gui_default_font);

            u8 noise_period = ay->GetNoisePeriod();
            u32 noise_shift = ay->GetNoiseShift();

            ImGui::TextColored(violet, "PERIOD      "); ImGui::SameLine();
            ImGui::TextColored(white, "$%02X (%d)", noise_period, noise_period);

            ImGui::TextColored(violet, "PERIOD REG  "); ImGui::SameLine();
            ImGui::TextColored(white, "$%02X", regs[6]);

            ImGui::TextColored(violet, "LFSR        "); ImGui::SameLine();
            ImGui::TextColored(white, "$%05X", noise_shift & 0x1FFFF);

            ImGui::TextColored(violet, "OUTPUT HZ   "); ImGui::SameLine();
            if (noise_period > 0 && clock_rate > 0)
                draw_frequency((float)clock_rate / (16.0f * noise_period));
            else
                ImGui::TextColored(gray, "DC");

            ImGui::NewLine();

            ImGui::TextColored(brown, "MIXER (R7)");
            ImGui::Separator();
            u8 mixer = regs[7];
            ImGui::TextColored(violet, "VALUE       "); ImGui::SameLine();
            ImGui::Text("$%02X  ", mixer); ImGui::SameLine(0, 0);
            ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(mixer));

            for (int i = 0; i < 3; i++)
            {
                ImGui::TextColored(violet, "CH %c        ", 'A' + i); ImGui::SameLine();
                ImGui::TextColored(tone_disable[i] ? gray : green, "T:%s", tone_disable[i] ? "OFF" : "ON "); ImGui::SameLine();
                ImGui::TextColored(noise_disable[i] ? gray : green, " N:%s", noise_disable[i] ? "OFF" : "ON ");
            }

            ImGui::PopFont();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Envelope"))
        {
            ImGui::PushFont(gui_default_font);

            u16 env_period = ay->GetEnvelopePeriod();
            u8 env_step = ay->GetEnvelopeStep();
            u8 env_volume = ay->GetEnvelopeVolume();
            u8 env_shape = regs[13] & 0x0F;

            ImGui::TextColored(violet, "PERIOD      "); ImGui::SameLine();
            ImGui::TextColored(white, "$%04X (%d)", env_period, env_period);

            ImGui::TextColored(violet, "PERIOD LOW  "); ImGui::SameLine();
            ImGui::TextColored(white, "$%02X", regs[11]);

            ImGui::TextColored(violet, "PERIOD HIGH "); ImGui::SameLine();
            ImGui::TextColored(white, "$%02X", regs[12]);

            ImGui::TextColored(violet, "SHAPE       "); ImGui::SameLine();
            ImGui::TextColored(white, "$%X", env_shape); ImGui::SameLine();
            ImGui::TextColored(orange, " %s", k_env_shape_names[env_shape]);

            ImGui::TextColored(violet, "STEP        "); ImGui::SameLine();
            ImGui::TextColored(white, "%d / 15", env_step);

            ImGui::TextColored(violet, "VOLUME      "); ImGui::SameLine();
            ImGui::TextColored(env_volume == 0 ? gray : cyan, "%d", env_volume);

            ImGui::TextColored(violet, "OUTPUT HZ   "); ImGui::SameLine();
            if (env_period > 0 && clock_rate > 0)
                draw_frequency((float)clock_rate / (256.0f * env_period));
            else
                ImGui::TextColored(gray, "DC");

            ImGui::NewLine();

            ImGui::TextColored(brown, "CHANNELS USING ENVELOPE");
            ImGui::Separator();

            for (int i = 0; i < 3; i++)
            {
                ImGui::TextColored(violet, "CH %c        ", 'A' + i); ImGui::SameLine();
                ImGui::TextColored(envelope_mode[i] ? green : gray, "%s", envelope_mode[i] ? "YES" : "NO");
            }

            ImGui::PopFont();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Registers"))
        {
            ImGui::PushFont(gui_default_font);

            ImGui::TextColored(violet, "SELECTED    "); ImGui::SameLine();
            ImGui::TextColored(white, "$%X", ay->GetSelectedRegister());

            ImGui::Separator();

            const char* reg_names[16] = {
                "TONE A FINE  ", "TONE A COARSE", "TONE B FINE  ", "TONE B COARSE",
                "TONE C FINE  ", "TONE C COARSE", "NOISE PERIOD ", "MIXER CONTROL",
                "AMP A        ", "AMP B        ", "AMP C        ", "ENV FINE     ",
                "ENV COARSE   ", "ENV SHAPE    ", "I/O PORT A   ", "I/O PORT B   "
            };

            for (int i = 0; i < 16; i++)
            {
                u8 reg_val = regs[i];
                ImGui::TextColored(cyan, " R%X ", i); ImGui::SameLine();
                ImGui::TextColored(violet, "%s ", reg_names[i]); ImGui::SameLine();
                ImGui::Text("$%02X  ", reg_val); ImGui::SameLine(0, 0);
                ImGui::TextColored(gray, "(" BYTE_TO_BINARY_PATTERN_SPACED ")", BYTE_TO_BINARY(reg_val));
            }

            ImGui::PopFont();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

static void draw_channel_scope(Audio* audio, AY8910* ay, int channel)
{
    bool* mute = ay->GetChannelMute(channel);
    if (!mute)
        return;

    if (ImGui::BeginTable("##audio", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX))
    {
        ImGui::TableNextColumn();

        ImGui::PushStyleColor(ImGuiCol_Text, *mute ? mid_gray : white);
        ImGui::PushFont(gui_material_icons_font);

        char label[32];
        snprintf(label, sizeof(label), "%s##ay_mute%d", *mute ? ICON_MD_MUSIC_OFF : ICON_MD_MUSIC_NOTE, channel);
        if (ImGui::Button(label))
        {
            for (int i = 0; i < 3; i++)
                exclusive_channel[i] = false;
            *mute = !*mute;
        }
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Mute Channel");

        snprintf(label, sizeof(label), "%s##ay_exc%d", ICON_MD_STAR, channel);

        ImGui::PushStyleColor(ImGuiCol_Text, exclusive_channel[channel] ? yellow : white);
        if (ImGui::Button(label))
        {
            exclusive_channel[channel] = !exclusive_channel[channel];
            *mute = false;
            for (int i = 0; i < 3; i++)
            {
                if (i != channel)
                {
                    exclusive_channel[i] = false;
                    bool* other_mute = ay->GetChannelMute(i);
                    if (other_mute)
                        *other_mute = exclusive_channel[channel] ? true : false;
                }
            }
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Solo Channel");
        ImGui::PopFont();
        ImGui::PopStyleColor();

        ImGui::TableNextColumn();

        ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(1, 1));

        s16* ch_buf = audio->GetAY8910DebugChannelBuffer(channel);
        int data_size = audio->GetAY8910DebugChannelSamples(channel);

        if (ch_buf && data_size > 0)
        {
            for (int i = 0; i < data_size; i++)
                wave_buffer[i] = (float)ch_buf[i] / 4096.0f;
        }
        else
        {
            data_size = 1;
            wave_buffer[0] = 0.0f;
        }

        int trigger = 0;
        for (int i = 1; i < data_size; ++i)
        {
            if (wave_buffer[i - 1] <= 0.01f && wave_buffer[i] > 0.01f)
            {
                trigger = i;
                break;
            }
        }

        int half_window_size = 100;
        int x_min = MAX(0, trigger - half_window_size);
        int x_max = MIN(data_size, trigger + half_window_size);

        ImPlotAxisFlags flags = ImPlotAxisFlags_NoGridLines | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoHighlight | ImPlotAxisFlags_Lock | ImPlotAxisFlags_NoTickMarks;

        if (ImPlot::BeginPlot("Waveform", ImVec2(200, 50), ImPlotFlags_CanvasOnly))
        {
            ImPlot::SetupAxes("x", "y", flags, flags);
            ImPlot::SetupAxesLimits(x_min, x_max, -0.05f, 1.11f, ImPlotCond_Always);
            ImPlot::SetNextLineStyle(green, 1.0f);
            ImPlot::PlotLine("Wave", wave_buffer, data_size);
            ImPlot::EndPlot();
        }

        ImPlot::PopStyleVar();

        ImGui::EndTable();
    }
}

static void draw_frequency(float freq_hz)
{
    if (freq_hz >= 1000.0f)
        ImGui::TextColored(cyan, "%.2f KHz", freq_hz / 1000.0f);
    else
        ImGui::TextColored(cyan, "%.4f Hz", freq_hz);
}