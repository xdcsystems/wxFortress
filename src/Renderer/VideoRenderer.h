#pragma once

#include <cstdint>
#include <memory>

#include <glm/vec2.hpp>

#include "Common/NonCopyable.hpp"

// Forward declarations
class Shader;
class Texture2D;
class SpriteRenderer;

class VideoRenderer : private NonCopyable
{
        using shaderPtr = std::shared_ptr<Shader>;
        using texture2DPtr = std::shared_ptr<Texture2D>;

    public:
        VideoRenderer();
        ~VideoRenderer();

        bool ok();
        void setViewport( int x, int y, int width, int height );

        void draw( int width, int height, uint8_t** data, const int* linesize );
        void drawNV12( int width, int height, uint8_t** data, const int* linesize );

        int viewWidth() const { return m_viewWidth; }
        int viewHeight() const { return m_viewHeight; }

    private:
        enum class FrameFormat
        {
            None,
            YUV420P,
            NV12
        };

        void init();
        bool updateFrameGeometry( int width, int height, FrameFormat format );
        void uploadPlane( int unit, unsigned int tex, int width, int height, int rowLengthTexels, unsigned int format, const uint8_t* data, bool alloc );
        void drawQuad( const shaderPtr& shader, int attrVertex, int attrUVs, int locScale );

        unsigned int m_VBO { 0 };
        unsigned int m_EBO { 0 };
        unsigned int m_texs[ 3 ] {};

        int m_attrVertex { -1 };
        int m_attrUVs { -1 };
        int m_locScale { -1 };

        int m_attrVertexNV12 { -1 };
        int m_attrUVsNV12 { -1 };
        int m_locScaleNV12 { -1 };

        glm::vec2 m_scale { 1.0f, 1.0f };
        int m_viewWidth { -1 };
        int m_viewHeight { -1 };
        int m_texWidth { -1 };
        int m_texHeight { -1 };
        FrameFormat m_lastFormat { FrameFormat::None };

        shaderPtr m_shader;
        shaderPtr m_shaderNV12;
};
