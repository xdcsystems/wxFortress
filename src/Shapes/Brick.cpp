// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "Common/xRect.hpp"
#include "Renderer/Texture.h"
#include "Renderer/SpriteRenderer.h"
#include "Base.h"
#include "Brick.h"

using namespace Shapes;

Brick::Brick( float x, float y, texture2DPtr sprite )
 : Base()
{
    if ( !sprite ) // empty place
    {
        m_size = s_defaultSize;
        m_alive = false;
    }
    else
        load( sprite );

    m_position = { x * m_size.x, y * m_size.y };
}
