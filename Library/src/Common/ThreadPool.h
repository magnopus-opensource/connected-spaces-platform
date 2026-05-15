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

    virtual void Enqueue(std::function<void*(void*)> work) = 0;
    virtual void Shutdown() = 0;
};

class ThreadPool : public ITaskQueue
{
public:
    explicit ThreadPool(size_t size)
        : m_shutdownFlag(false)
    {
        while (size)
        {
            m_threads.emplace_back(Worker(*this));
            size--;
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ~ThreadPool() override = default;

    void Enqueue(std::function<void*(void*)> work) override
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_jobs.push_back(std::move(work));
        m_cond.notify_one();
    }

    void Shutdown() override
    {
        // Stop all worker threads...
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_shutdownFlag = true;
        }

        m_cond.notify_all();

        // Join...
        for (auto& t : m_threads)
        {
            t.join();
        }
    }

private:
    struct Worker
    {
        explicit Worker(ThreadPool& inPool)
            : Pool(inPool)
        {
        }

        void operator()()
        {
            for (;;)
            {
                std::function<void*(void*)> work;
                {
                    std::unique_lock<std::mutex> lock(Pool.m_mutex);

                    Pool.m_cond.wait(lock, [&] { return !Pool.m_jobs.empty() || Pool.m_shutdownFlag; });

                    if (Pool.m_shutdownFlag && Pool.m_jobs.empty())
                    {
                        break;
                    }

                    work = Pool.m_jobs.front();
                    Pool.m_jobs.pop_front();
                }

                work(nullptr);
            }
        }

        ThreadPool& Pool;
    };

    friend struct Worker;

    std::vector<std::thread> m_threads;
    std::list<std::function<void*(void*)>> m_jobs;

    bool m_shutdownFlag;

    std::condition_variable m_cond;
    std::mutex m_mutex;
};

} // namespace csp
