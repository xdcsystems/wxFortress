#pragma once

namespace Shapes
{
    enum class ContactPosition : unsigned char;

    class MoveController
    {
        public:
            enum class MoveDirection : char
            {
                Left = -1,
                None = 0,
                Right = 1,
                Top,
                Down,
                TopRight,
                RightDown,
                LeftDown,
                TopLeft
            };

            friend float operator* ( MoveDirection m, float i )
            {
                return ( char )m * i;
            }
            friend float operator+ ( MoveDirection m, float i )
            {
                return ( char )m + i;
            }
            friend float operator- ( MoveDirection m, float i )
            {
                return ( char )m - i;
            }

            enum class TypeContact : unsigned char
            {
                Null,
                BrickContact,
                PaddleContact,
                WallContact,
                BallLost
            };

         protected:
            void initDirection();
            
            inline bool isMovingHorizontal() const {
                return ( m_moveDirection == MoveDirection::LeftDown || m_moveDirection == MoveDirection::RightDown );
            };

            template <MoveDirection>
            void changeDirection( ContactPosition contactPosition );
            void changeDirection( const glm::vec2& ballCenter, const glm::vec2& paddleCenter );

            void changeMoveDirection( ContactPosition contactPosition );

            // Protected data
            double m_angle = 90;
            double m_scaleDivisionValue = 1;
            MoveDirection m_moveDirection = MoveDirection::TopRight;
    };
}
