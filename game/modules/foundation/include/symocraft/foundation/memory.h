#pragma once
#include <cstddef>
#include <cstdint>

// Compatibility entry points; allocator bookkeeping stays private to foundation.
namespace AmoBase {
    void* _AmoMemory_Allocate(const char* filename, int line, std::size_t bytes);
    void* _AmoMemory_ReAlloc(const char* filename, int line, void* memory, std::size_t bytes);
    void _AmoMemory_Free(const char* filename, int line, void* memory);
    void AmoMemory_Init(bool detect_leaks, std::uint16_t buffer_unit = 5);
    void AmoMemory_MemoryLeaksDetected();
    int AmoMemory_CompareMem(void* left, void* right, std::size_t bytes);
    void AmoMemory_ZeroMem(void* memory, std::size_t bytes);
    void AmoMemory_CopyMem(void* destination, void* source, std::size_t bytes);
}
#define AmoMemory_Allocate(bytes) AmoBase::_AmoMemory_Allocate(__FILE__, __LINE__, bytes)
#define AmoMemory_ReAlloc(memory, bytes) AmoBase::_AmoMemory_ReAlloc(__FILE__, __LINE__, memory, bytes)
#define AmoMemory_Free(memory) AmoBase::_AmoMemory_Free(__FILE__, __LINE__, memory)
