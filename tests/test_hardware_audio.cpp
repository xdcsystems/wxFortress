#include <limits>
#include <gtest/gtest.h>

#include "HardwareCaps.hpp"
#include "MusicGain.hpp"

TEST( HardwareCapsTest, LegacyAmdUvdFamiliesMatch )
{
    // ATI Mobility Radeon HD 3470 (RV620, 0x95C4) + representative IDs across all 12 UVD/UVD+ families
    EXPECT_TRUE( HardwareCaps::isLegacyAmdUvd( 0x95C4u ) );
    EXPECT_TRUE( HardwareCaps::isLegacyAmdUvd( 0x9400u ) );
    EXPECT_TRUE( HardwareCaps::isLegacyAmdUvd( 0x9442u ) );
    EXPECT_TRUE( HardwareCaps::isLegacyAmdUvd( 0x945Au ) );
    EXPECT_TRUE( HardwareCaps::isLegacyAmdUvd( 0x9460u ) );
    EXPECT_TRUE( HardwareCaps::isLegacyAmdUvd( 0x94C3u ) );
    EXPECT_TRUE( HardwareCaps::isLegacyAmdUvd( 0x9501u ) );
    EXPECT_TRUE( HardwareCaps::isLegacyAmdUvd( 0x9515u ) );
    EXPECT_TRUE( HardwareCaps::isLegacyAmdUvd( 0x9588u ) );
    EXPECT_TRUE( HardwareCaps::isLegacyAmdUvd( 0x9598u ) );
    EXPECT_TRUE( HardwareCaps::isLegacyAmdUvd( 0x9610u ) );
    EXPECT_TRUE( HardwareCaps::isLegacyAmdUvd( 0x9715u ) );
}

TEST( HardwareCapsTest, ModernAmdAndNonUvdRejected )
{
    EXPECT_FALSE( HardwareCaps::isLegacyAmdUvd( 0x68F9u ) ); // HD 5450 (Cedar / UVD 2.2)
    EXPECT_FALSE( HardwareCaps::isLegacyAmdUvd( 0x67DFu ) ); // RX 480 / 580 (Polaris)
    EXPECT_FALSE( HardwareCaps::isLegacyAmdUvd( 0x731Fu ) ); // RX 5700 (RDNA 1)
    EXPECT_FALSE( HardwareCaps::isLegacyAmdUvd( 0x95D0u ) ); // Adjacent family outside table
    EXPECT_FALSE( HardwareCaps::isLegacyAmdUvd( 0x0000u ) );
    EXPECT_FALSE( HardwareCaps::isLegacyAmdUvd( 0xFFFFu ) );
}

TEST( HardwareCapsTest, MaxDpb41RefFrames )
{
    EXPECT_EQ( HardwareCaps::maxDpb41RefFrames( 1024, 576 ), 11 );
    EXPECT_EQ( HardwareCaps::maxDpb41RefFrames( 1280, 720 ), 9 );
    EXPECT_EQ( HardwareCaps::maxDpb41RefFrames( 1920, 1080 ), 4 );
    EXPECT_EQ( HardwareCaps::maxDpb41RefFrames( 0, 0 ), 4 );
    EXPECT_EQ( HardwareCaps::maxDpb41RefFrames( -100, 576 ), 4 );
    EXPECT_EQ( HardwareCaps::maxDpb41RefFrames( 100000, 100000 ), 0 );
}

TEST( HardwareCapsTest, LegacyAmdUvdStreamAllowed )
{
    // Intro.mp4 profile: 1024x576, H.264 High@L4.1, 4 ref frames
    EXPECT_TRUE( HardwareCaps::isLegacyAmdUvdStreamAllowed( 41, 4, 1024, 576 ) );
    EXPECT_TRUE( HardwareCaps::isLegacyAmdUvdStreamAllowed( 41, 11, 1024, 576 ) );
    EXPECT_FALSE( HardwareCaps::isLegacyAmdUvdStreamAllowed( 41, 12, 1024, 576 ) );
    EXPECT_FALSE( HardwareCaps::isLegacyAmdUvdStreamAllowed( 51, 4, 1024, 576 ) );

    // 1080p Level 4.1 allows at most 4 ref frames
    EXPECT_TRUE( HardwareCaps::isLegacyAmdUvdStreamAllowed( 41, 4, 1920, 1080 ) );
    EXPECT_FALSE( HardwareCaps::isLegacyAmdUvdStreamAllowed( 41, 5, 1920, 1080 ) );
}

