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
    #include <libavutil/channel_layout.h>
    #include <libavutil/opt.h>
}

#include "Video/FFmpeg.h"

#include "Common/Tools.h"
#include "AudioFile/AudioFile.h"
#include "SoundManager.h"

namespace
{
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
    m_device.reset( alcOpenDevice( defaultDeviceString ) );
    if ( !m_device )
    {
        std::cerr << "failed to get the default device for OpenAL" << std::endl;
        throw std::runtime_error { "failed to get the default device for OpenAL" };
    }
    std::cout << "OpenAL Device: " << alcGetString( m_device.get(), ALC_DEVICE_SPECIFIER ) << std::endl;

    // Create an OpenAL audio context from the device
    m_context.reset( alcCreateContext( m_device.get(), /*attrlist*/ nullptr ) );
    if ( !m_context )
    {
        std::cerr << "failed to create the OpenAL context" << std::endl;
        throw std::runtime_error { "failed to create the OpenAL context" };
    }

    // Activate this context so that OpenAL state modifications are applied to the context
    if ( !alcMakeContextCurrent( m_context.get() ) )
    {
        std::cerr << "failed to make the OpenAL context the current context" << std::endl;
        throw std::runtime_error { "failed to make the OpenAL context the current context" };
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
    if ( m_victoryMusicBuffer != 0 )
    {
        alec( alDeleteBuffers( 1, &m_victoryMusicBuffer ) );
        m_victoryMusicBuffer = 0;
    }

    alec( alDeleteSources( 1, &m_monoSource ) );
    alec( alDeleteSources( static_cast<ALsizei>( kSfxSourceCount ), m_stereoSource ) );
    if ( m_monoSoundBuffer != 0 )
    {
        alec( alDeleteBuffers( 1, &m_monoSoundBuffer ) );
        m_monoSoundBuffer = 0;
    }
    for ( ALuint& buf : m_sfxBuffers )
    {
        if ( buf != 0 )
        {
            alec( alDeleteBuffers( 1, &buf ) );
            buf = 0;
        }
    }

    // m_context and m_device are released automatically in reverse declaration order
    // after all sources and buffers above are deleted.
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
        rawBytes = Tools::Instance().loadResourceBytes( "resources/sounds/" + name );
    }
    catch ( const std::exception& e )
    {
        std::cerr << "Failed to read music resource " << name << ": " << e.what() << std::endl;
        return -1;
    }

    if ( rawBytes.empty() )
        return -1;

    auto fail = [ &name ]() -> int {
        std::cerr << "Failed to decode OGG music resource: " << name << std::endl;
        return -1;
    };

    // Strict destruction order: fmtCtx -> avioCtx -> reader
    MemBufferReader reader { rawBytes.data(), rawBytes.size(), 0 };
    constexpr int kAvioBufSize = 16384;
    auto* avioBuf = static_cast<uint8_t*>( av_malloc( kAvioBufSize ) );
    if ( !avioBuf )
        return fail();

    CustomAVIOContextPtr avioCtx { avio_alloc_context( avioBuf, kAvioBufSize, 0, &reader, &readMemPacket, nullptr, &seekMemPacket ) };
    if ( !avioCtx )
    {
        av_free( avioBuf );
        return fail();
    }

    AVFormatContextPtr fmtCtx { avformat_alloc_context() };
    if ( !fmtCtx )
        return fail();

    fmtCtx->pb = avioCtx.get();
    fmtCtx->flags |= AVFMT_FLAG_CUSTOM_IO;

    // avformat_open_input frees and nulls its AVFormatContext* argument on failure; release/reset avoids double-free.
    AVFormatContext* rawFmt = fmtCtx.release();
    const int openRet = avformat_open_input( &rawFmt, nullptr, nullptr, nullptr );
    fmtCtx.reset( rawFmt );
    if ( openRet != 0 || avformat_find_stream_info( fmtCtx.get(), nullptr ) < 0 )
        return fail();

    int audioStreamIdx = -1;
    for ( unsigned int i = 0; i < fmtCtx->nb_streams; ++i )
    {
        if ( fmtCtx->streams[ i ]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO )
        {
            audioStreamIdx = static_cast<int>( i );
            break;
        }
    }
    if ( audioStreamIdx < 0 )
        return fail();

    const AVCodecParameters* codecpar = fmtCtx->streams[ audioStreamIdx ]->codecpar;
    const AVCodec* decoder = avcodec_find_decoder( codecpar->codec_id );
    if ( !decoder )
        return fail();

    AVCodecContextPtr codecCtx { avcodec_alloc_context3( decoder ) };
    if ( !codecCtx ||
         avcodec_parameters_to_context( codecCtx.get(), codecpar ) < 0 ||
         avcodec_open2( codecCtx.get(), decoder, nullptr ) < 0 )
        return fail();

