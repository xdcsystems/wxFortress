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
            template <MoveDirection direction>
            void checkDirection( ContactPosition contactPosition );

        protected:
            double m_angle = 90;
            MoveDirection m_moveDirection = DirectionTopRight;
    };
}
