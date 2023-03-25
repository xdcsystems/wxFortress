#pragma once

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

        Particle() : Position( 0.0f ), Velocity( 0.0f ), Color( 1.0f ), Life( 0.0f ) {}
    };


    // ParticleGenerator acts as a container for rendering a large number of 
    // particles by repeatedly spawning and updating particles and killing 
    // them after a given amount of time.
    class ParticleGenerator
    {
        using basePtr = std::shared_ptr<Base>;
        using texture2DPtr = std::shared_ptr<Texture2D>;
        using shaderPtr = std::shared_ptr<Shader>;

        public:
            ParticleGenerator( unsigned int amount );
            ~ParticleGenerator();

            // update all particles
            void update( float dt, const basePtr &object, unsigned int newParticles, glm::vec2 offset = glm::vec2( 0.0f, 0.0f ) );

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
            void respawnParticle( Particle& particle, const basePtr &object, glm::vec2 offset = glm::vec2( 0.0f, 0.0f ) );


            // state
            unsigned short m_amount = 0;
            unsigned int m_VBO = 0;

            int m_attrPos = 0;
            int m_attrIndex = 0;

            std::vector<Particle> m_particles;

            // render state
            shaderPtr m_shader;
            texture2DPtr m_texture;
    };
}