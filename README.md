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

### Raspberry Pi 4 - Raspbian (Desktop)

``` shell
sudo apt install build-essential libsdl2-dev libglew-dev
cd platforms/raspberrypi4
make
```

### Raspberry Pi 2 & 3 - Raspbian (CLI)

- Install and configure [SDL 2](http://www.libsdl.org/download-2.0.php) for development:

``` shell
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install build-essential libfreeimage-dev libopenal-dev libpango1.0-dev libsndfile-dev libudev-dev libasound2-dev libjpeg-dev libtiff5-dev libwebp-dev automake
cd ~
wget https://www.libsdl.org/release/SDL2-2.0.12.tar.gz
tar zxvf SDL2-2.0.12.tar.gz
cd SDL2-2.0.12 && mkdir build && cd build
../configure --disable-pulseaudio --disable-esd --disable-video-mir --disable-video-wayland --disable-video-x11 --disable-video-opengl --host=armv7l-raspberry-linux-gnueabihf
make -j 4
sudo make install
```

- Install libconfig library dependencies for development: `sudo apt-get install libconfig++-dev`
- Use `make -j 4` in the `platforms/raspberrypi3/x64/` folder to build the project.
- Use `export SDL_AUDIODRIVER=ALSA` before running the emulator for the best performance.
- Gearcoleco generates a `gearcoleco.cfg` configuration file where you can customize keyboard and gamepads. Key codes are from [SDL](https://wiki.libsdl.org/SDL_Keycode).

## Contributors

Thank you to all the people who have already contributed to Gearcoleco!

[![Contributors](https://contrib.rocks/image?repo=drhelius/gearcoleco)]("https://github.com/drhelius/gearcoleco/graphs/contributors)

## License

Gearcoleco is licensed under the GNU General Public License v3.0 License, see [LICENSE](LICENSE) for more information.
