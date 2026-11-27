#include <limits>
#include <gtest/gtest.h>

#include "BrickMechanics.hpp"
#include "StageDirector.hpp"

TEST( StageDirectorTest, StageProfileEscalatesSmoothlyAndStaysBounded )
{
    const auto s0 = StageDirector::profileForStage( 0u );
    EXPECT_FLOAT_EQ( s0.startSpeedBonusPx, 0.0f );
    EXPECT_FLOAT_EQ( s0.levelStartMaxSpeed, 360.0f );
    EXPECT_FLOAT_EQ( s0.capsuleFallScale, 1.0f );
    EXPECT_FLOAT_EQ( s0.magLockDurationSec, 10.0f );

    const auto s2 = StageDirector::profileForStage( 2u );
    EXPECT_GT( s2.startSpeedBonusPx, s0.startSpeedBonusPx );
    EXPECT_GT( s2.levelStartMaxSpeed, s0.levelStartMaxSpeed );
    EXPECT_GT( s2.capsuleFallScale, s0.capsuleFallScale );
    EXPECT_LT( s2.magLockDurationSec, s0.magLockDurationSec );

    // High stages stay capped & playable
    const auto s20 = StageDirector::profileForStage( 20u );
    EXPECT_FLOAT_EQ( s20.startSpeedBonusPx, 90.0f );
    EXPECT_FLOAT_EQ( s20.levelStartMaxSpeed, 440.0f );
    EXPECT_FLOAT_EQ( s20.capsuleFallScale, 1.48f );
    EXPECT_FLOAT_EQ( s20.magLockDurationSec, 6.0f );
}

TEST( StageDirectorTest, BrickHpEscalationPreservesBlueCapAndReinforcesHigherStages )
{
    using namespace BrickMechanics;
    // Stage 0 & 1: BLUE 2 -> 3 HP, PURPLE/ORANGE 1 HP
    EXPECT_EQ( initialHpForType( kTypeBlue, 0u ), 2u );
    EXPECT_EQ( initialHpForType( kTypeBlue, 1u ), 3u );
    EXPECT_EQ( initialHpForType( kTypePurple, 1u ), 1u );
    EXPECT_EQ( initialHpForType( kTypeOrange, 1u ), 1u );

    // Stage 2 (Stage 3 in UI): PURPLE reinforced to 2 HP
    EXPECT_EQ( initialHpForType( kTypePurple, 2u ), 2u );
    EXPECT_EQ( initialHpForType( kTypeOrange, 2u ), 1u );

    // Stage 3+ (Stage 4+ in UI): ORANGE reinforced to 2 HP, BLUE stays capped at 3 HP
    EXPECT_EQ( initialHpForType( kTypeOrange, 3u ), 2u );
    EXPECT_EQ( initialHpForType( kTypeBlue, 5u ), 3u );
}

TEST( StageDirectorTest, SelectiveHighSpeedCaptureTriggersOnlyOnClimacticFinishes )
{
    // Ordinary finish -> no slow-mo
    EXPECT_FALSE( StageDirector::shouldTriggerHighSpeedCapture( false, false, 1u ) );
    EXPECT_FALSE( StageDirector::shouldTriggerHighSpeedCapture( false, false, 5u ) );

    // Climactic finishes -> triggers slow-mo
    EXPECT_TRUE( StageDirector::shouldTriggerHighSpeedCapture( true, false, 1u ) );
    EXPECT_TRUE( StageDirector::shouldTriggerHighSpeedCapture( false, true, 1u ) );
    EXPECT_TRUE( StageDirector::shouldTriggerHighSpeedCapture( false, false, 6u ) );
    EXPECT_TRUE( StageDirector::shouldTriggerHighSpeedCapture( false, false, 8u ) );
}

TEST( StageDirectorTest, HighSpeedCaptureTimerScalesSimulationAndExpiresCleanly )
{
    StageDirector::HighSpeedCaptureState cap {};
    EXPECT_FALSE( cap.isActive() );
    EXPECT_DOUBLE_EQ( cap.timeScale(), 1.0 );

    cap.trigger();
    EXPECT_TRUE( cap.isActive() );
    EXPECT_DOUBLE_EQ( cap.timeScale(), StageDirector::kHighSpeedCaptureTimeScale );

    // Non-finite or negative dt ignored safely
    cap.step( std::numeric_limits<double>::quiet_NaN() );
    cap.step( -0.5 );
    EXPECT_TRUE( cap.isActive() );

    // Step 0.45s in 0.05s increments -> expires
    for ( int i = 0; i < 9; ++i )
        cap.step( 0.05 );
    EXPECT_FALSE( cap.isActive() );
    EXPECT_DOUBLE_EQ( cap.timeScale(), 1.0 );
}

TEST( StageDirectorTest, TelemetryRespectsCooldownAndNeverOverridesHigherPriorityBanners )
{
    StageDirector::TelemetryState tel {};

    // Higher-priority transient (e.g. BREACH / NEW RECORD) blocks telemetry without consuming cooldown
    EXPECT_TRUE( tel.tryEmit( StageDirector::TelemetryEvent::PlasmaPeak, 0u, true ).empty() );

    // Now emits cleanly when Zone 2 has no transient
    const std::string peak = tel.tryEmit( StageDirector::TelemetryEvent::PlasmaPeak, 0u, false );
    EXPECT_EQ( peak, "PLASMA PEAK: OPTIMAL" );

    // Immediately suppressed during 6s cooldown
    EXPECT_TRUE( tel.tryEmit( StageDirector::TelemetryEvent::VectorCut, 0u, false ).empty() );

    // Advance 6.1s -> cooldown expires and next telemetry emits
    for ( int i = 0; i < 61; ++i )
        tel.step( 0.1 );
    EXPECT_EQ( tel.tryEmit( StageDirector::TelemetryEvent::VectorCut, 0u, false ), "VECTOR CUT: CONFIRMED" );
}

