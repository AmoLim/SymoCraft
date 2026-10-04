//
// Created by Amo on 2022/7/5.
//

#ifndef SYMOCRAFT_PLAYERCONTROLLER_H
#define SYMOCRAFT_PLAYERCONTROLLER_H
#include "core.h"
#include <array>

namespace SymoCraft
{
    namespace Application
    {
        extern const float kBlockPlaceDebounceTime;
        extern float block_place_debounce;
        extern int new_block_id;
    }
    namespace ECS
    {
        class Registry;
    }

    class Window;
    namespace PlayerController
    {
        inline constexpr std::array<uint16, 8> kBlockInventor = {2, 3, 4, 5, 6, 7, 10, 11};
        void DoRayCast(ECS::Registry &registry, Window &window, bool allow_input = true);
        void DisplayCurrentBlockName();
    }
}

#endif //SYMOCRAFT_PLAYERCONTROLLER_H
