#pragma once
#include <cstddef>
namespace SymoCraft {
    struct RenderStats {
        std::size_t vertices{}, upload_bytes{}, draw_calls{};
        double upload_cpu_ms{};
    };
}
