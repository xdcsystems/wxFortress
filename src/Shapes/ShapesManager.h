#pragma once

#include <glm/glm.hpp>

#include <future>
#include "MoveController.h"
#include "Common/Semaphore.hpp"


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

    class ShapesManager final: public MoveController, public Semaphore
    {
        using rendererPtr = std::shared_ptr<SpriteRenderer>;
        using particlesPtr = std::shared_ptr<ParticleGenerator>;

        public:
            ShapesManager( wxWindow* parent );
            virtual ~ShapesManager();

            void run( bool bNewRound = false );
            void pause() { m_state = State::PAUSED; };
            void stop();

            bool isRunning() const { return m_state == State::RUN; };
            bool isPaused() const { return m_state == State::PAUSED; }
            bool isStopped() const { return m_state == State::STOPPED; }

            void renderFrame( const rendererPtr &spriteRenderer );

            void resize( const wxSize& size, const glm::mat4& projection );
            void loadLevel( unsigned short level ) const;

        protected:
            // Helper functions
            void update( double deltaTime );
            void calculateDelta();
            void changeMoveDirection( ContactPosition contactPosition, TypeContact typeContact = TypeContact::WallContact );
            void moveBoard( float value ) const;
            void checkKeysState();
            ContactPosition checkPaddleContact( bool checkOnly = true );
            ContactPosition checkContact( const glm::vec2& ballPosition, float beginValue, float endValue, float increment );
            Rect updateBallPosition( const Rect& boardBounds ) const;

        private:
            // Private data
            wxSize m_size;
            State m_state { State::STOPPED };

            bool m_isRobot { true };
            bool m_isRoundCompleted { false };

            float m_ballTopLimit { 0 };
            float m_ballBottomLimit { 0 };
            float m_accelerate { 0 };

            glm::vec2 m_delta { 0.f, 0.f };

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

            std::shared_ptr<ParticleGenerator> m_particles;

            std::atomic<bool> m_keepGoing { true };
            std::future<void> m_asyncWorker;
    };
}