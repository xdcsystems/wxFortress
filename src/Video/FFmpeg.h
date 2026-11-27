#pragma once

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavutil/time.h>
    #include <libavutil/hwcontext.h>
    #include <libswresample/swresample.h>
    #include <libswscale/swscale.h>
    #include <libavcodec/avcodec.h>
}

#include <memory>
#include <type_traits>

#include "Chronons.h"

constexpr auto AVNoPtsValue = AV_NOPTS_VALUE;
constexpr auto AVErrorEOF = AVERROR_EOF;

enum class SyncMaster
{
    Audio,
    Video,
    External,
    Default = Audio,
};

inline microseconds get_avtime()
{
    return microseconds { av_gettime() };
}

/* C++17 NTTP deleter supporting both T* (sws_freeContext) and T** (av*_free/close) FFmpeg cleanup APIs. */
template <auto Fn>
struct FfmpegDeleter
{
    template <typename T>
    void operator()( T* ptr ) const noexcept
    {
        if ( !ptr )
            return;

        if constexpr ( std::is_invocable_v<decltype( Fn ), T*> )
            Fn( ptr );
        else
            Fn( &ptr );
    }
};

using AVIOContextPtr     = std::unique_ptr<AVIOContext, FfmpegDeleter<avio_closep>>;
using AVFormatContextPtr = std::unique_ptr<AVFormatContext, FfmpegDeleter<avformat_close_input>>;
using AVCodecContextPtr  = std::unique_ptr<AVCodecContext, FfmpegDeleter<avcodec_free_context>>;
using AVFramePtr         = std::unique_ptr<AVFrame, FfmpegDeleter<av_frame_free>>;
using AVPacketPtr        = std::unique_ptr<AVPacket, FfmpegDeleter<av_packet_free>>;
using SwrContextPtr      = std::unique_ptr<SwrContext, FfmpegDeleter<swr_free>>;
using SwsContextPtr      = std::unique_ptr<SwsContext, FfmpegDeleter<sws_freeContext>>;
