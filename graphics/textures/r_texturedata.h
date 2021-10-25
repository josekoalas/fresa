//project verse, 2017-2021
//by jose pazos perez
//all rights reserved uwu

#pragma once

#include "dtypes.h"
#include <glm/glm.hpp>

namespace Verse::Graphics
{
    struct TextureData {
        std::vector<float> vertices;
        glm::mat4 model;
        
        int layer;
        ui16 w, h;
        
    #if defined USE_OPENGL
        ui32 gl_id;
    #elif defined USE_VULKAN
            
    #endif
    };
}
