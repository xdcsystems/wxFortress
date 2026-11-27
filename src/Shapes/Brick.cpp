// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "Base.h"
#include "Brick.h"

using namespace Shapes;

Brick::Brick( const glm::vec2& position, BrickType type )
    : m_type( type )
{
    m_size = SizeOf( type );
    m_position = position;
    m_alive = !( type == BrickType::NONE || type == BrickType::HALF );
}
