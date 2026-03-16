
# this is a heavily modified fork of `4jcraft` featuring lua mods and well-optimized game code

## Building (Linux)

### Dependencies

Install the following packages before building (Debian/Ubuntu names shown):

```bash
sudo apt-get install -y build-essential libsdl2-dev libgl-dev libglu1-mesa-dev libpthread-stubs0-dev liblua5.4-dev lua5.4
```

#### Arch/Manjaro

```bash
sudo pacman -S base-devel gcc pkgconf cmake sdl2-compat mesa glu lua
```

#### Fedora/Red Hat/Nobara

```bash
sudo dnf in gcc gcc-c++ make cmake SDL2-devel mesa-libGL-devel mesa-libGLU-devel openssl-devel lua-devel
```

#### Docker

If you don't want to deal with installing dependencies, you can use the included devcontainer. Open the project in VS Code with the [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) extension and it will set everything up for you - GCC 15, Meson, Ninja, lld, and all the libraries.

Alternatively, you can build and use the container manually:

```bash
docker build -t 4jcraft-dev .devcontainer/
docker run -it -v $(pwd):/workspaces/4jcraft -w /workspaces/4jcraft 4jcraft-dev bash
```

### Configure & Build

This project uses the [Meson](https://mesonbuild.com/) build system (with [Ninja](https://ninja-build.org/)).

> [!IMPORTANT]
> If you are using GCC, then GCC 15 or newer is currently *required* to build this project. Ubuntu installations in particular may have older versions preinstalled, so verify your compiler version with `gcc --version`.

#### Install Tooling

Follow [this Quickstart guide](https://mesonbuild.com/Quick-guide.html) for installing or building Meson and Ninja on your respective distro.

#### Configure & Build

The binary outputs to:

```
./build/Minecraft.Client/Minecraft.Client
```

```bash
# 1. Configure a build directory (we'll name it `build`)
meson setup build

# 2. Compile and run the project
meson compile -C build
cd build/Minecraft.Client
MODS=../../Mods ./Minecraft.Client
```

> [!TIP]
>
> For the fastest compilation speeds, you may want to use the compilers and linkers provided by an [LLVM toolchain](https://llvm.org/) (`clang`/`lld`) over your system compiler and linker. To do this, install `clang` and `lld`, and configure your build using the `llvm_native.txt` nativescript in `/scripts`:
>
> ```bash
> meson setup --native-file ./scripts/llvm_native.txt build
> ```
>
> ...or if you've already configured a build directory:
>
> ```bash
> meson setup --native-file ./scripts/llvm_native.txt build --reconfigure
> ```

#### Clean

To perform a clean compilation:

```bash
meson compile --clean -C build
```

...or to reconfigure an existing build directory:

```bash
meson setup build --reconfigure 
```

...or to hard reset the build directory:

```bash
rm -r ./build
meson setup build
```

## Contributions are always welcome