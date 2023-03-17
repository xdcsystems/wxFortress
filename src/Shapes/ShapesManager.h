#pragma once

#include "ShapesRender.h"

// Forward declarations
class SpriteRenderer;

namespace Shapes
{
    class ShapesManager : public ShapesRender
    {
        using rendererPtr = std::shared_ptr<SpriteRenderer>;

        public:
            ShapesManager( wxWindow* parent );

            bool switchRun( bool bNewRound = false );
            void renderFrame( rendererPtr spriteRenderer, double deltaTime );
    };
}