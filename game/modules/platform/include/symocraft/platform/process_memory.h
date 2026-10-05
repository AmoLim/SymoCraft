#pragma once
#include <cstdint>
#include <optional>

namespace SymoCraft::Platform {
    struct ProcessMemory {
        std::optional<std::uint64_t> working_set_bytes, private_bytes;
    };
    ProcessMemory SampleProcessMemory();
}
