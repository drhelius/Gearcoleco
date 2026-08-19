#include <stdio.h>
#include <string.h>
#include "trace_logger_formatter.h"

static void strip_tags(const char* source, char* destination, size_t size)
{
    size_t output = 0;
    for (size_t input = 0; source[input] && output + 1 < size; input++)
    {
        if (source[input] == '{')
        {
            const char* end = strchr(source + input, '}');
            if (end)
            {
                input = (size_t)(end - source);
                continue;
            }
        }
        destination[output++] = source[input];
    }
    destination[output] = 0;
}

void trace_log_format_cycle_prefix(const GC_Trace_Entry& entry, const GC_Trace_Entry* previous,
                                   char* buffer, size_t buffer_size)
{
    if (!previous)
        snprintf(buffer, buffer_size, "@%012llu                ", (unsigned long long)entry.cycle);
    else if (entry.cycle < previous->cycle)
        snprintf(buffer, buffer_size, "@%012llu RESET          ", (unsigned long long)entry.cycle);
    else
        snprintf(buffer, buffer_size, "@%012llu +%-12llu ", (unsigned long long)entry.cycle,
                 (unsigned long long)(entry.cycle - previous->cycle));
}

GC_Disassembler_Record* trace_log_get_cpu_record(Memory* memory, const GC_Trace_Entry& entry)
{
    GC_Disassembler_Record* record = entry.cpu.bank <= 0xFF ?
        memory->GetDisassemblerRecord(entry.cpu.pc, (u8)entry.cpu.bank) : NULL;
    bool valid = IsValidPointer(record) &&
                 record->address == memory->GetTracePhysicalAddress(entry.cpu.pc, (u8)entry.cpu.bank) &&
                 record->bank == entry.cpu.bank && record->size == entry.cpu.size &&
                 entry.cpu.size <= sizeof(entry.cpu.opcodes);
    if (valid)
    {
        for (u8 i = 0; i < entry.cpu.size; i++)
        {
            if (record->opcodes[i] != entry.cpu.opcodes[i])
            {
                valid = false;
                break;
            }
        }
    }
    return valid ? record : NULL;
}

void trace_log_format_cpu_bytes(const GC_Trace_Entry& entry, char* buffer, size_t buffer_size)
{
    size_t offset = 0;
    buffer[0] = 0;
    for (u8 i = 0; i < entry.cpu.size && offset + 4 < buffer_size; i++)
        offset += (size_t)snprintf(buffer + offset, buffer_size - offset,
                                  "%02X ", entry.cpu.opcodes[i]);
}

static void format_cpu(const GC_Trace_Entry& entry, Memory* memory,
                       const GC_Trace_Format_Options& options,
                       char* buffer, size_t size)
{
    char mnemonic[80] = "???";
    GC_Disassembler_Record* record = trace_log_get_cpu_record(memory, entry);
    if (IsValidPointer(record))
        strip_tags(record->name, mnemonic, sizeof(mnemonic));

    char bank[16] = "";
    if (options.bank)
        snprintf(bank, sizeof(bank), "%03X:", entry.cpu.bank);

    char registers[160] = "";
    if (options.registers)
    {
        snprintf(registers, sizeof(registers),
                 "AF:%04X BC:%04X DE:%04X HL:%04X IX:%04X IY:%04X SP:%04X I:%02X R:%02X IM:%u ",
                 entry.cpu.af, entry.cpu.bc, entry.cpu.de, entry.cpu.hl,
                 entry.cpu.ix, entry.cpu.iy, entry.cpu.sp, entry.cpu.i,
                 entry.cpu.r, entry.cpu.im);
    }

    char flags[24] = "";
    if (options.flags)
    {
        u8 value = (u8)entry.cpu.af;
        snprintf(flags, sizeof(flags), "%c%c%c%c%c%c%c%c ",
                 value & FLAG_SIGN ? 'S' : 's', value & FLAG_ZERO ? 'Z' : 'z',
                 value & FLAG_Y ? 'Y' : 'y', value & FLAG_HALF ? 'H' : 'h',
                 value & FLAG_X ? 'X' : 'x', value & FLAG_PARITY ? 'P' : 'p',
                 value & FLAG_NEGATIVE ? 'N' : 'n', value & FLAG_CARRY ? 'C' : 'c');
    }

    char bytes[32] = "";
    if (options.bytes)
        trace_log_format_cpu_bytes(entry, bytes, sizeof(bytes));

    snprintf(buffer, size, "[CPU] %s%04X  %s%s%-24s %s", bank,
             entry.cpu.pc, registers, flags, mnemonic, bytes);
}

