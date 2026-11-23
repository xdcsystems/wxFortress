#pragma once

#include <array>
#include <string>

#include <AL/al.h>
#include <AL/alc.h>
#include "Common/defs.h"

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

class SoundManager final
{
    public:
        static constexpr size_t kMusicTrackCount = 5;
        static constexpr size_t kSfxSourceCount = 64;

        SoundManager();
        ~SoundManager();

        int init();
        void playSiren();
        void playCountdown();
        void playRocket() const;
        void playDestroyBrick();
        void playPing();
        void playPong();
        void playLevelComplete();
        void playBallLost();
        void playCharShow();

        // Background music (OpenAL looping + smooth ducking/mute)
        void playMusic( unsigned short level );
        void playHelpMusic();
        void stopMusic();
        void setMusicDucked( bool ducked );
        void duckMusicTemporarily( double durationSec );
        void toggleMusicMute();
        bool isMusicMuted() const { return m_musicMuted; }
        void updateMusic( double dtSec );

    private:
        int loadBuffer( AudioFile<float>& soundFile, const std::string& name, ALuint* buffer );
        int loadOggBuffer( const std::string& name, ALuint* buffer );
        ALuint getFreeStereoSource() const;
        void cleanSources();
        void playStereoSource( ALuint buffer, float gain = 1.f, bool playAsync = true );
        void applyMusicGain( float gain );

        ALCdevice* m_device { nullptr };
        ALCcontext* m_context { nullptr };
        ALuint m_monoSoundBuffer { 0 };

        ALuint m_stereoLounchSoundBuffer { 0 };
        ALuint m_stereoCountdownSoundBuffer { 0 };
        ALuint m_stereoDesroyBrickSoundBuffer { 0 };
        ALuint m_stereoPingSoundBuffer { 0 };
        ALuint m_stereoPongSoundBuffer { 0 };
        ALuint m_stereoLevelCompleteSoundBuffer { 0 };
        ALuint m_stereoBallLostSoundBuffer { 0 };
        ALuint m_stereoCharShowSoundBuffer { 0 };

        ALuint m_monoSource { 0 };
        ALuint m_stereoSource[ kSfxSourceCount ] {};

        ALuint m_musicSource { 0 };
        std::array<ALuint, kMusicTrackCount> m_musicBuffers {};
        ALuint m_helpMusicBuffer { 0 };
        bool m_inHelpMusic { false };
        unsigned short m_currentMusicTrack { 0 };
        bool m_musicPlaying { false };
        bool m_musicMuted { false };
        bool m_musicDucked { false };
        double m_tempDuckRemaining { 0.0 };
        float m_currentMusicGain { 0.030f };
};
