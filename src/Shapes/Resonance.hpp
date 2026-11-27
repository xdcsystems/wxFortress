#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>

#include <glm/vec3.hpp>

#include "Common/MathUtil.hpp"

namespace Resonance
{
    inline constexpr unsigned int kMaxMultiplier = 8u;
    inline constexpr uint32_t kRecordProximityThreshold = 100u;

    inline constexpr std::array<float, kMaxMultiplier> kPentatonicPitch {{
        1.000f, // x1 (base)
        1.060f, // x2
        1.122f, // x3
        1.189f, // x4 (micro-hit-stop threshold)
        1.260f, // x5
        1.335f, // x6
        1.414f, // x7
        1.500f  // x8 (max resonance, micro-hit-stop threshold)
    }};

    struct HitResult
    {
        unsigned int streak { 0 };
        unsigned int multiplier { 1 };
        uint32_t scoreDelta { 1 };
        float pitch { 1.0f };
        bool crossedHitStopThreshold { false };
    };

    inline unsigned int multiplierForStreak( unsigned int streak ) noexcept
    {
        if ( streak <= 1u )
            return 1u;
        return ( std::min )( streak, kMaxMultiplier );
    }

    inline float pitchForStreak( unsigned int streak ) noexcept
    {
        const unsigned int mult = multiplierForStreak( streak );
        return kPentatonicPitch[ static_cast<size_t>( mult - 1u ) ];
    }

    inline float musicDuckScaleForMultiplier( unsigned int multiplier ) noexcept
    {
        if ( multiplier < 3u )
            return 1.0f;
        const unsigned int clamped = ( std::min )( multiplier, kMaxMultiplier );
        // Linearly attenuate from 0.78 (x3) down to 0.62 (x8) so SFX pitch escalation cuts cleanly through the mix.
        const float t = static_cast<float>( clamped - 3u ) / static_cast<float>( kMaxMultiplier - 3u );
        return 0.78f - 0.16f * t;
    }

    struct State
    {
        unsigned int streak { 0 };

        unsigned int multiplier() const noexcept
        {
            return multiplierForStreak( streak );
        }

        HitResult onBrickHit() noexcept
        {
            const unsigned int prevMult = multiplier();
            ++streak;
            const unsigned int newMult = multiplier();

            HitResult res {};
            res.streak = streak;
            res.multiplier = newMult;
            res.scoreDelta = static_cast<uint32_t>( newMult );
            res.pitch = pitchForStreak( streak );
            res.crossedHitStopThreshold =
                ( prevMult < 4u && newMult >= 4u ) ||
                ( prevMult < 8u && newMult >= 8u );
            return res;
        }

        void onPaddleHit() noexcept
        {
            streak = 0u;
        }

        void reset() noexcept
        {
            streak = 0u;
        }
    };

    // Purely visual downward recoil of the manipulator on ball contact (never affects 120 Hz BallPhysics).
    struct PaddleRecoil
    {
        static constexpr float kMaxOffsetPx = 3.0f;
        static constexpr float kReturnRatePerSec = 32.0f;

        float offsetY { 0.0f };

        void trigger( float strengthPx = kMaxOffsetPx ) noexcept
        {
            if ( !std::isfinite( strengthPx ) || strengthPx <= 0.0f )
                return;
            offsetY = std::clamp( strengthPx, 0.0f, kMaxOffsetPx );
        }

        void step( float dtSec ) noexcept
        {
            if ( !MathUtil::isPositiveFinite( dtSec ) || offsetY <= 0.0f )
                return;
            const float clampedDt = ( std::min )( dtSec, 0.05f );
            offsetY = MathUtil::moveToward( offsetY, 0.0f, kReturnRatePerSec * clampedDt );
        }

        void reset() noexcept
        {
            offsetY = 0.0f;
        }
    };

    // Thresholded 40 ms micro-hit-stop + 1-quad additive sensor flash (~30 ms, 0 FBOs, ATI HD 3470 safe).
    struct ImpactFeedback
    {
        static constexpr double kHitStopDurationSec = 0.040;
        static constexpr double kFlashDurationSec = 0.032;
        static constexpr double kMaxStepDtSec = 0.05;

