// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <mutex>
#include <vector>

#include <AL/al.h>
#include <AL/alc.h>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/channel_layout.h>
    #include <libavutil/opt.h>
    #include <libswresample/swresample.h>
}

#include "Common/Tools.h"
#include "AudioFile/AudioFile.h"
#include "SoundManager.h"

namespace
{
    constexpr float kMusicNormalGain = 0.030f;
    constexpr float kMusicHelpGain = 0.065f;
    constexpr float kMusicDuckedGain = 0.009f;
    constexpr float kMusicGainSlewPerSec = 0.14f;

    struct MemBufferReader
    {
        const uint8_t* data { nullptr };
        size_t size { 0 };
        size_t pos { 0 };
    };

    int readMemPacket( void* opaque, uint8_t* buf, int bufSize )
    {
        auto* reader = static_cast<MemBufferReader*>( opaque );
        if ( !reader || reader->pos >= reader->size )
            return AVERROR_EOF;

        const size_t remaining = reader->size - reader->pos;
        const size_t toRead = ( std::min )( static_cast<size_t>( bufSize ), remaining );
        std::memcpy( buf, reader->data + reader->pos, toRead );
        reader->pos += toRead;
        return static_cast<int>( toRead );
    }

    int64_t seekMemPacket( void* opaque, int64_t offset, int whence )
    {
        auto* reader = static_cast<MemBufferReader*>( opaque );
        if ( !reader )
            return AVERROR( EINVAL );

        if ( whence == AVSEEK_SIZE )
            return static_cast<int64_t>( reader->size );

        int64_t newPos = 0;
        switch ( whence & ~AVSEEK_FORCE )
        {
            case SEEK_SET: newPos = offset; break;
            case SEEK_CUR: newPos = static_cast<int64_t>( reader->pos ) + offset; break;
            case SEEK_END: newPos = static_cast<int64_t>( reader->size ) + offset; break;
            default: return AVERROR( EINVAL );
        }

        if ( newPos < 0 || static_cast<size_t>( newPos ) > reader->size )
            return AVERROR( EINVAL );

        reader->pos = static_cast<size_t>( newPos );
        return newPos;
    }
} // namespace

SoundManager::SoundManager()
{
    // find the default audio device
    const ALCchar* defaultDeviceString = alcGetString( /*device*/ nullptr, ALC_DEFAULT_DEVICE_SPECIFIER );
    m_device = alcOpenDevice( defaultDeviceString );
    if ( !m_device )
    {
        std::cerr << "failed to get the default device for OpenAL" << std::endl;
        throw std::runtime_error { "failed to get the default device for OpenAL" };
    }
    std::cout << "OpenAL Device: " << alcGetString( m_device, ALC_DEVICE_SPECIFIER ) << std::endl;

    // Create an OpenAL audio context from the device
    m_context = alcCreateContext( m_device, /*attrlist*/ nullptr );

    // Activate this context so that OpenAL state modifications are applied to the context
    if ( !alcMakeContextCurrent( m_context ) )
    {
        std::cerr << "failed to make the OpenAL context the current context" << std::endl;
        throw std::runtime_error{ "failed to make the OpenAL context the current context" };
    }

    // Create a listener in 3d space (ie the player)
    alec( alListener3f( AL_POSITION, 0.f, 0.f, 0.f ) );
    alec( alListener3f( AL_VELOCITY, 0.f, 0.f, 0.f ) );
    ALfloat forwardAndUpVectors[] = { /*forward = */ 1.f,
        0.f,
        0.f,
        /* up = */ 0.f,
        1.f,
        0.f };
    alec( alListenerfv( AL_ORIENTATION, forwardAndUpVectors ) );
}

