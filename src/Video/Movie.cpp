#include <cassert>
#include <iostream>
#include <algorithm>
#include <condition_variable>

#include "Movie.h"
#include "Audio.h"
#include "Chronons.h"
#include "HardwareCaps.hpp"
#include "Video.h"

#if defined( _WIN32 )
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
#endif

extern "C" {
    #include <libavutil/pixdesc.h>
    #include <libavutil/dict.h>
#if defined( _WIN32 )
    #include <libavutil/hwcontext_dxva2.h>
#endif
}

namespace {
#if defined( _WIN32 )
    bool detectLegacyAmdUvdFromDxva2( AVBufferRef* hwDeviceCtx )
    {
        if ( !hwDeviceCtx || !hwDeviceCtx->data )
            return false;

        const auto* devCtx = reinterpret_cast<const AVHWDeviceContext*>( hwDeviceCtx->data );
        const auto* dxva2Ctx = static_cast<const AVDXVA2DeviceContext*>( devCtx->hwctx );
        if ( !dxva2Ctx || !dxva2Ctx->devmgr )
            return false;

        HANDLE hDevice = nullptr;
        if ( FAILED( dxva2Ctx->devmgr->OpenDeviceHandle( &hDevice ) ) )
            return false;

        bool isUvd = false;
        IDirect3DDevice9* pDevice = nullptr;
        if ( SUCCEEDED( dxva2Ctx->devmgr->LockDevice( hDevice, &pDevice, TRUE ) ) )
        {
            IDirect3D9* pD3D = nullptr;
            D3DDEVICE_CREATION_PARAMETERS params {};
            if ( pDevice &&
                 SUCCEEDED( pDevice->GetDirect3D( &pD3D ) ) &&
                 SUCCEEDED( pDevice->GetCreationParameters( &params ) ) )
            {
                D3DADAPTER_IDENTIFIER9 ident {};
                if ( SUCCEEDED( pD3D->GetAdapterIdentifier( params.AdapterOrdinal, 0, &ident ) ) )
                {
                    if ( ident.VendorId == HardwareCaps::kAmdVendorId && HardwareCaps::isLegacyAmdUvd( ident.DeviceId ) )
                        isUvd = true;
                }
            }
            if ( pD3D )
                pD3D->Release();
            if ( pDevice )
                pDevice->Release();
            dxva2Ctx->devmgr->UnlockDevice( hDevice, FALSE );
        }
        dxva2Ctx->devmgr->CloseDeviceHandle( hDevice );
        return isUvd;
    }
#endif

    int InitDecoderHW( AVCodecContext* ctx, const enum AVHWDeviceType type )
    {
        auto d = reinterpret_cast<Movie*>(ctx->opaque);
        d->m_isLegacyAmdUvd = false;
        AVDictionary *dict = nullptr;
        
        if (type == AV_HWDEVICE_TYPE_VAAPI)
            av_dict_set(&dict, "connection_type", "x11", 0);

        int err { av_hwdevice_ctx_create(&d->m_hwDeviceCtx, type, nullptr, dict, 0 ) };
        if ( err < 0 )
        {
            av_dict_free(&dict);
            std::cerr << "Failed to create specified HW device" << std::endl;
            return err;
        }

#if defined( _WIN32 )
        if ( type == AV_HWDEVICE_TYPE_DXVA2 )
        {
            d->m_isLegacyAmdUvd = detectLegacyAmdUvdFromDxva2( d->m_hwDeviceCtx );
            if ( d->m_isLegacyAmdUvd && ctx->codec_id == AV_CODEC_ID_H264 && ctx->level > 41 )
            {
                std::cerr << "Legacy ATI/AMD UVD/UVD+ DXVA2 does not support H.264 level "
                          << ctx->level << " (> 4.1); falling back to software decoder" << std::endl;
                av_buffer_unref( &d->m_hwDeviceCtx );
                av_dict_free( &dict );
                return AVERROR( ENOTSUP );
            }
        }
#endif

        ctx->hw_device_ctx = av_buffer_ref( d->m_hwDeviceCtx );
        if ( !ctx->hw_device_ctx )
        {
            av_buffer_unref( &d->m_hwDeviceCtx );
            av_dict_free(&dict);
            err = AVERROR( ENOMEM );
            return err;
        }
        
        av_dict_free(&dict);
        return err;
    }

