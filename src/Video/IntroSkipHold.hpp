#pragma once

#include <algorithm>
#include <cmath>

namespace IntroSkipHold
{
    constexpr double kHoldDurationSec = 3.0;

    struct State
    {
        bool holding { false };
        bool triggered { false };
        double startSec { 0.0 };
        double progress { 0.0 };
    };

    inline void reset( State& s )
    {
        s = State {};
    }

    inline bool onKeyDown( State& s, double nowSec )
    {
        if ( s.triggered || s.holding || !std::isfinite( nowSec ) )
            return false;

        s.holding = true;
        s.startSec = nowSec;
        s.progress = 0.0;
        return true;
    }

    inline bool onKeyUp( State& s )
    {
        if ( s.triggered )
            return false;

        const bool wasHolding = s.holding || s.progress > 0.0;
        s.holding = false;
        s.progress = 0.0;
        return wasHolding;
    }

    inline bool update( State& s, double nowSec )
    {
        if ( !s.holding || s.triggered )
        {
            if ( !s.holding && !s.triggered )
                s.progress = 0.0;
            return false;
        }

        if ( !std::isfinite( nowSec ) )
            return false;

        const double elapsed = std::max( 0.0, nowSec - s.startSec );
        s.progress = std::clamp( elapsed / kHoldDurationSec, 0.0, 1.0 );

        if ( elapsed >= kHoldDurationSec )
        {
            s.triggered = true;
            s.holding = false;
            s.progress = 1.0;
            return true;
        }

        return false;
    }
}
