#pragma once

#include "Common/NonCopyable.hpp"

#include <memory>
#include <string>
#include <vector>
#include <glm/mat4x4.hpp>

#include "AiProtocols.hpp"
#include "StageDirector.hpp"
#include "BallPhysics.hpp"
#include "Resonance.hpp"


// Forward declarations
class SpriteRenderer;
class ParticleGenerator;

namespace Shapes
{
    class Ball;
    class Board;
    class Bricks;
    class Explosions;

    enum class State : unsigned char
    {
        RUN,
        PAUSED,
        STOPPED,
    };

    class ShapesManager final : private NonCopyable
    {
        using rendererPtr = std::shared_ptr<SpriteRenderer>;
        using particlesPtr = std::shared_ptr<ParticleGenerator>;

        public:
            ShapesManager( wxWindow* parent, const rendererPtr& spriteRenderer );
            ~ShapesManager() = default;


            void run( bool bNewRound = false );
            void pause() { m_state = State::PAUSED; m_smoothedPaddleVelX = 0.0f; }
            void stop();

            bool isRunning() const { return m_state == State::RUN; }
            bool isPaused() const { return m_state == State::PAUSED; }
            bool isStopped() const { return m_state == State::STOPPED; }

            bool isRobotMode() const noexcept { return m_isRobot; }
            bool toggleRobotMode() noexcept;

            unsigned int resonanceMultiplier() const noexcept { return m_resonance.multiplier(); }
            float resonanceMusicDuckScale() const noexcept
            {
                return Resonance::musicDuckScaleForMultiplier( m_resonance.multiplier() );
            }
            bool consumePendingImpact( bool& outMaxOrFinal ) noexcept;
            bool consumeRoundFinishTelemetry( bool& outLastKillChainBlast, unsigned int& outResonanceMult ) noexcept;

            std::string activeProtocolHudText() const { return m_protocols.hudStatusText( hasMagLockedBalls() ); }
            bool hasMagLockedBalls() const noexcept;
            bool releaseMagLockedBalls() noexcept;
            void setStageIncidents( unsigned int incidents ) noexcept { m_stageIncidents = incidents; }

            void update( double deltaTimeSec, int remainingSteps = 1 );
            void setPaddleMouseX( float mouseX );
            void setRenderAlpha( float alpha );
            void syncPreviousState();
            void renderFrame();

            void resize( const wxSize& size, const glm::mat4& projection );
            void loadLevel( unsigned short level, unsigned short stage = 0 );
            void resetBallSpeed();

        protected:
            struct BallInstance
            {
                Physics::BallState state {};
                Physics::Vec2 prevCenter { 0.0f, 0.0f };
                Physics::Vec2 magLaunchVel { 0.0f, 0.0f };
                bool magLocked { false };
                float magOffsetX { 0.0f };
                float robotHoldSec { 0.0f };
                float limiterSavedSpeed { 0.0f };
            };

            // Helper functions
            void initBallVelocity();
            void updatePaddleInput( float dtSec, int remainingSteps = 1 );
            Physics::Aabb updateBallPosition( const Physics::Aabb& boardBounds, bool syncPrev = false );
            void activateProtocol( AiProtocols::ProtocolKind kind );
            void syncPhysBricksFromBricks();

        private:
            // Private data
            wxSize m_size;
            State m_state { State::STOPPED };

            bool m_isRobot { false };
            unsigned int m_robotDryBounces { 0 };
            unsigned short m_stage { 0 };
            unsigned int m_stageIncidents { 0 };
            float m_currentBallSpeed { 0.0f };
            bool m_isRoundCompleted { false };
            bool m_hasMouseTarget { false };
            float m_mouseTargetX { 0.0f };

            Resonance::State m_resonance {};
            Resonance::PaddleRecoil m_paddleRecoil {};
            AiProtocols::ActiveState m_protocols {};
            std::vector<AiProtocols::ProtocolCapsule> m_capsules;
            float m_smoothedPaddleVelX { 0.0f };
            float m_paddleSkillFlashSec { 0.0f };
            bool m_pendingImpact { false };
            bool m_pendingImpactMaxOrFinal { false };
            bool m_pendingHighSpeedCapture { false };
            bool m_pendingHighSpeedChainBlast { false };
            unsigned int m_pendingHighSpeedResonance { 1u };
            bool m_plasmaPeakAnnounced { false };
            unsigned int m_roundSkillShots { 0u };
            bool m_vectorCutAnnounced { false };

            Physics::ArenaConfig m_arenaCfg {};
            std::vector<BallInstance> m_balls;
            Physics::PaddleState m_paddleState {};
            float m_prevPaddleX { 0.0f };
            float m_renderAlpha { 1.0f };
            float m_particleSpawnAccum { 0.0f };
            std::size_t m_particleOrbCursor { 0u };
            std::vector<Physics::BallState> m_robotBallScratch;
            std::vector<Physics::BrickItem> m_physBricks;

            wxEvtHandler* m_eventHandler { nullptr };

            std::shared_ptr<Ball> m_ball;
            std::shared_ptr<Board> m_board;
            std::shared_ptr<Bricks> m_bricks;
            std::shared_ptr<Explosions> m_explosions;

            particlesPtr m_particles;
            rendererPtr m_renderer;
    };
}
