// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <algorithm>
#include <cstddef>
#include <random>

#include <glm/glm.hpp>
#include <GL/glew.h>

#include "Common/Tools.h"
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
        "resources/images/Particle.png",
        "particle" );

    m_shader = ResourceManager::LoadShader(
        "resources/shaders/Particle.vs",
        "resources/shaders/Particle.frag",
        "",
        "particle" );

    m_shader->setInteger( "sprite", 0, true );

    init();
}

void ParticleGenerator::resize( const glm::mat4& projection )
{
    m_shader->setMatrix4( "projection", projection, true );
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
            particle.Color.a = std::max( 0.0f, particle.Color.a - dt * 2.5f );
            if ( particle.Color.a <= 0.0f )
                particle.Life = 0.0f;
        }
    }
}

// render all particles
void ParticleGenerator::draw()
{
    if ( !m_shader || m_shader->ID == 0 || !m_texture || m_attrPos < 0 || m_attrColor < 0 || !m_VBOs[ 0 ] )
        return;

    m_vertexBuf.clear();
    constexpr float kScale = 10.0f;
    for ( const auto& particle : m_particles )
    {
        if ( particle.Life <= 0.0f || particle.Color.a <= 0.0f )
            continue;

        const float x0 = particle.Position.x;
        const float y0 = particle.Position.y;
        const float x1 = x0 + kScale;
        const float y1 = y0 + kScale;
        const float r = particle.Color.r;
        const float g = particle.Color.g;
        const float b = particle.Color.b;
        const float a = particle.Color.a;

        m_vertexBuf.push_back( { x0, y1, 0.f, 1.f, r, g, b, a } );
        m_vertexBuf.push_back( { x1, y0, 1.f, 0.f, r, g, b, a } );
        m_vertexBuf.push_back( { x0, y0, 0.f, 0.f, r, g, b, a } );

        m_vertexBuf.push_back( { x0, y1, 0.f, 1.f, r, g, b, a } );
        m_vertexBuf.push_back( { x1, y0, 1.f, 0.f, r, g, b, a } );
        m_vertexBuf.push_back( { x1, y1, 1.f, 1.f, r, g, b, a } );
    }

    if ( m_vertexBuf.empty() )
        return;

    static_assert( sizeof( ParticleVertex ) == 8 * sizeof( float ), "ParticleVertex must be tightly packed" );
    const auto maxBytes = static_cast<GLsizeiptr>( m_particles.size() * 6 * sizeof( ParticleVertex ) );
    const auto usedBytes = std::min(
        maxBytes,
        static_cast<GLsizeiptr>( m_vertexBuf.size() * sizeof( ParticleVertex ) ) );
    const unsigned int activeVbo = m_VBOs[ m_vboIdx ].id();
    m_vboIdx = ( m_vboIdx + 1 ) % kVboRingSize;
    if ( activeVbo == 0 || usedBytes <= 0 )
        return;

    // use additive blending to give it a 'glow' effect
    GL_CHECK( glBlendFunc( GL_SRC_ALPHA, GL_ONE ) );

    m_shader->use();
    m_texture->bind();

    // Triple-buffered VBO ring avoids per-frame glBufferData(NULL) pool stalls on WDDM / legacy ATI drivers.
    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, activeVbo ) );
    GL_CHECK( glBufferSubData( GL_ARRAY_BUFFER, 0, usedBytes, m_vertexBuf.data() ) );

    GL_CHECK( glVertexAttribPointer(
        static_cast<GLuint>( m_attrPos ),
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof( ParticleVertex ),
        nullptr ) );
    GL_CHECK( glVertexAttribPointer(
        static_cast<GLuint>( m_attrColor ),
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof( ParticleVertex ),
        reinterpret_cast<const void*>( offsetof( ParticleVertex, r ) ) ) );

    GL_CHECK( glEnableVertexAttribArray( static_cast<GLuint>( m_attrPos ) ) );
    GL_CHECK( glEnableVertexAttribArray( static_cast<GLuint>( m_attrColor ) ) );

    GL_CHECK( glDrawArrays( GL_TRIANGLES, 0, static_cast<GLsizei>( m_vertexBuf.size() ) ) );

    GL_CHECK( glDisableVertexAttribArray( static_cast<GLuint>( m_attrPos ) ) );
    GL_CHECK( glDisableVertexAttribArray( static_cast<GLuint>( m_attrColor ) ) );
    GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, 0 ) );

    // don't forget to reset to default blending mode
    GL_CHECK( glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ) );
}

void ParticleGenerator::init()
{
    if ( m_shader && m_shader->ID != 0 )
    {
        GL_CHECK( m_attrPos = glGetAttribLocation( m_shader->ID, "vertex" ) );
        GL_CHECK( m_attrColor = glGetAttribLocation( m_shader->ID, "color" ) );
    }

    m_vertexBuf.reserve( static_cast<size_t>( m_amount ) * 6 );

    // Pre-allocate triple-buffered VBO ring for stall-free per-frame streaming
    const auto maxBytes = static_cast<GLsizeiptr>( m_particles.size() * 6 * sizeof( ParticleVertex ) );
    for ( GlBuffer& vbo : m_VBOs )
    {
        vbo.create();
        if ( !vbo )
            continue;
        GL_CHECK( glBindBuffer( GL_ARRAY_BUFFER, vbo.id() ) );
        GL_CHECK( glBufferData( GL_ARRAY_BUFFER, maxBytes, nullptr, GL_STREAM_DRAW ) );
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

    static thread_local std::mt19937 rng( std::random_device {}() );
    static thread_local std::uniform_real_distribution<float> posDist( -5.0f, 5.0f );
    static thread_local std::uniform_real_distribution<float> colorDist( 0.5f, 1.5f );

    const float randomX = posDist( rng ) + position.x + offset.x;
    const float randomY = posDist( rng ) + position.y + offset.y;

    const float rColor = colorDist( rng );

    particle.Position = { randomX, randomY };
    particle.Color = glm::vec4( rColor, rColor, rColor, 1.0f );
    particle.Life = 1.0f;
    particle.Velocity = object->velocity() * 0.1f;
}