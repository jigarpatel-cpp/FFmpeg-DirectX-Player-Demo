// File: src/ffmpeg_decoder.cpp
#include "ffmpeg_decoder.h"

FFmpegDecoder::FFmpegDecoder() {}

FFmpegDecoder::~FFmpegDecoder() {
    cleanup();
}

bool FFmpegDecoder::open(const char* filename) 
{
    avformat_network_init();

    if (avformat_open_input(&fmtCtx, filename, nullptr, nullptr) != 0)
        return false;
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0)
        return false;

    videoStreamIndex = -1;
    for (unsigned i = 0; i < fmtCtx->nb_streams; i++) 
    {
        if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) 
        {
            videoStreamIndex = i;
            break;
        }
    }
    if (videoStreamIndex == -1)
        return false;

    codec = avcodec_find_decoder(fmtCtx->streams[videoStreamIndex]->codecpar->codec_id);
    if (!codec)
        return false;

    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, fmtCtx->streams[videoStreamIndex]->codecpar);
    if (avcodec_open2(codecCtx, codec, nullptr) < 0)
        return false;

    frame = av_frame_alloc();
    packet = av_packet_alloc();
    return true;
}

AVFrame* FFmpegDecoder::decodeFrame() 
{
    while (av_read_frame(fmtCtx, packet) >= 0) 
    {
        if (packet->stream_index == videoStreamIndex) 
        {
            if (avcodec_send_packet(codecCtx, packet) == 0) 
            {
                if (avcodec_receive_frame(codecCtx, frame) == 0) 
                {
                    if (!swsCtx)
                    {
                        swsCtx = sws_getContext(
                            codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
                            codecCtx->width, codecCtx->height, AV_PIX_FMT_RGBA,
                            SWS_BICUBIC, nullptr, nullptr, nullptr
                        );
                    }

                    AVFrame* bgraFrame = av_frame_alloc();
                    bgraFrame->format = AV_PIX_FMT_RGBA;
                    bgraFrame->width = codecCtx->width;
                    bgraFrame->height = codecCtx->height;
                    av_frame_get_buffer(bgraFrame, 32);

                    sws_scale(swsCtx, frame->data, frame->linesize, 0,
                              codecCtx->height, bgraFrame->data, bgraFrame->linesize);

                    av_packet_unref(packet);
                    return bgraFrame;
                }
            }
        }
        av_packet_unref(packet);
    }
    return nullptr;
}

void FFmpegDecoder::cleanup() 
{
    if (swsCtx) sws_freeContext(swsCtx);
    if (packet) av_packet_free(&packet);
    if (frame) av_frame_free(&frame);
    if (codecCtx) avcodec_free_context(&codecCtx);
    if (fmtCtx) avformat_close_input(&fmtCtx);
}
