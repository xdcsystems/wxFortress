// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <map>

#include "Common/Tools.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "SpriteRenderer.h"
#include "CountDown.h"


CountDown::CountDown()
{
    m_countdownSprite = ResourceManager::LoadTexture(
        "/../resources/images/CountDown.png",
        "pause" );
}

CountDown::~CountDown()
{
    clear();
}

void CountDown::clear()
{
    for ( const auto& countdown : m_countdownMap )
    {
        SpriteRenderer::clearBuffer( countdown.second );
    }
}

void CountDown::resize( const wxSize& size )
{
    const float divisionScale = 1.f / m_countdownSprite->Width;
    m_countDownPosition = {
        ( size.x - s_countdownSize.x ) / 2,
        ( size.y - s_countdownSize.y ) / 2
    };

    clear();

    for ( unsigned char count = 1; count <= 3; ++count )
    {
        const float left = divisionScale * ( count - 1 ) * s_countdownSize.x; // left coord
        const float right = left + divisionScale * s_countdownSize.x; // right coord

        m_countdownMap[ count ] = SpriteRenderer::generateBuffer( {
            // pos      // tex
            0.f, 0.f,    left, 0.f,
            0.f, 1.f,    left, -1.f,
            1.f, 0.f,    right, 0.f,
            1.f, 1.f,    right, -1.f,
        } );
    }
}

void CountDown::show( const rendererPtr& renderer, unsigned char count ) const
{
    m_countdownSprite->bind();
    renderer->drawSprite(
        m_countdownMap.at( count ),
        m_countDownPosition,
        s_countdownSize );
}
