// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <GL/glew.h>

#include "Common/Tools.h"
#include "ResourceManager.h"
#include "Shader.h"
#include "SpriteRenderer.h"


SpriteRenderer::SpriteRenderer()
{
    m_shader = ResourceManager::LoadShader(
        "resources/shaders/Sprite.vs",
        "resources/shaders/Sprite.fraq",
        "",
        "sprite" );

    m_shader->setInteger( "image", 0, true );
    // GLSL #version 110 forbids uniform initializers; initialize uvRect explicitly to full texture.
    m_shader->setVector4f( "uvRect", 0.0f, 0.0f, 1.0f, 1.0f );

    initRenderData();
}

void SpriteRenderer::selectShader()
{
    m_shader->use();
}

void SpriteRenderer::resize( const glm::mat4& projection )
{
    m_shader->setMatrix4( "projection", projection, true );
}

void SpriteRenderer::drawSprite( const glm::vec2& position, glm::vec2 size, glm::vec3 color, const glm::vec4& uvRect )
{
    if ( m_attrVertex < 0 || !m_VBO )
        return;

    // GPU-first: upload packed (pos.xy, size.xy) and (uOff, vOff, uScale, vScale) over a single static VBO.
    if ( m_locRect >= 0 )
        GL_CHECK( glUniform4f( m_locRect, position.x, position.y, size.x, size.y ) );
    if ( m_locUvRect >= 0 )
        GL_CHECK( glUniform4f( m_locUvRect, uvRect.x, uvRect.y, uvRect.z, uvRect.w ) );
    if ( m_locSpriteColor >= 0 )
        GL_CHECK( glUniform3f( m_locSpriteColor, color.x, color.y, color.z ) );

    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, m_VBO.id() ) );
    GL_CHECK( glVertexAttribPointer( static_cast<GLuint>( m_attrVertex ), 4, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), ( void* )nullptr ) );
    GL_CHECK( glEnableVertexAttribArray( static_cast<GLuint>( m_attrVertex ) ) );

    GL_CHECK( glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 ) );

    GL_CHECK( glDisableVertexAttribArray( static_cast<GLuint>( m_attrVertex ) ) );
    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, 0 ) );
}

void SpriteRenderer::initRenderData()
{
    if ( m_shader && m_shader->ID != 0 )
    {
        GL_CHECK( m_attrVertex = glGetAttribLocation( m_shader->ID, "vertex" ) );
        GL_CHECK( m_locRect = glGetUniformLocation( m_shader->ID, "rect" ) );
        GL_CHECK( m_locUvRect = glGetUniformLocation( m_shader->ID, "uvRect" ) );
        GL_CHECK( m_locSpriteColor = glGetUniformLocation( m_shader->ID, "spriteColor" ) );
    }

    // clang-format off
    static constexpr float kUnitQuad[] {
        // pos      // tex
        0.f, 0.f,   0.f, 1.f,
        0.f, 1.f,   0.f, 0.f,
        1.f, 0.f,   1.f, 1.f,
        1.f, 1.f,   1.f, 0.f,
    };
    // clang-format on

    m_VBO.create();
    if ( m_VBO )
    {
        GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, m_VBO.id() ) );
        GL_CHECK( glBufferData( GL_ARRAY_BUFFER, sizeof( kUnitQuad ), kUnitQuad, GL_STATIC_DRAW ) );
        GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, 0 ) );
    }
}
