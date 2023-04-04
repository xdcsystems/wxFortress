#pragma once

// Forward declarations
class Shader;

class VideoRenderer
{
    using shaderPtr = std::shared_ptr<Shader>;

    public:
        VideoRenderer();
        bool ok();
        void setViewport( int x, int y, int width, int height );
        void draw( int width, int height, uint8_t **data, int *linesize );

    private:
        unsigned int m_VBO, m_VAO, m_EBO;
        unsigned int m_texs[3];

        float m_ratio { 1 };
        int m_viewWidth;
        int m_viewHeight;
        int m_texWidth { -1 };
        int m_texHeight { -1 };

        shaderPtr m_shader;
};
