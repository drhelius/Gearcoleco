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

void trace_log_format_cpu_bytes(const GC_Trace_Entry& entry, char* buffer, size_t buffer_size)
{
    size_t offset = 0;
    buffer[0] = 0;
    for (u8 i = 0; i < entry.cpu.size && offset + 4 < buffer_size; i++)
        offset += (size_t)snprintf(buffer + offset, buffer_size - offset,
                                  "%02X ", entry.cpu.opcodes[i]);
}

static void format_cpu(const GC_Trace_Entry& entry,
                       const GC_Trace_Format_Options& options,
                       char* buffer, size_t size)
{
    char mnemonic[80] = "???";
    if (entry.cpu.name[0] != 0)
        strip_tags(entry.cpu.name, mnemonic, sizeof(mnemonic));

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

static const char* vdp_register_name(u8 reg)
{
    static const char* names[] = {"MODE_1", "MODE_2", "NAME_TABLE", "COLOR_TABLE",
        "PATTERN_TABLE", "SPRITE_ATTR", "SPRITE_PATTERN", "COLORS"};
    return reg < sizeof(names) / sizeof(names[0]) ? names[reg] : "UNKNOWN";
}

static const char* ay_register_name(u8 reg)
{
    static const char* names[] = {"TONE_A_FINE", "TONE_A_COARSE", "TONE_B_FINE",
        "TONE_B_COARSE", "TONE_C_FINE", "TONE_C_COARSE", "NOISE", "MIXER",
        "VOLUME_A", "VOLUME_B", "VOLUME_C", "ENVELOPE_FINE", "ENVELOPE_COARSE",
        "ENVELOPE_SHAPE", "IO_A", "IO_B"};
    return reg < sizeof(names) / sizeof(names[0]) ? names[reg] : "UNKNOWN";
}

static const char* ay_channel_name(u8 channel)
{
    static const char* names[] = {"A", "B", "C"};
    return channel < sizeof(names) / sizeof(names[0]) ? names[channel] : "?";
}

static const char* input_segment_name(u8 segment)
{
    return segment == 0 ? "KEYPAD" : "JOYSTICK";
}

static const char* input_key_name(u8 key)
{
    switch (key)
    {
        case 0x01: return "KEYPAD_8";
        case 0x02: return "KEYPAD_4";
        case 0x03: return "KEYPAD_5";
        case 0x04: return "BLUE";
        case 0x05: return "KEYPAD_7";
        case 0x06: return "KEYPAD_HASH";
        case 0x07: return "KEYPAD_2";
        case 0x08: return "PURPLE";
        case 0x09: return "KEYPAD_ASTERISK";
        case 0x0A: return "KEYPAD_0";
        case 0x0B: return "KEYPAD_9";
        case 0x0C: return "KEYPAD_3";
        case 0x0D: return "KEYPAD_1";
        case 0x0E: return "KEYPAD_6";
        case 0x10: return "UP";
        case 0x11: return "RIGHT";
        case 0x12: return "DOWN";
        case 0x13: return "LEFT";
        case 0x14: return "LEFT_BUTTON";
        case 0x15: return "RIGHT_BUTTON";
        default: return "UNKNOWN";
    }
}

static const char* noise_rate_name(u8 rate)
{
    static const char* names[] = {"CLOCK/512", "CLOCK/1024", "CLOCK/2048", "TONE_2"};
    return rate < sizeof(names) / sizeof(names[0]) ? names[rate] : "UNKNOWN";
}

static const char* eeprom_state_name(u8 state)
{
    static const char* names[] = {"NONE", "INIT", "STATUS", "WRITE"};
    return state < sizeof(names) / sizeof(names[0]) ? names[state] : "UNKNOWN";
}

static const char* io_target_name(u8 target, bool write)
{
    static const char* names[] = {"UNKNOWN", "VDP DATA", "VDP STATUS", "INPUT",
        "PSG", "AY SELECT", "AY DATA", "SGM UPPER", "SGM LOWER"};
    if (target == TRACE_IO_TARGET_VDP_STATUS && write)
        return "VDP CONTROL";
    return target < sizeof(names) / sizeof(names[0]) ? names[target] : "UNKNOWN";
}

static const char* mapper_name(u8 mapper)
{
    static const char* names[] = {"STANDARD", "MEGACART", "ACTIVISION", "OCM"};
    return mapper < sizeof(names) / sizeof(names[0]) ? names[mapper] : "UNKNOWN";
}

void trace_logger_format_entry(const GC_Trace_Entry& entry,
    const GC_Trace_Format_Options& options, char* buffer, size_t buffer_size)
{
    char cycles[48];
    char text[GC_TRACE_FORMAT_BUFFER_SIZE];
    cycles[0] = 0;
    if (options.cycles)
        trace_log_format_cycle_prefix(entry, options.previous, cycles, sizeof(cycles));

    switch (entry.type)
    {
        case TRACE_CPU:
            format_cpu(entry, options, text, sizeof(text));
            break;
        case TRACE_CPU_IRQ:
            snprintf(text, sizeof(text), "[CPU] %s PC:$%04X Vector:$%04X",
                     entry.irq.type == 2 ? "NMI" : "INT", entry.irq.pc,
                     entry.irq.vector);
            break;
        case TRACE_VDP:
        {
            switch (entry.vdp.event)
            {
                case TRACE_VDP_REG_WRITE:
                    snprintf(text, sizeof(text), "[VDP] REG %s(R%u) Raw:$%02X Effective:$%02X Mode:$%X Line:%u Dot:%u",
                             vdp_register_name(entry.vdp.reg), entry.vdp.reg, entry.vdp.raw,
                             entry.vdp.effective, entry.vdp.mode, entry.vdp.line, entry.vdp.hpos);
                    break;
                case TRACE_VDP_NMI_REQUEST:
                    snprintf(text, sizeof(text), "[VDP] NMI REQUEST Source:%s R%u:$%02X Status:$%02X Line:%u Dot:%u",
                             entry.vdp.auxiliary ? "R1_ENABLE" : "VINT", entry.vdp.reg,
                             entry.vdp.effective, entry.vdp.status_before,
                             entry.vdp.line, entry.vdp.hpos);
                    break;
                case TRACE_VDP_VINT_FLAG:
                    snprintf(text, sizeof(text), "[VDP] VINT FLAG Status:$%02X->$%02X Line:%u Dot:%u",
                             entry.vdp.status_before, entry.vdp.status_after,
                             entry.vdp.line, entry.vdp.hpos);
                    break;
                case TRACE_VDP_STATUS_READ:
                    snprintf(text, sizeof(text), "[VDP] STATUS READ Result:$%02X Status:$%02X->$%02X Line:%u Dot:%u",
                             entry.vdp.effective, entry.vdp.status_before, entry.vdp.status_after,
                             entry.vdp.line, entry.vdp.hpos);
                    break;
                case TRACE_VDP_SPRITE_OVERFLOW:
                    snprintf(text, sizeof(text), "[VDP] SPRITE OVERFLOW Index:%u Status:$%02X->$%02X Line:%u Dot:%u",
                             entry.vdp.sprite, entry.vdp.status_before, entry.vdp.status_after,
                             entry.vdp.line, entry.vdp.hpos);
                    break;
                case TRACE_VDP_SPRITE_COLLISION:
                    snprintf(text, sizeof(text), "[VDP] SPRITE COLLISION Index:%u X:%u Status:$%02X->$%02X Line:%u Dot:%u",
                             entry.vdp.sprite, entry.vdp.auxiliary, entry.vdp.status_before,
                             entry.vdp.status_after, entry.vdp.line, entry.vdp.hpos);
                    break;
                case TRACE_VDP_DISPLAY_CHANGE:
                    snprintf(text, sizeof(text), "[VDP] DISPLAY %s->%s R1:$%02X Mode:$%X Line:%u Dot:%u",
                             entry.vdp.auxiliary ? "OFF" : "ON",
                             entry.vdp.auxiliary ? "ON" : "OFF", entry.vdp.effective,
                             entry.vdp.mode, entry.vdp.line, entry.vdp.hpos);
                    break;
                case TRACE_VDP_VBLANK:
                    snprintf(text, sizeof(text), "[VDP] VBLANK Status:$%02X->$%02X Line:%u Dot:%u",
                             entry.vdp.status_before, entry.vdp.status_after,
                             entry.vdp.line, entry.vdp.hpos);
                    break;
                case TRACE_VDP_FRAME:
                    snprintf(text, sizeof(text), "[VDP] FRAME Line:%u Dot:%u",
                             entry.vdp.line, entry.vdp.hpos);
                    break;
                case TRACE_VDP_DATA_READ:
                    snprintf(text, sizeof(text), "[VDP] VRAM READ Addr:$%04X Result:$%02X Prefetch:$%02X Next:$%04X Line:%u Dot:%u",
                             entry.vdp.address, entry.vdp.raw, entry.vdp.effective,
                             entry.vdp.auxiliary, entry.vdp.line, entry.vdp.hpos);
                    break;
                case TRACE_VDP_DATA_WRITE:
                    snprintf(text, sizeof(text), "[VDP] VRAM WRITE Addr:$%04X Value:$%02X Next:$%04X Line:%u Dot:%u",
                             entry.vdp.address, entry.vdp.raw, entry.vdp.auxiliary,
                             entry.vdp.line, entry.vdp.hpos);
                    break;
                default:
                    snprintf(text, sizeof(text), "[VDP] UNKNOWN Event:$%02X", entry.vdp.event);
                    break;
            }
            break;
        }
        case TRACE_PSG:
        {
            if (entry.psg.event == TRACE_PSG_TONE)
                snprintf(text, sizeof(text), "[PSG] TONE CH%u Write:$%02X Latch:$%02X Period:$%03X",
                         entry.psg.channel, entry.psg.value, entry.psg.latch, entry.psg.period);
            else if (entry.psg.event == TRACE_PSG_VOLUME)
                snprintf(text, sizeof(text), "[PSG] VOLUME CH%u Write:$%02X Attenuation:$%X",
                         entry.psg.channel, entry.psg.value, entry.psg.attenuation);
            else if (entry.psg.event == TRACE_PSG_NOISE)
                snprintf(text, sizeof(text), "[PSG] NOISE Write:$%02X Latch:$%02X Type:%s Rate:%s",
                         entry.psg.value, entry.psg.latch,
                         entry.psg.noise_white ? "WHITE" : "PERIODIC",
                         noise_rate_name(entry.psg.noise_rate));
            else
                snprintf(text, sizeof(text), "[PSG] UNKNOWN Event:$%02X", entry.psg.event);
            break;
        }
        case TRACE_AY8910:
        {
            switch (entry.ay8910.event)
            {
                case TRACE_AY8910_SELECT:
                    snprintf(text, sizeof(text), "[AY] SELECT %s(R%u) Raw:$%02X Value:$%02X",
                             ay_register_name(entry.ay8910.reg), entry.ay8910.reg,
                             entry.ay8910.raw, entry.ay8910.effective);
                    break;
                case TRACE_AY8910_READ:
                    snprintf(text, sizeof(text), "[AY] READ %s(R%u) Value:$%02X",
                             ay_register_name(entry.ay8910.reg), entry.ay8910.reg,
                             entry.ay8910.effective);
                    break;
                case TRACE_AY8910_TONE:
                    snprintf(text, sizeof(text), "[AY] TONE CH%s %s(R%u) Raw:$%02X Effective:$%02X Period:$%03X",
                             ay_channel_name(entry.ay8910.channel), ay_register_name(entry.ay8910.reg),
                             entry.ay8910.reg, entry.ay8910.raw, entry.ay8910.effective,
                             entry.ay8910.period);
                    break;
                case TRACE_AY8910_NOISE_MIXER:
                    if (entry.ay8910.reg == 6)
                    {
                        snprintf(text, sizeof(text), "[AY] NOISE R6 Raw:$%02X Effective:$%02X Period:$%02X",
                                 entry.ay8910.raw, entry.ay8910.effective,
                                 (u8)entry.ay8910.period);
                    }
                    else
                    {
                        snprintf(text, sizeof(text), "[AY] MIXER R%u Raw:$%02X Effective:$%02X ToneOff:$%X NoiseOff:$%X IO:$%X",
                                 entry.ay8910.reg, entry.ay8910.raw, entry.ay8910.effective,
                                 entry.ay8910.effective & 0x07,
                                 (entry.ay8910.effective >> 3) & 0x07,
                                 (entry.ay8910.effective >> 6) & 0x03);
                    }
                    break;
                case TRACE_AY8910_VOLUME:
                    snprintf(text, sizeof(text), "[AY] VOLUME CH%s R%u Raw:$%02X Effective:$%02X Level:$%X Envelope:%s",
                             ay_channel_name(entry.ay8910.channel), entry.ay8910.reg,
                             entry.ay8910.raw, entry.ay8910.effective,
                             entry.ay8910.amplitude,
                             entry.ay8910.envelope_enabled ? "ON" : "OFF");
                    break;
                case TRACE_AY8910_ENVELOPE:
                    if (entry.ay8910.reg == 13)
                    {
                        snprintf(text, sizeof(text), "[AY] ENVELOPE SHAPE R13 Raw:$%02X Effective:$%02X C:%u A:%u Alt:%u H:%u",
                                 entry.ay8910.raw, entry.ay8910.effective,
                                 (entry.ay8910.effective >> 3) & 1,
                                 (entry.ay8910.effective >> 2) & 1,
                                 (entry.ay8910.effective >> 1) & 1,
                                 entry.ay8910.effective & 1);
                    }
                    else
                    {
                        snprintf(text, sizeof(text), "[AY] ENVELOPE PERIOD %s(R%u) Raw:$%02X Effective:$%02X Period:$%04X",
                                 ay_register_name(entry.ay8910.reg), entry.ay8910.reg,
                                 entry.ay8910.raw, entry.ay8910.effective,
                                 entry.ay8910.period);
                    }
                    break;
                case TRACE_AY8910_IO:
                    snprintf(text, sizeof(text), "[AY] IO ACCESS Port:%s(R%u) Raw:$%02X Effective:$%02X",
                             entry.ay8910.reg == 14 ? "A" : entry.ay8910.reg == 15 ? "B" : "?",
                             entry.ay8910.reg, entry.ay8910.raw, entry.ay8910.effective);
                    break;
                default:
                    snprintf(text, sizeof(text), "[AY] UNKNOWN Event:$%02X", entry.ay8910.event);
                    break;
            }
            break;
        }
        case TRACE_IO:
            if (entry.io.event == TRACE_IO_READ || entry.io.event == TRACE_IO_WRITE)
            {
                bool write = entry.io.event == TRACE_IO_WRITE;
                snprintf(text, sizeof(text), "[IO] %s Port:$%02X Value:$%02X Target:%s",
                         write ? "OUT" : "IN", entry.io.port, entry.io.value,
                         io_target_name(entry.io.target, write));
            }
            else
                snprintf(text, sizeof(text), "[IO] UNKNOWN Event:$%02X", entry.io.event);
            break;
        case TRACE_INPUT:
            if (entry.input.event == TRACE_INPUT_WRITE)
            {
                snprintf(text, sizeof(text), "[INP] SELECT %s->%s",
                         input_segment_name(entry.input.previous_segment),
                         input_segment_name(entry.input.segment));
            }
            else if (entry.input.event == TRACE_INPUT_CHANGE)
            {
                snprintf(text, sizeof(text), "[INP] CHANGE P%u %s($%02X) Value:$%02X->$%02X Segment:%s",
                         entry.input.player, input_key_name(entry.input.port), entry.input.port,
                         entry.input.previous_value, entry.input.effective_value,
                         input_segment_name(entry.input.segment));
            }
            else if (entry.input.event == TRACE_INPUT_READ)
            {
                snprintf(text, sizeof(text), "[INP] READ P%u Port:$%02X Segment:%s Gamepad:$%02X "
                         "Keypad:$%02X Result:$%02X Spinner:%d->%d INT:%s",
                         entry.input.player, entry.input.port,
                         input_segment_name(entry.input.segment),
                         entry.input.gamepad, entry.input.keypad, entry.input.result,
                         entry.input.spinner_before,
                         entry.input.spinner_before - entry.input.spinner_consumed,
                         entry.input.int_asserted ? "YES" : "NO");
            }
            else
                snprintf(text, sizeof(text), "[INP] UNKNOWN Event:$%02X", entry.input.event);
            break;
        case TRACE_SGM:
            if (entry.sgm.event != TRACE_SGM_CONTROL)
                snprintf(text, sizeof(text), "[SGM] UNKNOWN Event:$%02X", entry.sgm.event);
            else if (entry.sgm.port == 0x53)
                snprintf(text, sizeof(text), "[SGM] UPPER RAM Port:$%02X Write:$%02X Enabled:%s->%s",
                         entry.sgm.port, entry.sgm.raw,
                         entry.sgm.old_upper ? "ON" : "OFF",
                         entry.sgm.new_upper ? "ON" : "OFF");
            else if (entry.sgm.port == 0x7F)
                snprintf(text, sizeof(text), "[SGM] LOWER RAM Port:$%02X Write:$%02X Enabled:%s->%s",
                         entry.sgm.port, entry.sgm.raw,
                         entry.sgm.old_lower ? "ON" : "OFF",
                         entry.sgm.new_lower ? "ON" : "OFF");
            else
                snprintf(text, sizeof(text), "[SGM] CONTROL Port:$%02X Write:$%02X",
                         entry.sgm.port, entry.sgm.raw);
            break;
        case TRACE_MAPPER:
        {
            if (entry.mapper.event == TRACE_MAPPER_BANK)
            {
                if (entry.mapper.mapper == 3 && entry.mapper.state < 4)
                {
                    snprintf(text, sizeof(text), "[MAP] OCM BANK Slot:%u Addr:$%04X Raw:$%02X "
                             "Bank:$%02X->$%02X Map:$%02X/$%02X/$%02X/$%02X",
                             entry.mapper.state, entry.mapper.address, entry.mapper.value,
                             entry.mapper.auxiliary, entry.mapper.banks[entry.mapper.state],
                             entry.mapper.banks[0], entry.mapper.banks[1],
                             entry.mapper.banks[2], entry.mapper.banks[3]);
                }
                else
                {
                    snprintf(text, sizeof(text), "[MAP] %s BANK Addr:$%04X Raw:$%02X Bank:$%02X->$%02X",
                             mapper_name(entry.mapper.mapper), entry.mapper.address,
                             entry.mapper.value, entry.mapper.auxiliary,
                             entry.mapper.banks[0]);
                }
            }
            else if (entry.mapper.event == TRACE_MAPPER_EEPROM)
            {
                if (entry.mapper.mapper == 3 && entry.mapper.address == 0xFFFE)
                {
                    snprintf(text, sizeof(text), "[MAP] OCM EEPROM WINDOW Write:$%02X Armed:%s State:%s",
                             entry.mapper.value, entry.mapper.state ? "YES" : "NO",
                             eeprom_state_name((u8)entry.mapper.auxiliary));
                }
                else
                {
                    snprintf(text, sizeof(text), "[MAP] %s EEPROM ACCESS Addr:$%04X Value:$%02X State:%s Step/Offset:$%04X",
                             mapper_name(entry.mapper.mapper), entry.mapper.address,
                             entry.mapper.value, eeprom_state_name(entry.mapper.state),
                             entry.mapper.auxiliary);
                }
            }
            else if (entry.mapper.event == TRACE_MAPPER_SRAM)
            {
                snprintf(text, sizeof(text), "[MAP] %s SRAM WRITE Addr:$%04X Offset:$%04X Value:$%02X",
                         mapper_name(entry.mapper.mapper), entry.mapper.address,
                         entry.mapper.auxiliary, entry.mapper.value);
            }
            else
                snprintf(text, sizeof(text), "[MAP] UNKNOWN Event:$%02X", entry.mapper.event);
            break;
        }
        default:
            snprintf(text, sizeof(text), "[???]");
            break;
    }

    snprintf(buffer, buffer_size, "%s%s", cycles, text);
}
