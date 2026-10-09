//
// Created by Amo on 2022/6/15.
//

#ifndef SYMOCRAFT_GLOBALTHREADPOOL_H
#define SYMOCRAFT_GLOBALTHREADPOOL_H

#include "core.h"

namespace SymoCraft
{

    enum class Priority : uint8_t
    {
        High = 0,
        Medium,
        Low,
        None
    };

    typedef void (*TaskFunction)(void* data, size_t data_size);
    typedef void (*ThreadCallback)(void* data, size_t data_size);

    struct ThreadTask
    {
        TaskFunction func;  // store a function pointer to task function
        ThreadCallback callback;    // store a function pointer for thread callback
        uint64_t counter;       // queue order of the task  (counter of thread)
        void* data;         // a pointer to data
        size_t data_size;   // store data size
        Priority priority;  // task's priority
        const char* task_name;  // the name of the task
    };

    struct CompareThreadTask
    {
        // return true means task1's priority is greater than task2
        bool operator() (const ThreadTask& task1, ThreadTask& task2) const;
    };

    class GlobalThreadPool
    {
    public:
        // Constructor of Global Thread Pool
        // Parameters: the Number of threads in global thread pool
        GlobalThreadPool(uint64_t num);

        // Free global thread pool
        void Free();

        // Process Loop for task
        // Parameters: thread index for OpTick
        void ProcessLoop(uint32_t thread_index);

        // Queue Task
        // Parameters: task function pointer, task name, data pointer, data size
        //            ,priority, thread call back function pointer
        void QueueTask(TaskFunction task_func,
                       const char* task_name = "Default",
                       void* data = nullptr,
                       size_t data_size = 0,
                       Priority priority = Priority::None,
                       ThreadCallback callback = nullptr);

        // Begin Work Function
        // Parameters: _is notifyALL?
        void BeginWork(bool _is_notify_all = true);

    private:
        // Priority queue for thread tasks
        // the lesser, the more forward
        std::priority_queue<ThreadTask, std::vector<ThreadTask>, CompareThreadTask> tasks;
        std::thread* worker_threads;    // thread pointer
        std::condition_variable cv;     // condition_variable
        std::mutex general_mtx;         // general mutex in thread pool
        std::mutex queue_mtx;           // mutex for queueing task
        bool do_work;                   // working status of thread pool
        uint32_t num_threads;           // number of threads
    };

}

#endif //SYMOCRAFT_GLOBALTHREADPOOL_H
