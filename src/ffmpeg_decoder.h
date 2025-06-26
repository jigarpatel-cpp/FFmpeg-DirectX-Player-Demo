// File: src/ffmpeg_decoder.h
#pragma once
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class FFmpegDecoder {
public:
    FFmpegDecoder();
    ~FFmpegDecoder();

    bool open(const char* filename);
    AVFrame* decodeFrame();
    void cleanup();

    int getWidth() const { return codecCtx ? codecCtx->width : 0; }
    int getHeight() const { return codecCtx ? codecCtx->height : 0; }

private:
    AVFormatContext* fmtCtx = nullptr;
    AVCodecContext* codecCtx = nullptr;
    const AVCodec* codec = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* packet = nullptr;
    SwsContext* swsCtx = nullptr;
    int videoStreamIndex = -1;
};