SoundManager::~SoundManager()
{
    // clean up our resources!
    stopMusic();
    cleanSources();

    if ( m_musicSource != 0 )
        alec( alDeleteSources( 1, &m_musicSource ) );

    for ( ALuint& buf : m_musicBuffers )
    {
        if ( buf != 0 )
        {
            alec( alDeleteBuffers( 1, &buf ) );
            buf = 0;
        }
    }
    if ( m_helpMusicBuffer != 0 )
    {
        alec( alDeleteBuffers( 1, &m_helpMusicBuffer ) );
        m_helpMusicBuffer = 0;
    }

    alec( alDeleteSources( 1, &m_monoSource ) );
    alec( alDeleteSources( static_cast<ALsizei>( kSfxSourceCount ), m_stereoSource ) );
    alec( alDeleteBuffers( 1, &m_monoSoundBuffer ) );
    alec( alDeleteBuffers( 1, &m_stereoLounchSoundBuffer ) );
    alec( alDeleteBuffers( 1, &m_stereoCountdownSoundBuffer ) );
    alec( alDeleteBuffers( 1, &m_stereoDesroyBrickSoundBuffer ) );
    alec( alDeleteBuffers( 1, &m_stereoPingSoundBuffer ) );
    alec( alDeleteBuffers( 1, &m_stereoPongSoundBuffer ) );
    alec( alDeleteBuffers( 1, &m_stereoLevelCompleteSoundBuffer ) );
    alec( alDeleteBuffers( 1, &m_stereoBallLostSoundBuffer ) );
    alec( alDeleteBuffers( 1, &m_stereoCharShowSoundBuffer ) );

    alcMakeContextCurrent( nullptr );
    alcDestroyContext( m_context );
    alcCloseDevice( m_device );
}

int SoundManager::loadBuffer( AudioFile<float>& soundFile, const std::string& name, ALuint* buffer )
{
    std::vector<uint8_t> dataBytesPCM;

    if ( !soundFile.load( "resources/sounds/" + name ) )
    {
        std::cerr << "failed to load sound resource: " << name << std::endl;
        return -1;
    }

    const auto convertFileToOpenALFormat = []( const AudioFile<float>& audioFile ) {
        int bitDepth = audioFile.getBitDepth();
        if ( bitDepth == 16 )
        {
            return audioFile.isStereo() ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
        }

        if ( bitDepth == 8 )
        {
            return audioFile.isStereo() ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
        }

        return -1;
    };

    soundFile.writePCMToBuffer( dataBytesPCM );

    alec( alGenBuffers( 1, buffer ) );
    alec( alBufferData( *buffer,
            convertFileToOpenALFormat( soundFile ),
            dataBytesPCM.data(),
            static_cast<ALsizei>( dataBytesPCM.size() ),
            soundFile.getSampleRate() ) );

    return 0;
}

