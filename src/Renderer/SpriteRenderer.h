#pragma once

#include <memory>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include "Common/GlBuffer.hpp"
#include "Common/NonCopyable.hpp"

// Forward declarations
class Shader;

class SpriteRenderer : private NonCopyable
{
    using shaderPtr = std::shared_ptr<Shader>;

    public:
        SpriteRenderer();
        ~SpriteRenderer() = default;

        void selectShader();
        void resize( const glm::mat4& projection );

        // Renders the static unit quad textured with the currently bound sprite and optional sub-rect UV.
        void drawSprite(
            const glm::vec2& position,
            glm::vec2 size = glm::vec2( 10.0f, 10.0f ),
            glm::vec3 color = glm::vec3( 1.0f ),
            const glm::vec4& uvRect = glm::vec4( 0.0f, 0.0f, 1.0f, 1.0f ) );

    private:
        void initRenderData();

        shaderPtr m_shader;
        GlBuffer  m_VBO;
        int m_attrVertex { -1 };
        int m_locRect { -1 };
        int m_locUvRect { -1 };
        int m_locSpriteColor { -1 };
};

using rendererPtr = std::shared_ptr<SpriteRenderer>;
