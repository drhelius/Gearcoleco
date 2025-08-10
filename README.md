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

<img src="http://www.geardome.com/files/gearcoleco/gearcoleco_debug_01.png">

## Downloads

<table>
  <thead>
    <tr>
      <th>Platform</th>
      <th>Architecture</th>
      <th>Download Link</th>
      <th>Notes</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td rowspan="2"><strong>Windows</strong></td>
      <td>x64</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.5.2/Gearcoleco-1.5.2-windows-x64.zip">Gearcoleco-1.5.2-windows-x64.zip</a></td>
      <td rowspan="2">May need <a href="https://go.microsoft.com/fwlink/?LinkId=746572">Visual C++ Redistributable</a> and <a href="https://apps.microsoft.com/detail/9nqpsl29bfff">OpenGL Compatibility Pack</a></td>
    </tr>
    <tr>
      <td>ARM64</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.5.2/Gearcoleco-1.5.2-windows-arm64.zip">Gearcoleco-1.5.2-windows-arm64.zip</a></td>
    </tr>
    <tr>
      <td rowspan="2"><strong>macOS</strong></td>
      <td>Apple Silicon</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.5.2/Gearcoleco-1.5.2-macos-arm.zip">Gearcoleco-1.5.2-macos-arm.zip</a></td>
      <td rowspan="2"></td>
    </tr>
    <tr>
      <td>Intel</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.5.2/Gearcoleco-1.5.2-macos-intel.zip">Gearcoleco-1.5.2-macos-intel.zip</a></td>
    </tr>
    <tr>
      <td rowspan="2"><strong>Linux</strong></td>
      <td>Ubuntu 24.04</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.5.2/Gearcoleco-1.5.2-ubuntu-24.04.zip">Gearcoleco-1.5.2-ubuntu-24.04.zip</a></td>
      <td rowspan="2">May need <code>libsdl2</code></td>
    </tr>
    <tr>
      <td>Ubuntu 22.04</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/releases/download/1.5.2/Gearcoleco-1.5.2-ubuntu-22.04.zip">Gearcoleco-1.5.2-ubuntu-22.04.zip</a></td>
    </tr>
    <tr>
      <td><strong>RetroArch</strong></td>
      <td>All platforms</td>
      <td><a href="https://docs.libretro.com/library/gearcoleco/">Libretro core documentation</a></td>
      <td></td>
    </tr>
    <tr>
      <td><strong>Dev Builds</strong></td>
      <td>All platforms</td>
      <td><a href="https://github.com/drhelius/Gearcoleco/actions/workflows/gearcoleco.yml">GitHub Actions</a></td>
      <td>Latest development builds</td>
    </tr>
  </tbody>
</table>

## Features

- Accurate Z80 core, including undocumented opcodes and behavior like R and [MEMPTR](https://gist.github.com/drhelius/8497817) registers.
- Accurate TMS9918 emulation.
- Support for ColecoVision Super Game Module (SGM) and MegaCart ROMs.
- Support for Super Action Controller (SAC), Wheel Controller and Roller Controller.
- Save states.
- Compressed rom support (ZIP).
- Supported platforms (standalone): Windows, Linux, BSD and macOS.
- Supported platforms (libretro): Windows, Linux, macOS, Raspberry Pi, Android, iOS, tvOS, PlayStation Vita, PlayStation 3, Nintendo 3DS, Nintendo GameCube, Nintendo Wii, Nintendo WiiU, Nintendo Switch, Emscripten, Classic Mini systems (NES, SNES, C64, ...), OpenDingux, RetroFW and QNX.
- Full debugger with just-in-time disassembler, CPU breakpoints, memory access breakpoints, code navigation (goto address, JP JR and CALL double clicking), debug symbols, memory editor, IO inspector and VRAM viewer including registries, tiles, sprites and backgrounds.
- Windows and Linux *Portable Mode*.
- ROM loading from the command line by adding the ROM path as an argument.
- ROM loading using drag & drop.
- Support for modern game controllers through [gamecontrollerdb.txt](https://github.com/mdqinc/SDL_GameControllerDB) file located in the same directory as the application binary.

## Tips

### Basic Usage
- **BIOS**: Gearcoleco needs a BIOS to run. It is possible to load any BIOS but the original one with md5 ```2c66f5911e5b42b8ebe113403548eee7``` is recommended.
- **Spinners**: When using any kind of spinner it is useful to capture the mouse by pressing ```F12```. It is also recommended to disable spinners for software that don't use them.
- **Overscan**: For a precise representation of the original image, using Overscan Top+Bottom and 4:3 Display Aspect Ratio is recommended.
- **Mouse Cursor**: Automatically hides when hovering over the main output window or when Main Menu is disabled.
- **Portable Mode**: Create an empty file named `portable.ini` in the same directory as the application binary to enable portable mode.

### Debugging Features
- **Docking Windows**: In debug mode, you can dock windows together by pressing SHIFT and dragging a window onto another.
- **Multi-viewport**: In Windows or macOS, you can enable "multi-viewport" in the debug menu. You must restart the emulator for the change to take effect. Once enabled, you can drag debugger windows outside the main window.
- **Debug Symbols**: The emulator automatically tries to load a symbol file when loading a ROM. For example, for ```path_to_rom_file.rom``` it tries to load ```path_to_rom_file.sym```. You can also load a symbol file using the GUI or the CLI.

### Command Line Usage
```
gearcoleco [options] [rom_file] [symbol_file]

Options:
  -f, --fullscreen    Start in fullscreen mode
  -w, --windowed      Start in windowed mode with menu visible
  -v, --version       Display version information
  -h, --help          Display this help message
```
  
## Build Instructions

### Windows

- Install Microsoft Visual Studio Community 2022 or later.
- Open the Gearcoleco Visual Studio solution `platforms/windows/Gearcoleco.sln` and build.

### macOS

- Install Xcode and run `xcode-select --install` in the terminal for the compiler to be available on the command line.
- Run these commands to generate a Mac *app* bundle:

``` shell
brew install sdl2
cd platforms/macos
make dist
```

### Linux

- Ubuntu / Debian / Raspberry Pi (Raspbian):

``` shell
sudo apt install build-essential libsdl2-dev libgtk-3-dev
cd platforms/linux
make
```

- Fedora:

``` shell
sudo dnf install @development-tools gcc-c++ SDL2-devel gtk3-devel
cd platforms/linux
make
```

- Arch Linux:

``` shell
sudo pacman -S base-devel sdl2 gtk3
cd platforms/linux
make
```

### BSD

- FreeBSD:

``` shell
su root -c "pkg install -y git gmake pkgconf SDL2 lang/gcc gtk3"
cd platforms/bsd
gmake
```

- NetBSD:

``` shell
su root -c "pkgin install gmake pkgconf SDL2 lang/gcc gtk3"
cd platforms/bsd
gmake
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
