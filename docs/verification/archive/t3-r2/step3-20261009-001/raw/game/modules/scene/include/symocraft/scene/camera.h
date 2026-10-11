#pragma once
#include "symocraft/foundation/math.h"

namespace SymoCraft {
    struct CameraView {
        glm::mat4 projection{1.0f};
        glm::mat4 view{1.0f};
    };
}
