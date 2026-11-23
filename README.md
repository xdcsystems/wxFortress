# wxFortress

[![Build wxFortress](https://github.com/xdcsystems/wxFortress/actions/workflows/build.yml/badge.svg)](https://github.com/xdcsystems/wxFortress/actions/workflows/build.yml)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![OpenGL](https://img.shields.io/badge/OpenGL-2.0%2B-green.svg)
![Platforms](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey.svg)

A lightweight, cross-platform Arkanoid-style desktop game built with **C++17**, **OpenGL 2.0+**, **wxWidgets**, **OpenAL Soft**, and **FFmpeg**. Specifically engineered and tested to run smoothly on low-power and legacy hardware (verified on Core 2 Duo + ATI Mobility Radeon HD 3470 with hardware DXVA2 video decoding).

---

## Key Features & Architecture

- **Analytical Swept-Circle Physics (`src/Shapes/BallPhysics.hpp`)**: Continuous collision detection (CCD) for walls, bricks, and paddle edges — eliminates tunneling at high ball speeds, resolves exact corner/seam bounces, and runs deterministically on the UI loop without async data races.
- **Hardware-Accelerated Video Playback (`src/Video/`, `src/Renderer/VideoRenderer.*`)**: Plays the intro sequence (`wxFortress.vid`) using FFmpeg with **DXVA2** on Windows and **VAAPI/VDPAU** on Linux, rendering YUV420P/NV12 frames via OpenGL fragment shaders. Includes automatic DPB Level 4.1 validation for legacy ATI/AMD UVD/UVD+ GPUs with clean software fallback.
- **Self-Contained Vendored Dependencies**: All third-party libraries are pinned as Git submodules or vendored archives in `external/` for reproducible, offline-safe builds that do not break on upstream API deprecations.
- **Automated GoogleTest Verification**: Both `ut_BallPhysics` (10 analytical physics & 100k-tick endurance tests) and `ut_Fortress` (9 wxWidgets/OpenGL/GLEW integration tests) are wired into the CMake build (`run_tests` target) and run automatically.

### Vendored Dependencies

| Component | Location | Role |
| --- | --- | --- |
| **wxWidgets 3.2.2** | `external/wxWidgets` | Cross-platform windowing, events, `wxGLCanvas` |
| **GLEW** | `external/glew-cmake` | OpenGL extension loading (`wglSwapIntervalEXT`, shaders) |
| **GLM 0.9.9.9** | `external/glm` | Vector & matrix math |
| **OpenAL Soft** | `external/openal-soft` | 3D/stereo audio playback & mixing |
| **FFmpeg 4.x** | `external/ffmpeg` | H.264/AAC demuxing & hardware/software decoding |
| **GoogleTest / GoogleMock** | `external/googletest` | Automated unit & integration test suites |

---

## Controls

| Key | Action |
| --- | --- |
| `Left` / `Right` | Move paddle |
| `Space` | Launch ball / Pause & Resume |
| `R` | Skip intro video |
| `F1` | Show help overlay |
| `Esc` | Close help overlay / Exit game |

---

## Prerequisites & Build

Clone the repository with all submodules:

```bash
git clone --recursive https://github.com/xdcsystems/wxFortress.git
cd wxFortress
```

> If `external/googletest` was not initialized during clone, CMake automatically runs `git submodule update --init -- external/googletest` during configuration.

### Windows (WinLibs i686 / MinGW32 + Ninja — Recommended & CI-Verified)

1. Install [WinLibs i686 (GCC 16.2.0 UCRT + CMake + Ninja)](https://github.com/brechtsanders/winlibs_mingw/releases) and add `mingw32\bin` to `PATH`.
2. Configure and build (builds `wxFortress.exe`, copies runtime DLLs/resources to `.bin/`, and runs all 19 GoogleTest checks automatically):

```powershell
cmake -S . -B ..\wxFortress-build -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
cmake --build ..\wxFortress-build -j
```

3. Run the game:

```powershell
cd ..\wxFortress-build\.bin
.\wxFortress.exe
```

> Pre-built Windows x86 bundles (`wxFortress-win32`) are also published as build artifacts on every run of [GitHub Actions](https://github.com/xdcsystems/wxFortress/actions/workflows/build.yml).

### Windows (Visual Studio 2019+)

Run `build.bat` from the repository root to generate an x86 Visual Studio solution in `x86/wxFortress.sln` (Debug by default; adjust `build.bat` for Release), then open and build it in Visual Studio.

### Linux (Ubuntu / Debian — Vendored Submodules)

1. Install OS-level build tools and headers required to build vendored `wxWidgets`, `openal-soft`, and `FFmpeg 4.3`:

```bash
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  build-essential cmake ninja-build pkg-config nasm yasm \
  libgtk-3-dev libgl-dev libglu1-mesa-dev libx11-dev libxext-dev libxi-dev libxrandr-dev libxxf86vm-dev \
  libasound2-dev libpulse-dev libva-dev libvdpau-dev libdrm-dev \
  libgnutls28-dev libaom-dev libass-dev libfreetype-dev libmp3lame-dev \
  libopus-dev libfontconfig-dev libvorbis-dev libvpx-dev libx264-dev libx265-dev libfribidi-dev
```

2. Extract and build vendored FFmpeg 4.3 into `external/ffmpeg/linux/.build` (one-time step):

```bash
mkdir -p /tmp/ffmpeg_src external/ffmpeg/linux/.build
tar -xzf external/ffmpeg/linux/ffmpeg_4.3.tar.gz -C /tmp/ffmpeg_src
FFMPEG_SRC=$(find /tmp/ffmpeg_src -maxdepth 2 -name configure -printf '%h\n' | head -n 1)
(cd "$FFMPEG_SRC" && ./configure --prefix="$PWD/external/ffmpeg/linux/.build" \
  --disable-doc --disable-programs --disable-shared --enable-static \
  --enable-pic --enable-gpl --enable-vaapi --enable-libdrm \
  --enable-libx264 --enable-libopus --enable-libvorbis && make -j"$(nproc)" && make install)
```

3. Configure, build, and run tests:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build -j
```

---

## Automated Testing

Every default build (`cmake --build <build-dir> -j`) executes the `run_tests` target automatically:

- **`ut_BallPhysics`** (10 GoogleTest cases): diagonal & face reflections, paddle edge & side-graze handling, simultaneous corner hits, brick seam hits, anti-tunneling at 1200 px/s, NaN safety, bounded paddle acceleration, and a 100,000-tick endurance simulation.
- **`ut_Fortress`** (9 GoogleTest/GoogleMock cases): `wxApp` lifecycle, resource archive loading (`wxFortress.dat`), `wxFrame`/`wxGLCanvas` creation, GLEW/OpenGL initialization, `SpriteRenderer`, `Brick`, and `Ball` creation. (In headless CI environments without a GL 2.0+ GPU, `run_tests` automatically filters to the non-shader integration subset.)

You can also invoke tests via CTest:

```bash
ctest --test-dir <build-dir> --output-on-failure
```
