// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "Common/Tools.h"
#include "Renderer/Texture.h"
#include "Renderer/SpriteRenderer.h"
#include "Base.h"

using namespace Shapes;

void Base::load( texture2DPtr sprite )
{
    m_sprite = sprite;
    m_size = { m_sprite->Width, m_sprite->Height };
}

void Base::moveTo( double x, double y )
{
    m_position.x = x;
    m_position.y = y;
}

void Base::moveTo( const wxPoint2DDouble& position )
{
    moveTo( position.m_x, position.m_y );
}

void Base::draw( rendererPtr renderer ) const
{
    if ( !m_sprite )
        return;

    renderer->drawSprite( m_sprite, m_position, m_size );
}
