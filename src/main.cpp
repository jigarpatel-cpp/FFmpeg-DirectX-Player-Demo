// File: src/main.cpp
#include "ffmpeg_decoder.h"
#include "dx11_renderer.h"
#include <windows.h>
#include <thread>
#include <chrono>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nCmdShow) {
    const char* videoPath = "D:\\jigarpatel-cpp\\media\\sample.mp4";
    if (GetFileAttributesA(videoPath) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxA(0, "Video file not found. Please place a video file at the specified path.", "Error", MB_OK);
        return -1;
    }

    FFmpegDecoder decoder;
    if (!decoder.open(videoPath)) {
        MessageBoxA(0, "Failed to open video file", "Error", MB_OK);
        return -1;
    }

    DX11Renderer renderer;
    if (!renderer.initialize(decoder.getWidth(), decoder.getHeight())) {
        MessageBoxA(0, "Failed to initialize DirectX", "Error", MB_OK);
        return -1;
    }

    AVFrame* frame = nullptr;
    auto frameStart = std::chrono::high_resolution_clock::now();
    const int64_t frameDelay = static_cast<int64_t>(1000.0 / 30.0); // 30 fps

    while ((frame = decoder.decodeFrame()) != nullptr) {
        renderer.renderFrame(frame);

        // Calculate and apply precise frame timing
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart).count();
        
        if (frameDuration < frameDelay) {
            std::this_thread::sleep_for(std::chrono::milliseconds(frameDelay - frameDuration));
        }
        
        frameStart = std::chrono::high_resolution_clock::now();
        av_frame_free(&frame);
    }

    renderer.cleanup();
    decoder.cleanup();
    return 0;
}
