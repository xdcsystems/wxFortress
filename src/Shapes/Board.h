#pragma once

namespace Shapes
{
    class Board : public Base
    {
        public:
            Board();
            xRect admissibleBounds( const xRect& bounds ) const;
    };
}