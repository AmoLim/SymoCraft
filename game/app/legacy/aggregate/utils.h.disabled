//
// Created by Administrator on 2022/6/17.
//

#ifndef SYMOCRAFT_UTILS_H
#define SYMOCRAFT_UTILS_H
#include "core.h"
#include "world/block.h"
namespace SymoCraft{
    namespace DebugStats
    {
        extern uint32 numDrawCalls;
        extern float lastFrameTime;
        extern glm::vec3 playerPos;
        extern glm::vec3 playerOrientation;
        extern std::atomic<float> totalChunkRamUsed;
        extern float totalChunkRamAvailable;
        extern SymoCraft::Block blockLookingAt;
        extern SymoCraft::Block airBlockLookingAt;
    }

    inline float Remap(float x, float in_min, float in_max, float out_min, float out_max)
    {
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

    // Returns Value normalized to the given range.  (e.g. 20 normalized to the range 10->50 would result in 0.25)
    template <typename T, typename U>
    T NormalizeToRange(T input, U output_min, U output_max)
    {
        if (output_min == output_max)
        {
            if (input < output_min)
            {
                return 0;
            }
            else
            {
                return 1;
            }
        }

        if (output_min > output_max)
        {
            std::swap(output_min, output_max);
        }
        return (input - output_min) / (output_max - output_min);
    }

}

#endif //SYMOCRAFT_UTILS_H
