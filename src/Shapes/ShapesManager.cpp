// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <random>

#include <glm/glm.hpp>

#include "Common/xRect.hpp"
#include "Base.h"
#include "Ball.h"
#include "Brick.h"
#include "Bricks.h"
#include "Board.h"
#include "ParticleGenerator.h"
#include "ShapesManager.h"
#include "Renderer/SpriteRenderer.h"


using namespace Shapes;

ShapesManager::ShapesManager( wxWindow* parent )
    : ShapesRender( parent )
{
}

bool ShapesManager::switchRun( bool bNewRound )
{
    m_bRun = !m_bRun;

    if ( m_bRun && bNewRound )
    {
        std::random_device dev;
        std::mt19937 rng( dev() );
        std::uniform_int_distribution<std::mt19937::result_type> dist160( 30, 150 ); // distribution in range [30, 150]

        do
        {
            m_angle = dist160( rng );
        } while ( m_angle > 70 && m_angle < 110 );
        m_moveDirection = m_angle <= 90 ? DirectionTopRight : DirectionTopLeft;
    }

    return m_bRun;
};

void ShapesManager::renderFrame( rendererPtr spriteRenderer, double deltaTime )
{
    const auto step = round( deltaTime / 16 );

    if ( pauseWorker() == std::cv_status::timeout )
    {
        wxLogDebug("Can't pause Worker");
        return;
    }

    spriteRenderer->selectShader();

    m_bricks->draw( spriteRenderer );
    m_board->draw( spriteRenderer );

    if ( m_bRun )
    {
        m_particles->draw();
        spriteRenderer->selectShader();
    }

    m_ball->draw( spriteRenderer );

    resumeWorker();
}