TEST( MusicGainTest, NormalHelpVictoryBaseTargets )
{
    const MusicGain::MusicGainConfig cfg {};
    MusicGain::MusicGainState st {};

    EXPECT_FLOAT_EQ( MusicGain::computeTargetMusicGain( st, cfg ), cfg.normalGain );

    st.inHelpMusic = true;
    EXPECT_FLOAT_EQ( MusicGain::computeTargetMusicGain( st, cfg ), cfg.helpGain );

    st.inHelpMusic = false;
    st.inVictoryMusic = true;
    EXPECT_FLOAT_EQ( MusicGain::computeTargetMusicGain( st, cfg ), cfg.victoryGain );
}

TEST( MusicGainTest, DuckingAppliesOnlyToGameplayNotHelpOrVictory )
{
    const MusicGain::MusicGainConfig cfg {};
    MusicGain::MusicGainState st {};
    st.ducked = true;
    EXPECT_FLOAT_EQ( MusicGain::computeTargetMusicGain( st, cfg ), cfg.duckedGain );

    st.inHelpMusic = true;
    EXPECT_FLOAT_EQ( MusicGain::computeTargetMusicGain( st, cfg ), cfg.helpGain );

    st.inHelpMusic = false;
    st.inVictoryMusic = true;
    EXPECT_FLOAT_EQ( MusicGain::computeTargetMusicGain( st, cfg ), cfg.victoryGain );
}

TEST( MusicGainTest, MuteOverridesAllModesAndRestoresCleanly )
{
    const MusicGain::MusicGainConfig cfg {};
    MusicGain::MusicGainState st {};
    st.ducked = true;
    st.muted = true;
    EXPECT_FLOAT_EQ( MusicGain::computeTargetMusicGain( st, cfg ), 0.0f );

    st.inVictoryMusic = true;
    EXPECT_FLOAT_EQ( MusicGain::computeTargetMusicGain( st, cfg ), 0.0f );

    st.muted = false;
    st.inVictoryMusic = false;
    EXPECT_FLOAT_EQ( MusicGain::computeTargetMusicGain( st, cfg ), cfg.duckedGain );
}

TEST( MusicGainTest, StepMusicGainSlewsSmoothlyAndClampsDt )
{
    const MusicGain::MusicGainConfig cfg {};
    MusicGain::MusicGainState st {};
    st.currentGain = cfg.duckedGain;

    // Large dt (5.0s) must be clamped to maxDtSec (0.1s) -> max step = 0.14 * 0.1 = 0.014
    ASSERT_TRUE( MusicGain::stepMusicGain( st, 5.0, cfg ) );
    EXPECT_NEAR( st.currentGain, cfg.duckedGain + 0.014f, 1e-5f );

    // Second 0.1s step reaches normalGain (0.030f)
    ASSERT_TRUE( MusicGain::stepMusicGain( st, 0.1, cfg ) );
    EXPECT_FLOAT_EQ( st.currentGain, cfg.normalGain );

    // Already at target -> no further update
    EXPECT_FALSE( MusicGain::stepMusicGain( st, 0.1, cfg ) );
    EXPECT_FALSE( MusicGain::stepMusicGain( st, 0.0, cfg ) );
    EXPECT_FALSE( MusicGain::stepMusicGain( st, -0.5, cfg ) );
    EXPECT_FALSE( MusicGain::stepMusicGain( st, std::numeric_limits<double>::quiet_NaN(), cfg ) );
    EXPECT_FALSE( MusicGain::stepMusicGain( st, std::numeric_limits<double>::infinity(), cfg ) );
}

