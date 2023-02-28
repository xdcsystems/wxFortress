// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "Base.h"
#include "Board.h"

using namespace Shapes;

Board::Board()
 : Base()
{
    load( wxT( "/../resources/images/simple_board.png" ) );
}

wxRect Board::admissibleBounds( const wxRect& bounds ) const
{
    return Base::bounds().Inflate( bounds.GetWidth() >> 1, 0 );
}