    bool isSoftwarePixelFormat(AVPixelFormat from)
    {
        switch (from) {
            case AV_PIX_FMT_VAAPI:
            case AV_PIX_FMT_VDPAU:
            case AV_PIX_FMT_MEDIACODEC:
            case AV_PIX_FMT_VIDEOTOOLBOX:
            case AV_PIX_FMT_D3D11:
            case AV_PIX_FMT_D3D11VA_VLD:
        #if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(56, 0, 0)
            case AV_PIX_FMT_OPENCL:
        #endif
            case AV_PIX_FMT_CUDA:
            case AV_PIX_FMT_DXVA2_VLD:
        #if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(52, 58, 101) && LIBAVUTIL_VERSION_INT < AV_VERSION_INT(59, 8, 0)
            case AV_PIX_FMT_XVMC:
        #endif
        #if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(58, 134, 0)
            case AV_PIX_FMT_VULKAN:
        #endif
            case AV_PIX_FMT_DRM_PRIME:
            case AV_PIX_FMT_MMAL:
            case AV_PIX_FMT_QSV:
                return false;
            default:
                return true;
        }
    }

    AVPixelFormat negotiate_pixel_format(AVCodecContext *c, const AVPixelFormat *f)
    {
        auto d = reinterpret_cast<Movie*>(c->opaque);

        if (d->m_hwDevicesSupported.empty()) 
        {
            std::cout << "None of the hardware accelerations are supported" << std::endl;
        }

        std::list<AVPixelFormat> softwareFormats;
        std::list<AVPixelFormat> hardwareFormats;
        
        for (int i = 0; f[i] != AV_PIX_FMT_NONE; ++i) 
        {
            if (!isSoftwarePixelFormat(f[i])) 
            {
                hardwareFormats.emplace_back(f[i]);
                continue;
            }
            softwareFormats.emplace_back(f[i]);
        }

        std::cout << "Available pixel formats:" << std::endl;
        for (auto a : softwareFormats) 
        {
            auto dsc = av_pix_fmt_desc_get(a);
            std::cout << "sw:  " << dsc->name << ": AVPixelFormat(" << a << ")" << std::endl;
        }

        for (auto a : hardwareFormats) 
        {
            auto dsc = av_pix_fmt_desc_get(a);
            std::cout << "hw:  " << dsc->name << ": AVPixelFormat(" << a << ")" << std::endl;
        }

        AVPixelFormat pf = !softwareFormats.empty() ? softwareFormats.front() : AV_PIX_FMT_NONE;
        const char *decStr = "software";
        
        bool allowHw = true;
        if ( d->m_isLegacyAmdUvd && c->codec_id == AV_CODEC_ID_H264 )
        {
            const int w = ( c->coded_width > 0 ) ? c->coded_width : c->width;
            const int h = ( c->coded_height > 0 ) ? c->coded_height : c->height;
            const int maxRefs = HardwareCaps::maxDpb41RefFrames( w, h );
            if ( !HardwareCaps::isLegacyAmdUvdStreamAllowed( c->level, c->refs, w, h ) )
            {
                std::cerr << "Legacy ATI/AMD UVD/UVD+ DXVA2 rejected H.264 stream (level="
                          << c->level << ", refs=" << c->refs << ", maxRefs=" << maxRefs
                          << "); falling back to software" << std::endl;
                d->m_hwPixFormat = AV_PIX_FMT_NONE;
                allowHw = false;
            }
        }

        if ( allowHw )
        {
            const auto itor = std::find_if(hardwareFormats.cbegin(), hardwareFormats.cend(), [&](const auto& format){
                return format == d->m_hwPixFormat;
            });
            if (itor != hardwareFormats.cend())
            {
                pf = *itor;
                decStr = "hardware";
            }
        }
        
        if (auto dsc = av_pix_fmt_desc_get(pf); dsc)
            std::cout << "Using " << decStr << " decoding in " << dsc->name << std::endl;
        else
            std::cout << "None of the pixel formats" << std::endl;

        return pf;
    }
}

Movie::Movie()
  : m_audio( *this )
  , m_video( *this )
{}

Movie::~Movie()
{
    close();
}

int Movie::open( std::string &&filename )
{
    if ( m_parseThread.joinable() )
    {
        return -1;
    }
    m_filename = std::move( filename );
    m_parseThread = std::thread( &Movie::start, this );

    return 0;
}

void Movie::close()
{
    {
        std::lock_guard<std::mutex> lock { m_video.m_pictQMutex };
        m_quit.store( true, std::memory_order_release );
    }
    m_video.m_pictQCond.notify_one();
    {
        std::lock_guard<std::mutex> alock { m_audio.m_srcMutex };
    }
    m_audio.m_srcCond.notify_all();
    m_video.m_packets.abort();
    m_audio.m_packets.abort();
    if ( m_parseThread.joinable() )
    {
        m_parseThread.join();
    }
    m_fmtCtx.reset();
    m_video.m_codecCtx.reset();
    m_audio.m_codecCtx.reset();
    if ( m_hwDeviceCtx )
    {
        av_buffer_unref( &m_hwDeviceCtx );
    }
    m_hwDevicesSupported.clear();
}

