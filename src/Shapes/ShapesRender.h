#pragma once

#include <future>
#include "Common/Semaphore.hpp"
#include "MoveController.h"

#define NUM_OF_CALCULATED_POINTS 5

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
    enum class ContactPosition : unsigned char;

    class ShapesRender : public MoveController, public Semaphore
    {
        using rendererPtr = std::shared_ptr<SpriteRenderer>;
        using particlesPtr = std::shared_ptr<ParticleGenerator>;

        public:
            ShapesRender( wxWindow* parent );
            virtual ~ShapesRender();

            void resize( const wxSize& size );
            void loadLevel( unsigned short level );

        protected:
            void stop();
            void update( double deltaTime );
            void calculateTrajectory();
            void changeMoveDirection( ContactPosition contactPosition, TypeContact typeContact = WallContact );
            void moveBoard();
            void checkBallContact();
            void checkKeysState();
            xRect updateBallPosition( const xRect& boardBounds ) const;

        protected:
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

            std::atomic<bool> m_keepGoing{ true };
            std::future<void> m_asyncWorker;
    };
}