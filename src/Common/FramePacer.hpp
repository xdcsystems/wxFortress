#pragma once

#include <algorithm>
#include <cmath>

namespace FramePacing
{
    struct Config
    {
        double nominalPeriodSec { 1.0 / 60.0 };
        double maxDtSec { 0.05 };
        double jitterLowRatio { 0.55 };
        double jitterHighRatio { 1.45 };
        double emaAlpha { 0.15 };
        double minExternalStepSec { 0.002 };
        double fastSyncThresholdMs { 2.0 };
        int fastSyncFallbackCount { 5 };
    };

    struct State
    {
        double nominalPeriodSec { 1.0 / 60.0 };
        double smoothedDtSec { 1.0 / 60.0 };
        bool hasFirstFrame { false };
        bool paceExternally { false };
        int fastSyncStreak { 0 };
    };

    /** Updates the display refresh period and clamps or initializes smoothed dt. */
    inline void setNominalPeriodSec( State& st, double periodSec )
    {
        if ( !std::isfinite( periodSec ) )
            return;

        if ( periodSec < ( 1.0 / 360.0 ) || periodSec > ( 1.0 / 24.0 ) )
            return;

        st.nominalPeriodSec = periodSec;
        if ( !st.hasFirstFrame )
        {
            st.smoothedDtSec = periodSec;
        }
        else
        {
            st.smoothedDtSec = std::clamp( st.smoothedDtSec, 0.9 * periodSec, 1.1 * periodSec );
        }
    }

    /** Resets per-run frame timing across pauses or round launches while keeping sync state. */
    inline void resetFrameTiming( State& st )
    {
        st.hasFirstFrame = false;
        st.smoothedDtSec = st.nominalPeriodSec;
    }

    /** Returns true when enough time has elapsed to advance simulation and present a frame. */
    inline bool shouldStep( const State& st, double elapsedSec, const Config& cfg = {} )
    {
        if ( !std::isfinite( elapsedSec ) || elapsedSec < 0.0 )
            return false;

        if ( st.paceExternally )
            return elapsedSec >= cfg.minExternalStepSec;

        return elapsedSec >= std::max( 0.001, st.nominalPeriodSec - 0.0005 );
    }

    /** Computes bounded idle sleep duration in milliseconds while waiting for the next frame. */
    inline unsigned long computeSleepMs( const State& st, double elapsedSec )
    {
        if ( !std::isfinite( elapsedSec ) )
            return 1UL;

        const double remainingMs = ( st.nominalPeriodSec - elapsedSec ) * 1000.0 - 1.0;
        const double clampedMs = std::clamp( remainingMs, 1.0, 4.0 );
        return static_cast<unsigned long>( clampedMs );
    }

    /** Computes simulation dt without letting one-off OS hitches poison the EMA baseline. */
    inline double computeStepDt( State& st, double rawDtSec, const Config& cfg = {} )
    {
        if ( !std::isfinite( rawDtSec ) || rawDtSec <= 0.0 )
            return 0.0;

        if ( !st.hasFirstFrame )
        {
            st.hasFirstFrame = true;
            st.smoothedDtSec = st.nominalPeriodSec;
            return std::clamp( rawDtSec, 0.0, st.nominalPeriodSec );
        }

        if ( st.paceExternally )
        {
            if ( rawDtSec >= cfg.jitterLowRatio * st.nominalPeriodSec &&
                 rawDtSec <= cfg.jitterHighRatio * st.nominalPeriodSec )
            {
                st.smoothedDtSec = ( 1.0 - cfg.emaAlpha ) * st.smoothedDtSec + cfg.emaAlpha * rawDtSec;
                st.smoothedDtSec = std::clamp( st.smoothedDtSec, 0.9 * st.nominalPeriodSec, 1.1 * st.nominalPeriodSec );
                return st.smoothedDtSec;
            }

            // Do not overwrite smoothedDtSec on spikes or short wakes to prevent rubber-banding.
            return std::clamp( rawDtSec, 0.0, cfg.maxDtSec );
        }

        return std::clamp( rawDtSec, 0.0, cfg.maxDtSec );
    }

    /** Tracks consecutive non-blocking presents to fall back cleanly to software pacing. */
    inline void recordSyncDuration( State& st, bool syncSucceeded, double syncWaitMs, const Config& cfg = {} )
    {
        if ( syncSucceeded && std::isfinite( syncWaitMs ) && syncWaitMs >= cfg.fastSyncThresholdMs )
        {
            st.fastSyncStreak = 0;
            st.paceExternally = true;
            return;
        }

        if ( ++st.fastSyncStreak >= cfg.fastSyncFallbackCount )
        {
            st.paceExternally = false;
        }
    }
} // namespace FramePacing
