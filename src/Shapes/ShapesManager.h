#pragma once

#include <memory>
#include <glm/glm.hpp>

#include "BallPhysics.hpp"


// Forward declarations
class Rect;
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

    class ShapesManager final
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

            void update( double deltaTimeSec );
            void renderFrame();

            void resize( const wxSize& size, const glm::mat4& projection );
            void loadLevel( unsigned short level ) const;

        protected:
            // Helper functions
            void initBallVelocity();
            void updatePaddleInput( float dtSec );
            Rect updateBallPosition( const Rect& boardBounds );

        private:
            // Private data
            wxSize m_size;
            State m_state { State::STOPPED };

            bool m_isRobot { false };
            bool m_isRoundCompleted { false };

            Physics::ArenaConfig m_arenaCfg {};
            Physics::BallState m_ballState {};
            Physics::PaddleState m_paddleState {};
            std::vector<Physics::BrickItem> m_physBricks;

            wxEvtHandler* m_eventHandler { nullptr };
            wxCommandEvent m_eventCurrentScoreInc;
            wxCommandEvent m_eventRoundCompleted;
            wxCommandEvent m_eventPing;
            wxCommandEvent m_eventPong;
            wxCommandEvent m_eventBallLost;

            std::shared_ptr<Ball> m_ball;
            std::shared_ptr<Board> m_board;
            std::shared_ptr<Bricks> m_bricks;
            std::shared_ptr<Explosions> m_explosions;

            particlesPtr m_particles;
            rendererPtr m_renderer;
    };
}
