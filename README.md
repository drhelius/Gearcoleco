# Gearcoleco

[![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/drhelius/Gearcoleco/gearcoleco.yml)](https://github.com/drhelius/Gearcoleco/actions/workflows/gearcoleco.yml)
[![GitHub Releases)](https://img.shields.io/github/v/tag/drhelius/Gearcoleco?label=version)](https://github.com/drhelius/Gearcoleco/releases)
[![commits)](https://img.shields.io/github/commit-activity/t/drhelius/Gearcoleco)](https://github.com/drhelius/Gearcoleco/commits/main)
[![GitHub contributors](https://img.shields.io/github/contributors/drhelius/Gearcoleco)](https://github.com/drhelius/Gearcoleco/graphs/contributors)
[![GitHub Sponsors](https://img.shields.io/github/sponsors/drhelius)](https://github.com/sponsors/drhelius)
[![License](https://img.shields.io/github/license/drhelius/Gearcoleco)](https://github.com/drhelius/Gearcoleco/blob/main/LICENSE)
[![Twitter Follow](https://img.shields.io/twitter/follow/drhelius)](https://x.com/drhelius)

Gearcoleco is a very accurate, cross-platform ColecoVision and Coleco ADAM emulator written in C++ that runs on Windows, macOS, Linux, BSD and RetroArch, with an embedded MCP server for AI debugging and development.

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
      <td>Desktop x64</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.7.0/Gearcoleco-1.7.0-desktop-windows-x64.zip">Gearcoleco-1.7.0-desktop-windows-x64.zip</a></td>
    </tr>
    <tr>
      <td>Desktop ARM64</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.7.0/Gearcoleco-1.7.0-desktop-windows-arm64.zip">Gearcoleco-1.7.0-desktop-windows-arm64.zip</a></td>
    </tr>
    <tr>
      <td rowspan="3"><strong>macOS</strong></td>
      <td>Homebrew</td>
      <td><code>brew install --cask drhelius/geardome/gearcoleco</code></td>
    </tr>
    <tr>
      <td>Desktop Apple Silicon</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.7.0/Gearcoleco-1.7.0-desktop-macos-arm64.zip">Gearcoleco-1.7.0-desktop-macos-arm64.zip</a></td>
    </tr>
    <tr>
      <td>Desktop Intel</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.7.0/Gearcoleco-1.7.0-desktop-macos-intel.zip">Gearcoleco-1.7.0-desktop-macos-intel.zip</a></td>
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
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.7.0/Gearcoleco-1.7.0-desktop-ubuntu24.04-x64.zip">Gearcoleco-1.7.0-desktop-ubuntu24.04-x64.zip</a></td>
    </tr>
    <tr>
      <td>Desktop Ubuntu 22.04 x64</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.7.0/Gearcoleco-1.7.0-desktop-ubuntu22.04-x64.zip">Gearcoleco-1.7.0-desktop-ubuntu22.04-x64.zip</a></td>
    </tr>
    <tr>
      <td>Desktop Ubuntu 24.04 ARM64</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.7.0/Gearcoleco-1.7.0-desktop-ubuntu24.04-arm64.zip">Gearcoleco-1.7.0-desktop-ubuntu24.04-arm64.zip</a></td>
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
- **Homebrew**: If Homebrew asks you to trust the third-party tap, run `brew trust --tap drhelius/geardome`
- **Linux**: May need `libsdl3`

## Features

- Very accurate Z80 core, TMS9918A VDP, SN76489 PSG and AY-3-8910 SGM emulation.
- Support for ColecoVision Super Game Module (SGM) and MegaCart ROMs.
- Coleco ADAM computer and cartridge boot modes, full keyboard, ADAMnet, printer, Digital Data Pack and floppy support.
- Optional F18A v1.9 emulation.
- Support for Super Action Controller (SAC), Wheel Controller and Roller Controller.
- Save states with preview and rewind support.
- Run-ahead support to reduce input latency.
- Compressed ROM support (ZIP).
- VGM recorder.
- Supported platforms (standalone): Windows, Linux, BSD and macOS.
- Supported platforms (libretro): Windows, Linux, macOS, Raspberry Pi, Android, iOS, tvOS, webOS, PlayStation Vita, PlayStation 3, Nintendo 3DS, Nintendo GameCube, Nintendo Wii, Nintendo WiiU, Nintendo Switch, Emscripten, Classic Mini systems (NES, SNES, C64, ...), OpenDingux, RetroFW and QNX.
- Full debugger with just-in-time disassembler, CPU breakpoints, memory access breakpoints, code navigation (goto address, JP JR and CALL double clicking), debug symbols, automatic labels, memory editor, trace logger, IO inspector and VRAM viewer including registers, tiles, sprites and backgrounds.
- MCP server for AI-assisted debugging with GitHub Copilot, Claude, Codex and similar, exposing tools for execution control, memory inspection, hardware status, rewind and more.
- Windows, Linux and macOS *Portable Mode*.
- [Programmable Shader Chain](platforms/shared/desktop/shaders/README.md).
- Content loading from the command line by adding the cartridge, ADAM media or playlist path as an argument.
- Content loading using drag & drop.
- Support for modern game controllers through [gamecontrollerdb.txt](https://github.com/mdqinc/SDL_GameControllerDB) file located in the same directory as the application binary.

## Tips

### Basic Usage

- **BIOS and ADAM Firmware**: Firmware is not included. Select the ColecoVision BIOS (OS-7) under **Emulator > BIOS**; the original BIOS is recommended (CRC32 `3aa93ef3`; filenames `colecovision.rom`, `coleco.rom` or `os7.u2`). ADAM additionally needs **ADAM > EOS ROM** (`eos.rom`, 8192 bytes, common CRC32 `05a37a34`) and **ADAM > SmartWriter ROM** (`writer.rom`, `wp.rom` or `wp_r80.rom`, 32768 bytes, common CRC32 `58d86a2a`).
- **ADAM Startup and Media**: Insert images through the **ADAM** menu's disk or data pack submenus, then select **Power On**. Supported formats are 256 KiB `.ddp`, 160/320 KiB `.dsk`, ZIP archives containing one valid ADAM image, and `.m3u` playlists containing images of the same type. Each drive has its own recent images, playlist and write protection controls. Insert/eject affects only that drive; dropped images use an available matching drive or prompt for a destination. **Gearcoleco > Open ROM...** starts ColecoVision cartridges.
- **ADAM Power and Saves**: **Power Off** saves modified media and keeps it inserted. Firmware changes apply at the next **Power On**; **Computer Reset** retains the current firmware and mounted media. Changes are stored as complete images under **Save Files**, leaving source files untouched. Drive menus also provide **Save Changes** and **Save As**; a failed save keeps the modified image mounted.
- **ADAM Input**: Type directly into ADAM; in debug mode, focus **Output** to type or a debugger tool to use its shortcuts. Configure special keys under **ADAM > Keyboard** (see [defaults below](#desktop-adam-keyboard)); `F12` toggles fullscreen. **ADAM > Controller 1 / Controller 2** selects gamepad (default), keyboard or no hand controller per port. Bindings come from **Input > Gamepads** or **Input > Keyboard Configuration**; keys mapped to a hand controller do not type into ADAM. ADAM controller choices are saved separately from ColecoVision settings.
- **Spinners**: Use **Input > Spinners > Capture Mouse** or the configured Capture Mouse shortcut. Disable spinners for software that does not use them.
- **Rewind**: Hold the configured rewind hotkey (`Backspace` by default) or a mapped gamepad shortcut to step backwards through recent gameplay.
- **Overscan**: For a precise representation of the original image, select **Overscan** `Top+Bottom` and **Aspect Ratio** `Standard (4:3 DAR)` in the **Video** menu.
- **Mouse Cursor**: Automatically hides when hovering over the main output window or when Main Menu is disabled.
- **Portable Mode**: Run with `--portable`, or create an empty file named `portable.ini` in the same directory as the application binary. On macOS, place the file next to the `.app` bundle.

### Desktop ADAM Keyboard

Normal letters, digits, punctuation, Return, Escape, Backspace, Tab, Shift, Control, Caps Lock and arrow keys map directly. The dedicated ADAM keys use these defaults:

| Host key | ADAM key |
|---|---|
| F1-F6 | SmartKey I-VI |
| F7 / F8 | Undo / Wild Card |
| Home / Insert / Delete | ADAM Home / Insert / Delete |
| Page Up / Page Down | Move-Copy / Store-Fetch |
| End | Clear |
| Print Screen | Print |

Saved keyboard mappings are preserved. Use **ADAM > Keyboard > Restore Defaults** to adopt this layout in an existing configuration.

When ADAM owns keyboard focus, overlapping emulator hotkeys are available from the menus so SmartKeys and editing keys reach the emulated keyboard. Leaving the window releases every held ADAM key.

ADAM keyboard input follows focus automatically. In normal use, type directly; in debug mode, focus Output to type into ADAM or focus a debugger tool to use its shortcuts. `F12` remains the default fullscreen shortcut.

### ADAM in Libretro

- **Startup**: Place OS-7, EOS and SmartWriter in the frontend system directory or its `gearcoleco` subdirectory using the [firmware names above](#basic-usage). ADAM starts automatically for `.ddp`, `.dsk`, ADAM `.zip`, `.m3u` or no content; without bootable media it opens SmartWriter. Cartridges use ColecoVision unless **Cartridge Hardware** is set to ADAM.
- **Media and Saves**: Media is write protected by default. Enable `Save-directory working copy` to save complete modified images in the frontend save directory, leaving source files untouched. **ADAM Disk Control Drive** selects Disk 1/2 or Data Pack 1/2; **Loaded media** (default) targets the primary content drive. Each drive has its own image list. Swap through the frontend's eject, select and close sequence; other drives stay mounted.
- **Multiple Images**: The optional **ADAM** subsystem (`adam`) loads up to five slots before boot: **Cartridge, Disk 1, Disk 2, Data Pack 1, Data Pack 2**, in that order. All slots are optional and each drive supports playlists. Update older three-slot configurations to this layout; use normal content loading for a single image or playlist.
- **Keyboard**: Bindings are fixed to the [desktop defaults](#desktop-adam-keyboard), including Escape for Escape/WP and Shift/Control as modifiers. In RetroArch, set **Settings > Input > Auto Enable 'Game Focus' Mode** to **Detect**, or toggle Game Focus with Scroll Lock before typing. Turn it off to use RetroArch shortcuts. Without Scroll Lock, assign **Game Focus Toggle** to F10; F9-F12 and Scroll Lock are unmapped in ADAM.
- **Reset**: Frontend Reset keeps the current boot mode. **ADAM Computer Reset** boots from mounted media without ejecting it; if the option does not return to Idle automatically, select Idle before requesting another reset.

### Debugging Features
- **Docking Windows**: In debug mode, you can dock windows together by pressing SHIFT and dragging a window onto another.
- **Multi-viewport**: In Windows or macOS, you can enable "multi-viewport" in the debug menu. You must restart the emulator for the change to take effect. Once enabled, you can drag debugger windows outside the main window.
- **Single Instance**: You can enable "Single Instance" in the `Emulator` menu. When enabled, opening a ROM while another instance is running will send the ROM to the running instance instead of starting a new one.
- **Debug Symbols**: The emulator automatically tries to load a symbol file when loading a ROM (.sym, .noi). For example, for `path_to_rom_file.rom` it tries to load `path_to_rom_file.sym`. You can also load a symbol file using the GUI or the CLI. It supports SDCC/NoICE (.noi), wla-dx and vasm/generic file formats.
- **Rewind Scrubbing**: In debug mode, pause emulation and open the Rewind window to scrub through captured snapshots.

### Command Line Usage
```
gearcoleco [options] [content_file] [symbol_file]

Arguments:
  [content_file]              Cartridge or ADAM media (.col, .cv, .rom, .bin, .ddp, .dsk, .zip, .m3u)
  [symbol_file]               Optional symbol file for debugging

Options:
  -f, --fullscreen            Start in fullscreen mode
  -w, --windowed              Start in windowed mode with menu visible
      --mcp-stdio             Auto-start MCP server with stdio transport
      --mcp-http              Auto-start MCP server with HTTP transport
      --mcp-router            Enable compact MCP tool routing
      --mcp-http-address A    HTTP bind address (default: 127.0.0.1)
      --mcp-http-port N       HTTP port for MCP server (default: 7777)
      --headless              Run without GUI (requires --mcp-stdio or --mcp-http)
      --portable              Store configuration and user data beside the application
  -v, --version               Display version information
  -h, --help                  Display this help message
```

### MCP Server

Gearcoleco includes a [Model Context Protocol](https://modelcontextprotocol.io/introduction) (MCP) server that enables AI-assisted debugging through AI agents like GitHub Copilot, Claude, Codex and similar. The server provides tools for execution control, memory inspection, breakpoints, disassembly, hardware status, and more. STDIO and HTTP transports are supported, with STDIO preferred.

For complete setup instructions and tool documentation, see [MCP_README.md](MCP_README.md).

### Agent Skills

Gearcoleco provides [Agent Skills](https://agentskills.io/) that teach AI assistants how to effectively use the emulator for specific tasks:

- **[gearcoleco-debugging](skills/gearcoleco-debugging/SKILL.md)** — Game debugging, code tracing, breakpoint management, hardware inspection, and reverse engineering.
- **[gearcoleco-romhacking](skills/gearcoleco-romhacking/SKILL.md)** — Cheat creation, memory searching, ROM data modification, text translation, and game patching.

Install with `npx skills add drhelius/gearcoleco`. See the [skills README](skills/README.md) for details.

## Build Instructions

### Windows

- Install Microsoft Visual Studio Community 2026 or later.
- Download the latest SDL3 VC development libraries from [SDL3 Releases](https://github.com/libsdl-org/SDL/releases) (the file named `SDL3-devel-x.y.z-VC.zip`).
- Extract the archive and rename the resulting folder (e.g. `SDL3-x.y.z`) to `SDL3`.
- Place the `SDL3` folder inside `platforms/windows/dependencies/` so that the include path is `platforms/windows/dependencies/SDL3/include/SDL3/`.
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

If you are using Ubuntu 25.04 or later, you can install SDL3 directly. Use the following commands to build:

``` shell
sudo apt install build-essential libsdl3-dev
cd platforms/linux
make
```

For older Ubuntu versions (22.04, 24.04), you need to build SDL3 from source first. Use the following commands to build both SDL3 and Gearcoleco:

``` shell
sudo apt install build-essential cmake git curl jq pkg-config \
  libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev \
  libxi-dev libxss-dev libxkbcommon-dev libwayland-dev libdecor-0-dev \
  libdrm-dev libgbm-dev libgl1-mesa-dev libegl1-mesa-dev libdbus-1-dev libudev-dev libxtst-dev
SDL3_TAG=$(curl -s https://api.github.com/repos/libsdl-org/SDL/releases/latest | jq -r '.tag_name')
git clone --depth 1 --branch "$SDL3_TAG" https://github.com/libsdl-org/SDL.git /tmp/SDL3
cmake -S /tmp/SDL3 -B /tmp/SDL3/build -DCMAKE_INSTALL_PREFIX=/usr -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF
cmake --build /tmp/SDL3/build -j$(nproc)
sudo cmake --install /tmp/SDL3/build
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
