#pragma once

#include <memory>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "Common/NonCopyable.hpp"

// Forward declarations
class SpriteRenderer;
class Texture2D;

namespace Shapes
{
    class Base;
    class Explosion;

    class Explosions : private NonCopyable
    {
        using basePtr = std::shared_ptr<Base>;
        using explosionPtr = std::shared_ptr<Explosion>;
        using texture2DPtr = std::shared_ptr<Texture2D>;
        using rendererPtr = std::shared_ptr<SpriteRenderer>;

        public:
            Explosions();
            ~Explosions() = default;

            void add( const basePtr& shape );
            void clear();
            void update( float dtSec );
            void draw( const rendererPtr& renderer );
            bool empty() const;

        protected:
            static inline const glm::vec2 s_defaultSize { 128.f, 128.f };
            std::vector<explosionPtr> m_explosions;
            texture2DPtr m_explosionsSprite;
            std::vector<glm::vec4> m_uvRects;
    };
}
