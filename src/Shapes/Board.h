#pragma once

namespace Shapes
{
    class Board : public Base
    {
        public:
            Board();
            Rect admissibleBounds( const Rect& bounds ) const;
    };
}