int SoundManager::loadOggBuffer( const std::string& name, ALuint* buffer )
{
    if ( !buffer )
        return -1;

    std::vector<uint8_t> rawBytes;
    try
    {
        wxInputStream& stream = Tools::Instance().loadResource( "resources/sounds/" + name );
        uint8_t chunk[ 16384 ];
        while ( !stream.Eof() )
        {
            stream.Read( chunk, sizeof( chunk ) );
            const size_t n = stream.LastRead();
            if ( n == 0 )
                break;
            rawBytes.insert( rawBytes.end(), chunk, chunk + n );
        }
    }
    catch ( const std::exception& e )
    {
        std::cerr << "Failed to read music resource " << name << ": " << e.what() << std::endl;
        return -1;
    }

    if ( rawBytes.empty() )
        return -1;

    MemBufferReader reader { rawBytes.data(), rawBytes.size(), 0 };
    constexpr int kAvioBufSize = 16384;
    auto* avioBuf = static_cast<uint8_t*>( av_malloc( kAvioBufSize ) );
    if ( !avioBuf )
        return -1;

    AVIOContext* avioCtx = avio_alloc_context( avioBuf, kAvioBufSize, 0, &reader, &readMemPacket, nullptr, &seekMemPacket );
    if ( !avioCtx )
    {
        av_free( avioBuf );
        return -1;
    }

    AVFormatContext* fmtCtx = avformat_alloc_context();
    if ( !fmtCtx )
    {
        av_freep( &avioCtx->buffer );
        avio_context_free( &avioCtx );
        return -1;
    }
    fmtCtx->pb = avioCtx;
    fmtCtx->flags |= AVFMT_FLAG_CUSTOM_IO;

    AVCodecContext* codecCtx = nullptr;
    SwrContext* swrCtx = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* pkt = nullptr;
    std::vector<int16_t> pcm;
    int result = -1;

    constexpr int kTargetRate = 44100;
    constexpr int kTargetChannels = 2;

    if ( avformat_open_input( &fmtCtx, nullptr, nullptr, nullptr ) == 0 &&
         avformat_find_stream_info( fmtCtx, nullptr ) >= 0 )
    {
        int audioStreamIdx = -1;
        for ( unsigned int i = 0; i < fmtCtx->nb_streams; ++i )
        {
            if ( fmtCtx->streams[ i ]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO )
            {
                audioStreamIdx = static_cast<int>( i );
                break;
            }
        }

        if ( audioStreamIdx >= 0 )
        {
            const AVCodecParameters* codecpar = fmtCtx->streams[ audioStreamIdx ]->codecpar;
            const AVCodec* decoder = avcodec_find_decoder( codecpar->codec_id );
            if ( decoder )
            {
                codecCtx = avcodec_alloc_context3( decoder );
                if ( codecCtx &&
                     avcodec_parameters_to_context( codecCtx, codecpar ) >= 0 &&
                     avcodec_open2( codecCtx, decoder, nullptr ) >= 0 )
                {
                    int64_t inChLayout = codecCtx->channel_layout
                        ? static_cast<int64_t>( codecCtx->channel_layout )
                        : av_get_default_channel_layout( codecCtx->channels );

                    swrCtx = swr_alloc_set_opts(
                        nullptr,
                        AV_CH_LAYOUT_STEREO,
                        AV_SAMPLE_FMT_S16,
                        kTargetRate,
                        inChLayout,
                        codecCtx->sample_fmt,
                        codecCtx->sample_rate,
                        0,
                        nullptr );

                    if ( swrCtx && swr_init( swrCtx ) >= 0 )
                    {
                        frame = av_frame_alloc();
                        pkt = av_packet_alloc();
                        if ( frame && pkt )
                        {
                            auto drainFrames = [ & ]() {
                                while ( avcodec_receive_frame( codecCtx, frame ) == 0 )
                                {
                                    const int outSamples = static_cast<int>( av_rescale_rnd(
                                        swr_get_delay( swrCtx, codecCtx->sample_rate ) + frame->nb_samples,
                                        kTargetRate,
                                        codecCtx->sample_rate,
                                        AV_ROUND_UP ) );
                                    if ( outSamples <= 0 )
                                        continue;

                                    const size_t prevSize = pcm.size();
                                    pcm.resize( prevSize + static_cast<size_t>( outSamples * kTargetChannels ) );
                                    uint8_t* outPtr = reinterpret_cast<uint8_t*>( pcm.data() + prevSize );
                                    const int converted = swr_convert(
                                        swrCtx,
                                        &outPtr,
                                        outSamples,
                                        const_cast<const uint8_t**>( frame->extended_data ),
                                        frame->nb_samples );
                                    if ( converted > 0 )
                                        pcm.resize( prevSize + static_cast<size_t>( converted * kTargetChannels ) );
                                    else
                                        pcm.resize( prevSize );
                                    av_frame_unref( frame );
                                }
                            };

                            while ( av_read_frame( fmtCtx, pkt ) >= 0 )
                            {
                                if ( pkt->stream_index == audioStreamIdx )
                                {
                                    if ( avcodec_send_packet( codecCtx, pkt ) >= 0 )
                                        drainFrames();
                                }
                                av_packet_unref( pkt );
                            }

                            // Flush decoder and resampler
                            avcodec_send_packet( codecCtx, nullptr );
                            drainFrames();

                            const int remaining = static_cast<int>( swr_get_delay( swrCtx, kTargetRate ) );
                            if ( remaining > 0 )
                            {
                                const size_t prevSize = pcm.size();
                                pcm.resize( prevSize + static_cast<size_t>( remaining * kTargetChannels ) );
                                uint8_t* outPtr = reinterpret_cast<uint8_t*>( pcm.data() + prevSize );
                                const int flushed = swr_convert( swrCtx, &outPtr, remaining, nullptr, 0 );
                                if ( flushed > 0 )
                                    pcm.resize( prevSize + static_cast<size_t>( flushed * kTargetChannels ) );
                                else
                                    pcm.resize( prevSize );
                            }

                            if ( !pcm.empty() )
                            {
                                alec( alGenBuffers( 1, buffer ) );
                                alec( alBufferData(
                                    *buffer,
                                    AL_FORMAT_STEREO16,
                                    pcm.data(),
                                    static_cast<ALsizei>( pcm.size() * sizeof( int16_t ) ),
                                    kTargetRate ) );
                                result = 0;
                            }
                        }
                    }
                }
            }
        }
    }

    if ( pkt ) av_packet_free( &pkt );
    if ( frame ) av_frame_free( &frame );
    if ( swrCtx ) swr_free( &swrCtx );
    if ( codecCtx ) avcodec_free_context( &codecCtx );
    if ( fmtCtx ) avformat_close_input( &fmtCtx );
    if ( avioCtx )
    {
        av_freep( &avioCtx->buffer );
        avio_context_free( &avioCtx );
    }

    if ( result != 0 )
        std::cerr << "Failed to decode OGG music resource: " << name << std::endl;

    return result;
}

