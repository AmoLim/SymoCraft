//
// Created by Amo on 2022/6/14.
//

#ifndef SYMOCRAFT_RAWMEMORY_H
#define SYMOCRAFT_RAWMEMORY_H
#include "symocraft/foundation/memory.h"
#include "symocraft/foundation/diagnostics.h"


struct RawMemory
{
    uint8_t * data;        // data location
    size_t size;        // data size
    size_t offset;      // Read Write Cursor's offset

    // Initialize RawMemory
    // Parameters: required buffer size
    void Init( size_t initial_size)
    {
        size = initial_size;
        data = (uint8_t  *) AmoMemory_Allocate(initial_size);        // allocate a buffer for the data
        offset = 0;     // set the Read Write Cursor's offset to 0
    }

    // Free RawMemory
    void Free()
    {
        if (data)
        {
            AmoMemory_Free(data);
            size = 0;
            data = nullptr;
            offset = 0;
        }
    }

    // Re-allocate / Shrink the data from 0 to offset
    void ShrinkToFit()
    {
        data = (uint8_t  *)AmoMemory_ReAlloc( data, offset);
        size = offset;
    }

    // Set the offset to 0
    void ResetReadWriteCursor()
    {
        offset = 0;
    }

    // Set Read Write Cursor to offset
    // Parameters: offset
    void SetCursor(size_t offset)
    {
        this->offset = offset;
    }

    // A Dangerous Writing to memory
    // that may cause a memory leak if the data_size does not match the data type
    // Parameters: data pointer, data size
    void WriteDangerous( const uint8_t * data, size_t data_size)
    {
        if (this->offset + data_size >= this->size)     // if the new data size is larger than the current data size,
                                                        // then we reallocate the memory
        {
            size_t new_size = (this->offset + data_size) * 2;
        }
    }

    // A Dangerous Reading of memory
    // Parameters: data pointer, data size
    void ReadDangerous( uint8_t * data, size_t data_size)
    {
        if (this->offset + data_size > this->size)
        {
            AmoLogger_Error("Deserialized bad data. Read boundary out of bounds"
                            ", cannot access '%zu' bytes in memory of size '%zu' bytes",
                            this->offset + data_size,
                            this->size);
            return;
        }

        AmoBase::AmoMemory_CopyMem((uint8_t *)data, this->data + this->offset, data_size);
        this->offset += data_size;
    }


    // a Safer Writing to memory
    // Parameters: data pointer
    template<typename T>
    void Write(const T* data)
    {
        WriteDangerous((uint8_t *)data, sizeof(T));
    }

    // a Safer Reading of memory
    // Parameters: data pointer
    template<typename T>
    void Read(T* data)
    {
        ReadDangerous((uint8_t *)data, sizeof(T));
    }

};
#endif //SYMOCRAFT_RAWMEMORY_H
