---
name: gearcoleco-romhacking
description: >-
  Hack, modify, and translate ColecoVision and Super Game Module ROMs using the
  Gearcoleco emulator MCP server. Provides workflows for memory searching,
  value discovery, cheat creation, data modification, sprite/text finding,
  translation patching, and rewind-assisted experimentation. Use when the user
  wants to create cheats, find game values in memory, modify ROM data,
  translate a ColecoVision game, patch game behavior, create ROM hacks,
  discover hidden content, change sprites or graphics, find text strings, do
  infinite lives or health hacks, search for score or item counters, inspect
  SGM RAM, or reverse engineer data structures in ColecoVision or Super Game
  Module games. Also use for any ROM hacking, memory poking, or game
  modification task involving Gearcoleco.
compatibility: >-
  Requires the Gearcoleco MCP server. Before installing or configuring, call
  debug_get_status to check if the server is already connected. If it responds,
  the server is ready - skip setup entirely.
metadata:
  author: drhelius
  version: "1.0"
---

# ColecoVision / Super Game Module ROM Hacking with Gearcoleco

## Overview

Hack, modify, and translate ColecoVision and Super Game Module ROMs using the Gearcoleco emulator as an MCP server. Search memory for game variables, create cheats, find text strings for translation, locate sprite and tile data, inspect VRAM, patch RAM or ROM bytes in the emulator's in-memory copy, and reverse engineer data structures - all through MCP tool calls. Use save states or rewind as checkpoints and fast forward to reach specific game states. Hardware documentation is available in the [references/](references/) directory.

## MCP Server Prerequisite

**IMPORTANT - Check before installing:** Before attempting any installation or configuration, you MUST first verify if the Gearcoleco MCP server is already connected in your current session. Call `debug_get_status` - if it returns a valid response, the server is active and ready.

Only if the tool is not available or the call fails, you need to help install and configure the Gearcoleco MCP server:

### Installing Gearcoleco

Run the bundled install script (macOS/Linux):

```bash
bash scripts/install.sh
```

This installs Gearcoleco via Homebrew on macOS or downloads the latest release on Linux. It prints the binary path on completion. You can also set `INSTALL_DIR` to control where the binary goes (default: `~/.local/bin`).

