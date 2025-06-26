
# FFmpeg DirectX11 Video Player

This is a simple video player that uses FFmpeg for decoding and DirectX11 for rendering. It provides smooth video playback with proper frame timing and window management.

## Features

- Hardware-accelerated video decoding using FFmpeg
- DirectX11 rendering for efficient display
- Proper frame timing for smooth playback
- Window management with ESC key support
- Support for various video formats

## Setup

1. Place your video file at `D:\jigarpatel-cpp\media\sample.mp4`
2. Build the project using CMake:
   ```bash
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Release
   ```
3. Run the player.exe from the build directory

## Controls

- ESC: Close the player
- Close button: Exit application

## Requirements

- Windows OS
- DirectX11 compatible graphics card
- FFmpeg libraries (included in the project)
- Visual Studio 2019 or later with C++17 support
