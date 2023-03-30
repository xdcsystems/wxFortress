#pragma once

#include <glm/glm.hpp>

// Forward declarations
class Texture2D;
class Shader;

class TextRenderer
{
    using texture2DPtr = std::shared_ptr<Texture2D>;
    using shaderPtr = std::shared_ptr<Shader>;

    public:
        ~TextRenderer();

        void init();
        void renderFrame();
        void print( const std::string &text, int x, int y, const glm::vec2& size );
        void cleanup();

    private:
        texture2DPtr m_texture;
        shaderPtr     m_shader;

        unsigned int m_vertexBufferID = 0;
        unsigned int m_UVBufferID = 0;
        unsigned int m_uniformID = 0;
        unsigned int m_attrVertex = 0;
        unsigned int m_attrUVs = 0;

        std::vector<glm::vec2> m_vertices;
        std::vector<glm::vec2> m_UVs;

        std::shared_ptr<Timer> m_timer;
};