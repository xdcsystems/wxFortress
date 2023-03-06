#include <GL/glew.h>

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
    glDeleteVertexArrays( 1, &m_quadVAO );
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

    glActiveTexture( GL_TEXTURE0 );
    texture->bind();

    glBindVertexArray( m_quadVAO );
    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
    glBindVertexArray( 0 );
}

void SpriteRenderer::initRenderData()
{
    // configure VAO/VBO
    unsigned int VBO;
    float vertices[] = {
        // pos      // tex
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 1.0f, 
        1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
    };

    glGenVertexArrays( 1, &m_quadVAO );
    glGenBuffers( 1, &VBO );

    glBindBuffer( GL_ARRAY_BUFFER, VBO );
    glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );

    glBindVertexArray( m_quadVAO );
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), ( void* )0 );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArray( 0 );
}