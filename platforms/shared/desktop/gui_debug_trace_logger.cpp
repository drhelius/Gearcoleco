#define GUI_DEBUG_TRACE_LOGGER_IMPORT
#include "gui_debug_trace_logger.h"

#include "imgui.h"
#include "gui.h"
#include "gui_filedialogs.h"
#include "gui_debug_constants.h"
#include "gui_debug_text.h"
#include "config.h"
#include "emu.h"
#include "gui.h"
#include "log.h"
#include "utils.h"
#include "trace_logger_formatter.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static bool trace_enabled = false;
static int trace_output = 0;
static FILE* trace_file = NULL;
static char trace_file_buffer[1024 * 1024];
static char trace_file_path[1024];
static char trace_logger_disk_directory[4096] = {};
static u64 trace_disk_limit = 0;
static u64 trace_disk_bytes = 0;
static u64 trace_disk_entries = 0;
static u64 trace_disk_next = 0;
static u64 trace_last_flush = 0;
static bool trace_draining = false;
static bool trace_disk_error = false;
static bool trace_follow_latest = true;
static bool trace_scroll_to_bottom = false;
static bool trace_wait_for_scroll_away = false;
static bool trace_choose_output_path = false;
static const GC_Trace_Entry* trace_previous = NULL;
static GC_Trace_Entry trace_disk_previous = {};
static bool trace_disk_previous_valid = false;
static const u32 trace_capacities[] = {100000, 500000, 1000000, 2000000, 5000000};
static const char* const trace_capacity_labels[] = {"100K (10 MB)", "500K (50 MB)", "1M (100 MB)", "2M (200 MB)", "5M (500 MB)"};
static const u64 trace_limits[] = {10ULL << 20, 50ULL << 20, 100ULL << 20,
    250ULL << 20, 500ULL << 20, 1024ULL << 20, 0};

static void trace_logger_menu(void);
static void trace_logger_sync_flags(void);
static u32 trace_logger_get_config_flags(void);
static void trace_logger_set_config_flags(u32 flags);
static u32 trace_logger_get_config_event_filter(GC_Trace_Type type);
static void trace_logger_set_config_event_filter(GC_Trace_Type type, u32 filter);
static bool trace_logger_apply_capacity(void);
static bool trace_logger_start_disk(void);
static bool trace_logger_start(u32 flags, bool update_config);
static bool trace_logger_stop(bool show_status);
static bool trace_logger_stop_disk(bool show_status, bool flush_entries);
static bool trace_logger_flush_disk_entries(void);

static const char* trace_directory(void)
{
    if (config_debug.trace_disk_dir_option == 1)
    {
        const char* path = emu_get_core()->GetCartridge()->GetFileDirectory();
        if (path && path[0])
            return path;
    }
    if (config_debug.trace_disk_dir_option == 2 && !config_debug.trace_disk_path.empty())
        return config_debug.trace_disk_path.c_str();
    return config_root_path;
}

static bool trace_path_exists(const char* path)
{
    FILE* file = fopen_utf8(path, "rb");
    if (!file)
        return false;
    fclose(file);
    return true;
}

static bool trace_open_file(const char* directory, char* error, size_t error_size)
{
    const char* name = emu_get_core()->GetCartridge()->GetFileName();
    if (!name || !name[0])
        name = "Gearcoleco";
    char base[512];
    snprintf(base, sizeof(base), "%s", name);
    char* extension = strrchr(base, '.');
    if (extension)
        *extension = 0;

    time_t now = time(NULL);
    struct tm local;
    if (!get_local_time(now, &local))
    {
        snprintf(error, error_size, "Unable to read local time");
        return false;
    }
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H%M%S", &local);

    for (int suffix = 1; suffix <= 1000; suffix++)
    {
        if (suffix == 1)
            snprintf(trace_file_path, sizeof(trace_file_path), "%s/%s - Trace - %s.txt", directory, base, timestamp);
        else
            snprintf(trace_file_path, sizeof(trace_file_path), "%s/%s - Trace - %s (%d).txt", directory, base, timestamp, suffix);
        if (!trace_path_exists(trace_file_path))
            break;
        if (suffix == 1000)
        {
            snprintf(error, error_size, "Unable to create a unique trace file name");
            return false;
        }
    }

    trace_file = fopen_utf8(trace_file_path, "wb");
    if (!trace_file)
    {
        snprintf(error, error_size, "Unable to open trace file: %s", strerror(errno));
        trace_file_path[0] = 0;
        return false;
    }
    setvbuf(trace_file, trace_file_buffer, _IOFBF, sizeof(trace_file_buffer));
    return true;
}

