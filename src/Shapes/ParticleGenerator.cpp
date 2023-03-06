// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "Renderer/Shader.h"
#include "Renderer/Texture.h"
#include "Base.h"
#include "ParticleGenerator.h"

using namespace Shapes;

ParticleGenerator::ParticleGenerator( shaderPtr shader, texture2DPtr texture, unsigned int amount )
    : m_shader( shader ), m_texture( texture ), m_amount( amount )
{
    init();
}

void ParticleGenerator::update( float dt, basePtr object, unsigned int newParticles, glm::vec2 offset )
{
    // add new particles 
    for ( unsigned int i = 0; i < newParticles; ++i )
    {
        int unusedParticle = firstUnusedParticle();
        respawnParticle( m_particles[ unusedParticle ], object, offset );
    }

    // update all particles
    for ( auto& particle : m_particles )
    {
        particle.Life -= dt; // reduce life
        if ( particle.Life > 0.0f )
        {	// particle is alive, thus update
            particle.Position -= particle.Velocity * dt;
            particle.Color.a -= dt * 2.5f;
        }
    }
}

// render all particles
void ParticleGenerator::draw()
{
    // use additive blending to give it a 'glow' effect
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );
    
    m_shader->use();
    m_texture->bind();

    for ( unsigned int i = 0; i < m_particles.size(); ++i  )// const auto &particle : m_particles )
    {
        if ( m_particles[ i ].Life > 0.0f )
        {
            m_shader->setVector2f( ("offset[" + std::to_string( i ) + "]").data(), m_particles[ i ].Position );
            m_shader->setVector4f( ("color[" + std::to_string( i ) + "]").data(), m_particles[ i ].Color );
        }
    }
    glBindVertexArray( m_VAO );
    glDrawArraysInstanced( GL_TRIANGLE_STRIP, 0, 4, m_particles.size() );
    glBindVertexArray( 0 );

    // don't forget to reset to default blending mode
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
}

void ParticleGenerator::init()
{
    m_particles.clear();
    m_particles.reserve( m_amount );
    
    // set up mesh and attribute properties
    unsigned int VBO;
    float particle_quad[] = {
        // pos      // tex
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
    };

    glGenVertexArrays( 1, &m_VAO );
    glGenBuffers( 1, &VBO );
    glBindVertexArray( m_VAO );

    // fill mesh buffer
    glBindBuffer( GL_ARRAY_BUFFER, VBO );
    glBufferData( GL_ARRAY_BUFFER, sizeof( particle_quad ), particle_quad, GL_STATIC_DRAW );
    
    // set mesh attributes
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof( float ), ( void* )0 );
    glBindVertexArray( 0 );

    // create amount default particle instances
    for ( unsigned int i = 0; i < m_amount; ++i )
        m_particles.push_back( {} );
}

unsigned int ParticleGenerator::firstUnusedParticle()
{
    // first search from last used particle, this will usually return almost instantly
    const auto iter = std::find_if( m_particles.begin(), m_particles.end(), []( const auto& patrticle ) {
        return patrticle.Life <= 0;
    } );
    return iter == m_particles.end() ? 0 : std::distance( m_particles.begin(), iter );
}

void ParticleGenerator::respawnParticle( Particle& particle, basePtr object, glm::vec2 offset )
{
    const auto& position = object->position();
    
    const float randomX = ( ( rand() % 100 ) - 50 ) / 10.0f + position.x + offset.x;
    const float randomY = ( ( rand() % 100 ) - 50 ) / 10.0f + position.y + offset.y;

    const float rColor = 0.5f + ( ( rand() % 100 ) / 100.0f );
    
    particle.Position = { randomX, randomY };
    particle.Color = glm::vec4( rColor, rColor, rColor, 1.0f );
    particle.Life = 1.0f;
    particle.Velocity = object->velocity() * 0.1f;
}