static const char* io_target_name(u8 target)
{
    static const char* names[] = {"UNKNOWN", "VDP DATA", "VDP STATUS", "INPUT",
        "PSG", "AY SELECT", "AY DATA", "SGM UPPER", "SGM LOWER"};
    return target < sizeof(names) / sizeof(names[0]) ? names[target] : "UNKNOWN";
}

static const char* mapper_name(u8 mapper)
{
    static const char* names[] = {"STANDARD", "MEGACART", "ACTIVISION", "OCM"};
    return mapper < sizeof(names) / sizeof(names[0]) ? names[mapper] : "UNKNOWN";
}

void trace_logger_format_entry(const GC_Trace_Entry& entry, Memory* memory,
                               const GC_Trace_Format_Options& options,
                               char* buffer, size_t buffer_size)
{
    char cycles[48];
    char text[GC_TRACE_FORMAT_BUFFER_SIZE];
    cycles[0] = 0;
    if (options.cycles)
        trace_log_format_cycle_prefix(entry, options.previous, cycles, sizeof(cycles));

    switch (entry.type)
    {
        case TRACE_CPU:
            format_cpu(entry, memory, options, text, sizeof(text));
            break;
        case TRACE_CPU_IRQ:
            snprintf(text, sizeof(text), "[CPU] %s PC:$%04X Vector:$%04X",
                     entry.irq.type == 2 ? "NMI" : "INT", entry.irq.pc,
                     entry.irq.vector);
            break;
        case TRACE_VDP:
        {
            static const char* names[] = {"REG", "NMI", "VFLAG", "STATUS", "SPR OVR",
                "SPR COL", "DISPLAY", "VBLANK", "FRAME", "VRAM RD", "VRAM WR"};
            const char* name = entry.vdp.event < 11 ? names[entry.vdp.event] : "???";
            if (entry.vdp.event == TRACE_VDP_REG_WRITE)
                snprintf(text, sizeof(text), "[VDP] %-8s R%u Raw:$%02X Effective:$%02X Line:%u Dot:%u Mode:%u",
                         name, entry.vdp.reg, entry.vdp.raw, entry.vdp.effective,
                         entry.vdp.line, entry.vdp.hpos, entry.vdp.mode);
            else if (entry.vdp.event == TRACE_VDP_STATUS_READ)
                snprintf(text, sizeof(text), "[VDP] %-8s Before:$%02X Result:$%02X After:$%02X Line:%u Dot:%u",
                         name, entry.vdp.status_before, entry.vdp.effective,
                         entry.vdp.status_after, entry.vdp.line, entry.vdp.hpos);
            else if (entry.vdp.event == TRACE_VDP_DATA_READ || entry.vdp.event == TRACE_VDP_DATA_WRITE)
                snprintf(text, sizeof(text), "[VDP] %-8s Addr:$%04X Value:$%02X Next:$%04X Line:%u Dot:%u",
                         name, entry.vdp.address, entry.vdp.raw, entry.vdp.auxiliary,
                         entry.vdp.line, entry.vdp.hpos);
            else
                snprintf(text, sizeof(text), "[VDP] %-8s Line:%u Dot:%u Status:$%02X->$%02X Sprite:%u Aux:$%04X",
                         name, entry.vdp.line, entry.vdp.hpos, entry.vdp.status_before,
                         entry.vdp.status_after, entry.vdp.sprite, entry.vdp.auxiliary);
            break;
        }
        case TRACE_PSG:
        {
            static const char* names[] = {"TONE", "VOLUME", "NOISE"};
            const char* name = entry.psg.event < 3 ? names[entry.psg.event] : "???";
            snprintf(text, sizeof(text), "[PSG] %-7s Raw:$%02X Channel:%u Latch:$%02X Period:$%04X Attenuation:%u Noise:%s/%u",
                     name, entry.psg.value, entry.psg.channel, entry.psg.latch,
                     entry.psg.period, entry.psg.attenuation,
                     entry.psg.noise_white ? "white" : "periodic", entry.psg.noise_rate);
            break;
        }
        case TRACE_AY8910:
        {
            static const char* names[] = {"SELECT", "READ", "TONE", "NOISE/MIX", "VOLUME", "ENVELOPE", "IO"};
            const char* name = entry.ay8910.event < 7 ? names[entry.ay8910.event] : "???";
            snprintf(text, sizeof(text), "[AY] %-9s R%u Raw:$%02X Effective:$%02X Channel:%u Period:$%04X Mixer:$%02X Amp:%u Env:%s",
                     name, entry.ay8910.reg, entry.ay8910.raw, entry.ay8910.effective,
                     entry.ay8910.channel, entry.ay8910.period, entry.ay8910.mixer,
                     entry.ay8910.amplitude, entry.ay8910.envelope_enabled ? "on" : "off");
            break;
        }
        case TRACE_IO:
            snprintf(text, sizeof(text), "[IO] %s Port:$%02X Value:$%02X Target:%s",
                     entry.io.event == TRACE_IO_READ ? "IN " : "OUT",
                     entry.io.port, entry.io.value, io_target_name(entry.io.target));
            break;
        case TRACE_INPUT:
            if (entry.input.event == TRACE_INPUT_WRITE)
            {
                snprintf(text, sizeof(text), "[INP] WRITE Segment:%s->%s",
                         entry.input.previous_segment == 0 ? "keypad" : "joystick",
                         entry.input.segment == 0 ? "keypad" : "joystick");
            }
            else if (entry.input.event == TRACE_INPUT_CHANGE)
            {
                snprintf(text, sizeof(text), "[INP] CHANGE Player:%u Key:$%02X Value:$%02X->$%02X Segment:%s",
                         entry.input.player, entry.input.port, entry.input.previous_value,
                         entry.input.effective_value,
                         entry.input.segment == 0 ? "keypad" : "joystick");
            }
            else
            {
                snprintf(text, sizeof(text), "[INP] READ Player:%u Port:$%02X Segment:%s Gamepad:$%02X Keypad:$%02X Result:$%02X Spinner:%d/%d INT:%s",
                         entry.input.player, entry.input.port,
                         entry.input.segment == 0 ? "keypad" : "joystick",
                         entry.input.gamepad, entry.input.keypad, entry.input.result,
                         entry.input.spinner_before, entry.input.spinner_consumed,
                         entry.input.int_asserted ? "yes" : "no");
            }
            break;
        case TRACE_SGM:
            snprintf(text, sizeof(text), "[SGM] CONTROL Port:$%02X Raw:$%02X Upper:%s->%s Lower:%s->%s",
                     entry.sgm.port, entry.sgm.raw,
                     entry.sgm.old_upper ? "on" : "off", entry.sgm.new_upper ? "on" : "off",
                     entry.sgm.old_lower ? "on" : "off", entry.sgm.new_lower ? "on" : "off");
            break;
        case TRACE_MAPPER:
        {
            static const char* names[] = {"BANK", "EEPROM", "SRAM"};
            const char* name = entry.mapper.event < 3 ? names[entry.mapper.event] : "???";
            snprintf(text, sizeof(text), "[MAP] %s %-6s Addr:$%04X Raw:$%02X Banks:%03X/%03X/%03X/%03X State:%u Aux:$%04X",
                     mapper_name(entry.mapper.mapper), name, entry.mapper.address,
                     entry.mapper.value, entry.mapper.banks[0], entry.mapper.banks[1],
                     entry.mapper.banks[2], entry.mapper.banks[3], entry.mapper.state,
                     entry.mapper.auxiliary);
            break;
        }
        default:
            snprintf(text, sizeof(text), "[???]");
            break;
    }

    snprintf(buffer, buffer_size, "%s%s", cycles, text);
}
