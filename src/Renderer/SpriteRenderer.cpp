// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <GL/glew.h>

#include "Common/Tools.h"
#include "Texture.h"
#include "Shader.h"
#include "SpriteRenderer.h"

SpriteRenderer::SpriteRenderer( shaderPtr shader )
{
    m_shader = shader;
    initRenderData();
}

SpriteRenderer::~SpriteRenderer()
{
    GL_CHECK( glDeleteVertexArrays( 1, &m_VBO ) );
}

void SpriteRenderer::drawSprite( texture2DPtr texture, glm::vec2 position, glm::vec2 size, glm::vec3 color, float rotate )
{
    // prepare transformations
    m_shader->use();

    glm::mat4 model = glm::mat4( 1.0f );
    model = glm::translate( model, glm::vec3( position, 0.0f ) );  // first translate (transformations are: scale happens first, then rotation, and then final translation happens; reversed order)

    model = glm::translate( model, glm::vec3( 0.5f * size.x, 0.5f * size.y, 0.0f ) ); // move origin of rotation to center of quad
    model = glm::rotate( model, glm::radians( rotate ), glm::vec3( 0.0f, 0.0f, 1.0f ) ); // then rotate
    model = glm::translate( model, glm::vec3( -0.5f * size.x, -0.5f * size.y, 0.0f ) ); // move origin back

    model = glm::scale( model, glm::vec3( size, 1.0f ) ); // last scale

    m_shader->setMatrix4( "model", model );

    // render textured quad
    m_shader->setVector3f( "spriteColor", color );

    GL_CHECK( glActiveTexture( GL_TEXTURE0 ) );

    texture->bind();

    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, m_VBO ) );

    //glBindVertexArray( m_quadVAO );
    GL_CHECK( glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 ) );
    //glBindVertexArray( 0 );
}

void SpriteRenderer::initRenderData()
{
    // configure VBO
    float vertices[] = {
        // pos      // tex
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
    };

    // glGenVertexArrays( 1, &m_quadVAO );

    GL_CHECK( glGenBuffers( 1, &m_VBO ) );
    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, m_VBO ) );
    GL_CHECK( glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW ) );

    //glBindVertexArray( m_quadVAO );

    GL_CHECK( glEnableVertexAttribArray( 0 ) );
    GL_CHECK( glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), ( void* )0 ) );
    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, 0 ) );

    //glBindVertexArray( 0 );
}