# Gearcoleco

[![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/drhelius/Gearcoleco/gearcoleco.yml)](https://github.com/drhelius/Gearcoleco/actions/workflows/gearcoleco.yml)
[![GitHub Releases)](https://img.shields.io/github/v/tag/drhelius/Gearcoleco?label=version)](https://github.com/drhelius/Gearcoleco/releases)
[![commits)](https://img.shields.io/github/commit-activity/t/drhelius/Gearcoleco)](https://github.com/drhelius/Gearcoleco/commits/main)
[![GitHub contributors](https://img.shields.io/github/contributors/drhelius/Gearcoleco)](https://github.com/drhelius/Gearcoleco/graphs/contributors)
[![GitHub Sponsors](https://img.shields.io/github/sponsors/drhelius)](https://github.com/sponsors/drhelius)
[![License](https://img.shields.io/github/license/drhelius/Gearcoleco)](https://github.com/drhelius/Gearcoleco/blob/main/LICENSE)
[![Twitter Follow](https://img.shields.io/twitter/follow/drhelius)](https://x.com/drhelius)

Gearcoleco is a very accurate cross-platform ColecoVision emulator written in C++ that runs on Windows, macOS, Linux, BSD and RetroArch.

This is an open source project with its ongoing development made possible thanks to the support by these awesome [backers](backers.md). If you find it useful, please consider [sponsoring](https://github.com/sponsors/drhelius).

Don't hesitate to report bugs or ask for new features by [opening an issue](https://github.com/drhelius/Gearcoleco/issues).

<img src="http://www.geardome.com/files/gearcoleco/gearcoleco_debug_02.png">

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
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.6.0/Gearcoleco-1.6.0-desktop-windows-x64.zip">Gearcoleco-1.6.0-desktop-windows-x64.zip</a></td>
    </tr>
    <tr>
      <td>ARM64</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.6.0/Gearcoleco-1.6.0-desktop-windows-arm64.zip">Gearcoleco-1.6.0-desktop-windows-arm64.zip</a></td>
    </tr>
    <tr>
      <td rowspan="3"><strong>macOS</strong></td>
      <td>Homebrew</td>
      <td><code>brew install --cask drhelius/geardome/gearcoleco</code></td>
    </tr>
    <tr>
      <td>Desktop Apple Silicon</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.6.0/Gearcoleco-1.6.0-desktop-macos-arm64.zip">Gearcoleco-1.6.0-desktop-macos-arm64.zip</a></td>
    </tr>
    <tr>
      <td>Desktop Intel</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.6.0/Gearcoleco-1.6.0-desktop-macos-intel.zip">Gearcoleco-1.6.0-desktop-macos-intel.zip</a></td>
    </tr>
    <tr>
      <td rowspan="5"><strong>Linux</strong></td>
      <td>Ubuntu PPA</td>
      <td><a href="https://github.com/drhelius/ppa-geardome">drhelius/ppa-geardome</a></td>
    </tr>
    <tr>
      <td>Fedora RPM</td>
      <td><a href="https://github.com/drhelius/rpm-geardome">drhelius/rpm-geardome</a></td>
    </tr>
    <tr>
      <td>Desktop Ubuntu 24.04 x64</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.6.0/Gearcoleco-1.6.0-desktop-ubuntu24.04-x64.zip">Gearcoleco-1.6.0-desktop-ubuntu24.04-x64.zip</a></td>
    </tr>
    <tr>
      <td>Desktop Ubuntu 22.04 x64</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.6.0/Gearcoleco-1.6.0-desktop-ubuntu22.04-x64.zip">Gearcoleco-1.6.0-desktop-ubuntu22.04-x64.zip</a></td>
    </tr>
    <tr>
      <td>Desktop Ubuntu 24.04 ARM64</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.6.0/Gearcoleco-1.6.0-desktop-ubuntu24.04-arm64.zip">Gearcoleco-1.6.0-desktop-ubuntu24.04-arm64.zip</a></td>
    </tr>
    <tr>
      <td><strong>MCPB</strong></td>
      <td>All platforms</td>
      <td><a href="MCP_README.md">MCP Readme</a></td>
    </tr>
    <tr>
      <td><strong>RetroArch</strong></td>
      <td>All platforms</td>
      <td><a href="https://docs.libretro.com/library/gearcoleco/">Libretro core documentation</a></td>
    </tr>
    <tr>
      <td><strong>Dev Builds</strong></td>
      <td>All platforms</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/actions/workflows/gearcoleco.yml">GitHub Actions</a></td>
    </tr>
  </tbody>
</table>

**Notes:**
- **Windows**: May need [Visual C++ Redistributable](https://go.microsoft.com/fwlink/?LinkId=746572) and [OpenGL Compatibility Pack](https://apps.microsoft.com/detail/9nqpsl29bfff)
- **Linux**: May need `libsdl3`

## Features

- Accurate Z80 core, including undocumented opcodes and behavior like R and [MEMPTR](https://gist.github.com/drhelius/8497817) registers.
- Accurate TMS9918 emulation.
- Support for ColecoVision Super Game Module (SGM) and MegaCart ROMs.
- Support for Super Action Controller (SAC), Wheel Controller and Roller Controller.
- Save states.
- Hold-to-rewind support in the desktop app.
- Compressed rom support (ZIP).
- VGM recorder.
- Supported platforms (standalone): Windows, Linux, BSD and macOS.
- Supported platforms (libretro): Windows, Linux, macOS, Raspberry Pi, Android, iOS, tvOS, PlayStation Vita, PlayStation 3, Nintendo 3DS, Nintendo GameCube, Nintendo Wii, Nintendo WiiU, Nintendo Switch, Emscripten, Classic Mini systems (NES, SNES, C64, ...), OpenDingux, RetroFW and QNX.
- Full debugger with just-in-time disassembler, CPU breakpoints, memory access breakpoints, code navigation (goto address, JP JR and CALL double clicking), debug symbols, memory editor, IO inspector and VRAM viewer including registries, tiles, sprites and backgrounds.
- MCP server for AI-assisted debugging with GitHub Copilot, Claude, Codex and similar, exposing tools for execution control, memory inspection, hardware status, rewind and more.
- Windows and Linux *Portable Mode*.
- ROM loading from the command line by adding the ROM path as an argument.
- ROM loading using drag & drop.
- Support for modern game controllers through [gamecontrollerdb.txt](https://github.com/mdqinc/SDL_GameControllerDB) file located in the same directory as the application binary.

## Tips

### Basic Usage
- **BIOS**: Gearcoleco needs a BIOS to run. It is possible to load any BIOS but the original one with md5 ```2c66f5911e5b42b8ebe113403548eee7``` is recommended.
- **Spinners**: When using any kind of spinner it is useful to capture the mouse by pressing ```F12```. It is also recommended to disable spinners for software that don't use them.
- **Rewind**: Hold the configured rewind hotkey (```Backspace``` by default) or a mapped gamepad shortcut to step backwards through recent gameplay.
- **Overscan**: For a precise representation of the original image, using Overscan Top+Bottom and 4:3 Display Aspect Ratio is recommended.
- **Mouse Cursor**: Automatically hides when hovering over the main output window or when Main Menu is disabled.
- **Portable Mode**: Create an empty file named `portable.ini` in the same directory as the application binary to enable portable mode.

### Debugging Features
- **Docking Windows**: In debug mode, you can dock windows together by pressing SHIFT and dragging a window onto another.
- **Multi-viewport**: In Windows or macOS, you can enable "multi-viewport" in the debug menu. You must restart the emulator for the change to take effect. Once enabled, you can drag debugger windows outside the main window.
- **Debug Symbols**: The emulator automatically tries to load a symbol file when loading a ROM. For example, for ```path_to_rom_file.rom``` it tries to load ```path_to_rom_file.sym```. You can also load a symbol file using the GUI or the CLI.
- **Rewind Scrubbing**: In debug mode, pause emulation and open the Rewind window to scrub through captured snapshots.

### Command Line Usage
```
gearcoleco [options] [rom_file] [symbol_file]

Arguments:
  [rom_file]               ROM file: accepts ROMs (.col, .cv, .rom, .bin) or ZIP (.zip)
  [symbol_file]            Optional symbol file for debugging

Options:
  -f, --fullscreen         Start in fullscreen mode
  -w, --windowed           Start in windowed mode with menu visible
      --mcp-stdio          Auto-start MCP server with stdio transport
      --mcp-http           Auto-start MCP server with HTTP transport
      --mcp-http-port N    HTTP port for MCP server (default: 7777)
      --headless           Run without GUI (requires --mcp-stdio or --mcp-http)
  -v, --version            Display version information
  -h, --help               Display this help message
```

### MCP Server

Gearcoleco includes a [Model Context Protocol](https://modelcontextprotocol.io/introduction) (MCP) server that enables AI-assisted debugging through AI agents like GitHub Copilot, Claude, Codex and similar. The server provides tools for execution control, memory inspection, breakpoints, disassembly, hardware status, and more.

For complete setup instructions and tool documentation, see [MCP_README.md](MCP_README.md).

### Agent Skills

Gearcoleco provides [Agent Skills](https://agentskills.io/) that teach AI assistants how to effectively use the emulator for specific tasks:

- **[gearcoleco-debugging](skills/gearcoleco-debugging/SKILL.md)** — Game debugging, code tracing, breakpoint management, hardware inspection, and reverse engineering.
- **[gearcoleco-romhacking](skills/gearcoleco-romhacking/SKILL.md)** — Cheat creation, memory searching, ROM data modification, text translation, and game patching.

Install with `npx skills add drhelius/gearcoleco`. See the [skills README](skills/README.md) for details.
  
## Build Instructions

### Windows

- Install Microsoft Visual Studio Community 2022 or later.
- Open the Gearcoleco Visual Studio solution `platforms/windows/Gearcoleco.sln` and build.

### macOS

- Install Xcode and run `xcode-select --install` in the terminal for the compiler to be available on the command line.
- Run these commands to generate a Mac *app* bundle:

``` shell
brew install sdl3
cd platforms/macos
make dist
```

### Linux

- Ubuntu / Debian / Raspberry Pi (Raspbian):

``` shell
sudo apt install build-essential libsdl3-dev
cd platforms/linux
make
```

- Fedora:

``` shell
sudo dnf install @development-tools gcc-c++ SDL3-devel
cd platforms/linux
make
```

- Arch Linux:

``` shell
sudo pacman -S base-devel sdl3
cd platforms/linux
make
```

### BSD

- FreeBSD:

``` shell
su root -c "pkg install -y git gmake pkgconf sdl3"
cd platforms/bsd
USE_CLANG=1 gmake
```

- NetBSD:

``` shell
su root -c "pkgin install gmake pkgconf SDL3"
cd platforms/bsd
gmake
```

- OpenBSD

``` shell
doas pkg_add gmake sdl3
cd platforms/bsd
LDFLAGS=-L/usr/X11R6/lib/ USE_CLANG=1 gmake
```

### Libretro

- Ubuntu / Debian / Raspberry Pi (Raspbian):

``` shell
sudo apt install build-essential
cd platforms/libretro
make
```

- Fedora:

``` shell
sudo dnf install @development-tools gcc-c++
cd platforms/libretro
make
```

## Screenshots

<img width="400" alt="Screen Shot 2021-08-14 at 21 20 23" src="https://user-images.githubusercontent.com/863613/129458245-3b358dfe-54f1-4f9a-b278-070bfba5046b.png"><img width="400" alt="Screen Shot 2021-08-14 at 21 18 33" src="https://user-images.githubusercontent.com/863613/129458264-267085c4-bd14-4db0-8565-01a0e9d0a61c.png">
<img width="400" alt="Screen Shot 2021-08-14 at 21 27 59" src="https://user-images.githubusercontent.com/863613/129458283-31f24c6b-fc68-4cf3-a0da-0ff0a5508892.png"><img width="400" alt="Screen Shot 2021-08-14 at 21 29 49" src="https://user-images.githubusercontent.com/863613/129458284-dd58f916-2f32-43f5-b657-69e567e2e6b8.png">
<img width="400" alt="Screen Shot 2021-08-14 at 21 24 57" src="https://user-images.githubusercontent.com/863613/129458294-09f56d23-338c-45b8-a714-de89db38c94c.png"><img width="400" alt="Screen Shot 2021-08-14 at 21 25 20" src="https://user-images.githubusercontent.com/863613/129458275-3f5333f3-76c0-4761-9cf4-8d105fa407d4.png">
<img width="400" alt="Screen Shot 2021-08-14 at 21 24 42" src="https://user-images.githubusercontent.com/863613/129458298-f9b51899-f81c-496d-a3c6-3b863cf459f5.png"><img width="400" alt="Screen Shot 2021-08-14 at 21 27 31" src="https://user-images.githubusercontent.com/863613/129458280-782daf18-b0a1-4803-a880-7476e8871642.png">

## Contributors

Thank you to all the people who have already contributed to Gearcoleco!

[![Contributors](https://contrib.rocks/image?repo=drhelius/gearcoleco)](https://github.com/drhelius/gearcoleco/graphs/contributors)

## License

Gearcoleco is licensed under the GNU General Public License v3.0 License, see [LICENSE](LICENSE) for more information.