        double hitStopRemainingSec { 0.0 };
        double flashRemainingSec { 0.0 };
        double flashDurationSec { kFlashDurationSec };
        glm::vec3 flashColor { 0.55f, 0.95f, 0.85f };
        float flashIntensity { 0.0f };

        void triggerThresholdImpact( bool isMaxOrFinal ) noexcept
        {
            hitStopRemainingSec = ( std::max )( hitStopRemainingSec, kHitStopDurationSec );
            flashDurationSec = kFlashDurationSec;
            flashRemainingSec = kFlashDurationSec;
            flashColor = isMaxOrFinal
                ? glm::vec3( 0.70f, 1.00f, 0.92f )
                : glm::vec3( 0.45f, 0.92f, 0.72f );
            flashIntensity = isMaxOrFinal ? 0.26f : 0.18f;
        }

        void triggerBreachFlash() noexcept
        {
            hitStopRemainingSec = 0.0;
            flashDurationSec = 0.045;
            flashRemainingSec = 0.045;
            flashColor = glm::vec3( 0.95f, 0.28f, 0.20f );
            flashIntensity = 0.24f;
        }

        void step( double dtSec ) noexcept
        {
            if ( !MathUtil::isPositiveFinite( dtSec ) )
                return;
            const double clampedDt = ( std::min )( dtSec, kMaxStepDtSec );
            if ( hitStopRemainingSec > 0.0 )
                hitStopRemainingSec = ( std::max )( 0.0, hitStopRemainingSec - clampedDt );
            if ( flashRemainingSec > 0.0 )
                flashRemainingSec = ( std::max )( 0.0, flashRemainingSec - clampedDt );
        }

        bool isHitStopActive() const noexcept
        {
            return std::isfinite( hitStopRemainingSec ) && hitStopRemainingSec > 0.0;
        }

        float flashAlpha() const noexcept
        {
            if ( !std::isfinite( flashRemainingSec ) || !std::isfinite( flashDurationSec ) ||
                 flashRemainingSec <= 0.0 || flashDurationSec <= 0.0 )
                return 0.0f;
            const float norm = static_cast<float>( std::clamp( flashRemainingSec / flashDurationSec, 0.0, 1.0 ) );
            return std::clamp( flashIntensity * norm, 0.0f, 1.0f );
        }

        bool isActive() const noexcept
        {
            return isHitStopActive() || flashAlpha() > 0.001f;
        }

        void reset() noexcept
        {
            hitStopRemainingSec = 0.0;
            flashRemainingSec = 0.0;
            flashIntensity = 0.0f;
        }
    };

    // Zone 2 HUD status controller: transient events (2-3s) override persistent status, then automatically restore it.
    struct HudStatus
    {
        struct DisplayLine
        {
            std::string text;
            bool visible { false };
        };

        std::string transientText;
        double transientRemainingSec { 0.0 };

        void showTransient( const std::string& text, double durationSec = 2.5 )
        {
            if ( text.empty() || !MathUtil::isPositiveFinite( durationSec ) )
                return;
            transientText = text;
            transientRemainingSec = durationSec;
        }

        void clearTransient() noexcept
        {
            transientRemainingSec = 0.0;
            transientText.clear();
        }

        void step( double dtSec ) noexcept
        {
            if ( !MathUtil::isPositiveFinite( dtSec ) || transientRemainingSec <= 0.0 )
                return;
            const double clampedDt = ( std::min )( dtSec, 0.1 );
            transientRemainingSec = ( std::max )( 0.0, transientRemainingSec - clampedDt );
            if ( transientRemainingSec <= 0.0 )
                transientText.clear();
        }

        bool hasTransient() const noexcept
        {
            return std::isfinite( transientRemainingSec ) && transientRemainingSec > 0.0 && !transientText.empty();
        }

        DisplayLine resolveZone2( bool isRobotMode, uint32_t currentScore, uint32_t hiScore ) const
        {
            if ( hasTransient() )
                return { transientText, true };

            if ( isRobotMode )
                return { "AUTO MODE", true };

            if ( hiScore > 0u && currentScore < hiScore )
            {
                const uint32_t gap = hiScore - currentScore;
                if ( gap <= kRecordProximityThreshold )
                    return { "RECORD -" + std::to_string( gap ), true };
            }

            return {};
        }
    };
} // namespace Resonance