Alternatively, download from [GitHub Releases](https://github.com/drhelius/Gearcoleco/releases/latest) or install with `brew install --cask drhelius/geardome/gearcoleco` on macOS.

### Connecting as MCP Server

Configure your AI client to run Gearcoleco as an MCP server via STDIO transport. Example for Claude Desktop (`~/Library/Application Support/Claude/claude_desktop_config.json`):

```json
{
  "mcpServers": {
    "gearcoleco": {
      "command": "/path/to/gearcoleco",
      "args": ["--mcp-stdio"]
    }
  }
}
```

Replace `/path/to/gearcoleco` with the actual binary path from the install script. Add `--headless` before `--mcp-stdio` on headless machines.

### Hardware Documentation (References)

ColecoVision and Super Game Module hardware documentation is available in the [references/](references/) directory. Load them into your context when investigating specific hardware, BIOS, memory-map, graphics, sound, or SGM behavior.

| Reference | File | Quality | Load when... |
|---|---|---|---|
| ColecoVision Technical Notes | [references/colecovision.md](references/colecovision.md) | **PRIMARY** - system quick reference | Cartridge headers, title-screen behavior, controller modes, keypad codes, memory map, I/O map, basic SN76489 and VDP context |
| TMS9918A VDP (Sean Young) | [references/tms9918a.md](references/tms9918a.md) | **PRIMARY** - detailed VDP reference | VDP registers, status flags, VRAM access, display modes, interrupts, sprites, fifth-sprite flag, collisions, undocumented modes |
| SN76489A PSG | [references/sn76489.md](references/sn76489.md) | **PRIMARY** - ColecoVision PSG quick reference | SN76489 latch/data writes, tone periods, attenuation, noise control, frequency formula, PSG debugging |
| Super Game Module Notes | [references/super_game_module.md](references/super_game_module.md) | **PRIMARY** - SGM memory reference | SGM RAM mapping, port $53, port $7F, ADAM compatibility, SGM initialization, SGM AY summary |

---

## Core Technique: Memory Search

Memory search is the primary tool for ROM hacking. It uses a capture -> change -> compare cycle to isolate memory addresses holding game values.

### The Search Loop

```
1. list_memory_areas          -> identify WRAM, SGM RAM, or another area ID
2. memory_search_capture      -> snapshot current memory state
3. (change the value in-game using controller_button, fast forward, etc.)
4. memory_search              -> compare against snapshot to find changed addresses
5. Repeat 2-4 until only a few candidates remain
6. read_memory / write_memory -> verify and modify the found addresses
```

### Search Operators and Types

`memory_search` supports these **operators**: `<`, `>`, `==`, `!=`, `<=`, `>=`

**Compare types**:

- `previous` - compare current value to last captured snapshot (most common)
- `value` - compare current value to a specific number
- `address` - compare current value to value at another address

**Data types**: `hex`, `signed`, `unsigned`

### Example: Finding the Lives Counter

```
1. list_memory_areas                                  -> find WRAM area ID
2. memory_search_capture                              -> snapshot with 3 lives
3. Lose a life in-game (play or use controller_button)
4. memory_search (operator: <, compare: previous)     -> values that decreased
5. memory_search_capture                              -> snapshot with 2 lives
6. Lose another life
7. memory_search (operator: <, compare: previous)     -> narrow further
8. Or use: memory_search (operator: ==, compare: value, value: 1)
   -> find addresses holding exactly 1
9. write_memory on the candidate offset to set lives to 99
10. get_screenshot to verify the change took effect
```

### Example: Finding a Score Counter

Score values are often stored as multi-byte (16-bit little-endian on Z80):

```
1. memory_search_capture                              -> snapshot at score 0
2. Score some points in-game
3. memory_search (operator: >, compare: previous)     -> values that increased
4. memory_search_capture
5. Score more points
6. memory_search (operator: >, compare: previous)     -> narrow down
7. read_memory on candidates - look for values matching current score
8. write_memory to set a custom score
```

For 16-bit values: the low byte is at address N, high byte at N+1 (Z80 is little-endian). Many ColecoVision games use BCD (Binary Coded Decimal) for score display - each nibble holds a single digit (e.g., score 1234 stored as `$12 $34`).

---

## Fast Forward for Efficiency

Use fast forward to speed through gameplay when you need to trigger in-game changes:

```
set_fast_forward_speed (4 = unlimited)
toggle_fast_forward (enabled: true)
(play through the game section)
toggle_fast_forward (enabled: false)
```

This is essential when you need to reach specific game states without waiting in real time.

---

## Save States as Checkpoints

Save states are critical for ROM hacking - they let you save your position and retry modifications:

```
select_save_state_slot (1-5) -> pick a slot
save_state                   -> save current state
(try modifications)
load_state                   -> revert if something breaks
```

Use different slots for different game states (e.g., slot 1 = start, slot 2 = boss fight, slot 3 = specific level).

`list_save_state_slots` shows all slots with ROM name, timestamp, and screenshot availability.

### Rewind as an Alternative

The emulator also records continuous snapshots into a rewind ring buffer. Use `get_rewind_status` to check availability, then `rewind_seek` to jump to any recorded point without manual save/load. This is especially useful for quickly reverting after a failed memory write - pause, seek back a few snapshots, and retry.

---

## Finding and Modifying Game Data

### Text and String Discovery

To find text strings for translation or modification:

1. Determine the character encoding - ColecoVision games often use custom character maps stored as VDP tile patterns, not ASCII
2. `read_memory` across ROM areas scanning for known byte patterns
3. Use `memory_find_bytes` to search for specific byte sequences
4. Set read breakpoints on suspected text addresses with `set_breakpoint` (read: true) to confirm they are read by the text rendering routine
5. `get_screenshot` to correlate displayed text with memory contents
6. `read_memory` (VRAM area from `list_memory_areas`) to examine the tile patterns used for font rendering
7. Load [references/colecovision.md](references/colecovision.md) for the Coleco BIOS title-screen character set and cartridge title format when dealing with BIOS-managed text

### Sprite and Graphics Data

1. `list_sprites` - view all 32 sprites with positions, sizes, and pattern indices
2. `get_sprite_image` - capture individual sprite images as PNG
3. `read_memory` (VRAM area from `list_memory_areas`) - examine pattern generator, color table, name table, and sprite attribute table data
4. `get_vdp_registers` - check table base addresses and sprite mode bits
5. Set read/write breakpoints (area: `vram`) on sprite or tile data to find the rendering code
6. `get_screenshot` before/after modifications to see visual changes

### VRAM and Tile Data

The TMS9918A uses VRAM for all graphics:

- **Pattern generator table**: bitmap patterns for characters/tiles/sprites
- **Pattern name table**: background tile map referencing pattern indices
- **Color table**: foreground/background color information depending on display mode
- **Sprite attribute table**: Y position, X position, pattern index, and color for each sprite
- **Sprite generator table**: sprite pattern data

Use `read_memory` (VRAM area from `list_memory_areas`) with base addresses from `get_vdp_registers` to access these. Use [references/tms9918a.md](references/tms9918a.md) for mode-specific table rules.

### Data Tables and Structures

1. `debug_pause` -> `get_disassembly` around code that loads data
2. Look for LD instructions with indexed or indirect addressing - these point to data tables
3. `read_memory` at the target offsets to dump table contents
4. `add_memory_bookmark` to mark discovered data regions
5. `add_symbol` to label data table entry points for future reference

---

## Creating Cheats

### Infinite Lives / Health

```
1. Find the address using the search loop (above)
2. Set a write breakpoint: set_breakpoint (write: true) on the logical address
3. debug_continue -> when it hits, get_disassembly to see the decrement code
4. Note the instruction (e.g., DEC (HL) or LD (addr),A)
5. Option A: Periodically write_memory to reset the value (simple poke cheat)
6. Option B: Identify the decrement routine for a NOP patch
```

### Watching Values in Real Time

Use `add_memory_watch` on discovered offsets. Watches appear in the emulator's GUI memory editor, letting you monitor values as the game runs - useful for verifying cheats work across different game situations.

### Write Breakpoint Technique

The most powerful cheat-finding technique:

1. Find the variable address via memory search
2. `set_breakpoint` (write: true) on that logical address
3. `debug_continue` - the emulator stops when the game writes to that address
4. `get_z80_status` + `get_disassembly` reveals the exact code modifying the value
5. `get_call_stack` shows what triggered the write
6. You now know exactly where and how the game manages that variable

---

## Translation Workflow

### 1. Identify the Font System

1. `get_screenshot` of a screen with text
2. `read_memory` (VRAM area) - find tile patterns used for font characters
3. Set read/write breakpoints on VRAM tile data addresses to trace back to the rendering code
4. `get_disassembly` to find the character mapping table (byte value -> tile index)
5. `add_symbol` to label the font table and rendering routine

### 2. Find String Data

1. Look for sequential text bytes in ROM using `read_memory` with large ranges
2. Use `memory_find_bytes` to search for known byte patterns
3. Cross-reference with the character table to decode strings
4. `add_memory_bookmark` to mark each string location

### 3. Measure Space Constraints

ROM hacking translations must fit within existing space:

1. `read_memory` to determine how much space each string occupies
2. Check for string terminators (commonly `$00`, `$FF`, delimiter bytes, or length-prefixed strings)
3. If the translation is longer, look for unused ROM space or abbreviate

### 4. Apply and Test

1. `write_memory` to patch translated strings into memory
2. `get_screenshot` to verify rendering
3. `save_state` before each change so you can `load_state` if it breaks
4. Test all screens that display modified text

---

## Memory Map Quick Reference

Use `list_memory_areas` to get the full list. Key logical areas:

| Area | Description | Typical Use |
|---|---|---|
| `rom_ram` | Full Z80 64K address space | General game code and variables |
| `vram` | Video RAM (16 KB) | Pattern tables, nametable, color table, sprite tables |
| `vdp_reg` | VDP registers | Display configuration |

### ColecoVision Memory Layout (rom_ram)

| Address | Content |
|---|---|
| `$0000-$1FFF` | BIOS ROM (or optional SGM lower RAM when mapped) |
| `$0066` | NMI handler vector (VDP VBlank path) |
| `$2000-$5FFF` | Expansion area / SGM upper RAM when enabled |
| `$6000-$63FF` | Internal 1 KB work RAM |
| `$6400-$7FFF` | Internal RAM mirrors, or SGM upper RAM when enabled |
| `$8000-$FFFF` | Cartridge ROM |

Internal RAM (`$6000-$63FF`, mirrored to `$7FFF`) is the most common location for game variables in non-SGM games. SGM games may use the 24 KB upper RAM window at `$2000-$7FFF` after enabling it through port `$53`. Load [references/super_game_module.md](references/super_game_module.md) before modifying SGM memory or port `$7F` BIOS/RAM mapping.

---

## Bookmarks and Organization

Keep your hacking session organized:

- `add_memory_bookmark` - mark discovered data regions, variable locations, string tables
- `add_memory_watch` - track values that change during gameplay
- `add_symbol` - label addresses in disassembly for readability
- `add_disassembler_bookmark` - mark code routines you have identified

Use `list_memory_bookmarks`, `list_memory_watches`, `list_symbols`, `list_disassembler_bookmarks` to review.

---

## Persisting Changes

Changes made via `write_memory` to ROM areas are applied to the emulator's in-memory copy only - they are **not** persisted to the ROM file on disk. To create a permanent patch, use command-line tools (for example, a binary patch script) to apply the discovered modifications to the actual ROM file.
