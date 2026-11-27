#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

#include "Common/MathUtil.hpp"


namespace StageDirector
{
    inline constexpr double kHighSpeedCaptureDurationSec = 0.40;
    inline constexpr double kHighSpeedCaptureTimeScale = 0.25;
    inline constexpr unsigned int kHighSpeedMinResonance = 6u;
    inline constexpr double kTelemetryCooldownSec = 6.0;
    inline constexpr unsigned int kSkillShotTelemetryThreshold = 2u;

    inline constexpr const char* kHighSpeedCaptureBanner = "HIGH-SPEED CAPTURE";

    struct StageProfile
    {
        float startSpeedBonusPx { 0.0f };
        float levelStartMaxSpeed { 360.0f };
        float capsuleFallScale { 1.0f };
        float magLockDurationSec { 10.0f };
    };

    // Progressive Stage 1 -> N escalation profile (bounded so Stage 10+ stays fair & playable).
    inline StageProfile profileForStage( unsigned short stageIndex0Based ) noexcept
    {
        const unsigned short s6 = ( std::min )( stageIndex0Based, static_cast<unsigned short>( 6u ) );
        const unsigned short s4 = ( std::min )( stageIndex0Based, static_cast<unsigned short>( 4u ) );

        StageProfile p {};
        p.startSpeedBonusPx = static_cast<float>( s6 ) * 15.0f;
        p.levelStartMaxSpeed = 360.0f + static_cast<float>( s4 ) * 20.0f;
        p.capsuleFallScale = 1.0f + static_cast<float>( s4 ) * 0.12f;
        p.magLockDurationSec = ( std::max )( 6.0f, 10.0f - static_cast<float>( stageIndex0Based ) * 1.0f );
        return p;
    }

    // Selective HIGH-SPEED CAPTURE gate: triggers only on climactic round finishes so it stays special.
    inline bool shouldTriggerHighSpeedCapture(
        bool newRecordInRound,
        bool lastKillWasChainBlast,
        unsigned int resonanceMultiplier ) noexcept
    {
        return newRecordInRound || lastKillWasChainBlast || ( resonanceMultiplier >= kHighSpeedMinResonance );
    }

    struct HighSpeedCaptureState
    {
        double remainingSec { 0.0 };

        void trigger() noexcept
        {
            remainingSec = kHighSpeedCaptureDurationSec;
        }

        void reset() noexcept
        {
            remainingSec = 0.0;
        }

        void step( double realDtSec ) noexcept
        {
            if ( !MathUtil::isPositiveFinite( realDtSec ) || remainingSec <= 0.0 )
                return;
            const double dt = ( std::min )( realDtSec, 0.05 );
            remainingSec = ( std::max )( 0.0, remainingSec - dt );
        }

        bool isActive() const noexcept
        {
            return std::isfinite( remainingSec ) && remainingSec > 0.0;
        }

        double timeScale() const noexcept
        {
            return isActive() ? kHighSpeedCaptureTimeScale : 1.0;
        }
    };

    enum class TelemetryEvent : std::uint8_t
    {
        None = 0,
        StageEscalation,
        PlasmaPeak,
        VectorCut,
    };

    inline std::string telemetryText( TelemetryEvent ev, unsigned short stageIndex0Based )
    {
        switch ( ev )
        {
            case TelemetryEvent::StageEscalation:
                return "STAGE " + std::to_string( static_cast<unsigned int>( stageIndex0Based ) + 1u ) + ": ESCALATION";
            case TelemetryEvent::PlasmaPeak:
                return "PLASMA PEAK: OPTIMAL";
            case TelemetryEvent::VectorCut:
                return "VECTOR CUT: CONFIRMED";
            default:
                return {};
        }
    }

    // Concise diegetic AI observer assessment line appended to the post-5th-level report.
    inline std::string finishAssessmentLine( unsigned short completedStage1Based, unsigned int incidents )
    {
        if ( incidents == 0u )
        {
            return ( completedStage1Based >= 2u )
                ? "AI Assessment: Zero breaches at escalated load."
                : "AI Assessment: Zero breaches. Operator nominal.";
        }
        if ( incidents <= 2u )
            return "AI Assessment: Contour stable. Escalation ready.";
        return "AI Assessment: High breach rate. Recalibrate.";
    }

    // Rate-limited Zone 2 telemetry controller: never spams and never overrides high-priority banners.
    struct TelemetryState
    {
        double cooldownRemainingSec { 0.0 };

        void reset() noexcept
        {
            cooldownRemainingSec = 0.0;
        }

        void step( double dtSec ) noexcept
        {
            if ( !MathUtil::isPositiveFinite( dtSec ) || cooldownRemainingSec <= 0.0 )
                return;
            const double dt = ( std::min )( dtSec, 0.1 );
            cooldownRemainingSec = ( std::max )( 0.0, cooldownRemainingSec - dt );
        }

        std::string tryEmit( TelemetryEvent ev, unsigned short stage, bool hasHigherPriorityTransient )
        {
            if ( ev == TelemetryEvent::None || hasHigherPriorityTransient )
                return {};
            if ( std::isfinite( cooldownRemainingSec ) && cooldownRemainingSec > 0.0 )
                return {};

            const std::string msg = telemetryText( ev, stage );
            if ( !msg.empty() )
                cooldownRemainingSec = kTelemetryCooldownSec;
            return msg;
        }
    };
} // namespace StageDirector