    constexpr int kTargetRate = 44100;
    constexpr int kTargetChannels = 2;

    const int64_t inChLayout = codecCtx->channel_layout
        ? static_cast<int64_t>( codecCtx->channel_layout )
        : av_get_default_channel_layout( codecCtx->channels );

    SwrContextPtr swrCtx { swr_alloc_set_opts(
        nullptr,
        AV_CH_LAYOUT_STEREO,
        AV_SAMPLE_FMT_S16,
        kTargetRate,
        inChLayout,
        codecCtx->sample_fmt,
        codecCtx->sample_rate,
        0,
        nullptr ) };
    if ( !swrCtx || swr_init( swrCtx.get() ) < 0 )
        return fail();

    AVFramePtr frame { av_frame_alloc() };
    AVPacketPtr pkt { av_packet_alloc() };
    if ( !frame || !pkt )
        return fail();

    std::vector<int16_t> pcm;
    auto drainFrames = [ & ]() {
        while ( avcodec_receive_frame( codecCtx.get(), frame.get() ) == 0 )
        {
            const int outSamples = static_cast<int>( av_rescale_rnd(
                swr_get_delay( swrCtx.get(), codecCtx->sample_rate ) + frame->nb_samples,
                kTargetRate,
                codecCtx->sample_rate,
                AV_ROUND_UP ) );
            if ( outSamples <= 0 )
                continue;

            const size_t prevSize = pcm.size();
            pcm.resize( prevSize + static_cast<size_t>( outSamples * kTargetChannels ) );
            uint8_t* outPtr = reinterpret_cast<uint8_t*>( pcm.data() + prevSize );
            const int converted = swr_convert(
                swrCtx.get(),
                &outPtr,
                outSamples,
                const_cast<const uint8_t**>( frame->extended_data ),
                frame->nb_samples );
            if ( converted > 0 )
                pcm.resize( prevSize + static_cast<size_t>( converted * kTargetChannels ) );
            else
                pcm.resize( prevSize );
            av_frame_unref( frame.get() );
        }
    };

    while ( av_read_frame( fmtCtx.get(), pkt.get() ) >= 0 )
    {
        if ( pkt->stream_index == audioStreamIdx )
        {
            if ( avcodec_send_packet( codecCtx.get(), pkt.get() ) >= 0 )
                drainFrames();
        }
        av_packet_unref( pkt.get() );
    }

    // Flush decoder and resampler
    avcodec_send_packet( codecCtx.get(), nullptr );
    drainFrames();

    const int remaining = static_cast<int>( swr_get_delay( swrCtx.get(), kTargetRate ) );
    if ( remaining > 0 )
    {
        const size_t prevSize = pcm.size();
        pcm.resize( prevSize + static_cast<size_t>( remaining * kTargetChannels ) );
        uint8_t* outPtr = reinterpret_cast<uint8_t*>( pcm.data() + prevSize );
        const int flushed = swr_convert( swrCtx.get(), &outPtr, remaining, nullptr, 0 );
        if ( flushed > 0 )
            pcm.resize( prevSize + static_cast<size_t>( flushed * kTargetChannels ) );
        else
            pcm.resize( prevSize );
    }

    if ( pcm.empty() )
        return fail();

    alec( alGenBuffers( 1, buffer ) );
    alec( alBufferData(
        *buffer,
        AL_FORMAT_STEREO16,
        pcm.data(),
        static_cast<ALsizei>( pcm.size() * sizeof( int16_t ) ),
        kTargetRate ) );
    return 0;
}

int SoundManager::init()
{
    static std::once_flag s_once;
    std::call_once( s_once, [ this ]() {
        AudioFile<float> soundFile;
        loadBuffer( soundFile, "Rocket.wav", &m_monoSoundBuffer );

        // load stereo SFX files into buffers
        static constexpr std::array<std::pair<SfxId, const char*>, static_cast<size_t>( SfxId::Count )> kSfxFiles {{
            { SfxId::Launch,        "Launch.wav"        },
            { SfxId::Countdown,     "Countdown.wav"     },
            { SfxId::DestroyBrick,  "DesroyBrick.wav"   },
            { SfxId::Ping,          "Ping.wav"          },
            { SfxId::Pong,          "Pong.wav"          },
            { SfxId::LevelComplete, "LevelComplete.wav" },
            { SfxId::BallLost,      "BallLost.wav"      },
            { SfxId::CharShow,      "Char.wav"          },
        }};
        for ( const auto& entry : kSfxFiles )
            loadBuffer( soundFile, entry.second, &sfxBuffer( entry.first ) );

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
        alec( alSourcef( m_musicSource, AL_GAIN, m_musicGain.currentGain ) );
        alec( alSourcei( m_musicSource, AL_LOOPING, AL_TRUE ) );
        alec( alSourcei( m_musicSource, AL_SOURCE_RELATIVE, AL_TRUE ) );
        alec( alSource3f( m_musicSource, AL_POSITION, 0.f, 0.f, 0.f ) );
    } );
    return 0;
}

