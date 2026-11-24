#pragma once

#include <algorithm>
#include <cmath>

namespace MusicGain
{
    struct MusicGainConfig
    {
        float normalGain { 0.030f };
        float helpGain { 0.065f };
        float victoryGain { 0.045f };
        float duckedGain { 0.009f };
        float slewPerSec { 0.14f };
        double maxDtSec { 0.1 };
    };

    struct MusicGainState
    {
        bool muted { false };
        bool ducked { false };
        bool inHelpMusic { false };
        bool inVictoryMusic { false };
        double tempDuckRemaining { 0.0 };
        float currentGain { 0.030f };
    };

    inline float computeTargetMusicGain(
        const MusicGainState& state,
        const MusicGainConfig& cfg = {} ) noexcept
    {
        if ( state.muted )
            return 0.0f;

        const float baseGain = state.inHelpMusic
            ? cfg.helpGain
            : ( state.inVictoryMusic ? cfg.victoryGain : cfg.normalGain );

        if ( !state.inHelpMusic && !state.inVictoryMusic &&
             ( state.ducked || state.tempDuckRemaining > 0.0 ) )
        {
            return cfg.duckedGain;
        }
        return baseGain;
    }

    inline bool stepMusicGain(
        MusicGainState& state,
        double dtSec,
        const MusicGainConfig& cfg = {} ) noexcept
    {
        if ( !std::isfinite( dtSec ) || dtSec <= 0.0 )
            return false;

        const double clampedDt = ( std::min )( dtSec, cfg.maxDtSec );
        if ( state.tempDuckRemaining > 0.0 )
        {
            state.tempDuckRemaining = ( std::max )( 0.0, state.tempDuckRemaining - clampedDt );
        }

        const float target = computeTargetMusicGain( state, cfg );
        const float diff = target - state.currentGain;
        if ( std::abs( diff ) < 1e-4f )
            return false;

        const float maxStep = cfg.slewPerSec * static_cast<float>( clampedDt );
        state.currentGain = ( std::abs( diff ) <= maxStep )
            ? target
            : ( state.currentGain + ( diff > 0.0f ? maxStep : -maxStep ) );
        return true;
    }
} // namespace MusicGain
