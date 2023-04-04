// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <glm/glm.hpp>
#include <GL/glew.h>

#include "Common/Tools.h"
#include "Common/Rect.hpp"
#include "Renderer/ResourceManager.h"
#include "Renderer/Shader.h"
#include "Renderer/Texture.h"
#include "Base.h"
#include "ParticleGenerator.h"


using namespace Shapes;

ParticleGenerator::ParticleGenerator( unsigned int amount )
    : m_amount( amount )
    , m_particles( amount )
{
    m_texture = ResourceManager::LoadTexture(
        "/../resources/images/Particle.png",
        "particle" );

    m_shader = ResourceManager::GetShader( "particle" );

    init();
}

ParticleGenerator::~ParticleGenerator()
{
    GL_CHECK( glDeleteBuffers( 1, &m_VBO ) );
}

void ParticleGenerator::update( float dt, const basePtr &object, unsigned int newParticles, glm::vec2 offset )
{
    // add new particles 
    for ( unsigned int i = 0; i < newParticles; ++i )
    {
        auto unusedParticle = firstUnusedParticle();
        respawnParticle( m_particles[ unusedParticle ], object, offset );
    }

    // update all particles
    for ( auto& particle : m_particles )
    {
        particle.Life -= dt; // reduce life
        if ( particle.Life > .0f )
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
    GL_CHECK( glBlendFunc( GL_SRC_ALPHA, GL_ONE ) );

    m_shader->use();
    m_texture->bind();

    // set mesh attributes
    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, m_VBO ) );
    GL_CHECK( glVertexAttribPointer( m_attrPos, 4, GL_FLOAT, GL_FALSE, 5 * sizeof( float ), ( void* )nullptr ) );
    GL_CHECK( glVertexAttribPointer( m_attrIndex, 1, GL_FLOAT, GL_FALSE, 5 * sizeof( float ), ( void* )( intptr_t )( 4 * sizeof( float ) ) ) );

    GL_CHECK( glEnableVertexAttribArray( m_attrPos ) );
    GL_CHECK( glEnableVertexAttribArray( m_attrIndex ) );

    unsigned short index = 0;
    for ( const auto& particle : m_particles )
    {
        if ( particle.Life > 0.0f )
        {
            const std::string instance = { "[" + std::to_string( index ) + "]" };
            m_shader->setVector2f( ( "offset" + instance ).data(), particle.Position );
            m_shader->setVector4f( ( "color" + instance ).data(), particle.Color );

            ++index;
        }
    }

    if ( index > 0 )
    {
        GL_CHECK( glDrawArrays( GL_TRIANGLES, 0, 6 * index ) );
    }

    GL_CHECK( glDisableVertexAttribArray( m_attrPos ) );
    GL_CHECK( glDisableVertexAttribArray( m_attrIndex ) );

    // don't forget to reset to default blending mode
    GL_CHECK( glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) );
}

void ParticleGenerator::init()
{
    // set up mesh and attribute properties
    float vertices[] = {
        // pos        // tex
        0.0f, 1.0f,   0.0f, 1.0f,   0.0f,
        1.0f, 0.0f,   1.0f, 0.0f,   0.0f,
        0.0f, 0.0f,   0.0f, 0.0f,   0.0f,

        0.0f, 1.0f,   0.0f, 1.0f,   0.0f,
        1.0f, 0.0f,   1.0f, 0.0f,   0.0f,
        1.0f, 1.0f,   1.0f, 1.0f,   0.0f,
    };

    const auto sizeVAO = sizeof( vertices );

    GL_CHECK( m_attrPos = glGetAttribLocation( m_shader->ID, "vertex" ) );
    GL_CHECK( m_attrIndex = glGetAttribLocation( m_shader->ID, "index" ) );

    // create VBOs
    GL_CHECK( glGenBuffers( 1, &m_VBO ) );   // for vertex buffer

    // fill mesh buffer
    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, m_VBO ) );
    GL_CHECK( glBufferData( GL_ARRAY_BUFFER, sizeVAO * m_amount, nullptr, GL_STATIC_DRAW ) );
    
    for ( unsigned short index = 0; index < m_amount; ++index )
    {
        vertices[ 4 ] = vertices[ 9 ] = vertices[ 14 ] = vertices[ 19 ] = vertices[ 24 ] = vertices[ 29 ]= index;
        GL_CHECK( glBufferSubData( GL_ARRAY_BUFFER, sizeVAO * index, sizeVAO, vertices ) );
    }

    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, 0 ) );
}

void ParticleGenerator::clear()
{
    for ( auto& particle : m_particles )
    {
        particle.Life = 0.f;
    }
}

unsigned int ParticleGenerator::firstUnusedParticle()
{
    // first search from last used particle, this will usually return almost instantly
    const auto iter = std::find_if( m_particles.begin(), m_particles.end(), []( const auto& patrticle ) {
        return patrticle.Life <= 0;
    } );
    return iter == m_particles.end() ? 0 : std::distance( m_particles.begin(), iter );
}

void ParticleGenerator::respawnParticle( Particle& particle, const basePtr &object, glm::vec2 offset )
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