int SoundManager::init()
{
    static std::once_flag s_once;
    std::call_once( s_once, [ this ]() {
        AudioFile<float> soundFile;
        loadBuffer( soundFile, "Rocket.wav", &m_monoSoundBuffer );

        // load stereo SFX files into buffers
        loadBuffer( soundFile, "Launch.wav", &m_stereoLounchSoundBuffer );
        loadBuffer( soundFile, "Countdown.wav", &m_stereoCountdownSoundBuffer );
        loadBuffer( soundFile, "DesroyBrick.wav", &m_stereoDesroyBrickSoundBuffer );
        loadBuffer( soundFile, "Ping.wav", &m_stereoPingSoundBuffer );
        loadBuffer( soundFile, "Pong.wav", &m_stereoPongSoundBuffer );
        loadBuffer( soundFile, "LevelComplete.wav", &m_stereoLevelCompleteSoundBuffer );
        loadBuffer( soundFile, "BallLost.wav", &m_stereoBallLostSoundBuffer );
        loadBuffer( soundFile, "Char.wav", &m_stereoCharShowSoundBuffer );

        alec( alGenSources( static_cast<ALsizei>( kSfxSourceCount ), m_stereoSource ) );

        alec( alGenSources( 1, &m_monoSource ) );
        alec( alSource3f( m_monoSource, AL_POSITION, 1.f, 0.f, 0.f ) );
        alec( alSource3f( m_monoSource, AL_VELOCITY, 0.f, 0.f, 0.f ) );
        alec( alSourcef( m_monoSource, AL_PITCH, 1.f ) );
        alec( alSourcef( m_monoSource, AL_GAIN, 1.f ) );
        alec( alSourcei( m_monoSource, AL_LOOPING, AL_FALSE ) );
        alec( alSourcei( m_monoSource, AL_BUFFER, m_monoSoundBuffer ) );

        // Dedicated looping background music source
        alec( alGenSources( 1, &m_musicSource ) );
        alec( alSourcef( m_musicSource, AL_PITCH, 1.f ) );
        alec( alSourcef( m_musicSource, AL_GAIN, kMusicNormalGain ) );
        alec( alSourcei( m_musicSource, AL_LOOPING, AL_TRUE ) );
        alec( alSourcei( m_musicSource, AL_SOURCE_RELATIVE, AL_TRUE ) );
        alec( alSource3f( m_musicSource, AL_POSITION, 0.f, 0.f, 0.f ) );
    } );
    return 0;
}

ALuint SoundManager::getFreeStereoSource() const
{
    ALint sourceState( 0 );
    for ( const auto source : m_stereoSource )
    {
        alec( alGetSourcei( source, AL_SOURCE_STATE, &sourceState ) );
        if ( sourceState == AL_PLAYING )
            continue;

        return source;
    }
    return 0;
}

