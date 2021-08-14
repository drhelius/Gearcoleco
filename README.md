# Gearcoleco

[![Gearcoleco CI](https://github.com/drhelius/Gearcoleco/workflows/Gearcoleco%20CI/badge.svg)](https://github.com/drhelius/Gearcoleco/actions)

Gearcoleco is a very accurate cross-platform ColecoVision emulator written in C++ that runs on Windows, macOS, Linux, BSD, Raspberry Pi and RetroArch.

This is an open source project with its ongoing development made possible thanks to the support by these awesome [backers](backers.md).

Please, consider [sponsoring](https://github.com/sponsors/drhelius) and following me on [Twitter](https://twitter.com/drhelius) for updates.

If you find a bug or want new features, you can help me [openning an issue](https://github.com/drhelius/Gearcoleco/issues).

----------

## Downloads

- **Windows**: [Gearcoleco-1.0.0-Windows.zip](https://github.com/drhelius/Gearcoleco/releases/download/gearcoleco-1.0.0/Gearcoleco-1.0.0-Windows.zip)
  - NOTE: You may need to install the [Microsoft Visual C++ Redistributable](https://go.microsoft.com/fwlink/?LinkId=746572)
- **macOS**:
  - `brew install --cask gearcoleco`
  - Or install manually: [Gearcoleco-1.0.0-macOS.zip](https://github.com/drhelius/Gearcoleco/releases/download/gearcoleco-1.0.0/Gearcoleco-1.0.0-macOS.zip)
- **Linux**: [Gearcoleco-1.0.0-Linux.tar.xz](https://github.com/drhelius/Gearcoleco/releases/download/gearcoleco-1.0.0/Gearcoleco-1.0.0-Linux.tar.xz)
  - NOTE: You may need to install `libsdl2` and `libglew`

## Features

- Accurate Z80 core, including undocumented opcodes and behavior like R and [MEMPTR](https://gist.github.com/drhelius/8497817) registers.
- Accurate TMS9918 emulation.
- Sound emulation using SDL Audio and [Sms_Snd_Emu library](https://www.slack.net/~ant/libs/audio.html#Sms_Snd_Emu).
- Save states.
- Compressed rom support (ZIP).
- Supported platforms (standalone): Windows, Linux, BSD, macOS, and Raspberry Pi.
- Supported platforms (libretro): Windows, Linux, macOS, Raspberry Pi, Android, iOS, tvOS, PlayStation Vita, PlayStation 3, Nintendo 3DS, Nintendo GameCube, Nintendo Wii, Nintendo WiiU, Nintendo Switch, Emscripten, Classic Mini systems (NES, SNES, C64, ...), OpenDingux and QNX.
- Full debugger with just-in-time disassembler, cpu breakpoints, memory access breakpoints, code navigation (goto address, JP JR and CALL double clicking), debug symbols, memory editor, IO inspector and VRAM viewer including registries, tiles, sprites and backgrounds.
- Windows and Linux *Portable Mode* by creating a file named `portable.ini` in the same directory as the application binary.
- Support for modern game controllers through [gamecontrollerdb.txt](https://github.com/gabomdq/SDL_GameControllerDB) file located in the same directory as the application binary.

<img width="810" alt="Screen Shot 2021-08-14 at 21 15 02" src="https://user-images.githubusercontent.com/863613/129458358-58e280ce-8c4f-4685-8b8f-c3dbb0c72f13.png">


## Build Instructions

### Windows

- Install Microsoft Visual Studio Community 2019 or later.
- Open the Visual Studio solution in `platforms/windows/Gearcoleco.sln` and build.
- You may want to use the `platforms/windows/Makefile` to build the application using MinGW.

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
sudo apt-get install build-essential libsdl2-dev libglew-dev
cd platforms/linux
make
```

- Fedora:

``` shell
sudo dnf install @development-tools gcc-c++ SDL2-devel glew-devel
cd platforms/linux
make
```

### BSD

- NetBSD:

``` shell
su root -c "pkgin install gmake pkgconf SDL2 glew"
cd platforms/bsd
gmake
```

### Libretro

- Ubuntu / Debian:

``` shell
sudo apt-get install build-essential
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

<img width="441" alt="Screen Shot 2021-08-14 at 21 20 23" src="https://user-images.githubusercontent.com/863613/129458245-3b358dfe-54f1-4f9a-b278-070bfba5046b.png"><img width="441" alt="Screen Shot 2021-08-14 at 21 18 33" src="https://user-images.githubusercontent.com/863613/129458264-267085c4-bd14-4db0-8565-01a0e9d0a61c.png">
<img width="441" alt="Screen Shot 2021-08-14 at 21 27 59" src="https://user-images.githubusercontent.com/863613/129458283-31f24c6b-fc68-4cf3-a0da-0ff0a5508892.png"><img width="441" alt="Screen Shot 2021-08-14 at 21 29 49" src="https://user-images.githubusercontent.com/863613/129458284-dd58f916-2f32-43f5-b657-69e567e2e6b8.png">
<img width="441" alt="Screen Shot 2021-08-14 at 21 24 57" src="https://user-images.githubusercontent.com/863613/129458294-09f56d23-338c-45b8-a714-de89db38c94c.png"><img width="441" alt="Screen Shot 2021-08-14 at 21 25 20" src="https://user-images.githubusercontent.com/863613/129458275-3f5333f3-76c0-4761-9cf4-8d105fa407d4.png">
<img width="441" alt="Screen Shot 2021-08-14 at 21 24 42" src="https://user-images.githubusercontent.com/863613/129458298-f9b51899-f81c-496d-a3c6-3b863cf459f5.png"><img width="441" alt="Screen Shot 2021-08-14 at 21 27 31" src="https://user-images.githubusercontent.com/863613/129458280-782daf18-b0a1-4803-a880-7476e8871642.png">

## Contributors

Thank you to all the people who have already contributed to Gearcoleco!

[![Contributors](https://contrib.rocks/image?repo=drhelius/gearcoleco)]("https://github.com/drhelius/gearcoleco/graphs/contributors)

## License

Gearcoleco is licensed under the GNU General Public License v3.0 License, see [LICENSE](LICENSE) for more information.