ALuint SoundManager::getFreeStereoSource() const
{
    ALint sourceState( 0 );
    for ( size_t i = 0; i < kSfxSourceCount; ++i )
    {
        const size_t idx = ( m_nextStereoSource + i ) % kSfxSourceCount;
        const ALuint source = m_stereoSource[ idx ];
        if ( source == 0 || source == m_launchSource )
            continue;
        alec( alGetSourcei( source, AL_SOURCE_STATE, &sourceState ) );
        if ( sourceState == AL_PLAYING )
            continue;

        m_nextStereoSource = ( idx + 1 ) % kSfxSourceCount;
        return source;
    }
    return 0;
}

void SoundManager::cleanSources()
{
    for ( const auto source : m_stereoSource )
    {
        if ( source == 0 )
            continue;
        alSourceStop( source );
        alSourcei( source, AL_BUFFER, 0 );
    }
}

ALuint SoundManager::playStereoSource( ALuint buffer, float gain, float pitch )
{
    if ( buffer == 0 )
        return 0;

    auto source = getFreeStereoSource();
    if ( source == 0 )
    {
        for ( size_t i = 0; i < kSfxSourceCount; ++i )
        {
            const size_t idx = ( m_nextStereoSource + i ) % kSfxSourceCount;
            const ALuint candidate = m_stereoSource[ idx ];
            if ( candidate != 0 && candidate != m_launchSource )
            {
                source = candidate;
                m_nextStereoSource = ( idx + 1 ) % kSfxSourceCount;
                alec( alSourceStop( source ) );
                break;
            }
        }
    }
    if ( source == 0 )
        return 0;

    const float clampedPitch = std::isfinite( pitch ) ? std::clamp( pitch, 0.5f, 2.0f ) : 1.f;
    alec( alSourcef( source, AL_PITCH, clampedPitch ) );
    alec( alSourcef( source, AL_GAIN, gain ) );
    alec( alSourcei( source, AL_LOOPING, AL_FALSE ) );
    alec( alSourcei( source, AL_BUFFER, buffer ) );

    alec( alSourcePlay( source ) );

    return source;
}

void SoundManager::applyMusicGain( float gain )
{
    m_musicGain.currentGain = gain;
    if ( m_musicSource != 0 )
        alec( alSourcef( m_musicSource, AL_GAIN, m_musicGain.currentGain ) );
}

void SoundManager::startMusicTrack( ALuint buffer, bool isHelp, bool isVictory )
{
    if ( m_musicSource == 0 || buffer == 0 )
        return;

    alec( alSourceStop( m_musicSource ) );
    alec( alSourcei( m_musicSource, AL_BUFFER, static_cast<ALint>( buffer ) ) );
    alec( alSourcei( m_musicSource, AL_LOOPING, AL_TRUE ) );

    m_musicGain.inHelpMusic = isHelp;
    m_musicGain.inVictoryMusic = isVictory;
    applyMusicGain( MusicGain::computeTargetMusicGain( m_musicGain ) );
    alec( alSourcePlay( m_musicSource ) );

    m_musicPlaying = true;
}

void SoundManager::playMusic( unsigned short level )
{
    if ( m_musicSource == 0 )
        return;

    const unsigned short trackIdx = static_cast<unsigned short>(
        ( ( level > 0 ? level - 1 : 0 ) % kMusicTrackCount ) );
    const unsigned short trackNum = static_cast<unsigned short>( trackIdx + 1 );

    if ( m_musicPlaying && !m_musicGain.inHelpMusic && !m_musicGain.inVictoryMusic && m_currentMusicTrack == trackNum )
        return;

    // Lazy-decode level music track on first use (keeps startup instant)
    if ( m_musicBuffers[ trackIdx ] == 0 )
    {
        const std::string filename = "music_level_" + std::to_string( trackNum ) + ".ogg";
        if ( loadOggBuffer( filename, &m_musicBuffers[ trackIdx ] ) != 0 )
            return;
    }

    m_currentMusicTrack = trackNum;
    startMusicTrack( m_musicBuffers[ trackIdx ], false, false );
}

void SoundManager::playHelpMusic()
{
    if ( m_musicSource == 0 )
        return;

    if ( m_musicPlaying && m_musicGain.inHelpMusic )
        return;

    if ( m_helpMusicBuffer == 0 )
    {
        if ( loadOggBuffer( "music_lab_help.ogg", &m_helpMusicBuffer ) != 0 )
            return;
    }

    startMusicTrack( m_helpMusicBuffer, true, false );
}