void gui_debug_trace_logger_init(void)
{
    trace_enabled = false;
    trace_output = 0;
    trace_file = NULL;
    trace_file_path[0] = 0;
    strncpy_fit(trace_logger_disk_directory, config_debug.trace_disk_path.c_str(), sizeof(trace_logger_disk_directory));
    if (!trace_logger_apply_capacity())
    {
        config_debug.trace_capacity = 0;
        trace_logger_apply_capacity();
    }
}

bool gui_debug_trace_logger_configure(int output, int memory_size, int disk_size,
    const char* output_path)
{
    if (trace_enabled)
        return false;
    if (output < gui_TraceOutput_Memory || output > gui_TraceOutput_Disk)
        return false;
    if (memory_size < 0 || memory_size >= (int)(sizeof(trace_capacities) / sizeof(trace_capacities[0])))
        return false;
    if (disk_size < 0 || disk_size >= (int)(sizeof(trace_limits) / sizeof(trace_limits[0])))
        return false;

    int old_output = config_debug.trace_output;
    int old_capacity = config_debug.trace_capacity;
    int old_disk_size = config_debug.trace_disk_size;
    int old_dir_option = config_debug.trace_disk_dir_option;
    std::string old_path = config_debug.trace_disk_path;

    config_debug.trace_output = output;
    config_debug.trace_capacity = memory_size;
    config_debug.trace_disk_size = disk_size;
    if (output == gui_TraceOutput_Disk && output_path && output_path[0])
    {
        config_debug.trace_disk_dir_option = 2;
        gui_debug_trace_logger_set_output_directory(output_path);
    }

    if (!trace_logger_apply_capacity())
    {
        config_debug.trace_output = old_output;
        config_debug.trace_capacity = old_capacity;
        config_debug.trace_disk_size = old_disk_size;
        config_debug.trace_disk_dir_option = old_dir_option;
        config_debug.trace_disk_path = old_path;
        strncpy_fit(trace_logger_disk_directory, old_path.c_str(), sizeof(trace_logger_disk_directory));
        return false;
    }
    return true;
}

bool gui_debug_trace_logger_start(u32 flags)
{
    return trace_logger_start(flags, true);
}

static bool trace_logger_start(u32 flags, bool update_config)
{
    if (update_config)
        trace_logger_set_config_flags(flags);

    if (trace_enabled)
    {
        trace_logger_sync_flags();
        return true;
    }

    trace_output = config_debug.trace_output;
    if (trace_output == gui_TraceOutput_Disk)
    {
        if (!trace_logger_start_disk())
            return false;
    }
    else if (!trace_logger_apply_capacity())
        return false;

    trace_enabled = true;
    trace_follow_latest = true;
    trace_scroll_to_bottom = true;
    trace_wait_for_scroll_away = false;
    trace_logger_sync_flags();
    return true;
}

static bool trace_logger_start_disk(void)
{
    if (!trace_logger_apply_capacity())
        return false;

    char error[256] = {};
    const char* path = trace_directory();
    if (!path || !path[0] || !trace_open_file(path, error, sizeof(error)))
    {
        gui_set_error_message(error[0] ? error : "Unable to create the trace log file.");
        Error("Unable to start trace disk output: %s", error);
        return false;
    }

    TraceLogger* logger = emu_get_core()->GetTraceLogger();
    logger->Reset();
    trace_disk_limit = trace_limits[config_debug.trace_disk_size];
    trace_disk_bytes = 0;
    trace_disk_entries = 0;
    trace_disk_next = 0;
    trace_last_flush = SDL_GetTicks();
    trace_disk_error = false;
    trace_disk_previous_valid = false;
    gui_set_status_message("Trace recording started", 3000);
    return true;
}

static bool trace_logger_flush_disk_entries(void)
{
    if (!trace_file)
        return false;

    TraceLogger* logger = emu_get_core()->GetTraceLogger();
    u64 total = logger->GetTotalLogged();
    u64 oldest = total - logger->GetCount();
    if (trace_disk_next < oldest)
    {
        trace_disk_error = true;
        return false;
    }

    while (trace_disk_next < total)
    {
        u32 index = (u32)(trace_disk_next - oldest);
        const GC_Trace_Entry& entry = logger->GetEntry(index);
        GC_Trace_Format_Options options = {config_debug.trace_bank,
            config_debug.trace_registers, config_debug.trace_flags,
            config_debug.trace_bytes, config_debug.trace_cycles,
            index > 0 ? &logger->GetEntry(index - 1) :
            (trace_disk_previous_valid ? &trace_disk_previous : NULL)};
        char text[GC_TRACE_FORMAT_BUFFER_SIZE];
        char line[GC_TRACE_FORMAT_BUFFER_SIZE + 64];
        trace_logger_format_entry(entry, options, text, sizeof(text));
        int length;
        if (config_debug.trace_counter)
            length = snprintf(line, sizeof(line), "%012llu %s\n",
                (unsigned long long)trace_disk_entries, text);
        else
            length = snprintf(line, sizeof(line), "%s\n", text);
        if (length < 0)
        {
            trace_disk_error = true;
            return false;
        }

        size_t line_size = MIN((size_t)length, sizeof(line) - 1);
        if (trace_disk_limit && trace_disk_bytes + line_size > trace_disk_limit)
        {
            trace_enabled = false;
            break;
        }
        if (fwrite(line, 1, line_size, trace_file) != line_size)
        {
            trace_disk_error = true;
            return false;
        }
        trace_disk_bytes += line_size;
        trace_disk_entries++;
        trace_disk_next++;
        trace_disk_previous = entry;
        trace_disk_previous_valid = true;
    }
    return true;
}

