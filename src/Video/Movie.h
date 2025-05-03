#pragma once

#include <atomic>
#include <string>
#include <thread>
#include <list>

#include "Audio.h"
#include "Video.h"

#include "Chronons.h"

// Forward declarations
struct AVCodecHWConfig;

class Movie
{
    public:
        Movie();
        ~Movie();

        int open( std::string &&filename );
        void close();
        bool isFinished() const { return m_quit.load( std::memory_order_relaxed ) == true; }

        std::pair<AVFrame *, int64_t> currentFrame();
        int64_t duration();

        nanoseconds getMasterClock();
        nanoseconds getClock();

        enum AVPixelFormat m_hwPixFormat { AV_PIX_FMT_NONE };
        AVBufferRef* m_hwDeviceCtx { nullptr };
        std::list<const AVCodecHWConfig*> m_hwDevicesSupported;

    private:
        int start();
        int streamComponentOpen( unsigned int streamIndex );

        friend class Audio;
        friend class Video;

        Audio m_audio;
        Video m_video;
        std::string m_filename;
        AVFormatContextPtr m_fmtCtx;

        microseconds m_clockBase { microseconds::min() };
        SyncMaster m_sync { SyncMaster::Default };

        std::atomic<bool> m_quit { false };

        std::thread m_parseThread;
        std::thread m_audioThread;
        std::thread m_videoThread;
};