std::pair<AVFrame *, int64_t> Movie::currentFrame()
{
    return m_video.currentFrame();
}

int64_t Movie::duration()
{
    return m_durationNs.load( std::memory_order_acquire );
}

int Movie::streamComponentOpen( unsigned int streamIndex )
{
    const AVCodecHWConfig* config {nullptr};
    auto *codecpar = m_fmtCtx->streams[ streamIndex ]->codecpar;

    // find decoder for the stream
    auto *decoder = avcodec_find_decoder( codecpar->codec_id );
    if ( !decoder )
    {
        std::cerr << "Failed to find "
            << av_get_media_type_string( codecpar->codec_type ) << " codec"
            << std::endl;
    
        return AVERROR( EINVAL );
    }
    
    if ( codecpar->codec_type == AVMEDIA_TYPE_VIDEO && codecpar->codec_id == AV_CODEC_ID_H264 )
    {
        for (int i = 0;; ++i) 
        {
            const AVCodecHWConfig *config = avcodec_get_hw_config(decoder, i);
            if (!config)
                break;

            if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)
                m_hwDevicesSupported.emplace_back(config);
        }

        if (!m_hwDevicesSupported.empty()) 
        {
            std::cout << decoder->name << ": supported hardware device contexts:" << std::endl;
            for (const auto& a: m_hwDevicesSupported)
            {
                std::cout << "   " << av_hwdevice_get_type_name(a->device_type) << std::endl;
                if (a->device_type == AV_HWDEVICE_TYPE_VDPAU || a->device_type == AV_HWDEVICE_TYPE_DXVA2) 
                    config = a;
            }
            if (!config)
                config = m_hwDevicesSupported.front();
                
            m_hwPixFormat = config->pix_fmt;
        } 
        else 
        {
            std::cout << "None of the hardware accelerations are supported" << std::endl;
        }
    }

    // Allocate a codec context for the decoder
    AVCodecContextPtr avctx { avcodec_alloc_context3( decoder ) };
    if ( !avctx )
    {
        std::cerr << "Failed to allocate the "
            << av_get_media_type_string( codecpar->codec_type ) << " codec context"
            << std::endl;
        return -1;
    }

    // Copy codec parameters from input stream to output codec context
    int ret = avcodec_parameters_to_context( avctx.get(), codecpar );
    if ( ret < 0 )
    {
        std::cerr << "Failed to copy "
            << av_get_media_type_string( codecpar->codec_type )
            << " codec parameters to decoder context"
            << std::endl;
        return ret;
    }

    // If exist hardware video decoder
    if ( avctx->codec_type == AVMEDIA_TYPE_VIDEO && config )
    {
        auto deviceName = av_hwdevice_get_type_name(config->device_type);
        std::cout << "Try to create hardware device context: " << deviceName << std::endl;

        avctx->opaque = this;
        avctx->get_format = negotiate_pixel_format;

        // try to use hw decoder
        if ( InitDecoderHW( avctx.get(), config->device_type ) < 0 )
        {
            std::cerr << "Failed to init hw decoder for "
                << av_get_media_type_string( codecpar->codec_type )
                << " codec, will use sw decoder"
                << std::endl;

            m_hwPixFormat = AV_PIX_FMT_NONE;
            avctx->get_format = nullptr;
            avctx->hw_device_ctx = nullptr;
            avctx->pix_fmt = AV_PIX_FMT_YUV420P; // https://habr.com/ru/companies/intel/articles/575632/
        }
        else
        {
            std::cout << "Using hardware device context:" << deviceName << std::endl;
        }
    }

    // DXVA2/D3D11VA decoders in FFmpeg 4.x are not thread-safe under frame threading
    // and can trigger GPU TDR / BSOD on Windows; force single-threaded decode for HW.
    if ( avctx->hw_device_ctx )
    {
        avctx->thread_count = 1;
        avctx->thread_type = 0;
    }
    else
    {
        // set codec to automatically determine how many threads suits best for software decoding
        avctx->thread_count = 0;

        if ( decoder->capabilities & AV_CODEC_CAP_FRAME_THREADS )
            avctx->thread_type = FF_THREAD_FRAME;
        else if ( decoder->capabilities & AV_CODEC_CAP_SLICE_THREADS )
            avctx->thread_type = FF_THREAD_SLICE;
        else
            avctx->thread_count = 1; // don't use multithreading
    }

    // Init the decoders
    if ( ( ret = avcodec_open2( avctx.get(), decoder, nullptr ) ) < 0 )
    {
        std::cerr << "Failed to open "
            << av_get_media_type_string( codecpar->codec_type )
            << " codec"
            << std::endl;

        return ret;
    }

    switch ( avctx->codec_type )
    {
        case AVMEDIA_TYPE_AUDIO:
        {
            m_audio.m_stream = m_fmtCtx->streams[ streamIndex ];
            m_audio.m_codecCtx = std::move( avctx );
        }
        break;

        case AVMEDIA_TYPE_VIDEO:
        {
            m_video.m_stream = m_fmtCtx->streams[ streamIndex ];
            m_video.m_codecCtx = std::move( avctx );
        }
        break;

        default:
        return -1;
    }
    
    return static_cast<int>( streamIndex );
}

