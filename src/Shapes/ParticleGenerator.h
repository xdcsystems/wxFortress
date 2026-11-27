#pragma once

#include "Common/NonCopyable.hpp"

#include <array>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

// Forward declarations
class Texture2D;
class Shader;

namespace Shapes
{
    class Base;

    // Represents a single particle and its state
    struct Particle
    {
        glm::vec2 Position, Velocity;
        glm::vec4 Color;
        float     Life;

        Particle() : Position( 0.f ), Velocity( 0.f ), Color( 1.f ), Life( 0.f ) {}
    };


    // ParticleGenerator acts as a container for rendering a large number of 
    // particles by repeatedly spawning and updating particles and killing 
    // them after a given amount of time.
    class ParticleGenerator : private NonCopyable
    {
        using basePtr = std::shared_ptr<Base>;
        using texture2DPtr = std::shared_ptr<Texture2D>;
        using shaderPtr = std::shared_ptr<Shader>;

        public:
            ParticleGenerator( unsigned int amount );
            ~ParticleGenerator();

            void resize( const glm::mat4& projection );

            // update all particles
            void update( float dt, const basePtr &object, unsigned int newParticles, glm::vec2 offset = glm::vec2( 0.f, 0.f ) );

            // render all particles
            void draw();

            // clear all particles
            void clear();

        private:
            // initializes buffer and vertex attributes
            void init();

            // returns the first Particle index that's currently unused e.g. Life <= 0.0f or 0 if no particle is currently inactive
            unsigned int firstUnusedParticle();

            // respawns particle
            void respawnParticle( Particle& particle, const basePtr &object, glm::vec2 offset = glm::vec2( 0.f, 0.f ) );


            // state
            static constexpr size_t kVboRingSize = 3;
            unsigned short m_amount { 0 };
            std::array<unsigned int, kVboRingSize> m_VBOs {};
            size_t m_vboIdx { 0 };

            struct ParticleVertex
            {
                float x, y, u, v;
                float r, g, b, a;
            };

            int m_attrPos { -1 };
            int m_attrColor { -1 };

            std::vector<Particle> m_particles;
            std::vector<ParticleVertex> m_vertexBuf;

            // render state
            shaderPtr m_shader;
            texture2DPtr m_texture;
    };
}