TEST( MusicGainTest, TempDuckCountdownExpiresAndRecovers )
{
    const MusicGain::MusicGainConfig cfg {};
    MusicGain::MusicGainState st {};
    st.tempDuckRemaining = 0.25;
    st.currentGain = cfg.duckedGain;

    // 0.1s + 0.1s: still temporarily ducked
    MusicGain::stepMusicGain( st, 0.1, cfg );
    EXPECT_NEAR( st.tempDuckRemaining, 0.15, 1e-6 );
    EXPECT_FLOAT_EQ( st.currentGain, cfg.duckedGain );

    MusicGain::stepMusicGain( st, 0.1, cfg );
    EXPECT_NEAR( st.tempDuckRemaining, 0.05, 1e-6 );
    EXPECT_FLOAT_EQ( st.currentGain, cfg.duckedGain );

    // Third 0.1s step expires tempDuckRemaining and begins slewing back toward normalGain
    ASSERT_TRUE( MusicGain::stepMusicGain( st, 0.1, cfg ) );
    EXPECT_DOUBLE_EQ( st.tempDuckRemaining, 0.0 );
    EXPECT_GT( st.currentGain, cfg.duckedGain );

    // Subsequent step finishes recovery
    ASSERT_TRUE( MusicGain::stepMusicGain( st, 0.1, cfg ) );
    EXPECT_FLOAT_EQ( st.currentGain, cfg.normalGain );
}

TEST( MusicGainTest, IsTransitioningReflectsSlewTempDuckAndNonFiniteGuards )
{
    const MusicGain::MusicGainConfig cfg {};
    MusicGain::MusicGainState st {};

    EXPECT_FALSE( MusicGain::isMusicGainTransitioning( st, cfg ) );

    st.ducked = true;
    EXPECT_TRUE( MusicGain::isMusicGainTransitioning( st, cfg ) );
    for ( int i = 0; i < 10; ++i )
        MusicGain::stepMusicGain( st, 0.1, cfg );
    EXPECT_FALSE( MusicGain::isMusicGainTransitioning( st, cfg ) );

    st.tempDuckRemaining = 0.2;
    EXPECT_TRUE( MusicGain::isMusicGainTransitioning( st, cfg ) );

    // Non-finite state values must never trap the idle loop in transitioning=true
    st.tempDuckRemaining = std::numeric_limits<double>::infinity();
    st.currentGain = std::numeric_limits<float>::infinity();
    EXPECT_FALSE( MusicGain::isMusicGainTransitioning( st, cfg ) );

    st.tempDuckRemaining = std::numeric_limits<double>::quiet_NaN();
    st.currentGain = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE( MusicGain::isMusicGainTransitioning( st, cfg ) );
}

TEST( MusicGainTest, ResonanceScaleAttenuatesNormalGameplayWithoutOverridingDucked )
{
    const MusicGain::MusicGainConfig cfg {};
    MusicGain::MusicGainState st {};

    st.resonanceScale = 0.70f;
    EXPECT_NEAR( MusicGain::computeTargetMusicGain( st, cfg ), cfg.normalGain * 0.70f, 1e-6f );

    // Extreme scale clamps to 0.5f and stays >= duckedGain
    st.resonanceScale = 0.10f;
    EXPECT_NEAR( MusicGain::computeTargetMusicGain( st, cfg ), cfg.normalGain * 0.50f, 1e-6f );

    // Non-finite scale falls back cleanly to normalGain
    st.resonanceScale = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FLOAT_EQ( MusicGain::computeTargetMusicGain( st, cfg ), cfg.normalGain );

    // Binary ducked / tempDuckRemaining always takes precedence over resonanceScale
    st.resonanceScale = 0.78f;
    st.ducked = true;
    EXPECT_FLOAT_EQ( MusicGain::computeTargetMusicGain( st, cfg ), cfg.duckedGain );

    // Help / victory music ignores resonanceScale
    st.ducked = false;
    st.inHelpMusic = true;
    EXPECT_FLOAT_EQ( MusicGain::computeTargetMusicGain( st, cfg ), cfg.helpGain );
}
