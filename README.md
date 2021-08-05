# Gearcoleco (currently in development)

[![Gearcoleco CI](https://github.com/drhelius/Gearcoleco/workflows/Gearcoleco%20CI/badge.svg)](https://github.com/drhelius/Gearcoleco/actions)

*This is a work in progress project, not intended to be used right now.*

Gearcoleco is a cross-platform ColecoVision emulator written in C++.

This is an open source project with its ongoing development made possible thanks to the support by these awesome [backers](backers.md).

Please, consider [sponsoring](https://github.com/sponsors/drhelius) and following me on [Twitter](https://twitter.com/drhelius) for updates.

----------

## Build Instructions

### macOS

- Install Xcode and run `xcode-select --install` in the terminal for the compiler to be available on the command line.
- Run these commands to generate a Mac *app* bundle:

``` shell
brew install sdl2
cd platforms/macos
make dist
```

### Linux

- Ubuntu / Debian:

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

### Raspberry Pi 4 - Raspbian (Desktop)

``` shell
sudo apt install build-essential libsdl2-dev libglew-dev
cd platforms/raspberrypi4
make
```

## Contributors

Thank you to all the people who have already contributed to Gearcoleco!

[![Contributors](https://contrib.rocks/image?repo=drhelius/gearcoleco)]("https://github.com/drhelius/gearcoleco/graphs/contributors)

## License

Gearcoleco is licensed under the GNU General Public License v3.0 License, see [LICENSE](LICENSE) for more information.
