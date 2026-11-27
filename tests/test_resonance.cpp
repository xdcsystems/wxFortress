#include <limits>
#include <gtest/gtest.h>

#include "Resonance.hpp"

TEST( ResonanceTest, MultiplierAndPentatonicPitchScale )
{
    Resonance::State st {};
    EXPECT_EQ( st.multiplier(), 1u );

    float prevPitch = 0.0f;
    for ( unsigned int i = 1u; i <= 10u; ++i )
    {
        const Resonance::HitResult hit = st.onBrickHit();
        const unsigned int expectedMult = i <= 8u ? i : 8u;
        EXPECT_EQ( hit.streak, i );
        EXPECT_EQ( hit.multiplier, expectedMult );
        EXPECT_EQ( hit.scoreDelta, expectedMult );
        EXPECT_GE( hit.pitch, 1.0f );
        EXPECT_LE( hit.pitch, 1.5f );
        if ( i <= 8u )
            EXPECT_GT( hit.pitch, prevPitch );
        else
            EXPECT_FLOAT_EQ( hit.pitch, 1.5f );
        prevPitch = hit.pitch;

        const bool expectThreshold = ( i == 4u || i == 8u );
        EXPECT_EQ( hit.crossedHitStopThreshold, expectThreshold );
    }

    st.onPaddleHit();
    EXPECT_EQ( st.streak, 0u );
    EXPECT_EQ( st.multiplier(), 1u );
}

TEST( ResonanceTest, MusicDuckScaleActivatesAtX3 )
{
    EXPECT_FLOAT_EQ( Resonance::musicDuckScaleForMultiplier( 1u ), 1.0f );
    EXPECT_FLOAT_EQ( Resonance::musicDuckScaleForMultiplier( 2u ), 1.0f );
    EXPECT_NEAR( Resonance::musicDuckScaleForMultiplier( 3u ), 0.78f, 1e-5f );
    EXPECT_NEAR( Resonance::musicDuckScaleForMultiplier( 8u ), 0.62f, 1e-5f );
    EXPECT_FLOAT_EQ( Resonance::musicDuckScaleForMultiplier( 99u ), Resonance::musicDuckScaleForMultiplier( 8u ) );
}

TEST( ResonanceTest, PaddleRecoilClampsAndReturnsToZero )
{
    Resonance::PaddleRecoil recoil {};
    EXPECT_FLOAT_EQ( recoil.offsetY, 0.0f );

    recoil.trigger( 10.0f );
    EXPECT_FLOAT_EQ( recoil.offsetY, Resonance::PaddleRecoil::kMaxOffsetPx );

    recoil.trigger( std::numeric_limits<float>::quiet_NaN() );
    EXPECT_FLOAT_EQ( recoil.offsetY, Resonance::PaddleRecoil::kMaxOffsetPx );

    recoil.step( 0.0f );
    recoil.step( -0.1f );
    EXPECT_FLOAT_EQ( recoil.offsetY, Resonance::PaddleRecoil::kMaxOffsetPx );

    for ( int i = 0; i < 10; ++i )
        recoil.step( 0.016f );
    EXPECT_FLOAT_EQ( recoil.offsetY, 0.0f );
}

TEST( ResonanceTest, ImpactFeedbackHitStopAndFlashLifecycle )
{
    Resonance::ImpactFeedback fb {};
    EXPECT_FALSE( fb.isHitStopActive() );
    EXPECT_FLOAT_EQ( fb.flashAlpha(), 0.0f );

    fb.triggerThresholdImpact( false );
    EXPECT_TRUE( fb.isHitStopActive() );
    EXPECT_NEAR( fb.flashAlpha(), 0.18f, 1e-5f );

    // Non-finite dt must be ignored safely
    fb.step( std::numeric_limits<double>::quiet_NaN() );
    fb.step( -0.02 );
    EXPECT_TRUE( fb.isHitStopActive() );

    // Advance 20 ms: still in hit-stop, flash partially decayed
    fb.step( 0.020 );
    EXPECT_TRUE( fb.isHitStopActive() );
    EXPECT_GT( fb.flashAlpha(), 0.0f );
    EXPECT_LT( fb.flashAlpha(), 0.18f );

    // Advance another 25 ms: both hit-stop and flash completed
    fb.step( 0.025 );
    EXPECT_FALSE( fb.isHitStopActive() );
    EXPECT_FLOAT_EQ( fb.flashAlpha(), 0.0f );

    // Breach flash does not trigger hit-stop
    fb.triggerBreachFlash();
    EXPECT_FALSE( fb.isHitStopActive() );
    EXPECT_GT( fb.flashAlpha(), 0.20f );
}

TEST( ResonanceTest, HudStatusZone2PriorityAndFallback )
{
    Resonance::HudStatus hud {};

    // Idle / boundary checks: hiScore == 0, gap == 101, and score >= hiScore -> hidden
    EXPECT_FALSE( hud.resolveZone2( false, 0u, 0u ).visible );
    EXPECT_FALSE( hud.resolveZone2( false, 399u, 500u ).visible );
    EXPECT_FALSE( hud.resolveZone2( false, 500u, 500u ).visible );
    EXPECT_FALSE( hud.resolveZone2( false, 520u, 500u ).visible );

    // Exact boundary gap == 100 -> RECORD -100
    auto line = hud.resolveZone2( false, 400u, 500u );
    EXPECT_TRUE( line.visible );
    EXPECT_EQ( line.text, "RECORD -100" );

    // Robot mode takes persistent priority over RECORD -N
    line = hud.resolveZone2( true, 420u, 500u );
    EXPECT_TRUE( line.visible );
    EXPECT_EQ( line.text, "AUTO MODE" );

    // Transient event overrides persistent robot status for its duration
    hud.showTransient( "INCIDENT 1844/27: BREACH", 2.0 );
    line = hud.resolveZone2( true, 420u, 500u );
    EXPECT_TRUE( line.visible );
    EXPECT_EQ( line.text, "INCIDENT 1844/27: BREACH" );

    // clearTransient() immediately restores persistent status
    hud.clearTransient();
    EXPECT_FALSE( hud.hasTransient() );
    line = hud.resolveZone2( true, 420u, 500u );
    EXPECT_TRUE( line.visible );
    EXPECT_EQ( line.text, "AUTO MODE" );

    // Stepping past transient duration also restores persistent status automatically
    hud.showTransient( "NEW RECORD", 2.0 );
    for ( int i = 0; i < 25; ++i )
        hud.step( 0.1 );
    EXPECT_FALSE( hud.hasTransient() );
    line = hud.resolveZone2( true, 420u, 500u );
    EXPECT_TRUE( line.visible );
    EXPECT_EQ( line.text, "AUTO MODE" );
}
