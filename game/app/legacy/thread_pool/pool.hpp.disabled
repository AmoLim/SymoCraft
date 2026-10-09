//
// Created by Amo on 2022/6/15.
//

#ifndef SYMOCRAFT_POOL_H
#define SYMOCRAFT_POOL_H
#include "core.h"

namespace SymoCraft
{

    // Pool
    // a Class Template
    // store some non-component data in the memory for a typical class
    // mainly using for chunk

    template<typename T>
    class Pool
    {
    public:

        // Default constructor
        Pool()
        {
            num_pools = 0;
            data = nullptr;
            data_length = 0;
            pool_size = 0;
            std::lock_guard<std::mutex> lock(free_list_mtx);
            free_list = nullptr;
            free_list_size = 0;
            free_list_start = 0;
        }

        // Constructor
        // Parameters: Pool size, number of pools
        Pool( uint32 _pool_size, uint32 _num_pools)
        {
            this -> num_pools = _num_pools;
            data = (T*) AmoMemory_Allocate(sizeof(T) * pool_size * num_pools);
            data_length = num_pools * pool_size;
            pool_size = _pool_size;

            // initializing the free list
            std::lock_guard<std::mutex> lock(free_list_mtx);
            free_list_size = num_pools;
            free_list_start = 0;
            free_list = (T**) AmoMemory_Allocate(sizeof(T*) * num_pools);
            for (uint32 i = 0; i < num_pools; i++)
                free_list[i] = data + (pool_size * i);
        }

        // Destructor
        ~Pool()
        {
            if (data != nullptr)
            {
                AmoMemory_Free(data);
                data = nullptr;
                data_length = 0;
                pool_size = 0;
                num_pools = 0;

                AmoMemory_Free(free_list);
                free_list = nullptr;
                free_list_start = 0;
                free_list_size = 0;
            }
        }

        // []operator overloading
        // Parameters: [Pool Index]
        T* operator[](int pool_index)
        {
            AmoLogger_Notice(pool_index >= 0 && pool_index < (int)num_pools, "Pool index '%d' out of bounds in pool with size '%d'."
                             , pool_index, num_pools);
            return data + (pool_size + pool_index);
        }

        const T* operator[](int pool_index) const
        {
            AmoLogger_Notice(pool_index >= 0 && pool_index < (int)num_pools, "Pool index '%d' out of bounds in pool with size '%d'."
            , pool_index, num_pools);
            return data + (pool_size + pool_index);
        }

        // Get New Pool pointer
        T* GetNewPool()
        {
            std::lock_guard<std::mutex> lock(free_list_mtx);
            if (free_list_size > 0)
            {
                T* next_pool = free_list[free_list_start];
                free_list[free_list_start] = nullptr;
                free_list_start = (free_list_start + 1) % num_pools;
                free_list_size--;
                return next_pool;
            }
            AmoLogger_Error("Ran out of Pools.");
            return nullptr;
        }

        // Free Pool
        // Parameters: Pool Index
        void FreePool(uint32 pool_index)
        {
            AmoLogger_Notice(pool_index >= 0 && pool_index < num_pools,
                             "Pool index '%d' out of bounds in pool with size '%d'.", pool_index, num_pools);
            std::lock_guard<std::mutex> lock(free_list_mtx);
            uint32 next_index = (free_list_start + free_list_size) % num_pools;
            free_list[next_index] = (T*)(data + (pool_size * pool_index));
            free_list_size++;
        }


        // Free Pool Overloading
        // Parameters: Pool Pointer
        void FreePool(T* pool)
        {
            AmoLogger_Notice(pool >= data && pool <= data + (pool_size * num_pools)
            , "Data '%zu' does not exist in this pool.", pool);
            uint32 pool_index = (uint32)((pool - data) / pool_size);
            FreePool(pool_index);
        }

        // Return free_list_size
        uint32 GetFreeListSize()
        {
            std::lock_guard<std::mutex> lock(free_list_mtx);
            return free_list_size;
        }

        // Return num_pools
        uint32 GetNumPool() const
        {
            return num_pools;
        }

        // Return pool_size
        uint32 GetPoolSize() const
        {
            return pool_size;
        }

        // return data_length * sizeof(T);
        uint64 GetTotalDataLength() const
        {
            return data_length * sizeof(T);
        }

        // check if free_list is empty
        bool IsEmpty()
        {
            std::lock_guard<std::mutex> lock(free_list_mtx);
            return free_list_size == 0;
        }
    private:
        std::mutex free_list_mtx;   // Free list mutex
        uint32 free_list_start;     // Beginning of free list
        uint32 free_list_size;      // Size of free list
        T* * free_list;             // Free list pointer. A pointer to T*

        uint64 data_length;
        uint32 pool_size;
        uint32 num_pools;
        T* data;                    // Pointer to data

    };

}

#endif //SYMOCRAFT_POOL_H
