// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <algorithm>

#include "Common/Tools.h"
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
        "resources/images/Explosion.png",
        "explosions" );

    if ( !m_explosionsSprite || m_explosionsSprite->Width <= 0 || m_explosionsSprite->Height <= 0 )
        return;

    const float uScale = s_defaultSize.x / static_cast<float>( m_explosionsSprite->Width );
    const float vScale = s_defaultSize.y / static_cast<float>( m_explosionsSprite->Height );
    const auto numOfHorizontalSprites = static_cast<unsigned short>( m_explosionsSprite->Width / s_defaultSize.x );
    const auto numOfVerticalSprites = static_cast<unsigned short>( m_explosionsSprite->Height / s_defaultSize.y );

    m_uvRects.reserve( static_cast<size_t>( numOfHorizontalSprites ) * numOfVerticalSprites );

    for ( unsigned short y = 0; y < numOfVerticalSprites; ++y )
    {
        for ( unsigned short x = 0; x < numOfHorizontalSprites; ++x )
        {
            m_uvRects.emplace_back(
                static_cast<float>( x ) * uScale,
                static_cast<float>( y ) * vScale,
                uScale,
                vScale );
        }
    }
}

void Explosions::add( const basePtr& shape )
{
    m_explosions.emplace_back( std::make_shared<Explosion>( shape, s_defaultSize ) );
}

void Explosions::clear()
{
    m_explosions.clear();
}

bool Explosions::empty() const
{
    return std::none_of( m_explosions.begin(), m_explosions.end(), []( const explosionPtr& explosion ) {
        return explosion->isActive();
    } );
}

void Explosions::update( float dtSec )
{
    if ( m_explosions.empty() )
        return;

    const std::size_t frameCount = m_uvRects.size();
    for ( const auto& explosion : m_explosions )
    {
        if ( !explosion->isActive() )
            continue;
        if ( frameCount == 0u )
            explosion->kill();
        else
            explosion->step( dtSec, frameCount );
    }

    m_explosions.erase(
        std::remove_if(
            m_explosions.begin(),
            m_explosions.end(),
            []( const explosionPtr& explosion ) { return !explosion->isActive(); } ),
        m_explosions.end() );
}

void Explosions::draw( const rendererPtr& renderer )
{
    if ( !m_explosionsSprite )
        return;

    m_explosionsSprite->bind();

    for ( const auto& explosion : m_explosions )
    {
        if ( explosion->isActive() )
        {
            const auto current = explosion->currentFrame();
            if ( current < m_uvRects.size() )
            {
                renderer->drawSprite( explosion->position(), explosion->size(), glm::vec3( 1.0f ), m_uvRects[ current ] );
            }
        }
    }
}