void SoundManager::cleanSources()
{
    for ( const auto source : m_stereoSource )
    {
        ALint processed( 0 );
        alGetSourcei( source, AL_BUFFERS_PROCESSED, &processed );

        while ( processed-- )
        {
            ALuint buffer( 0 );
            alSourceUnqueueBuffers( source, 1, &buffer );
        }
    }
}

void SoundManager::playStereoSource( ALuint buffer, float gain, bool playAsync )
{
    auto source = getFreeStereoSource();
    if ( source == 0 )
    {
        source = m_stereoSource[ 0 ];
    }

    alec( alSourcef( source, AL_PITCH, 1.f ) );
    alec( alSourcef( source, AL_GAIN, gain ) );
    alec( alSourcei( source, AL_LOOPING, AL_FALSE ) );
    alec( alSourcei( source, AL_BUFFER, buffer ) );

    alec( alSourcePlay( source ) );

    ALint processed( 0 );
    alec( alGetSourcei( source, AL_BUFFERS_PROCESSED, &processed ) );
    while ( processed-- )
    {
        ALuint qbuf( 0 );
        alec( alSourceUnqueueBuffers( source, 1, &qbuf ) );
    }

    if ( playAsync )
        return;

    ALint sourceState( 0 );
    alec( alGetSourcei( source, AL_SOURCE_STATE, &sourceState ) );
    while ( sourceState == AL_PLAYING )
    {
        alec( alGetSourcei( source, AL_SOURCE_STATE, &sourceState ) );
        wxYield();
    }
}

void SoundManager::applyMusicGain( float gain )
{
    m_currentMusicGain = gain;
    if ( m_musicSource != 0 )
        alec( alSourcef( m_musicSource, AL_GAIN, m_currentMusicGain ) );
}

void SoundManager::playMusic( unsigned short level )
{
    if ( m_musicSource == 0 )
        return;

    const unsigned short trackIdx = static_cast<unsigned short>(
        ( ( level > 0 ? level - 1 : 0 ) % kMusicTrackCount ) );
    const unsigned short trackNum = static_cast<unsigned short>( trackIdx + 1 );

    if ( m_musicPlaying && !m_inHelpMusic && m_currentMusicTrack == trackNum )
        return;

    // Lazy-decode level music track on first use (keeps startup instant)
    if ( m_musicBuffers[ trackIdx ] == 0 )
    {
        const std::string filename = "music_level_" + std::to_string( trackNum ) + ".ogg";
        if ( loadOggBuffer( filename, &m_musicBuffers[ trackIdx ] ) != 0 )
            return;
    }

    alec( alSourceStop( m_musicSource ) );
    alec( alSourcei( m_musicSource, AL_BUFFER, static_cast<ALint>( m_musicBuffers[ trackIdx ] ) ) );
    alec( alSourcei( m_musicSource, AL_LOOPING, AL_TRUE ) );

    const float target = m_musicMuted ? 0.0f : ( ( m_musicDucked || m_tempDuckRemaining > 0.0 ) ? kMusicDuckedGain : kMusicNormalGain );
    applyMusicGain( target );
    alec( alSourcePlay( m_musicSource ) );

    m_currentMusicTrack = trackNum;
    m_inHelpMusic = false;
    m_musicPlaying = true;
}

void SoundManager::playHelpMusic()
{
    if ( m_musicSource == 0 )
        return;

    if ( m_musicPlaying && m_inHelpMusic )
        return;

    if ( m_helpMusicBuffer == 0 )
    {
        if ( loadOggBuffer( "music_lab_help.ogg", &m_helpMusicBuffer ) != 0 )
            return;
    }

    alec( alSourceStop( m_musicSource ) );
    alec( alSourcei( m_musicSource, AL_BUFFER, static_cast<ALint>( m_helpMusicBuffer ) ) );
    alec( alSourcei( m_musicSource, AL_LOOPING, AL_TRUE ) );

    applyMusicGain( m_musicMuted ? 0.0f : kMusicHelpGain );
    alec( alSourcePlay( m_musicSource ) );

    m_inHelpMusic = true;
    m_musicPlaying = true;
}

