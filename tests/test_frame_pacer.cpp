#include <limits>
#include <gtest/gtest.h>

#include "FramePacer.hpp"

TEST( FramePacerTest, NominalPeriodClampAndReset )
{
    FramePacing::State st {};
    const double defaultPeriod = st.nominalPeriodSec;

    // Out-of-range or non-finite values must be ignored.
    FramePacing::setNominalPeriodSec( st, 0.0 );
    EXPECT_DOUBLE_EQ( st.nominalPeriodSec, defaultPeriod );

    FramePacing::setNominalPeriodSec( st, -0.016 );
    EXPECT_DOUBLE_EQ( st.nominalPeriodSec, defaultPeriod );

    FramePacing::setNominalPeriodSec( st, 0.25 ); // < 24 Hz
    EXPECT_DOUBLE_EQ( st.nominalPeriodSec, defaultPeriod );

    FramePacing::setNominalPeriodSec( st, 0.001 ); // > 360 Hz
    EXPECT_DOUBLE_EQ( st.nominalPeriodSec, defaultPeriod );

    FramePacing::setNominalPeriodSec( st, std::numeric_limits<double>::quiet_NaN() );
    EXPECT_DOUBLE_EQ( st.nominalPeriodSec, defaultPeriod );

    FramePacing::setNominalPeriodSec( st, std::numeric_limits<double>::infinity() );
    EXPECT_DOUBLE_EQ( st.nominalPeriodSec, defaultPeriod );

    // Valid 75 Hz refresh period updates nominalPeriodSec and smoothedDtSec.
    const double period75 = 1.0 / 75.0;
    FramePacing::setNominalPeriodSec( st, period75 );
    EXPECT_DOUBLE_EQ( st.nominalPeriodSec, period75 );
    EXPECT_DOUBLE_EQ( st.smoothedDtSec, period75 );

    // Advancing past first frame, changing to 60 Hz clamps smoothedDtSec to [0.9*P, 1.1*P].
    st.hasFirstFrame = true;
    st.smoothedDtSec = period75;
    const double period60 = 1.0 / 60.0;
    FramePacing::setNominalPeriodSec( st, period60 );
    EXPECT_DOUBLE_EQ( st.nominalPeriodSec, period60 );
    EXPECT_NEAR( st.smoothedDtSec, 0.9 * period60, 1e-12 );

    // resetFrameTiming resets per-run state while preserving nominalPeriodSec & sync flags.
    st.paceExternally = true;
    st.fastSyncStreak = 3;
    FramePacing::resetFrameTiming( st );
    EXPECT_FALSE( st.hasFirstFrame );
    EXPECT_DOUBLE_EQ( st.smoothedDtSec, period60 );
    EXPECT_DOUBLE_EQ( st.nominalPeriodSec, period60 );
    EXPECT_TRUE( st.paceExternally );
    EXPECT_EQ( st.fastSyncStreak, 3 );
}

TEST( FramePacerTest, FirstFrameNeverJumps )
{
    FramePacing::State st {};
    st.paceExternally = true;

    // Even if a long delay occurred before the first frame, dt is bounded to nominalPeriodSec.
    const double dt1 = FramePacing::computeStepDt( st, 0.350 );
    EXPECT_DOUBLE_EQ( dt1, st.nominalPeriodSec );
    EXPECT_TRUE( st.hasFirstFrame );
    EXPECT_DOUBLE_EQ( st.smoothedDtSec, st.nominalPeriodSec );

    // Reset and launch with a smaller initial dt (< nominalPeriodSec).
    FramePacing::resetFrameTiming( st );
    const double dt2 = FramePacing::computeStepDt( st, 0.008 );
    EXPECT_DOUBLE_EQ( dt2, 0.008 );
    EXPECT_TRUE( st.hasFirstFrame );
    EXPECT_DOUBLE_EQ( st.smoothedDtSec, st.nominalPeriodSec );
}

TEST( FramePacerTest, ExternalPacingSmoothsWakeJitter )
{
    FramePacing::State st {};
    st.paceExternally = true;

    // Prime first frame
    EXPECT_DOUBLE_EQ( FramePacing::computeStepDt( st, 0.016667 ), st.nominalPeriodSec );

    // Feed alternating DWM/OS wake jitter (21.2 ms / 12.2 ms, same mean 16.7 ms)
    for ( int i = 0; i < 10; ++i )
    {
        const double raw = ( i % 2 == 0 ) ? 0.0212 : 0.0122;
        const double dt = FramePacing::computeStepDt( st, raw );
        EXPECT_GE( dt, 0.9 * st.nominalPeriodSec - 1e-12 );
        EXPECT_LE( dt, 1.1 * st.nominalPeriodSec + 1e-12 );
        EXPECT_NEAR( dt, st.nominalPeriodSec, 0.001 );
    }
}

