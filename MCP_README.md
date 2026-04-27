# Gearcoleco MCP Server

Gearcoleco includes a built-in [Model Context Protocol](https://modelcontextprotocol.io/introduction) (MCP) server that enables AI-assisted debugging through AI agents like GitHub Copilot, Claude, ChatGPT and similar.

This server provides tools for ColecoVision game development, rom hacking, reverse engineering, and debugging through standardized MCP protocols.

## Downloads

<table>
  <thead>
    <tr>
      <th>Platform</th>
      <th>Architecture</th>
      <th>Download Link</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td rowspan="2"><strong>Windows</strong></td>
      <td>x64</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.6.0/Gearcoleco-1.6.0-mcpb-windows-x64.mcpb">Gearcoleco-1.6.0-mcpb-windows-x64.mcpb</a></td>
    </tr>
    <tr>
      <td>ARM64</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.6.0/Gearcoleco-1.6.0-mcpb-windows-arm64.mcpb">Gearcoleco-1.6.0-mcpb-windows-arm64.mcpb</a></td>
    </tr>
    <tr>
      <td rowspan="2"><strong>macOS</strong></td>
      <td>x64</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.6.0/Gearcoleco-1.6.0-mcpb-macos-x64.mcpb">Gearcoleco-1.6.0-mcpb-macos-x64.mcpb</a></td>
    </tr>
    <tr>
      <td>ARM64</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.6.0/Gearcoleco-1.6.0-mcpb-macos-arm64.mcpb">Gearcoleco-1.6.0-mcpb-macos-arm64.mcpb</a></td>
    </tr>
    <tr>
      <td rowspan="2"><strong>Linux</strong></td>
      <td>x64</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.6.0/Gearcoleco-1.6.0-mcpb-linux-x64.mcpb">Gearcoleco-1.6.0-mcpb-linux-x64.mcpb</a></td>
    </tr>
    <tr>
      <td>ARM64</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.6.0/Gearcoleco-1.6.0-mcpb-linux-arm64.mcpb">Gearcoleco-1.6.0-mcpb-linux-arm64.mcpb</a></td>
    </tr>
  </tbody>
</table>

## Features

- Full debugger access: pause, continue, step into/over/out, step frame, reset
- Memory inspection across all ColecoVision memory areas: BIOS, RAM, SGM RAM, VRAM, ROM
- Just-in-time disassembly with symbols and bank resolution
- Z80 CPU register read/write
- Hardware state inspection: TMS9918 VDP registers/status, SN76489 PSG, AY-3-8910 (SGM)
- Sprite listing and PNG image capture
- Debug symbols management (load, add, remove, list)
- Disassembler bookmarks and call stack inspection
- Memory editor: bookmarks, watches, memory search, byte finding
- Trace logger: CPU instructions, IRQs, VDP writes/status, PSG, AY-3-8910, I/O ports, SGM
- Rewind (time travel debugging)
- Screenshot capture as base64-encoded PNG
- Save state management (5 slots)
- Controller input (directional, keypad 0-9, *, #, blue, purple, left/right buttons)
- Fast forward control
- GUI integration (works with or without the GUI running)
- Two transport modes: STDIO (for AI tool integration) and HTTP (for remote access)

## Transport Modes

### STDIO Mode
The server communicates through standard input/output using JSON-RPC 2.0 messages. This is the recommended mode for AI tool integration (VS Code, Claude Desktop, etc.).

### HTTP Mode
The server listens for HTTP POST requests on a configurable port (default: 7777). Useful for remote connections or custom tooling.

### Headless Mode
Run the emulator without a GUI, using only the MCP server for control. Ideal for automated testing and CI/CD.

## Quick Start

### STDIO Mode with VS Code

1. Install the [GitHub Copilot extension](https://code.visualstudio.com/docs/copilot/overview) in VS Code.

2. Add `.vscode/mcp.json` to your workspace:

   ```json
   {
     "servers": {
       "gearcoleco": {
         "command": "/path/to/gearcoleco",
         "args": ["--mcp-stdio"]
       }
     }
   }
   ```

   Update the `command` path to match your build or release binary:
   - macOS: `/path/to/gearcoleco`
   - Linux: `/path/to/gearcoleco`
   - Windows: `C:/path/to/Gearcoleco.exe`

3. Restart VS Code if the MCP server is not discovered immediately.

### STDIO Mode with Claude Desktop

#### Option 1: Desktop Extension (Recommended)

The easiest way to install the Gearcoleco MCP server on Claude Desktop is with the MCPB package:

1. Download the latest MCPB package for your platform from the [releases page](https://github.com/drhelius/Gearcoleco/releases).

2. Install the extension:
   - Open Claude Desktop
   - Navigate to **Settings > Extensions**
   - Click **Advanced settings**
   - In the Extension Developer section, click **Install Extension...**
   - Select the downloaded `.mcpb` file

3. Start debugging. The extension is available in conversations and launches Gearcoleco when the tool is enabled.

#### Option 2: Manual Configuration

If you prefer to build from source or configure manually, edit Claude Desktop's MCP settings:

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

Config file locations:
- macOS: `~/Library/Application Support/Claude/claude_desktop_config.json`
- Windows: `%APPDATA%\Claude\claude_desktop_config.json`
- Linux: `~/.config/Claude/claude_desktop_config.json`

Restart Claude Desktop after saving the configuration.

### STDIO Mode with Claude Code

1. Add the Gearcoleco MCP server using the CLI:

   ```bash
   claude mcp add --transport stdio gearcoleco -- /path/to/gearcoleco --mcp-stdio
   ```

2. Verify the server was added:

   ```bash
   claude mcp list
   ```

### HTTP Mode

1. Start Gearcoleco manually with HTTP transport:

   ```bash
   ./gearcoleco --mcp-http
   # Server starts on http://localhost:7777/mcp

   ./gearcoleco --mcp-http --mcp-http-port 3000
   # Server starts on http://localhost:3000/mcp
   ```

   You can also start the HTTP server from the debugger's MCP Server menu.

2. Configure VS Code `.vscode/mcp.json`:

   ```json
   {
     "servers": {
       "gearcoleco": {
         "type": "http",
         "url": "http://localhost:7777/mcp",
         "headers": {}
       }
     }
   }
   ```

3. Or configure Claude Desktop:

   ```json
   {
     "mcpServers": {
       "gearcoleco": {
         "type": "http",
         "url": "http://localhost:7777/mcp"
       }
     }
   }
   ```

4. Or configure Claude Code:

   ```bash
   claude mcp add --transport http gearcoleco http://localhost:7777/mcp
   ```

> **Note:** The MCP HTTP server must be running before connecting the AI client.

## Usage Examples

Once configured, you can ask your AI assistant:

### Basic Commands

- "What game is currently loaded?"
- "Load the ROM at /path/to/game.col"
- "Show me the current CPU registers"
- "Read 16 bytes from RAM starting at offset 0x0000"
- "Set a breakpoint at address 0x0038"
- "Pause execution and show me all sprites"
- "Step through the next 5 instructions"
- "Capture a screenshot of the current frame"
- "Tap keypad 1 on player 1 controller"

### Advanced Debugging Workflows

- "Find the interrupt handler, analyze what it does, and add symbols for the subroutines it calls"
- "Inspect the TMS9918 sprite attribute table, list active sprites, and capture the sprite images"
- "Locate the routine that updates score digits in RAM and add memory watches for those addresses"
- "Trace CPU instructions and VDP writes for one frame, then summarize the rendering flow"
- "Compare SGM RAM before and after this routine runs and identify what data structure changed"

## Available MCP Tools

### Execution Control
| Tool | Description |
|------|-------------|
| `debug_pause` | Pause emulation |
| `debug_continue` | Continue emulation |
| `debug_step_into` | Step one instruction |
| `debug_step_over` | Step over calls |
| `debug_step_out` | Step out of current call |
| `debug_step_frame` | Step one full frame |
| `debug_reset` | Reset the ColecoVision system |
| `debug_get_status` | Get current debug state |

### CPU & Registers
| Tool | Description |
|------|-------------|
| `get_z80_status` | Get Z80 CPU registers, flags, interrupt state |
| `write_z80_register` | Write to a Z80 register |

### Memory Operations
| Tool | Description |
|------|-------------|
| `list_memory_areas` | List available memory editor areas |
| `read_memory` | Read bytes from a memory area |
| `write_memory` | Write bytes to a memory area |

### Disassembly & Debugging
| Tool | Description |
|------|-------------|
| `get_disassembly` | Disassemble address range with symbols |
| `debug_run_to_cursor` | Run to a specific address |
| `add_disassembler_bookmark` / `remove_disassembler_bookmark` | Manage bookmarks |
| `list_disassembler_bookmarks` | List all bookmarks |
| `add_symbol` / `remove_symbol` | Manage debug symbols |
| `load_symbols` | Load symbols from file |
| `list_symbols` | List all symbols |
| `get_call_stack` | Get current call stack |

### Breakpoints
| Tool | Description |
|------|-------------|
| `set_breakpoint` | Set breakpoint (rom_ram, vram, vdp_reg) |
| `set_breakpoint_range` | Set breakpoint on address range |
| `remove_breakpoint` | Remove a breakpoint |
| `list_breakpoints` | List all breakpoints |
| `toggle_irq_breakpoints` | Enable/disable IRQ breakpoints |

### Hardware Status
| Tool | Description |
|------|-------------|
| `get_vdp_registers` | Get all 8 TMS9918 VDP register values and decoded fields |
| `get_vdp_status` | Get VDP status flags, mode, render state |
| `get_psg_status` | Get SN76489 PSG channel state |
| `get_ay8910_status` | Get AY-3-8910 SGM sound chip state |
| `get_media_info` | Get loaded ROM information |

### Sprites & Screen
| Tool | Description |
|------|-------------|
| `list_sprites` | List TMS9918 sprite attributes |
| `get_sprite_image` | Capture sprite as PNG |
| `get_screenshot` | Capture screen as PNG |

### Media & State Management
| Tool | Description |
|------|-------------|
| `load_media` | Load a ROM file |
| `list_save_state_slots` | List save state slots |
| `select_save_state_slot` | Select active slot |
| `save_state` / `load_state` | Save/load state |
| `set_fast_forward_speed` | Set fast forward multiplier |
| `toggle_fast_forward` | Toggle fast forward |
| `get_rewind_status` | Get rewind buffer status |
| `rewind_seek` | Seek to rewind snapshot |

### Tracing
| Tool | Description |
|------|-------------|
| `get_trace_log` | Get trace log entries |
| `set_trace_log` | Enable/disable trace logging with type flags |

### Controller Input
| Tool | Description |
|------|-------------|
| `controller_button` | Press/release controller buttons (directional, keypad, yellow/red, blue/purple) |

### Memory Editor
| Tool | Description |
|------|-------------|
| `select_memory_range` / `get_memory_selection` / `set_memory_selection_value` | Range operations |
| `add_memory_bookmark` / `remove_memory_bookmark` / `list_memory_bookmarks` | Bookmark management |
| `add_memory_watch` / `remove_memory_watch` / `list_memory_watches` | Watch management |
| `memory_search_capture` / `memory_search` / `memory_find_bytes` | Memory searching |

## Hardware Resources

The MCP server also exposes hardware reference documents via `resources/list` and `resources/read`:

| Resource URI | Description |
|--------------|-------------|
| `gearcoleco://hardware/colecovision` | Memory map, I/O ports, controllers, SGM RAM windows, breakpoint areas |
| `gearcoleco://hardware/tms9918a` | VDP registers, status flags, VRAM tables, sprites |
| `gearcoleco://hardware/sn76489` | Base SN76489 PSG channels and write model |
| `gearcoleco://hardware/super_game_module` | SGM RAM mapping, ADAM compatibility, and AY-3-8910 port summary |

## How MCP Works in Gearcoleco

The MCP server runs alongside the emulator GUI in a background thread.

- The emulator GUI remains fully functional while MCP tools are connected.
- Commands from the AI client are queued and executed on the emulator thread.
- Both GUI and MCP share the same emulator state.
- Changes made through MCP are reflected in the debugger UI and vice versa.

## Architecture

### STDIO Transport

```text
+-----------------+                    +------------------+
| VS Code /       |       stdio        | Gearcoleco       |
| Claude Desktop  |<------------------>| MCP Server       |
| (AI Client)     |       pipes        | (background)     |
+-----------------+                    +------------------+
  |                                      |
  +---- launches ---------------------->|
                 | shared state
                 v
              +------------------+
              | Emulator Core    |
              | + GUI Window     |
              +------------------+
```

### HTTP Transport

```text
+-----------------+                    +------------------+
| VS Code /       |  HTTP port 7777    | Gearcoleco       |
| Claude Desktop  |<------------------>| MCP HTTP Server  |
| (AI Client)     |                    | (listener)       |
+-----------------+                    +------------------+
                 |
                 | shared state
                 v
              +------------------+
              | Emulator Core    |
              | + GUI Window     |
              +------------------+
```
