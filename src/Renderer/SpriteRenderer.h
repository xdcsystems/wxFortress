#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Forward declarations
class Texture2D;
class Shader;

class SpriteRenderer
{
    using texture2DPtr = std::shared_ptr<Texture2D>;
    using shaderPtr = std::shared_ptr<Shader>;

    public:
        SpriteRenderer( shaderPtr shader );
        ~SpriteRenderer();

        void selectShader();

        // Renders a defined quad textured with given sprite
        void drawSprite(
            texture2DPtr texturePtr, 
            const glm::vec2& position, 
            glm::vec2 size = glm::vec2( 10.0f, 10.0f ), 
            glm::vec3 color = glm::vec3( 1.0f ),
            float rotate = 0.0f );

    private:
        // Initializes and configures the quad's buffer and vertex attributes
        void initRenderData();

    private:
        // Render state
        shaderPtr    m_shader;
        unsigned int m_VBO = 0;
        int m_attrVertex = 0;
};

using rendererPtr = std::shared_ptr<SpriteRenderer>;