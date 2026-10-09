#include <symocraft/platform/process_memory.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>

namespace SymoCraft::Platform {
    ProcessMemory SampleProcessMemory() {
        ProcessMemory result;
        PROCESS_MEMORY_COUNTERS_EX process{};
        process.cb = sizeof(process);
        if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&process), sizeof(process))) {
            result.working_set_bytes = process.WorkingSetSize;
            result.private_bytes = process.PrivateUsage;
        }
        return result;
    }
}
