// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif


#include "Common/defs.h"
#include "Common/Rect.hpp"
#include "Renderer/ResourceManager.h"
#include "Base.h"
#include "Board.h"

using namespace Shapes;

Board::Board()
{
    load( ResourceManager::LoadTexture(
        "resources/images/SimpleBoard.png",
        "simple_board" ) );

    m_velocity = { BEGIN_BOARD_VELOCITY, BEGIN_BOARD_VELOCITY };
}

Rect Board::admissibleBounds( const Rect& bounds ) const
{
    auto boundsRect = Base::bounds();
    boundsRect.inset( bounds.m_width / 2, .0f );
    
    return boundsRect;
}
