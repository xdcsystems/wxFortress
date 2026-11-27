#pragma once

#include <memory>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include "Common/NonCopyable.hpp"

// Forward declarations
class Texture2D;
class Shader;

class SpriteRenderer : private NonCopyable
{
    using texture2DPtr = std::shared_ptr<Texture2D>;
    using shaderPtr = std::shared_ptr<Shader>;

    public:
        SpriteRenderer();
        ~SpriteRenderer();

        void selectShader();
        void resize( const glm::mat4& projection );

        // Renders a defined quad textured with given sprite
        void drawSprite(
            const glm::vec2& position,
            glm::vec2 size = glm::vec2( 10.0f, 10.0f ),
            glm::vec3 color = glm::vec3( 1.0f ) );

        void drawSprite(
            unsigned int sourceVBO,
            const glm::vec2& position,
            glm::vec2 size = glm::vec2( 10.0f, 10.0f ),
            glm::vec3 color = glm::vec3( 1.0f ) );

        static unsigned int generateBuffer( const std::vector<float>& vertices );
        static void clearBuffer( unsigned int VBO );

    private:
        // Initializes and configures the quad's buffer and vertex attributes
        void initRenderData();

        // Render state
        shaderPtr    m_shader;
        unsigned int m_VBO { 0 };
        int m_attrVertex { -1 };
        int m_locRect { -1 };
        int m_locSpriteColor { -1 };
};

using rendererPtr = std::shared_ptr<SpriteRenderer>;