void gui_debug_trace_logger_update(void)
{
    if (!trace_enabled || trace_output != 1 || !trace_file || trace_draining)
        return;
    trace_draining = true;
    bool success = trace_logger_flush_disk_entries();

    u64 now = SDL_GetTicks();
    if (success && trace_enabled && now - trace_last_flush >= 1000)
    {
        if (fflush(trace_file) != 0)
            success = false;
        trace_last_flush = now;
    }
    trace_draining = false;
    if (!success)
    {
        trace_disk_error = true;
        trace_logger_stop_disk(true, false);
    }
    else if (!trace_enabled)
    {
        trace_logger_stop_disk(false, false);
        gui_set_status_message("Trace recording stopped: maximum file size reached", 4000);
    }
}

bool gui_debug_trace_logger_stop(void)
{
    return trace_logger_stop(true);
}

static bool trace_logger_stop(bool show_status)
{
    if (!trace_enabled && !trace_file)
        return true;

    trace_scroll_to_bottom = trace_follow_latest;

    if (trace_output == gui_TraceOutput_Disk)
        return trace_logger_stop_disk(show_status, true);

    trace_enabled = false;
    emu_get_core()->GetTraceLogger()->SetEnabledFlags(0);
    return true;
}

static bool trace_logger_stop_disk(bool show_status, bool flush_entries)
{
    bool success = trace_file != NULL;
    if (trace_file)
    {
        if (flush_entries && !trace_logger_flush_disk_entries())
            success = false;
        if (fflush(trace_file) != 0)
            success = false;
        if (fclose(trace_file) != 0)
            success = false;
        trace_file = NULL;
    }

    success = success && !trace_disk_error;
    trace_enabled = false;
    if (emu_get_core())
        emu_get_core()->GetTraceLogger()->SetEnabledFlags(0);
    if (success)
    {
        if (show_status)
            gui_set_status_message("Trace recording stopped", 3000);
    }
    else
    {
        gui_set_error_message("Trace recording stopped with a disk write, flush, or close error.");
        Error("Trace recording stopped with a disk write, flush, or close error: %s", trace_file_path);
    }
    trace_disk_error = false;
    return success;
}

void gui_debug_trace_logger_shutdown(void)
{
    trace_logger_stop(false);
}

void gui_debug_trace_logger_reset(void)
{
    trace_logger_stop(false);
    if (emu_get_core())
        emu_get_core()->GetTraceLogger()->Reset();
}

void gui_debug_trace_logger_set_output_directory(const char* path)
{
    strncpy_fit(trace_logger_disk_directory, path, sizeof(trace_logger_disk_directory));
    config_debug.trace_disk_path.assign(path);
}

int gui_debug_trace_logger_memory_size_index(const char* size)
{
    static const char* names[] = {"100K", "500K", "1M", "2M", "5M"};
    if (size)
    {
        for (int i = 0; i < (int)(sizeof(names) / sizeof(names[0])); i++)
        {
            if (strcmp(size, names[i]) == 0)
                return i;
        }
    }
    return -1;
}

int gui_debug_trace_logger_disk_size_index(const char* size)
{
    static const char* names[] = {"10MB", "50MB", "100MB", "250MB", "500MB", "1GB", "unbounded"};
    if (size)
    {
        for (int i = 0; i < (int)(sizeof(names) / sizeof(names[0])); i++)
        {
            if (strcmp(size, names[i]) == 0)
                return i;
        }
    }
    return -1;
}

const char* gui_debug_trace_logger_memory_size_name(int index)
{
    static const char* names[] = {"100K", "500K", "1M", "2M", "5M"};
    if (index < 0 || index >= (int)(sizeof(names) / sizeof(names[0])))
        index = 0;
    return names[index];
}

const char* gui_debug_trace_logger_disk_size_name(int index)
{
    static const char* names[] = {"10MB", "50MB", "100MB", "250MB", "500MB", "1GB", "unbounded"};
    if (index < 0 || index >= (int)(sizeof(names) / sizeof(names[0])))
        index = 2;
    return names[index];
}

