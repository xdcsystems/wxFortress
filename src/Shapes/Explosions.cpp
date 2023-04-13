// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "Common/Tools.h"
#include "Common/Rect.hpp"
#include "Renderer/Texture.h"
#include "Renderer/ResourceManager.h"
#include "Renderer/SpriteRenderer.h"
#include "Base.h"
#include "Explosion.h"
#include "Explosions.h"

using namespace Shapes;

Explosions::Explosions()
{
    m_explosionsSprite = ResourceManager::LoadTexture(
        "/../resources/images/Explosion.png",
        "explosions" );

    const glm::vec2 divisionScale = { 
        1.f / m_explosionsSprite->Width, 
        1.f / m_explosionsSprite->Height 
    };
    const auto numOfHorizontalSprites = m_explosionsSprite->Width / s_defaultSize.x;
    const auto numOfVerticalSprites = m_explosionsSprite->Height / s_defaultSize.y;

    m_VBO.reserve( numOfHorizontalSprites * numOfVerticalSprites );

    for ( unsigned short y = 0; y < numOfVerticalSprites; ++y )
    {
        for ( unsigned short x = 0; x < numOfHorizontalSprites; ++x )
        {
            const float left = divisionScale.x * x * s_defaultSize.x; // left coord
            const float right = left + divisionScale.x * s_defaultSize.x; // right coord
            const float top = divisionScale.y * y * s_defaultSize.y; // top coord
            const float bottom = top + divisionScale.y * s_defaultSize.y; // bottom coord

            m_VBO.emplace_back( SpriteRenderer::generateBuffer( {
                // pos      // tex
                0.f, 0.f,    left,   bottom,
                0.f, 1.f,    left,   top,
                1.f, 0.f,    right, bottom,
                1.f, 1.f,    right, top,
            } ) );
        }
    }
}

Explosions::~Explosions()
{
    for ( const auto& vbo : m_VBO )
    {
        SpriteRenderer::clearBuffer( vbo );
    }
}

void Explosions::add( const basePtr &shape )
{
    m_explosions.emplace_back( std::make_shared<Explosion>( shape, s_defaultSize ) );
}

void Explosions::clear()
{
    m_explosions.clear();
}

bool Explosions::empty() const
{
    return std::find_if( m_explosions.begin(), m_explosions.end(), []( const explosionPtr& explosion ) {
        return explosion->isActive(); } ) == m_explosions.end();
};


void Explosions::draw( const rendererPtr &renderer )
{
    m_explosionsSprite->bind();

    for ( const auto &expolosion : m_explosions )
    {
        if ( expolosion->isActive() )
        {
            const auto current = expolosion->current();
            if ( current < m_VBO.size() )
            {
                renderer->drawSprite( m_VBO[ current ], expolosion->position(), expolosion->size() );
                continue;
            }
            expolosion->kill();
        }
    }
}

