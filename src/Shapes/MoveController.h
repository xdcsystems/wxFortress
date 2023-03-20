#pragma once

namespace Shapes
{
    enum class ContactPosition : unsigned char;

    class MoveController
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
                return ( char )m * i;
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

         protected:
            void initDirection();
            
            bool isMovingHorizontal() const { 
                return ( m_moveDirection == DirectionLeftDown || m_moveDirection == DirectionRightDown ); 
            };

            template <MoveDirection>
            void changeDirection( ContactPosition contactPosition );
            void changeDirection( const glm::vec2& ballCenter, const glm::vec2& paddleCenter );

        protected:
            double m_angle = 90;
            double m_scaleDivisionValue = 1;
            MoveDirection m_moveDirection = DirectionTopRight;
    };
}