void SoundManager::playVictoryMusic()
{
    if ( m_musicSource == 0 )
        return;

    if ( m_musicPlaying && m_musicGain.inVictoryMusic )
        return;

    if ( m_victoryMusicBuffer == 0 )
    {
        if ( loadOggBuffer( "music_victory.ogg", &m_victoryMusicBuffer ) != 0 )
            return;
    }

    startMusicTrack( m_victoryMusicBuffer, false, true );
}

void SoundManager::stopMusic()
{
    if ( m_musicSource != 0 )
    {
        alec( alSourceStop( m_musicSource ) );
        alec( alSourcei( m_musicSource, AL_BUFFER, 0 ) );
    }
    m_musicPlaying = false;
    m_musicGain.inHelpMusic = false;
    m_musicGain.inVictoryMusic = false;
    m_musicGain.resonanceScale = 1.0f;
    m_currentMusicTrack = 0;
}

void SoundManager::setMusicDucked( bool ducked )
{
    m_musicGain.ducked = ducked;
}

void SoundManager::setResonanceDuckScale( float scale )
{
    m_musicGain.resonanceScale = std::isfinite( scale ) ? std::clamp( scale, 0.5f, 1.0f ) : 1.0f;
}

void SoundManager::duckMusicTemporarily( double durationSec )
{
    if ( durationSec > m_musicGain.tempDuckRemaining )
        m_musicGain.tempDuckRemaining = durationSec;

    // Drop immediately so fanfares/sirens are crisp from the first millisecond
    const MusicGain::MusicGainConfig cfg {};
    if ( !m_musicGain.muted && !m_musicGain.inHelpMusic && !m_musicGain.inVictoryMusic &&
         m_musicGain.currentGain > cfg.duckedGain )
    {
        applyMusicGain( cfg.duckedGain );
    }
}

void SoundManager::toggleMusicMute()
{
    m_musicGain.muted = !m_musicGain.muted;
    applyMusicGain( MusicGain::computeTargetMusicGain( m_musicGain ) );
}

void SoundManager::updateMusic( double dtSec )
{
    if ( !m_musicPlaying || m_musicSource == 0 )
        return;

    if ( MusicGain::stepMusicGain( m_musicGain, dtSec ) )
        applyMusicGain( m_musicGain.currentGain );
}

void SoundManager::playRocket() const
{
    alec( alSourcePlay( m_monoSource ) );
}

void SoundManager::playDestroyBrick( float pitch )
{
    alec( alSourceStop( m_monoSource ) );
    playStereoSource( sfxBuffer( SfxId::DestroyBrick ), 1.f, pitch );
}

void SoundManager::playPing( float pitch )
{
    // Intentional PR #26 behavior: soft Pong.wav at 0.2f for paddle bounce, velocity/skill-shot pitched.
    playStereoSource( sfxBuffer( SfxId::Pong ), 0.2f, pitch );
}

void SoundManager::playArmorHit( float pitch )
{
    // Metallic Ping.wav feedback when 2-HP/3-HP BLUE composite armor absorbs a hit.
    playStereoSource( sfxBuffer( SfxId::Ping ), 0.75f, pitch );
}

void SoundManager::playPong( float pitch )
{
    playStereoSource( sfxBuffer( SfxId::Pong ), 0.25f, pitch );
}

void SoundManager::playSiren()
{
    duckMusicTemporarily( 2.1 );
    m_launchSource = playStereoSource( sfxBuffer( SfxId::Launch ), 0.5f );
}

void SoundManager::playCountdown()
{
    duckMusicTemporarily( 1.1 );
    m_launchSource = playStereoSource( sfxBuffer( SfxId::Countdown ), 0.5f );
}

bool SoundManager::isLaunchAudioPlaying() const
{
    if ( m_launchSource == 0 )
        return false;

    ALint sourceState( 0 );
    alec( alGetSourcei( m_launchSource, AL_SOURCE_STATE, &sourceState ) );
    return sourceState == AL_PLAYING;
}

void SoundManager::stopLaunchAudio()
{
    if ( m_launchSource != 0 )
    {
        alec( alSourceStop( m_launchSource ) );
        m_launchSource = 0;
    }
}

void SoundManager::playLevelComplete()
{
    duckMusicTemporarily( 5.2 );
    playStereoSource( sfxBuffer( SfxId::LevelComplete ), 0.5f );
}

void SoundManager::playBallLost()
{
    duckMusicTemporarily( 3.4 );
    playStereoSource( sfxBuffer( SfxId::BallLost ) );
}

void SoundManager::playCharShow()
{
    playStereoSource( sfxBuffer( SfxId::CharShow ) );
}

void SoundManager::playRobotToggle( bool enabled )
{
    playStereoSource( sfxBuffer( SfxId::CharShow ), 0.55f, enabled ? 1.35f : 0.85f );
}
