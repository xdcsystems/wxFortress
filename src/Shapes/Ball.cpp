// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "Common/defs.h"
#include "Common/Tools.h"
#include "Renderer/ResourceManager.h"
#include "Renderer/SpriteRenderer.h"
#include "Renderer/Texture.h"
#include "Base.h"
#include "Ball.h"

using namespace Shapes;

Ball::Ball()
{
    load( ResourceManager::LoadTexture(
        "resources/images/ball.png",
        "simple_ball" ) );

    m_radius = m_size.x / 2.0f;
    m_velocity = { BEGIN_BALL_VELOCITY, BEGIN_BALL_VELOCITY };
}

void Ball::drawTinted( const std::shared_ptr<SpriteRenderer>& renderer, const glm::vec3& color ) const
{
    if ( !m_sprite || !renderer )
        return;

    m_sprite->bind();
    renderer->drawSprite( m_position, m_size, color );
}
