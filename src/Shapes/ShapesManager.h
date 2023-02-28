#pragma once

DECLARE_LOCAL_EVENT_TYPE( wxEVT_CURRENT_SCORE_INCREASED, wxID_ANY )
DECLARE_LOCAL_EVENT_TYPE( wxEVT_ROUND_COMLETED, wxID_ANY )
DECLARE_LOCAL_EVENT_TYPE( wxEVT_BALL_LOST, wxID_ANY )
DECLARE_LOCAL_EVENT_TYPE( wxEVT_PING, wxID_ANY )
DECLARE_LOCAL_EVENT_TYPE( wxEVT_PONG, wxID_ANY )

namespace Shapes
{
    class Ball;
    class Board;
    class Bricks;
    enum class ContactPosition : unsigned char;

    class ShapesManager : public wxObject
    {
        public:
            enum class MoveDirection : char
            {
                DirectionLeft = -1,
                DirectionNone = 0,
                DirectionRight = 1,
                DirectionTop,
                DirectionDown,
                DirectionTopRight,
                DirectionRightDown,
                DirectionLeftDown,
                DirectionTopLeft
            };

            friend int operator*( MoveDirection m, char i )
            {
                return (char)m * i;
            }
            friend int operator+( MoveDirection m, char i )
            {
                return ( char )m + i;
            }
            friend int operator-( MoveDirection m, char i )
            {
                return ( char )m - i;
            }

            enum class TypeContact : unsigned char
            {
                BrickContact,
                PaddleContact,
                WallContact,
                BallLost
            };

            using enum MoveDirection;
            using enum TypeContact;

            ShapesManager( wxWindow* parent );
            ~ShapesManager() wxOVERRIDE {};

            void loadLevel( unsigned short level );
            void renderFrame( wxMemoryDC& dc );
            void resize( const wxSize& size );
            bool switchRun( bool bNewRound = false );
            void moveBoard( int sizeMove ) { m_boardMove = sizeMove; }

        private:
            void stop();
            void calculateTrajectory();
            void changeMoveDirection( ContactPosition contactPosition, TypeContact typeContact = WallContact );
            void offsetBoard();
            void offsetBall();
            wxRect updateBallPosition( const wxRect& boardBounds ) const;

            template <MoveDirection direction>
            void checkDirection( ContactPosition contactPosition );

        private:
            wxWindow* m_parent = nullptr;

            wxSize m_size;
            bool m_bRun = false;
            bool m_isRobot = true;
            
            int m_boardMove = 0;
            int m_ballTopLimit = 0;
            int m_ballBottomLimit = 0;

            double m_diagonal = 0;
            double m_angle = 90;

            std::vector<wxPoint> m_trajectory;
            std::vector<wxPoint>::const_iterator m_currentTrajectoryPoint;

            std::shared_ptr<Ball> m_ball;
            std::shared_ptr<Board> m_board;
            std::shared_ptr<Bricks> m_bricks;

            wxEvtHandler* m_eventHandler = nullptr;
            wxCommandEvent m_eventCurrentScoreInc;
            wxCommandEvent m_eventRoundCompleted;
            wxCommandEvent m_eventPing;
            wxCommandEvent m_eventPong;
            wxCommandEvent m_eventBallLost;

            MoveDirection m_moveDirection = DirectionTopRight;
    };
}