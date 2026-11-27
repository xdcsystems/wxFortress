#include <limits>
#include <gtest/gtest.h>

#include <type_traits>
#include "FramePacer.hpp"
#include "MathUtil.hpp"
#include "NonCopyable.hpp"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include "ScopeExit.hpp"

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

TEST( FramePacerTest, FixedStepAccumulatorStepsDeterministicallyAndInterpolates )
{
    FramePacing::FixedStepConfig cfg {};
    cfg.fixedDtSec = 1.0 / 120.0;
    cfg.maxFrameSec = 0.05;
    cfg.maxStepsPerFrame = 6;

    FramePacing::FixedStepState st {};
    // 60 Hz frame (~16.6667 ms) -> exactly 2 ticks of 120 Hz, alpha near 0
    const int steps60 = FramePacing::consumeFixedSteps( st, 1.0 / 60.0, cfg );
    EXPECT_EQ( steps60, 2 );
    EXPECT_GE( st.alpha, 0.0 );
    EXPECT_LE( st.alpha, 1.0 );

    // 144 Hz frame (~6.944 ms < 8.333 ms) -> 0 ticks first frame, alpha ~0.833
    FramePacing::resetFixedStep( st );
    const int steps144a = FramePacing::consumeFixedSteps( st, 1.0 / 144.0, cfg );
    EXPECT_EQ( steps144a, 0 );
    EXPECT_NEAR( st.alpha, 120.0 / 144.0, 1e-4 );

    // Second 144 Hz frame -> 1 tick, remainder ~5.556 ms -> alpha ~0.667
    const int steps144b = FramePacing::consumeFixedSteps( st, 1.0 / 144.0, cfg );
    EXPECT_EQ( steps144b, 1 );
    EXPECT_NEAR( st.alpha, ( 2.0 / 144.0 - 1.0 / 120.0 ) * 120.0, 1e-4 );
}

TEST( FramePacerTest, FixedStepAccumulatorSpiralOfDeathAndNonFiniteSafe )
{
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();

    FramePacing::FixedStepConfig cfg {};
    FramePacing::FixedStepState st {};

    // Huge 2.0s OS stall must be clamped by maxFrameSec / maxStepsPerFrame
    const int capped = FramePacing::consumeFixedSteps( st, 2.0, cfg );
    EXPECT_LE( capped, cfg.maxStepsPerFrame );
    EXPECT_GE( st.alpha, 0.0 );
    EXPECT_LE( st.alpha, 1.0 );

    // Explicitly exercise backlog discard when maxStepsPerFrame < maxFrameSec / fixedDtSec
    FramePacing::FixedStepConfig tightCfg {};
    tightCfg.fixedDtSec = 1.0 / 120.0;
    tightCfg.maxFrameSec = 0.05;
    tightCfg.maxStepsPerFrame = 2;
    FramePacing::resetFixedStep( st );
    EXPECT_EQ( FramePacing::consumeFixedSteps( st, 0.05, tightCfg ), 2 );
    EXPECT_DOUBLE_EQ( st.accumulatorSec, 0.0 );
    EXPECT_DOUBLE_EQ( st.alpha, 0.0 );

    // Non-finite and negative inputs must be ignored safely
    const double prevAccum = st.accumulatorSec;
    EXPECT_EQ( FramePacing::consumeFixedSteps( st, nan, cfg ), 0 );
    EXPECT_EQ( FramePacing::consumeFixedSteps( st, inf, cfg ), 0 );
    EXPECT_EQ( FramePacing::consumeFixedSteps( st, -0.016, cfg ), 0 );
    EXPECT_DOUBLE_EQ( st.accumulatorSec, prevAccum );

    // Invalid config parameters and corrupted accumulator state must self-heal
    FramePacing::FixedStepConfig badCfg {};
    badCfg.fixedDtSec = 0.0;
    EXPECT_EQ( FramePacing::consumeFixedSteps( st, 1.0 / 60.0, badCfg ), 0 );
    EXPECT_DOUBLE_EQ( st.alpha, 1.0 );

    badCfg.fixedDtSec = nan;
    EXPECT_EQ( FramePacing::consumeFixedSteps( st, 1.0 / 60.0, badCfg ), 0 );

    badCfg.fixedDtSec = 1.0 / 120.0;
    badCfg.maxFrameSec = -1.0;
    badCfg.maxStepsPerFrame = 0;
    st.accumulatorSec = nan;
    EXPECT_EQ( FramePacing::consumeFixedSteps( st, 1.0 / 60.0, badCfg ), 1 );
    EXPECT_DOUBLE_EQ( st.accumulatorSec, 0.0 );
    EXPECT_GE( st.alpha, 0.0 );
    EXPECT_LE( st.alpha, 1.0 );
}

TEST( CommonUtilsTest, ScopeExitRunsAndDismisses )
{
    int count = 0;
    {
        auto guard = makeScopeExit( [ &count ]() { ++count; } );
        (void)guard;
    }
    EXPECT_EQ( count, 1 );

    {
        auto guard = makeScopeExit( [ &count ]() { ++count; } );
        guard.dismiss();
    }
    EXPECT_EQ( count, 1 );

    // Lvalue functor and move-construction transfer ownership without double execution
    auto inc = [ &count ]() { ++count; };
    {
        auto guard1 = makeScopeExit( inc );
        auto guard2 = std::move( guard1 );
        (void)guard2;
    }
    EXPECT_EQ( count, 2 );
}

