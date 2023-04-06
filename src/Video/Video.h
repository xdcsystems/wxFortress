#pragma once

#include <array>

#include "Chronons.h"
#include "FFmpeg.h"
#include "PacketQueue.hpp"

#define VIDEO_PICTURE_QUEUE_SIZE 24

// Forward declarations
class Movie;

class Video
{
    public:
        Video( Movie &movie );
        std::pair<AVFrame *, int64_t> currentFrame();

    private:
        int start();
        nanoseconds getClock();

        friend class Movie;
        Movie &m_movie;

        AVStream *m_stream { nullptr };
        AVCodecContextPtr m_codecCtx;

        PacketQueue<14 * 1024 * 1024> m_packets;

        nanoseconds m_displayPts { 0 };
        microseconds m_displayPtsTime { microseconds::min() };
        std::mutex m_dispPtsMutex;

        struct Picture
        {
                AVFramePtr frame;
                nanoseconds pts { nanoseconds::min() };
        };
        std::array<Picture, VIDEO_PICTURE_QUEUE_SIZE> m_pictQ;
        std::atomic<size_t> m_pictQRead { 0u };
        std::atomic<size_t> m_pictQWrite { 1u };
        std::mutex m_pictQMutex;
        std::condition_variable m_pictQCond;
};
