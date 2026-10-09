//
// Created by Amo on 2022/6/14.
//

#ifndef SYMOCRAFT_MEMORYHELPER_H
#define SYMOCRAFT_MEMORYHELPER_H
#include "symocraft/foundation/memory.h"
#include "symocraft/foundation/diagnostics.h"
#include <type_traits>

struct SizedMemory
{
    uint8_t* memory;        // memory cursor
    size_t size;            // data size
};

namespace MemoryHelper {
    // Copy data by Type
    // Basic copy data function
    // destination's type should equal to data's type
    // Parameters: copy destination, offset, data
    template<typename T>
    size_t CopyDataByType(uint8_t *destin, size_t offset, const T &data)
    {
        // set destin_data cursor to offset
        uint8_t *destin_data = destin + offset;
        // copy data
        *(T*)destin_data = data;
        // return new offset
        return sizeof(T);
    }

    // Copy data to type
    // To deal with multiple types data copying
    // Parameters: copy destination, offset, first data, rest of data
    template<typename First, typename... Rest>
    void CopyDataToType(uint8_t *destin, size_t offset, const First &data, Rest... rest_data)
    {
        static_assert(std::is_trivial<First>() && std::is_standard_layout<First>()
                , "Cannot accept non-POD values for dynamic memory packing.");
        // set new offset
        offset += CopyDataByType<First>(destin, offset, data);
        // if rest data exists, continue CopyDataToType for rest_data...
        if constexpr(sizeof...(Rest) != 0)
        {
            CopyDataToType<Rest...>(destin, offset, rest_data...);
        }
    }

    // Unpack data
    // unpack the data in sized memory form cursor to first data pointer, ...
    // Parameters: target sized_memory, cursor offset, first destination, ...rest of destinations
    template<typename First, typename... Rest>
    void UnpackData(const SizedMemory &sized_mem, size_t offset, First *destin, Rest*... rest_destin)
    {
#ifdef _DEBUG
        AmoLogger_Notice(offset + sizeof(First) <= sized_mem.size
                     , "Cannot unpack this memory. Would result in a buffer overrun.");
#endif
        static_assert(std::is_trivial<First>() && std::is_standard_layout<First>()
                , "Cannot accept non-POD values for dynamic memory unpacking");
        // set data cursor to the offset
        uint8_t *memory_bytes = sized_mem.memory + offset;
        // unpack sized_mem's data to destin
        *destin = *(First*)memory_bytes;

        // if rest destinations exist, continue UnpackData for rest_destin...
        if constexpr(sizeof...(Rest) != 0)
        {
            UnpackData<Rest...>(sized_mem, offset + sizeof(First), rest_destin...);
        }
    }
}

template<typename First, typename... Rest>
size_t SizeOfTypes()
{
    if constexpr(sizeof...(Rest) == 0)
    {
        // the rest types does not exist, return size of the first type
        return sizeof(First);
    }
    else
    {
        // the rest types exist, call itself
        return sizeof(First) + SizeOfTypes<Rest...>();
    }
}


template<typename... Types>
SizedMemory Pack(const Types&... data)
{
    // calculate the total type size of Types
    size_t type_size = SizeOfTypes<Types...>();
    // Allocate specified size of memory
    uint8_t *result = (uint8_t*) AmoMemory_Allocate(type_size);
    MemoryHelper::CopyDataToType<Types...>(result, 0, data...);
    return { result, type_size};
}

template<typename... Types>
void Unpack(const SizedMemory& target_memory, Types*... destinations)
{
    MemoryHelper::UnpackData<Types...>( target_memory, 0, destinations...);
}


#endif //SYMOCRAFT_MEMORYHELPER_H

