/*
 * Copyright 2023 Magnopus LLC

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <thread>
#include <vector>

namespace csp
{

class ITaskQueue
{
public:
    ITaskQueue() = default;
    virtual ~ITaskQueue() = default;

    virtual void Enqueue(std::function<void*(void*)> Work) = 0;
    virtual void Shutdown() = 0;
};

class ThreadPool : public ITaskQueue
{
public:
    explicit ThreadPool(size_t Size)
        : ShutdownFlag(false)
    {
        while (Size)
        {
            Threads.emplace_back(Worker(*this));
            Size--;
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ~ThreadPool() override = default;

    void Enqueue(std::function<void*(void*)> Work) override
    {
        std::unique_lock<std::mutex> Lock(Mutex);
        Jobs.push_back(std::move(Work));
        Cond.notify_one();
    }

    void Shutdown() override
    {
        // Stop all worker threads...
        {
            std::unique_lock<std::mutex> Lock(Mutex);
            ShutdownFlag = true;
        }

        Cond.notify_all();

        // Join...
        for (auto& T : Threads)
        {
            T.join();
        }
    }

private:
    struct Worker
    {
        explicit Worker(ThreadPool& InPool)
            : Pool(InPool)
        {
        }

        void operator()()
        {
            for (;;)
            {
                std::function<void*(void*)> Work;
                {
                    std::unique_lock<std::mutex> Lock(Pool.Mutex);

                    Pool.Cond.wait(Lock, [&] { return !Pool.Jobs.empty() || Pool.ShutdownFlag; });

                    if (Pool.ShutdownFlag && Pool.Jobs.empty())
                    {
                        break;
                    }

                    Work = Pool.Jobs.front();
                    Pool.Jobs.pop_front();
                }

                Work(nullptr);
            }
        }

        ThreadPool& Pool;
    };

    friend struct Worker;

    std::vector<std::thread> Threads;
    std::list<std::function<void*(void*)>> Jobs;

    bool ShutdownFlag;

    std::condition_variable Cond;
    std::mutex Mutex;
};

} // namespace csp
