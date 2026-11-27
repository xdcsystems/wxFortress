#pragma once

#include "Common/NonCopyable.hpp"

#include <array>
#include <memory>
#include <string>

#include <AL/al.h>
#include <AL/alc.h>
#include "Common/defs.h"
#include "MusicGain.hpp"

#if defined( _DEBUG ) && SKIP_AL_CHECKS == 0
//OpenAL error checking
#define OpenAL_ErrorCheck( message )                                                              \
    {                                                                                             \
        ALenum error = alGetError();                                                              \
        if ( error != AL_NO_ERROR )                                                               \
        {                                                                                         \
            std::cerr << "OpenAL Error: " << error << " with call for " << #message << std::endl; \
        }                                                                                         \
    }
#define alec( FUNCTION_CALL ) \
    FUNCTION_CALL;            \
    OpenAL_ErrorCheck( FUNCTION_CALL )
#else
#define alec( stmt ) stmt
#endif

template <class T>
class AudioFile;

class SoundManager final : private NonCopyable
{
    public:
        static constexpr size_t kMusicTrackCount = 5;
        static constexpr size_t kSfxSourceCount = 64;

        SoundManager();
        ~SoundManager();

        int init();
        void playSiren();
        void playCountdown();
        bool isLaunchAudioPlaying() const;
        void stopLaunchAudio();
        void playRocket() const;
        void playDestroyBrick( float pitch = 1.f );
        void playPing( float pitch = 1.f );
        void playArmorHit( float pitch = 1.25f );
        void playPong( float pitch = 1.f );
        void playLevelComplete();
        void playBallLost();
        void playCharShow();
        void playRobotToggle( bool enabled );

        // Background music (OpenAL looping + smooth ducking/mute)
        void playMusic( unsigned short level );
        void playHelpMusic();
        void playVictoryMusic();
        void stopMusic();
        void setMusicDucked( bool ducked );
        void setResonanceDuckScale( float scale );
        void duckMusicTemporarily( double durationSec );
        void toggleMusicMute();
        bool isMusicMuted() const { return m_musicGain.muted; }
        bool isMusicGainTransitioning() const
        {
            return m_musicPlaying && m_musicSource != 0 && MusicGain::isMusicGainTransitioning( m_musicGain );
        }
        void updateMusic( double dtSec );

    private:
        int loadBuffer( AudioFile<float>& soundFile, const std::string& name, ALuint* buffer );
        int loadOggBuffer( const std::string& name, ALuint* buffer );
        ALuint getFreeStereoSource() const;
        void cleanSources();
        ALuint playStereoSource( ALuint buffer, float gain = 1.f, float pitch = 1.f );
        void applyMusicGain( float gain );
        void startMusicTrack( ALuint buffer, bool isHelp, bool isVictory );

        struct AlDeviceDeleter
        {
            void operator()( ALCdevice* dev ) const noexcept
            {
                if ( dev )
                    alcCloseDevice( dev );
            }
        };
        struct AlContextDeleter
        {
            void operator()( ALCcontext* ctx ) const noexcept
            {
                if ( ctx )
                {
                    if ( alcGetCurrentContext() == ctx )
                        alcMakeContextCurrent( nullptr );
                    alcDestroyContext( ctx );
                }
            }
        };

        // Declared in strict teardown reverse order: context destroyed before device;
        // ~SoundManager() deletes all sources/buffers before member destructors run.
        std::unique_ptr<ALCdevice, AlDeviceDeleter> m_device;
        std::unique_ptr<ALCcontext, AlContextDeleter> m_context;
        ALuint m_monoSoundBuffer { 0 };

        enum class SfxId : size_t
        {
            Launch = 0,
            Countdown,
            DestroyBrick,
            Ping, // Kept loaded (Ping.wav) for upcoming armored-brick metallic SFX
            Pong,
            LevelComplete,
            BallLost,
            CharShow,
            Count
        };

        ALuint& sfxBuffer( SfxId id ) noexcept { return m_sfxBuffers[ static_cast<size_t>( id ) ]; }
        ALuint  sfxBuffer( SfxId id ) const noexcept { return m_sfxBuffers[ static_cast<size_t>( id ) ]; }

        std::array<ALuint, static_cast<size_t>( SfxId::Count )> m_sfxBuffers {};

        ALuint m_monoSource { 0 };
        ALuint m_stereoSource[ kSfxSourceCount ] {};
        mutable size_t m_nextStereoSource { 0 };
        ALuint m_launchSource { 0 };

        ALuint m_musicSource { 0 };
        std::array<ALuint, kMusicTrackCount> m_musicBuffers {};
        ALuint m_helpMusicBuffer { 0 };
        ALuint m_victoryMusicBuffer { 0 };
        unsigned short m_currentMusicTrack { 0 };
        bool m_musicPlaying { false };
        MusicGain::MusicGainState m_musicGain {};
};