TEST( StageDirectorTest, StageBannerAndFinishAssessmentLinesReflectIncidentsAndStage )
{
    EXPECT_EQ(
        StageDirector::telemetryText( StageDirector::TelemetryEvent::StageEscalation, 1u ),
        "STAGE 2: ESCALATION" );
    EXPECT_NE( StageDirector::finishAssessmentLine( 1u, 0u ).find( "Zero breaches" ), std::string::npos );
    EXPECT_NE( StageDirector::finishAssessmentLine( 2u, 0u ).find( "escalated" ), std::string::npos );
    EXPECT_NE( StageDirector::finishAssessmentLine( 1u, 2u ).find( "stable" ), std::string::npos );
    EXPECT_NE( StageDirector::finishAssessmentLine( 1u, 5u ).find( "Recalibrate" ), std::string::npos );
}

#include "AiProtocols.hpp"

TEST( StageDirectorTest, MagLockAcceptsEscalatedDurationAndGuardsInvalidInputs )
{
    AiProtocols::ActiveState proto {};
    const auto s4 = StageDirector::profileForStage( 4u );
    proto.activate( AiProtocols::ProtocolKind::MagLock, s4.magLockDurationSec );
    EXPECT_FLOAT_EQ( proto.magLockRemainingSec, 6.0f );
    EXPECT_EQ( proto.magLockCharges, AiProtocols::kMagLockMaxCharges );

    proto.activate( AiProtocols::ProtocolKind::MagLock, 0.0f );
    EXPECT_FLOAT_EQ( proto.magLockRemainingSec, AiProtocols::kMagLockDurationSec );

    proto.activate( AiProtocols::ProtocolKind::MagLock, -2.0f );
    EXPECT_FLOAT_EQ( proto.magLockRemainingSec, AiProtocols::kMagLockDurationSec );

    proto.activate( AiProtocols::ProtocolKind::MagLock, std::numeric_limits<float>::quiet_NaN() );
    EXPECT_FLOAT_EQ( proto.magLockRemainingSec, AiProtocols::kMagLockDurationSec );

    proto.activate( AiProtocols::ProtocolKind::MagLock, std::numeric_limits<float>::infinity() );
    EXPECT_FLOAT_EQ( proto.magLockRemainingSec, AiProtocols::kMagLockDurationSec );
}

TEST( StageDirectorTest, HighSpeedCaptureResetAndDtClamping )
{
    StageDirector::HighSpeedCaptureState hsc {};
    hsc.trigger();
    // Large dt is clamped to 0.05s per step so a single frame hitch cannot swallow slow-mo
    hsc.step( 10.0 );
    EXPECT_TRUE( hsc.isActive() );
    EXPECT_NEAR( hsc.remainingSec, StageDirector::kHighSpeedCaptureDurationSec - 0.05, 1e-6 );

    hsc.reset();
    EXPECT_FALSE( hsc.isActive() );
    EXPECT_DOUBLE_EQ( hsc.timeScale(), 1.0 );
}

TEST( StageDirectorTest, TelemetryEdgeCasesAndAssessmentBoundaries )
{
    StageDirector::TelemetryState tel {};
    EXPECT_TRUE( StageDirector::telemetryText( StageDirector::TelemetryEvent::None, 0u ).empty() );
    EXPECT_EQ( StageDirector::telemetryText( StageDirector::TelemetryEvent::StageEscalation, 2u ), "STAGE 3: ESCALATION" );

    // None event produces empty string and does not start cooldown
    EXPECT_TRUE( tel.tryEmit( StageDirector::TelemetryEvent::None, 0u, false ).empty() );
    EXPECT_DOUBLE_EQ( tel.cooldownRemainingSec, 0.0 );

    // Emit starts cooldown; NaN / negative dt ignored; huge dt clamped to 0.1s
    EXPECT_EQ( tel.tryEmit( StageDirector::TelemetryEvent::PlasmaPeak, 0u, false ), "PLASMA PEAK: OPTIMAL" );
    tel.step( std::numeric_limits<double>::quiet_NaN() );
    tel.step( -1.0 );
    EXPECT_DOUBLE_EQ( tel.cooldownRemainingSec, StageDirector::kTelemetryCooldownSec );
    tel.step( 100.0 );
    EXPECT_NEAR( tel.cooldownRemainingSec, StageDirector::kTelemetryCooldownSec - 0.1, 1e-6 );

    tel.reset();
    EXPECT_DOUBLE_EQ( tel.cooldownRemainingSec, 0.0 );

    // Assessment boundary: 2 incidents -> stable, 3 incidents -> Recalibrate
    EXPECT_NE( StageDirector::finishAssessmentLine( 1u, 2u ).find( "Contour stable" ), std::string::npos );
    EXPECT_NE( StageDirector::finishAssessmentLine( 1u, 3u ).find( "Recalibrate" ), std::string::npos );
}
