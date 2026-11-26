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
#include "Texture.h"
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

    initRenderData();
}

SpriteRenderer::~SpriteRenderer()
{
    clearBuffer( m_VBO );
}

void SpriteRenderer::clearBuffer( unsigned int sourceVBO)
{
    GL_CHECK( glDeleteBuffers( 1, &sourceVBO ) );
}

void SpriteRenderer::selectShader()
{
    m_shader->use();
}

void SpriteRenderer::resize( const glm::mat4 &projection )
{
    m_shader->setMatrix4( "projection", projection, true );
}

void SpriteRenderer::drawSprite( unsigned int sourceVBO, const glm::vec2& position, glm::vec2 size, glm::vec3 color )
{
    // prepare transformations
    glm::mat4 model( 1.0f );
    model = glm::translate( model, { position, 0.0f } );  // first translate (transformations are: scale happens first, then rotation, and then final translation happens; reversed order)

    if ( m_locSize >= 0 )
        GL_CHECK( glUniform2f( m_locSize, size.x, size.y ) );
    if ( m_locModel >= 0 )
        GL_CHECK( glUniformMatrix4fv( m_locModel, 1, GL_FALSE, glm::value_ptr( model ) ) );
    if ( m_locSpriteColor >= 0 )
        GL_CHECK( glUniform3f( m_locSpriteColor, color.x, color.y, color.z ) );

    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, sourceVBO ) );
    GL_CHECK( glVertexAttribPointer( m_attrVertex, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), ( void* )nullptr ) );
    GL_CHECK( glEnableVertexAttribArray( m_attrVertex ) );

    GL_CHECK( glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 ) );

    GL_CHECK( glDisableVertexAttribArray( m_attrVertex ) );
}

void SpriteRenderer::drawSprite( const glm::vec2& position, glm::vec2 size, glm::vec3 color )
{
    drawSprite( m_VBO, position, size, color );
}

void SpriteRenderer::initRenderData()
{
    // configure VBO
    GL_CHECK( m_attrVertex = glGetAttribLocation( m_shader->ID, "vertex" ) );
    GL_CHECK( m_locSize = glGetUniformLocation( m_shader->ID, "size" ) );
    GL_CHECK( m_locModel = glGetUniformLocation( m_shader->ID, "model" ) );
    GL_CHECK( m_locSpriteColor = glGetUniformLocation( m_shader->ID, "spriteColor" ) );

    m_VBO = generateBuffer( {
        // pos      // tex
        0.f, 0.f,   0.f, 1.f,
        0.f, 1.f,   0.f, 0.f,
        1.f, 0.f,   1.f, 1.f,
        1.f, 1.f,   1.f, 0.f,
     } );
}

unsigned int SpriteRenderer::generateBuffer( const std::vector<float>& vertices )
{
    unsigned int targetVBO = 0;

    GL_CHECK( glGenBuffers( 1, &targetVBO ) );
    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, targetVBO ) );
    GL_CHECK( glBufferData( GL_ARRAY_BUFFER, sizeof( vertices[ 0 ] ) * vertices.size(), vertices.data(), GL_STATIC_DRAW ) );

    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, 0 ) );

    return targetVBO;
}