void SoundManager::stopMusic()
{
    if ( m_musicSource != 0 )
    {
        alec( alSourceStop( m_musicSource ) );
        alec( alSourcei( m_musicSource, AL_BUFFER, 0 ) );
    }
    m_musicPlaying = false;
    m_inHelpMusic = false;
    m_currentMusicTrack = 0;
}

void SoundManager::setMusicDucked( bool ducked )
{
    m_musicDucked = ducked;
}

void SoundManager::duckMusicTemporarily( double durationSec )
{
    if ( durationSec > m_tempDuckRemaining )
        m_tempDuckRemaining = durationSec;

    // Drop immediately so fanfares/sirens are crisp from the first millisecond
    if ( !m_musicMuted && m_currentMusicGain > kMusicDuckedGain )
        applyMusicGain( kMusicDuckedGain );
}

void SoundManager::toggleMusicMute()
{
    m_musicMuted = !m_musicMuted;
    if ( m_musicMuted )
    {
        applyMusicGain( 0.0f );
    }
    else
    {
        const float baseGain = m_inHelpMusic ? kMusicHelpGain : kMusicNormalGain;
        const float target = ( !m_inHelpMusic && ( m_musicDucked || m_tempDuckRemaining > 0.0 ) ) ? kMusicDuckedGain : baseGain;
        applyMusicGain( target );
    }
}

void SoundManager::updateMusic( double dtSec )
{
    if ( !m_musicPlaying || m_musicSource == 0 || dtSec <= 0.0 )
        return;

    const double clampedDt = ( std::min )( dtSec, 0.1 );
    if ( m_tempDuckRemaining > 0.0 )
    {
        m_tempDuckRemaining = ( std::max )( 0.0, m_tempDuckRemaining - clampedDt );
    }

    const float baseGain = m_inHelpMusic ? kMusicHelpGain : kMusicNormalGain;
    const float target = m_musicMuted
        ? 0.0f
        : ( ( !m_inHelpMusic && ( m_musicDucked || m_tempDuckRemaining > 0.0 ) ) ? kMusicDuckedGain : baseGain );

    const float diff = target - m_currentMusicGain;
    if ( std::abs( diff ) < 1e-4f )
        return;

    const float maxStep = kMusicGainSlewPerSec * static_cast<float>( clampedDt );
    const float nextGain = ( std::abs( diff ) <= maxStep )
        ? target
        : ( m_currentMusicGain + ( diff > 0.0f ? maxStep : -maxStep ) );

    applyMusicGain( nextGain );
}

void SoundManager::playRocket() const
{
    alec( alSourcePlay( m_monoSource ) );
}

void SoundManager::playDestroyBrick()
{
    alec( alSourceStop( m_monoSource ) );
    playStereoSource( m_stereoDesroyBrickSoundBuffer );
}

void SoundManager::playPing()
{
    playStereoSource( m_stereoPingSoundBuffer, 0.55f );
}

void SoundManager::playPong()
{
    playStereoSource( m_stereoPongSoundBuffer, 0.50f );
}

void SoundManager::playSiren()
{
    duckMusicTemporarily( 2.1 );
    playStereoSource( m_stereoLounchSoundBuffer, 0.5f, false );
}

void SoundManager::playCountdown()
{
    duckMusicTemporarily( 1.1 );
    playStereoSource( m_stereoCountdownSoundBuffer, 0.5f, false );
}

void SoundManager::playLevelComplete()
{
    duckMusicTemporarily( 5.2 );
    playStereoSource( m_stereoLevelCompleteSoundBuffer, 0.5f );
}

void SoundManager::playBallLost()
{
    duckMusicTemporarily( 3.4 );
    playStereoSource( m_stereoBallLostSoundBuffer );
}

void SoundManager::playCharShow()
{
    playStereoSource( m_stereoCharShowSoundBuffer );
}
