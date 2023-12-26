#pragma once

#include <AL/al.h>
#include <AL/alc.h>

#include <array>

#include "FFmpeg.h"
#include "PacketQueue.hpp"

// Per-buffer size, in time
constexpr milliseconds AudioBufferTime { 20 };
// Buffer total size, in time (should be divisible by the buffer time)
constexpr milliseconds AudioBufferTotalTime { 800 };
constexpr auto AudioBufferCount = AudioBufferTotalTime / AudioBufferTime;

// Forward declarations
class Movie;

class Audio
{
    public:
        Audio( Movie &movie );
        ~Audio();

        bool play() const;
        void cleanup();

    private:
        int start();
        int decodeFrame();
        int readAudio( uint8_t* samples, unsigned int length );

        nanoseconds getClock();

        friend class Movie;
        Movie &m_movie;

        AVStream *m_stream { nullptr };
        AVCodecContextPtr m_codecCtx;

        PacketQueue<2 * 1024 * 1024> m_packets;

        nanoseconds m_currentPts { 0 };
        nanoseconds m_deviceStartTime { nanoseconds::min() };

        AVFramePtr m_decodedFrame;
        SwrContextPtr m_swrCtx;

        uint64_t m_dstChanLayout { 0 };
        AVSampleFormat m_dstSampleFmt { AV_SAMPLE_FMT_NONE };

        uint8_t *m_samples { nullptr };
        int m_samplesLen { 0 };
        int m_samplesPos { 0 };
        int m_samplesMax { 0 };

        ALenum m_format { AL_NONE };
        ALuint m_frameSize { 0 };

        std::mutex m_srcMutex;
        std::condition_variable m_srcCond;
        ALuint m_source { 0 };
        std::array<ALuint, AudioBufferCount> m_buffers {};
        ALuint m_bufferIdx { 0 };
};