int Movie::start()
{
    struct QuitGuard
    {
        std::atomic<bool> &quit;
        ~QuitGuard() { quit.store( true, std::memory_order_release ); }
    } quitGuard { m_quit };

    AVFormatContext *fmtctx = avformat_alloc_context();
    if ( !fmtctx )
    {
        std::cerr << "Fail to avformat_alloc_context" << std::endl;
        return AVERROR( ENOMEM );
    }
    auto interruptCb = []( void *opaque ) -> int {
        const auto *self = static_cast<const Movie*>( opaque );
        return ( self && self->m_quit.load( std::memory_order_acquire ) ) ? 1 : 0;
    };
    AVIOInterruptCB intrcb { +interruptCb, this };
    fmtctx->interrupt_callback = intrcb;
    if ( avformat_open_input( &fmtctx, m_filename.c_str(), nullptr, nullptr ) != 0 )
    {
        std::cerr << "Fail to avformat_open_input" << std::endl;
        return -1;
    }
    m_fmtCtx.reset( fmtctx );

    if ( avformat_find_stream_info( m_fmtCtx.get(), nullptr ) < 0 )
    {
        std::cerr << "Fail to avformat_find_stream_info" << std::endl;
        return -1;
    }

    if ( m_fmtCtx->duration != AV_NOPTS_VALUE )
    {
        const int64_t durNs = std::chrono::duration_cast<nanoseconds>(
            seconds_d64 { static_cast<double>( m_fmtCtx->duration ) / static_cast<double>( AV_TIME_BASE ) } ).count();
        m_durationNs.store( durNs, std::memory_order_release );
    }

    int videoIndex { -1 };
    int audioIndex { -1 };

    const int bestVideo = av_find_best_stream( m_fmtCtx.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0 );
    if ( bestVideo >= 0 )
    {
        videoIndex = streamComponentOpen( static_cast<unsigned int>( bestVideo ) );
    }

    const int bestAudio = av_find_best_stream( m_fmtCtx.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0 );
    if ( bestAudio >= 0 )
    {
        audioIndex = streamComponentOpen( static_cast<unsigned int>( bestAudio ) );
    }

    if ( videoIndex < 0 && audioIndex < 0 )
    {
        std::cerr << "Fail to open codecs" << std::endl;
        return -1;
    }

    m_clockBase = get_avtime() + milliseconds( 750 );
    m_sync.store( ( audioIndex >= 0 ) ? SyncMaster::Audio : SyncMaster::Video, std::memory_order_release );

    auto &audioQueue = m_audio.m_packets;
    auto &videoQueue = m_video.m_packets;

    if ( audioIndex >= 0 )
    {
        m_audioThread = std::thread( &Audio::start, &m_audio );
    }
    if ( videoIndex >= 0 )
    {
        m_videoThread = std::thread( &Video::start, &m_video );
    }

    while ( !m_quit.load( std::memory_order_acquire ) )
    {
        AVPacketPtr packet { av_packet_alloc() };
        if ( !packet )
        {
            std::cerr << "Fail to av_packet_alloc" << std::endl;
            break;
        }
        if ( av_read_frame( m_fmtCtx.get(), packet.get() ) < 0 )
            break;

        if ( packet->stream_index == videoIndex )
        {
            videoQueue.putBlocking( std::move( packet ) );
        }
        else if ( packet->stream_index == audioIndex )
        {
            audioQueue.putBlocking( std::move( packet ) );
        }
    }

    if ( m_video.m_codecCtx )
    {
        videoQueue.setFinished();
    }
    if ( m_audio.m_codecCtx )
    {
        audioQueue.setFinished();
    }

    if ( m_audioThread.joinable() )
    {
        m_audioThread.join();
    }
    if ( m_videoThread.joinable() )
    {
        m_videoThread.join();
    }

    return 0;
}

nanoseconds Movie::getMasterClock()
{
    switch ( m_sync.load( std::memory_order_acquire ) )
    {
        case SyncMaster::Audio :
            return m_audio.getClock();
       
        case SyncMaster::Video :
            return m_video.getClock();
        
        default :
            assert( 0 );
            return getClock();
    }
}

nanoseconds Movie::getClock()
{
    assert( 0 );
    return nanoseconds::min();
}
