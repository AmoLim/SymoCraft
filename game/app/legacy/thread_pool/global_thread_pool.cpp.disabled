//
// Created by Amo on 2022/6/15.
//

#include "core/global_thread_pool.h"
#include "core.h"

namespace SymoCraft
{

    bool CompareThreadTask::operator()(const ThreadTask &task1, ThreadTask &task2) const
    {
        if (task1.priority == task2.priority)
            return task1.counter > task2.counter;
        return (uint8_t)task1.priority > (uint8_t)task2.priority;
    }

    GlobalThreadPool::GlobalThreadPool(uint64_t num)
            : cv(), queue_mtx(), general_mtx(), do_work(true), num_threads(num)
    {
        worker_threads = new std::thread[num_threads];
        for (int i = 0; i <= num_threads; i++)
        {
            // point process loop
            // pass thread index to process loop
            worker_threads[i] = std::thread(&GlobalThreadPool::ProcessLoop, this, i);
        }
    }

    void GlobalThreadPool::Free()
    {
        {
            // access general mutex
            std::lock_guard lock(general_mtx);
            // as soon as it is accessed, set working status to be false
            do_work =false;
        }
        // unblock all the threads
        cv.notify_all();
        for (int i = 0; i < num_threads; i++)
        {
            // wait for all the threads to end its job
            worker_threads[i].join();
        }
        // free worker_threads
        delete []worker_threads;
    }

    void GlobalThreadPool::ProcessLoop(uint32_t thread_index)
    {
        bool should_continue = true;
        while (should_continue)
        {
            if (tasks.empty())
            {
                // Wait until tasks is not empty
                std::unique_lock<std::mutex> lock(general_mtx);
                cv.wait(lock, [&]{ return (!do_work || !tasks.empty());});  // an anonymous function
                should_continue = do_work && !tasks.empty();
            }

            // begin to finish task
            ThreadTask task;
            {
                std::lock_guard<std::mutex> queue_lock(queue_mtx);
                if (tasks.size() > 0)
                {
                    task = tasks.top();
                    tasks.pop();
                }
                else
                {
                    task.func = nullptr;
                }
            }

            // function pointer valid
            if (task.func)
            {
                {
                    // may use OpTick to track thread
                    task.func(task.data, task.data_size);
                }
                // callback function pointer valid
                if (task.callback)
                {
                    task.callback(task.data, task.data_size);
                }
            }
        }
    }

    void GlobalThreadPool::QueueTask(TaskFunction task_func, const char *task_name, void *data, size_t data_size,
                                     Priority priority, ThreadCallback callback)
    {
        static uint64_t counter = 0;    // initialize static task counter
        ThreadTask task;
        task.func = task_func;
        task.data = data;
        task.data_size = data_size;
        task.priority = priority;
        task.callback = callback;
        task.task_name = task_name;
        {
            std::lock_guard<std::mutex> lockGuard(queue_mtx);   // avoid access conflict
            task.counter = counter;
            tasks.push(task);
        }
    }

    void GlobalThreadPool::BeginWork(bool _is_notify_all)
    {
        if (_is_notify_all)
        {
            // unblock all the threads
            cv.notify_all();
        }
        else
        {
            // unblock a random thread
            cv.notify_one();
        }

    }


}