TEST( FramePacerTest, SpikeDoesNotPoisonSmoothedDt )
{
    FramePacing::State st {};
    st.paceExternally = true;

    // Prime baseline at 60 Hz
    FramePacing::computeStepDt( st, st.nominalPeriodSec );
    EXPECT_DOUBLE_EQ( st.smoothedDtSec, st.nominalPeriodSec );

    // Simulate OS hitches of 45 ms and 120 ms (> jitterHighRatio * nominalPeriodSec)
    const double dt45 = FramePacing::computeStepDt( st, 0.045 );
    EXPECT_DOUBLE_EQ( dt45, 0.045 );
    EXPECT_DOUBLE_EQ( st.smoothedDtSec, st.nominalPeriodSec );

    const double dt120 = FramePacing::computeStepDt( st, 0.120 );
    EXPECT_DOUBLE_EQ( dt120, 0.05 ); // clamped to maxDtSec
    EXPECT_DOUBLE_EQ( st.smoothedDtSec, st.nominalPeriodSec );

    // Very next normal frame must immediately resume smooth ~16.67 ms (no rubber-banding)
    const double dtNext = FramePacing::computeStepDt( st, st.nominalPeriodSec );
    EXPECT_NEAR( dtNext, st.nominalPeriodSec, 1e-9 );
}

TEST( FramePacerTest, SyncHysteresisPreventsFlipFlop )
{
    FramePacing::State st {};
    st.paceExternally = true;

    // 1..4 fast syncs (< 2.0 ms) must stay externally paced.
    for ( int i = 1; i <= 4; ++i )
    {
        FramePacing::recordSyncDuration( st, true, 0.5 );
        EXPECT_TRUE( st.paceExternally );
        EXPECT_EQ( st.fastSyncStreak, i );
    }

    // A single blocking sync resets streak and keeps external pacing.
    FramePacing::recordSyncDuration( st, true, 16.0 );
    EXPECT_TRUE( st.paceExternally );
    EXPECT_EQ( st.fastSyncStreak, 0 );

    // 5 consecutive fast syncs trigger fallback to software pacing.
    for ( int i = 0; i < 4; ++i )
        FramePacing::recordSyncDuration( st, true, 0.3 );
    EXPECT_TRUE( st.paceExternally );
    FramePacing::recordSyncDuration( st, true, 0.3 );
    EXPECT_FALSE( st.paceExternally );

    // Failed sync also counts toward fallback; a blocking sync >= 2.0 ms restores external pacing.
    FramePacing::recordSyncDuration( st, false, 16.0 );
    EXPECT_FALSE( st.paceExternally );

    FramePacing::recordSyncDuration( st, true, 2.0 );
    EXPECT_TRUE( st.paceExternally );
    EXPECT_EQ( st.fastSyncStreak, 0 );
}

TEST( FramePacerTest, SoftwareFallbackPacesAtTrueRefreshAndSleepsBounded )
{
    FramePacing::State st {};
    st.paceExternally = false;
    const double period = st.nominalPeriodSec; // 16.6667 ms

    // shouldStep requires elapsedSec >= period - 0.5 ms
    EXPECT_FALSE( FramePacing::shouldStep( st, period - 0.001 ) );
    EXPECT_TRUE( FramePacing::shouldStep( st, period - 0.0004 ) );
    EXPECT_TRUE( FramePacing::shouldStep( st, period ) );

    // computeSleepMs is always clamped to [1, 4]
    EXPECT_EQ( FramePacing::computeSleepMs( st, 0.0 ), 4UL );
    EXPECT_EQ( FramePacing::computeSleepMs( st, 0.0135 ), 2UL );
    EXPECT_EQ( FramePacing::computeSleepMs( st, 0.0160 ), 1UL );
    EXPECT_EQ( FramePacing::computeSleepMs( st, 0.0250 ), 1UL );

    // External pacing uses minExternalStepSec (2 ms)
    st.paceExternally = true;
    EXPECT_FALSE( FramePacing::shouldStep( st, 0.0015 ) );
    EXPECT_TRUE( FramePacing::shouldStep( st, 0.0020 ) );
}

TEST( FramePacerTest, NonFiniteAndNegativeInputsSafe )
{
    FramePacing::State st {};
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();

    EXPECT_FALSE( FramePacing::shouldStep( st, -0.01 ) );
    EXPECT_FALSE( FramePacing::shouldStep( st, nan ) );
    EXPECT_FALSE( FramePacing::shouldStep( st, inf ) );

    EXPECT_EQ( FramePacing::computeSleepMs( st, nan ), 1UL );
    EXPECT_EQ( FramePacing::computeSleepMs( st, inf ), 1UL );

    EXPECT_DOUBLE_EQ( FramePacing::computeStepDt( st, 0.0 ), 0.0 );
    EXPECT_DOUBLE_EQ( FramePacing::computeStepDt( st, -0.02 ), 0.0 );
    EXPECT_DOUBLE_EQ( FramePacing::computeStepDt( st, nan ), 0.0 );
    EXPECT_DOUBLE_EQ( FramePacing::computeStepDt( st, inf ), 0.0 );
    EXPECT_FALSE( st.hasFirstFrame );

    st.paceExternally = true;
    FramePacing::recordSyncDuration( st, true, nan );
    EXPECT_EQ( st.fastSyncStreak, 1 );
    FramePacing::recordSyncDuration( st, true, inf );
    EXPECT_EQ( st.fastSyncStreak, 2 );
}