void gui_debug_trace_logger_set_event_filters(const u32* filters)
{
    if (!filters)
        return;
    for (int i = 0; i < TRACE_TYPE_COUNT; i++)
        trace_logger_set_config_event_filter((GC_Trace_Type)i, filters[i]);
}

bool gui_debug_trace_logger_is_enabled(void) { return trace_enabled; }
const char* gui_debug_trace_logger_get_output_path(void) { return trace_file_path; }

void gui_debug_trace_logger_clear(void)
{
    TraceLogger* logger = emu_get_core()->GetTraceLogger();
    if (trace_enabled && trace_output == 1)
        gui_debug_trace_logger_update();
    logger->Reset();
    trace_previous = NULL;
    if (trace_enabled && trace_output == 1)
    {
        trace_disk_next = logger->GetTotalLogged();
        trace_disk_previous_valid = false;
    }
}

static void format_entry_text(const GC_Trace_Entry& entry, bool cycles,
    const GC_Trace_Entry* previous, char* buffer, size_t size)
{
    GC_Trace_Format_Options options = {config_debug.trace_bank,
        config_debug.trace_registers, config_debug.trace_flags,
        config_debug.trace_bytes, cycles, previous};
    trace_logger_format_entry(entry, options, buffer, size);
}

static void format_entry_text(const GC_Trace_Entry& entry, char* buffer, size_t size)
{
    format_entry_text(entry, config_debug.trace_cycles, trace_previous, buffer, size);
}

void gui_debug_save_log(const char* file_path)
{
    FILE* file = fopen_utf8(file_path, "w");
    if (!file)
    {
        Log("Unable to open trace log for writing: %s", file_path);
        return;
    }
    TraceLogger* logger = emu_get_core()->GetTraceLogger();
    u32 count = logger->GetCount();
    char buffer[GC_TRACE_FORMAT_BUFFER_SIZE];
    bool success = true;
    for (u32 i = 0; i < count; i++)
    {
        const GC_Trace_Entry& entry = logger->GetEntry(i);
        trace_previous = i > 0 ? &logger->GetEntry(i - 1) : NULL;
        format_entry_text(entry, buffer, sizeof(buffer));
        if (config_debug.trace_counter)
            success = fprintf(file, "%012llu %s\n", (unsigned long long)(logger->GetSequence() - count + i), buffer) >= 0;
        else
            success = fprintf(file, "%s\n", buffer) >= 0;
        if (!success)
            break;
    }
    if (fclose(file) != 0)
        success = false;
    if (!success)
        Log("Unable to write trace log: %s", file_path);
}

static void trace_logger_menu_event_filter(const char* label, int* filter, u32 mask)
{
    bool enabled = ((u32)*filter & mask) != 0;
    if (ImGui::MenuItem(label, "", &enabled))
    {
        if (enabled)
            *filter |= (int)mask;
        else
            *filter &= ~(int)mask;
    }
}

static void trace_submenu(const char* label, bool* enabled, int* events,
    const char* const* names, const u32* masks, int count)
{
    if (!ImGui::BeginMenu(label))
        return;
    ImGui::MenuItem("Enabled", "", enabled);
    ImGui::Separator();
    ImGui::BeginDisabled(!*enabled);
    for (int i = 0; i < count; i++)
        trace_logger_menu_event_filter(names[i], events, masks[i]);
    ImGui::EndDisabled();
    ImGui::EndMenu();
}

