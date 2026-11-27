#pragma once

#include "Common/NonCopyable.hpp"

#include <memory>
#include <glm/mat4x4.hpp>

#include "BallPhysics.hpp"


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
            void pause() { m_state = State::PAUSED; }
            void stop();

            bool isRunning() const { return m_state == State::RUN; }
            bool isPaused() const { return m_state == State::PAUSED; }
            bool isStopped() const { return m_state == State::STOPPED; }

            void update( double deltaTimeSec, int remainingSteps = 1 );
            void setPaddleMouseX( float mouseX );
            void setRenderAlpha( float alpha );
            void syncPreviousState();
            void renderFrame();

            void resize( const wxSize& size, const glm::mat4& projection );
            void loadLevel( unsigned short level ) const;
            void resetBallSpeed();

        protected:
            // Helper functions
            void initBallVelocity();
            void updatePaddleInput( float dtSec, int remainingSteps = 1 );
            Physics::Aabb updateBallPosition( const Physics::Aabb& boardBounds, bool syncPrev = false );

        private:
            // Private data
            wxSize m_size;
            State m_state { State::STOPPED };

            bool m_isRobot { false };
            unsigned int m_robotDryBounces { 0 };
            float m_currentBallSpeed { 0.0f };
            bool m_isRoundCompleted { false };
            bool m_hasMouseTarget { false };
            float m_mouseTargetX { 0.0f };

            Physics::ArenaConfig m_arenaCfg {};
            Physics::BallState m_ballState {};
            Physics::PaddleState m_paddleState {};
            Physics::Vec2 m_prevBallCenter { 0.0f, 0.0f };
            float m_prevPaddleX { 0.0f };
            float m_renderAlpha { 1.0f };
            float m_particleSpawnAccum { 0.0f };
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
