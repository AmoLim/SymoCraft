#include "AmoBase.h"
#include <vector>
#include <mutex>
#include <stdio.h>

#include <stdarg.h>
#include <stdlib.h>
#include <chrono>
#include <array>
#include <cstring>

#include <windows.h>
#include <crtdbg.h>


// ------------------------------------
// Memory Base Implementation
// ------------------------------------
struct DebugMemoryAllocation
{
    // use these two components to track allocations
    const char* file_allocator;     // the file name where the allocator in
    int file_allocator_line;        // the line where the allocator in

    int references;                 // references: <=0 the allocation is not in use;
                                    //             >0 the allocation is in use

    size_t memory_size;             // allocation's memory size
    void* memory;                   // allocation's memory pointer(store address)

    // allocation equality operator overload for std::find
    bool operator ==(const DebugMemoryAllocation& other) const
    {
        return other.memory == this->memory;
    }
};

static std::mutex memory_mtx;   // memory mutex
static std::vector<DebugMemoryAllocation> allocations;  // allocations vectors
static bool track_memory_allocations = false;       // memory tracking mode
static const std::array<uint8_t, 8> special_memory_flags = { (uint8_t)'A', (uint8_t)'M', (uint8_t)'O',
                                                             (uint8_t)'F', (uint8_t)'L', (uint8_t)'A',
                                                             (uint8_t)'G', (uint8_t)'S' };
static uint16_t buffer_unit = 5;    // bytes of the buffer filler

void AmoBase::AmoMemory_Init(bool detect_memory_leaks, uint16_t in_buffer_unit)
{
    track_memory_allocations = detect_memory_leaks;
    buffer_unit = in_buffer_unit;
}

void* AmoBase::_AmoMemory_Allocate(const char *filename, int line, size_t num_bytes)
{
    if (track_memory_allocations)
    {
        // Memory tracking mode
        // add sentinel values to the beginning and end of the block of memory
        // to ensure it doesn't have any errors on free
        num_bytes += (buffer_unit * 2) * sizeof(uint8_t);

        // allocate 10 extra bytes, 5 before the block and 5 after.
        // use these to detect Buffer overruns or under-runs
        void* memory = std::malloc(num_bytes);
        if (memory)
        {
            // cursor of memory
            uint8_t* memory_bytes = (uint8_t*)memory;
            for (int i = 0; i < buffer_unit; i++)
                memory_bytes[i] = special_memory_flags[i % special_memory_flags.size()];
            // add sentinel values to the front

            memory_bytes = ((uint8_t*)memory) + num_bytes - buffer_unit;
            for (int i = 0; i < buffer_unit; i++)
                memory_bytes[i] = special_memory_flags[i % special_memory_flags.size()];
            // add sentinel values to the back
        }

        std::lock_guard<std::mutex> lock(memory_mtx);   // lock memory mutex for multiple thread mode
        // in debug build, track all memory allocations to see
        // if we free them all as well

        // find the exact allocation in allocation-vector
        auto iterator = std::find(allocations.begin(), allocations.end(),
                                  DebugMemoryAllocation{ filename, line, 0, num_bytes, memory});


        if (iterator == allocations.end())
        {
            // if not found, create a new allocation at the end of allocation-vector
            allocations.emplace_back( DebugMemoryAllocation{ filename, line, 1, num_bytes, memory });
        }
        else
        {
            if (iterator->references <= 0)
            {
                // the memory is not in use, allocate it
                iterator->references++;         // set memory status to be in use
                iterator->file_allocator = filename;
                iterator->file_allocator_line = line;
                iterator->memory_size = num_bytes;
                iterator->memory = memory;
            }
            else
            {
                // memory is in use, throw error info
                AmoLogger_Error("Tried to allocate memory that has already been allocated..."
                                "This should never be hit. If it is, we have a problem.");
            }
        }

        return (void*)((uint8_t*)memory + buffer_unit);
    }

    // if not in tracking memory mode, just return malloc
    return std::malloc(num_bytes);
}

