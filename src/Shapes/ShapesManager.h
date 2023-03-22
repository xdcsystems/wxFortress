#pragma once

#include <future>
#include "MoveController.h"
#include "Common/Semaphore.hpp"

DECLARE_LOCAL_EVENT_TYPE( wxEVT_CURRENT_SCORE_INCREASED, wxID_ANY )
DECLARE_LOCAL_EVENT_TYPE( wxEVT_ROUND_COMLETED, wxID_ANY )
DECLARE_LOCAL_EVENT_TYPE( wxEVT_BALL_LOST, wxID_ANY )
DECLARE_LOCAL_EVENT_TYPE( wxEVT_PING, wxID_ANY )
DECLARE_LOCAL_EVENT_TYPE( wxEVT_PONG, wxID_ANY )

// Forward declarations
class SpriteRenderer;
class ParticleGenerator;

namespace Shapes
{
    class Ball;
    class Board;
    class Bricks;

    class ShapesManager final: public MoveController, public Semaphore
    {
        using rendererPtr = std::shared_ptr<SpriteRenderer>;
        using particlesPtr = std::shared_ptr<ParticleGenerator>;

        public:
            ShapesManager( wxWindow* parent );
            virtual ~ShapesManager();

            bool switchRun( bool bNewRound = false );
            void renderFrame( rendererPtr spriteRenderer );

            void resize( const wxSize& size );
            void loadLevel( unsigned short level );

        protected:
            // Helper functions
            void stop();
            void update( double deltaTime );
            void changeMoveDirection( ContactPosition contactPosition, TypeContact typeContact = TypeContact::WallContact );
            void moveBoard();
            void checkKeysState();
            void checkPaddleContact();
            ContactPosition checkBrickContact( const glm::vec2& ballPosition, const glm::vec2& delta, float beginValue, float endValue, float increment );
            Rect updateBallPosition( const Rect& boardBounds ) const;

        private:
            // Private data
            wxSize m_size;
            bool m_bRun = false;
            bool m_isRobot = true;

            int m_boardMove = 0;
            float m_ballTopLimit = 0;
            float m_ballBottomLimit = 0;

            double m_diagonal = 0;
            double m_accelerate = 0;

            wxEvtHandler* m_eventHandler = nullptr;
            wxCommandEvent m_eventCurrentScoreInc;
            wxCommandEvent m_eventRoundCompleted;
            wxCommandEvent m_eventPing;
            wxCommandEvent m_eventPong;
            wxCommandEvent m_eventBallLost;

            std::vector<glm::vec2> m_trajectory;
            std::vector<glm::vec2>::const_iterator m_currentTrajPoint;

            std::shared_ptr<Ball> m_ball;
            std::shared_ptr<Board> m_board;
            std::shared_ptr<Bricks> m_bricks;

            std::shared_ptr<ParticleGenerator> m_particles;

            std::atomic<bool> m_keepGoing { true };
            std::future<void> m_asyncWorker;
    };
}