static void trace_logger_menu(void)
{
    ImGui::BeginMenuBar();

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Save Log As...", NULL, false, config_debug.trace_output == gui_TraceOutput_Memory))
        {
            gui_file_dialog_save_log();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Settings"))
    {
        ImGui::MenuItem("Event Counter", "", &config_debug.trace_counter);
        ImGui::MenuItem("Master Clock Cycles", "", &config_debug.trace_cycles);

        if (ImGui::BeginMenu("CPU"))
        {
            ImGui::MenuItem("Bank Number", "", &config_debug.trace_bank);
            ImGui::MenuItem("Registers", "", &config_debug.trace_registers);
            ImGui::MenuItem("Flags", "", &config_debug.trace_flags);
            ImGui::MenuItem("Bytes", "", &config_debug.trace_bytes);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Disk Output"))
        {
            ImGui::BeginDisabled(trace_enabled);
            ImGui::SetNextItemWidth(180.0f);
            ImGui::Combo("##trace_disk_dir", &config_debug.trace_disk_dir_option, "Default Location\0Same as ROM\0Custom Location\0\0");

            switch ((Directory_Location)config_debug.trace_disk_dir_option)
            {
                default:
                case Directory_Location_Default:
                    ImGui::Text("%s", config_root_path);
                    break;
                case Directory_Location_ROM:
                    if (!emu_is_empty())
                        ImGui::Text("%s", emu_get_core()->GetCartridge()->GetFileDirectory());
                    break;
                case Directory_Location_Custom:
                    if (ImGui::MenuItem("Choose..."))
                        trace_choose_output_path = true;
                    ImGui::PushItemWidth(450.0f);
                    if (ImGui::InputText("##trace_disk_path", trace_logger_disk_directory, sizeof(trace_logger_disk_directory), ImGuiInputTextFlags_AutoSelectAll))
                        config_debug.trace_disk_path.assign(trace_logger_disk_directory);
                    ImGui::PopItemWidth();
                    break;
            }
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Filters"))
    {
        if (ImGui::BeginMenu("CPU"))
        {
            ImGui::MenuItem("Enabled", "", &config_debug.trace_cpu_enabled);
            ImGui::Separator();
            ImGui::BeginDisabled(!config_debug.trace_cpu_enabled);
            ImGui::MenuItem("Instructions", "", &config_debug.trace_cpu);
            ImGui::MenuItem("IRQs", "", &config_debug.trace_cpu_irq);
            ImGui::EndDisabled();
            ImGui::EndMenu();
        }

        const char* vdp_names[] = {"Registers", "Interrupts", "Status", "Sprites", "Timing", "VRAM"};
        const u32 vdp_masks[] = {TRACE_VDP_EVENT_REGISTERS, TRACE_VDP_EVENT_INTERRUPTS,
            TRACE_VDP_EVENT_STATUS, TRACE_VDP_EVENT_SPRITES, TRACE_VDP_EVENT_TIMING, TRACE_VDP_EVENT_VRAM};
        trace_submenu("VDP", &config_debug.trace_vdp, &config_debug.trace_vdp_events, vdp_names, vdp_masks, 6);
        const char* psg_names[] = {"Tone", "Volume", "Noise"};
        const u32 psg_masks[] = {TRACE_PSG_EVENT_TONE, TRACE_PSG_EVENT_VOLUME, TRACE_PSG_EVENT_NOISE};
        trace_submenu("PSG", &config_debug.trace_psg, &config_debug.trace_psg_events, psg_names, psg_masks, 3);
        const char* ay_names[] = {"Registers", "Tone", "Noise / Mixer", "Volume", "Envelope", "I/O"};
        const u32 ay_masks[] = {TRACE_AY8910_EVENT_REGISTERS, TRACE_AY8910_EVENT_TONE,
            TRACE_AY8910_EVENT_NOISE_MIXER, TRACE_AY8910_EVENT_VOLUME,
            TRACE_AY8910_EVENT_ENVELOPE, TRACE_AY8910_EVENT_IO};
        trace_submenu("AY-3-8910", &config_debug.trace_ay8910, &config_debug.trace_ay8910_events, ay_names, ay_masks, 6);
        const char* rw_names[] = {"Reads", "Writes"};
        const u32 io_masks[] = {TRACE_IO_EVENT_READS, TRACE_IO_EVENT_WRITES};
        trace_submenu("I/O Ports", &config_debug.trace_io, &config_debug.trace_io_events, rw_names, io_masks, 2);
        const u32 input_masks[] = {TRACE_INPUT_EVENT_READS, TRACE_INPUT_EVENT_WRITES};
        trace_submenu("Input", &config_debug.trace_input, &config_debug.trace_input_events, rw_names, input_masks, 2);
        const char* sgm_names[] = {"Control"};
        const u32 sgm_masks[] = {TRACE_SGM_EVENT_CONTROL};
        trace_submenu("SGM", &config_debug.trace_sgm, &config_debug.trace_sgm_events, sgm_names, sgm_masks, 1);
        const char* map_names[] = {"Banks", "EEPROM", "SRAM"};
        const u32 map_masks[] = {TRACE_MAPPER_EVENT_BANKS, TRACE_MAPPER_EVENT_EEPROM, TRACE_MAPPER_EVENT_SRAM};
        trace_submenu("Mapper", &config_debug.trace_mapper, &config_debug.trace_mapper_events, map_names, map_masks, 3);

        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}

static void trace_logger_sync_flags(void)
{
    u32 flags = trace_logger_get_config_flags();
    TraceLogger* logger = emu_get_core()->GetTraceLogger();
    for (int i = 0; i < TRACE_TYPE_COUNT; i++)
        logger->SetEventFilter((GC_Trace_Type)i, trace_logger_get_config_event_filter((GC_Trace_Type)i));
    logger->SetEnabledFlags(flags);
}

static u32 trace_logger_get_config_flags(void)
{
    u32 flags = 0;
    if (config_debug.trace_cpu_enabled && config_debug.trace_cpu) flags |= TRACE_FLAG_CPU;
    if (config_debug.trace_cpu_enabled && config_debug.trace_cpu_irq) flags |= TRACE_FLAG_CPU_IRQ;
    if (config_debug.trace_vdp) flags |= TRACE_FLAG_VDP;
    if (config_debug.trace_psg) flags |= TRACE_FLAG_PSG;
    if (config_debug.trace_ay8910) flags |= TRACE_FLAG_AY8910;
    if (config_debug.trace_io) flags |= TRACE_FLAG_IO;
    if (config_debug.trace_input) flags |= TRACE_FLAG_INPUT;
    if (config_debug.trace_sgm) flags |= TRACE_FLAG_SGM;
    if (config_debug.trace_mapper) flags |= TRACE_FLAG_MAPPER;
    return flags;
}

static void trace_logger_set_config_flags(u32 flags)
{
    config_debug.trace_cpu_enabled = (flags & (TRACE_FLAG_CPU | TRACE_FLAG_CPU_IRQ)) != 0;
    config_debug.trace_cpu = (flags & TRACE_FLAG_CPU) != 0;
    config_debug.trace_cpu_irq = (flags & TRACE_FLAG_CPU_IRQ) != 0;
    config_debug.trace_vdp = (flags & TRACE_FLAG_VDP) != 0;
    config_debug.trace_psg = (flags & TRACE_FLAG_PSG) != 0;
    config_debug.trace_ay8910 = (flags & TRACE_FLAG_AY8910) != 0;
    config_debug.trace_io = (flags & TRACE_FLAG_IO) != 0;
    config_debug.trace_input = (flags & TRACE_FLAG_INPUT) != 0;
    config_debug.trace_sgm = (flags & TRACE_FLAG_SGM) != 0;
    config_debug.trace_mapper = (flags & TRACE_FLAG_MAPPER) != 0;
}

static u32 trace_logger_get_config_event_filter(GC_Trace_Type type)
{
    switch (type)
    {
        case TRACE_VDP: return (u32)config_debug.trace_vdp_events;
        case TRACE_PSG: return (u32)config_debug.trace_psg_events;
        case TRACE_AY8910: return (u32)config_debug.trace_ay8910_events;
        case TRACE_IO: return (u32)config_debug.trace_io_events;
        case TRACE_INPUT: return (u32)config_debug.trace_input_events;
        case TRACE_SGM: return (u32)config_debug.trace_sgm_events;
        case TRACE_MAPPER: return (u32)config_debug.trace_mapper_events;
        default: return 0xFFFFFFFFU;
    }
}

static void trace_logger_set_config_event_filter(GC_Trace_Type type, u32 filter)
{
    switch (type)
    {
        case TRACE_VDP: config_debug.trace_vdp_events = filter; break;
        case TRACE_PSG: config_debug.trace_psg_events = filter; break;
        case TRACE_AY8910: config_debug.trace_ay8910_events = filter; break;
        case TRACE_IO: config_debug.trace_io_events = filter; break;
        case TRACE_INPUT: config_debug.trace_input_events = filter; break;
        case TRACE_SGM: config_debug.trace_sgm_events = filter; break;
        case TRACE_MAPPER: config_debug.trace_mapper_events = filter; break;
        default: break;
    }
}

static bool trace_logger_apply_capacity(void)
{
    u32 capacity = TRACE_BUFFER_SIZE;
    if (config_debug.trace_output == gui_TraceOutput_Memory)
        capacity = trace_capacities[config_debug.trace_capacity];
    if (!emu_get_core()->GetTraceLogger()->SetCapacity(capacity))
    {
        gui_set_error_message("Unable to allocate the selected trace logger capacity.");
        return false;
    }
    return true;
}

static void render_cpu_entry_colored(const GC_Trace_Entry& entry, int prefix_length)
{
    if (config_debug.trace_bank)
    {
        ImGui::TextColored(violet, "%03X:", entry.cpu.bank);
        ImGui::SameLine(0, 0);
    }

    ImGui::TextColored(cyan, "%04X", entry.cpu.pc);

    if (config_debug.trace_registers)
    {
        ImGui::SameLine(0, 0);
        ImGui::TextColored(magenta, "  AF:");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(white, "%04X", entry.cpu.af);
        ImGui::SameLine(0, 0);
        ImGui::TextColored(magenta, " BC:");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(white, "%04X", entry.cpu.bc);
        ImGui::SameLine(0, 0);
        ImGui::TextColored(magenta, " DE:");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(white, "%04X", entry.cpu.de);
        ImGui::SameLine(0, 0);
        ImGui::TextColored(magenta, " HL:");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(white, "%04X", entry.cpu.hl);
        ImGui::SameLine(0, 0);
        ImGui::TextColored(magenta, " IX:");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(white, "%04X", entry.cpu.ix);
        ImGui::SameLine(0, 0);
        ImGui::TextColored(magenta, " IY:");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(white, "%04X", entry.cpu.iy);
        ImGui::SameLine(0, 0);
        ImGui::TextColored(magenta, " SP:");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(white, "%04X", entry.cpu.sp);
        ImGui::SameLine(0, 0);
        ImGui::TextColored(magenta, " I:");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(white, "%02X", entry.cpu.i);
        ImGui::SameLine(0, 0);
        ImGui::TextColored(magenta, " R:");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(white, "%02X", entry.cpu.r);
        ImGui::SameLine(0, 0);
        ImGui::TextColored(magenta, " IM:");
        ImGui::SameLine(0, 0);
        ImGui::TextColored(white, "%u", entry.cpu.im);
    }

    if (config_debug.trace_flags)
    {
        u8 f = (u8)entry.cpu.af;
        ImGui::SameLine(0, 0);
        ImGui::TextColored(yellow, " %c%c%c%c%c%c%c%c",
                 (f & FLAG_SIGN) ? 'S' : 's',
                 (f & FLAG_ZERO) ? 'Z' : 'z',
                 (f & FLAG_Y) ? 'Y' : 'y',
                 (f & FLAG_HALF) ? 'H' : 'h',
                 (f & FLAG_X) ? 'X' : 'x',
                 (f & FLAG_PARITY) ? 'P' : 'p',
                 (f & FLAG_NEGATIVE) ? 'N' : 'n',
                 (f & FLAG_CARRY) ? 'C' : 'c');
    }

    if (entry.cpu.name[0] != 0)
    {
        std::string instr = entry.cpu.name;
        size_t pos;
        pos = instr.find("{n}");
        if (pos != std::string::npos)
            instr.replace(pos, 3, c_white);
        pos = instr.find("{o}");
        if (pos != std::string::npos)
            instr.replace(pos, 3, c_brown);
        pos = instr.find("{e}");
        if (pos != std::string::npos)
            instr.replace(pos, 3, c_blue);

        ImGui::SameLine(0, 0);
        TextColoredEx("  %s%s", c_white.c_str(), instr.c_str());
    }
    else
    {
        ImGui::SameLine(0, 0);
        ImGui::TextColored(gray, "  ???");
    }

    if (config_debug.trace_bytes)
    {
        char bytes[32];
        trace_log_format_cpu_bytes(entry, bytes, sizeof(bytes));
        float char_width = ImGui::CalcTextSize("A").x;
        float bytes_column = char_width * 35;
        if (config_debug.trace_bank)      bytes_column += char_width * 4;
        if (config_debug.trace_registers) bytes_column += char_width * 80;
        if (config_debug.trace_flags)     bytes_column += char_width * 9;
        bytes_column += char_width * prefix_length;
        ImGui::SameLine(bytes_column);
        ImGui::TextColored(gray, "%s", bytes);
    }
}

static void render_entry_colored(const GC_Trace_Entry& entry, u64 index)
{
    char buffer[GC_TRACE_FORMAT_BUFFER_SIZE];
    int prefix_length = 0;

    if (config_debug.trace_counter)
    {
        char counter[32];
        snprintf(counter, sizeof(counter), "%06llu ", (unsigned long long)index);
        prefix_length += (int)strlen(counter);
        ImGui::TextColored(gray, "%s", counter);
        ImGui::SameLine(0, 0);
    }

    if (config_debug.trace_cycles)
    {
        char cycles[64];
        trace_log_format_cycle_prefix(entry, trace_previous, cycles, sizeof(cycles));
        prefix_length += (int)strlen(cycles);
        ImGui::TextColored(gray, "%s", cycles);
        ImGui::SameLine(0, 0);
    }

    if (entry.type == TRACE_CPU)
    {
        render_cpu_entry_colored(entry, prefix_length);
        return;
    }

    format_entry_text(entry, false, NULL, buffer, sizeof(buffer));
    ImVec4 color = white;
    if (entry.type == TRACE_CPU_IRQ) color = red;
    else if (entry.type == TRACE_VDP) color = green;
    else if (entry.type == TRACE_IO) color = yellow;
    else if (entry.type == TRACE_INPUT) color = orange;
    else if (entry.type == TRACE_PSG) color = blue;
    else if (entry.type == TRACE_AY8910) color = violet;
    else if (entry.type == TRACE_SGM) color = cyan;
    else if (entry.type == TRACE_MAPPER) color = magenta;
    ImGui::TextColored(color, "%s", buffer);
}

void gui_debug_window_trace_logger(void)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::SetNextWindowPos(ImVec2(340, 168), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(544, 362), ImGuiCond_FirstUseEver);

    ImGui::Begin("Trace Logger", &config_debug.show_trace_logger, ImGuiWindowFlags_MenuBar);

    trace_logger_menu();

    TraceLogger* tl = emu_get_core()->GetTraceLogger();

    if (ImGui::Button(trace_enabled ? "Stop" : "Start"))
    {
        if (trace_enabled)
        {
            gui_debug_trace_logger_stop();
        }
        else
        {
            trace_logger_start(trace_logger_get_config_flags(), false);
        }
    }

    ImGui::SameLine();

    ImGui::BeginDisabled(trace_enabled && config_debug.trace_output == gui_TraceOutput_Disk);
    if (ImGui::Button("Clear"))
    {
        gui_debug_trace_logger_clear();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    ImGui::BeginDisabled(trace_enabled);
    ImGui::SetNextItemWidth(90.0f);
    int previous_output = config_debug.trace_output;
    if (ImGui::Combo("##trace_output", &config_debug.trace_output, "Memory\0Disk\0\0"))
    {
        if (!trace_logger_apply_capacity())
            config_debug.trace_output = previous_output;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(trace_enabled);
    ImGui::SetNextItemWidth(145.0f);
    if (config_debug.trace_output == gui_TraceOutput_Memory)
    {
        int previous_capacity = config_debug.trace_capacity;
        if (ImGui::Combo("##trace_capacity", &config_debug.trace_capacity, trace_capacity_labels, IM_ARRAYSIZE(trace_capacity_labels)) && !trace_logger_apply_capacity())
            config_debug.trace_capacity = previous_capacity;
    }
    else
    {
        ImGui::Combo("##trace_disk_size", &config_debug.trace_disk_size, "10 MB\0" "50 MB\0" "100 MB\0" "250 MB\0" "500 MB\0" "1 GB\0" "Unbounded\0\0");
    }
    ImGui::EndDisabled();
    if (config_debug.trace_output == gui_TraceOutput_Memory && ImGui::IsItemHovered())
    {
        double memory_mib = ((double)trace_capacities[config_debug.trace_capacity] * sizeof(GC_Trace_Entry)) / (1024.0 * 1024.0);
        ImGui::SetTooltip("Preallocated memory: %.1f MiB (%u bytes per entry).", memory_mib, (u32)sizeof(GC_Trace_Entry));
    }

    if (config_debug.trace_output == gui_TraceOutput_Memory)
    {
        ImGui::SameLine();
        ImGui::Text("Entries: %u / %u", tl->GetCount(), tl->GetCapacity());
    }
    if (config_debug.trace_output == gui_TraceOutput_Disk && trace_file_path[0] != '\0')
    {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputText("##trace_disk_file", trace_file_path, sizeof(trace_file_path), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_AutoSelectAll);
    }

    if (trace_enabled)
        trace_logger_sync_flags();

    u32 count = tl->GetCount();
    ImGui::PushFont(gui_default_font);
    float line_height = ImGui::GetTextLineHeightWithSpacing();
    float content_height = (float)count * line_height;
    ImGui::SetNextWindowContentSize(ImVec2(0.0f, content_height));
    if ((trace_enabled && trace_follow_latest) || trace_scroll_to_bottom)
        ImGui::SetNextWindowScroll(ImVec2(-1.0f, content_height));

    if (ImGui::BeginChild("##logger", ImVec2(ImGui::GetContentRegionAvail().x, 0), true, ImGuiWindowFlags_HorizontalScrollbar))
    {
        float scroll_y = ImGui::GetScrollY();
        float scroll_max_y = ImGui::GetScrollMaxY();
        bool at_bottom = scroll_y >= scroll_max_y - 0.5f;
        bool user_scrolling = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
            (ImGui::GetIO().MouseWheel != 0.0f || ImGui::IsMouseDragging(ImGuiMouseButton_Left));
        if (trace_enabled)
        {
            if (trace_scroll_to_bottom)
            {
                trace_follow_latest = true;
                trace_wait_for_scroll_away = false;
            }
            else if (trace_follow_latest && user_scrolling)
            {
                trace_follow_latest = false;
                trace_wait_for_scroll_away = true;
            }
            else if (!trace_follow_latest)
            {
                if (trace_wait_for_scroll_away)
                {
                    if (!at_bottom)
                        trace_wait_for_scroll_away = false;
                }
                else if (at_bottom)
                    trace_follow_latest = true;
            }
        }

        ImGuiListClipper clipper;
        clipper.Begin((int)count, line_height);

        while (clipper.Step())
        {
            for (int item = clipper.DisplayStart; item < clipper.DisplayEnd; item++)
            {
                const GC_Trace_Entry& entry = tl->GetEntry((u32)item);
                trace_previous = item > 0 ? &tl->GetEntry((u32)item - 1) : NULL;
                u64 entry_number = tl->GetSequence() - (u64)count + (u64)item;
                render_entry_colored(entry, entry_number);
            }
        }

        trace_scroll_to_bottom = false;
    }

    ImGui::EndChild();
    ImGui::PopFont();

    ImGui::End();
    ImGui::PopStyleVar();

    if (trace_choose_output_path)
    {
        trace_choose_output_path = false;
        gui_file_dialog_choose_trace_path();
    }
}