void* AmoBase::_AmoMemory_ReAlloc(const char *filename, int line, void *old_memory, size_t num_bytes)
{
    if (track_memory_allocations)
    {
        // Memory tracking mode
        // add sentinel values to the beginning and end of the block of memory
        // to ensure it doesn't have any errors on free
        old_memory = (void*)((uint8_t*)old_memory - buffer_unit);

        // find the old memory allocation
        auto old_memory_iter = std::find(allocations.begin(), allocations.end(),
                                         DebugMemoryAllocation{ filename, line, 0, num_bytes, old_memory});

        if (old_memory_iter == allocations.end())
        {
            AmoLogger_Error("Tried to reallocate a block of memory "
                            "that was not allocated by this library.");
            return nullptr;
        }

        // copy the sentinel values at the end to sentinel_copy
        void* sentinel_copy = std::malloc(sizeof(uint8_t) * buffer_unit);
        std::memcpy( sentinel_copy
                     , (void*)((uint8_t*)old_memory + old_memory_iter->memory_size - buffer_unit)
                     , sizeof(uint8_t) * buffer_unit);

        num_bytes += buffer_unit * 2 * sizeof(uint8_t);
        void* new_memory = std::realloc( old_memory, num_bytes);    // reallocate the allocation's memory
        if (new_memory)
        {
            // copy the old sentinel values that were at the end
            // to the end of the new memory block
            old_memory_iter->memory_size = num_bytes;
            std::memcpy((uint8_t*)new_memory + num_bytes - buffer_unit
                        , sentinel_copy
                        , buffer_unit * sizeof(uint8_t));

            std::free(sentinel_copy);
        }

        std::lock_guard<std::mutex> lock(memory_mtx);   // lock memory mutex for multiple thread mode
        // In debug mode, track all memory allocations to see if we free them as well
        auto new_memory_iter = std::find( allocations.begin(), allocations.end(),
                                          DebugMemoryAllocation{ filename, line, 0, num_bytes, new_memory});
        if (new_memory_iter != old_memory_iter)
        {
            // reallocate could not expand the current pointer, so it
            // should allocate a new memory block
            if ( old_memory_iter == allocations.end())
            {
                // old memory can not be found, invalid memory reallocate
                AmoLogger_Error("Tried to reallocate invalid memory in"
                                "'%s' line: %d.", filename, line);
            }
            else
            {
                old_memory_iter->references--;
                // reset the status of old memory to be not in use
            }

            if (new_memory_iter == allocations.end())
            {
                allocations.emplace_back(DebugMemoryAllocation{ filename, line, 1, num_bytes, new_memory});
                // emplace an allocation at the end of the allocations-vector for new_memory
            }
            else
            {
                if (new_memory_iter->references <= 0)
                {
                    new_memory_iter->references++;      // set memory status to be in use
                    new_memory_iter->file_allocator = filename;
                    new_memory_iter->file_allocator_line = line;
                    new_memory_iter->memory_size = num_bytes;
                    new_memory_iter->memory = new_memory;
                }
                else
                {
                    // memory is in use, throw error info
                    AmoLogger_Error("Tried to allocate memory that has already been allocated..."
                                    "This should never be hit. If it is, we have a problem.");
                }
            }
        }
        // If reallocate expanded the memory in-place,
        // then we don't need to do anything
        // because no "new" memory locations were allocated
        return (void*)((uint8_t*)new_memory + buffer_unit);

    }
    // if not in tracking mode, return ReAlloc
    return std::realloc(old_memory, num_bytes);
}

void AmoBase::_AmoMemory_Free(const char *filename, int line, void *memory)
{
    if (track_memory_allocations)
    {
        // Tracking Memory Mode
        memory = (void*)((uint8_t*)memory - buffer_unit);
        std::lock_guard<std::mutex> lock(memory_mtx);   // lock memory mutex for multiple thread mode

        auto iterator = std::find(allocations.begin(), allocations.end()
                                  , DebugMemoryAllocation{ filename, line, 0, 0, memory});
        if (iterator == allocations.end())
        {
            // try to free invalid memory, throw error info
            AmoLogger_Error("Tried to free invalid memory "
                            "that was never allocated at '%s' line: %d", filename, line);
        }
        else if (iterator->references <= 0)
        {
            // the memory is not in use, invalid operation
            AmoLogger_Error("Tried to free memory that has already been freed.");
            AmoLogger_Error("Code that attempted to free: '%s' line: %d"
                            , filename, line);
            AmoLogger_Error("Code that allocated the memory block: '%s' line: %d"
                            , iterator->file_allocator, iterator->file_allocator_line);
        }
        else
        {
            iterator->references--;     // set memory status to be not in use(free)

            if (iterator->references == 0)
            {
                // Check to see if our special flags were changed.
                // If they were, we have heap corruption!

                // memory cursor point to the end of memory
                uint8_t *memory_bytes = (uint8_t*)memory;
                for (int i = 0; i < buffer_unit; i++)
                {
                    if (memory_bytes[i] != special_memory_flags[i % special_memory_flags.size()])
                    {
                        AmoLogger_Warning(" Heap corruption detected."
                                          " Buffer under-run in memory allocated from: '%s' line: %d"
                                          , iterator->file_allocator, iterator->file_allocator_line);
                        break;
                    }
                }

                // memory cursor point to the very beginning of the memory
                memory_bytes = (uint8_t*)memory + iterator->memory_size - buffer_unit;
                for (int i = 0; i < buffer_unit; i++)
                {
                    if (memory_bytes[i] != special_memory_flags[i % special_memory_flags.size()])
                    {
                        AmoLogger_Warning(" Heap corruption detected."
                                          " Buffer over-run in memory allocated from: '%s' line: %d"
                        , iterator->file_allocator, iterator->file_allocator_line);
                        break;
                    }
                }
            }
        }
    }

    // When debug is turned off we literally just free the memory,
    // so it will throw a segfault if a faulty release build was published
    std::free(memory);
}