TEST( CommonUtilsTest, MathUtilGuardsAndMoveToward )
{
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();

    EXPECT_TRUE( MathUtil::isPositiveFinite( 0.016f ) );
    EXPECT_FALSE( MathUtil::isPositiveFinite( 0.0f ) );
    EXPECT_FALSE( MathUtil::isPositiveFinite( -1.0f ) );
    EXPECT_FALSE( MathUtil::isPositiveFinite( nan ) );
    EXPECT_FALSE( MathUtil::isPositiveFinite( inf ) );

    EXPECT_FLOAT_EQ( MathUtil::clampFinite( 5.0f, 0.0f, 10.0f, 1.0f ), 5.0f );
    EXPECT_FLOAT_EQ( MathUtil::clampFinite( nan, 0.0f, 10.0f, 1.0f ), 1.0f );
    EXPECT_FLOAT_EQ( MathUtil::clampFinite( 5.0f, 10.0f, 2.0f, 1.0f ), 10.0f );
    EXPECT_FLOAT_EQ( MathUtil::clampFinite( 5.0f, nan, 10.0f, 1.0f ), 1.0f );
    EXPECT_FLOAT_EQ( MathUtil::clampFinite( 5.0f, 0.0f, nan, 1.0f ), 1.0f );

    EXPECT_FLOAT_EQ( MathUtil::moveToward( 0.0f, 10.0f, 3.0f ), 3.0f );
    EXPECT_FLOAT_EQ( MathUtil::moveToward( 9.0f, 10.0f, 3.0f ), 10.0f );
    EXPECT_FLOAT_EQ( MathUtil::moveToward( 10.0f, 0.0f, 3.0f ), 7.0f );
    EXPECT_FLOAT_EQ( MathUtil::moveToward( 5.0f, 10.0f, nan ), 5.0f );
}

TEST( CommonUtilsTest, NonCopyableTraits )
{
    struct Resource : private NonCopyable {};
    static_assert( !std::is_copy_constructible_v<Resource> );
    static_assert( !std::is_copy_assignable_v<Resource> );
    static_assert( !std::is_move_constructible_v<Resource> );
    static_assert( !std::is_move_assignable_v<Resource> );
    EXPECT_EQ( sizeof( Resource ), 1u );
}

TEST( CommonUtilsTest, SpriteUvRectMappingMatchesLegacyQuads )
{
    // Matches Sprite.vs: uv = uvRect.xy + vertex.zw * uvRect.zw; TexCoords = vec2(uv.x, 1.0 - uv.y)
    auto evalTexCoord = []( const glm::vec2& vertZw, const glm::vec4& uvRect ) -> glm::vec2 {
        const glm::vec2 uv { uvRect.x + vertZw.x * uvRect.z, uvRect.y + vertZw.y * uvRect.w };
        return { uv.x, 1.0f - uv.y };
    };

    // Unit quad corners (pos -> vertex.zw): (0,0)->(0,1), (0,1)->(0,0), (1,0)->(1,1), (1,1)->(1,0)
    const glm::vec2 bl { 0.0f, 1.0f };
    const glm::vec2 tl { 0.0f, 0.0f };
    const glm::vec2 br { 1.0f, 1.0f };
    const glm::vec2 tr { 1.0f, 0.0f };

    // 1. Full texture (default uvRect = {0, 0, 1, 1})
    const glm::vec4 fullUv { 0.0f, 0.0f, 1.0f, 1.0f };
    EXPECT_FLOAT_EQ( evalTexCoord( bl, fullUv ).x, 0.0f );
    EXPECT_FLOAT_EQ( evalTexCoord( bl, fullUv ).y, 0.0f );
    EXPECT_FLOAT_EQ( evalTexCoord( tr, fullUv ).x, 1.0f );
    EXPECT_FLOAT_EQ( evalTexCoord( tr, fullUv ).y, 1.0f );

    // 2. Explosion sub-tile (left=0.25, top=0.5, uScale=0.25, vScale=0.25 -> right=0.5, bottom=0.75)
    const glm::vec4 expUv { 0.25f, 0.5f, 0.25f, 0.25f };
    EXPECT_FLOAT_EQ( evalTexCoord( bl, expUv ).x, 0.25f );
    EXPECT_FLOAT_EQ( evalTexCoord( bl, expUv ).y, 0.25f ); // 1.0 - bottom (0.75)
    EXPECT_FLOAT_EQ( evalTexCoord( tl, expUv ).x, 0.25f );
    EXPECT_FLOAT_EQ( evalTexCoord( tl, expUv ).y, 0.50f ); // 1.0 - top (0.50)
    EXPECT_FLOAT_EQ( evalTexCoord( br, expUv ).x, 0.50f );
    EXPECT_FLOAT_EQ( evalTexCoord( br, expUv ).y, 0.25f );
    EXPECT_FLOAT_EQ( evalTexCoord( tr, expUv ).x, 0.50f );
    EXPECT_FLOAT_EQ( evalTexCoord( tr, expUv ).y, 0.50f );
}