void AmoBase::AmoMemory_MemoryLeaksDetected()
{
    std::lock_guard<std::mutex> lock(memory_mtx);
    for (const auto& alloc : allocations)
    {
        if (alloc.references > 0)
        {
            AmoLogger_Warning("Memory leak detected. Leaked '%zu' bytes allocated from: '%s' line: %d"
                              , alloc.memory_size - 10, alloc.file_allocator, alloc.file_allocator_line);
        }
    }
}

int AmoBase::AmoMemory_CompareMem(void *mem1, void *mem2, size_t num_bytes)
{
    return std::memcmp( mem1, mem2, num_bytes);
}

void AmoBase::AmoMemory_ZeroMem(void *memory, size_t num_bytes)
{
    std::memset(memory, 0, num_bytes);
}

void AmoBase::AmoMemory_CopyMem(void *destin, void *source, size_t num_bytes)
{
    std::memcpy(destin, source, num_bytes);
}


// ------------------------------------
// Logging Implementation
// ------------------------------------
static std::mutex log_mtx;      // logger mutex

static AmoBase::LogLevel log_level = AmoBase::LogLevel::All;

void AmoBase::AmoLogger_SetLevel(LogLevel level)
{
    log_level = level;
}

AmoBase::LogLevel AmoBase::AmoLogger_GetLevel()
{
    return log_level;
}

void AmoBase::_AmoLogger_Log(const char *filename, int line, const char *var_format, ...)
{
    if (AmoLogger_GetLevel() <= LogLevel::Log)
    {
        std::lock_guard<std::mutex> lock(log_mtx);  // lock logging mutex for multiple thread mode

        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_BLUE | FOREGROUND_GREEN);
        printf("%s (line %d) Log: \n", filename, line);     // print basic info
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0F);

        // get system time
        std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        char buf[20] = { 0 };   // time store buffer
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %I:%M:%S", std::localtime(&now));
        printf("[%s]: ", buf);

        // variable argument print
        va_list args;
        va_start(args, var_format);
        vprintf(var_format, args);
        va_end(args);

        printf("\n");
    }
}

void AmoBase::_AmoLogger_Info(const char *filename, int line, const char *var_format, ...)
{
    if (AmoLogger_GetLevel() <= LogLevel::Info)
    {
        std::lock_guard<std::mutex> lock(log_mtx);  // lock logging mutex for multiple thread mode

        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN); // Green Text
        printf("%s (line %d) Info: \n", filename, line);
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0F);

        std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        char buf[20] = { 0 };
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %I:%M:%S", std::localtime(&now));
        printf("[%s]: ", buf);

        va_list args;
        va_start(args, var_format);
        vprintf(var_format, args);
        va_end(args);

        printf("\n");
    }
}

void AmoBase::_AmoLogger_Warning(const char *filename, int line, const char *var_format, ...)
{
    if (AmoLogger_GetLevel() <= LogLevel::Warning)
    {
        std::lock_guard<std::mutex> lock(log_mtx);  // lock logging mutex for multiple thread mode

        // Yellow Text
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_RED);
        printf("%s (line %d) Warning: \n", filename, line);
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0F);

        std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        char buf[20] = { 0 };
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %I:%M:%S", std::localtime(&now));
        printf("[%s]: ", buf);

        va_list args;
        va_start(args, var_format);
        vprintf(var_format, args);
        va_end(args);

        printf("\n");
    }
}

void AmoBase::_AmoLogger_Error(const char *filename, int line, const char *var_format, ...)
{
    if (AmoLogger_GetLevel() <= LogLevel::Error)
    {
        std::lock_guard<std::mutex> lock(log_mtx);  // lock logging mutex for multiple thread mode

        // Red Text
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
        printf("%s (line %d) Error: \n", filename, line);
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0F);

        std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        char buf[20] = { 0 };
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %I:%M:%S", std::localtime(&now));
        printf("[%s]: ", buf);

        va_list args;
        va_start(args, var_format);
        vprintf(var_format, args);
        va_end(args);

        printf("\n");
    }
}

void AmoBase::_AmoLogger_Assert(const char *filename, int line, int condition, const char *var_format, ...)
{
    if (AmoLogger_GetLevel() <= LogLevel::Assert)
    {
        if (!condition)
        {
            std::lock_guard<std::mutex> lock(log_mtx);  // lock logging mutex for multiple thread mode
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED);
            printf("%s (line %d) Assertion Failure: \n", filename, line);
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0F);

            std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            char buf[20] = { 0 };
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %I:%M:%S", std::localtime(&now));
            printf("[%s]: ", buf);

            va_list  args;
            va_start(args, var_format);
            vprintf(var_format, args);
            va_end(args);

            printf("\n");
            _CrtDbgBreak();
            exit(-1);
        }
    }
}
