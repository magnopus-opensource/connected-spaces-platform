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

#include "Common/DateTime.h"

#include <assert.h>
#include <chrono>
#include <functional>
#include <future>
#include <list>
#include <memory>
#include <queue>
#include <sstream>
#include <thread>

namespace csp
{

using ScheduledTaskId = uint32_t;

struct FunctionTimer
{
    std::function<void()> Func;
    std::chrono::system_clock::time_point Time;
    ScheduledTaskId Id;

    FunctionTimer() { }

    FunctionTimer(std::function<void()>&& inFunc, const std::chrono::system_clock::time_point& inTime, ScheduledTaskId inId)
        : Func(inFunc)
        , Time(inTime)
        , Id(inId)
    {
    }

    bool operator<(const FunctionTimer& rhs) const { return Time > rhs.Time; }

    bool operator==(const FunctionTimer& rhs) const { return Id == rhs.Id; }

    void Get() { Func(); }

    void operator()() { Func(); }

    uint32_t GetId() const { return Id; }
};

class Scheduler
{
public:
    Scheduler()
        : m_thread(nullptr)
        , m_idCounter(1)
        , m_shouldExit(false)
    {
    }

    ~Scheduler() { Shutdown(); }

    void Initialise()
    {
        assert(m_thread == nullptr);
        m_shouldExit = false;
        m_thread = new std::thread([this]() { ThreadLoop(); });
    }

    void Shutdown()
    {
        if (m_thread != nullptr)
        {
            m_shouldExit = true;
            m_thread->join();
            delete (m_thread);
            m_thread = nullptr;
        }
    }

    ScheduledTaskId ScheduleAt(const std::chrono::system_clock::time_point& time, std::function<void()> func)
    {
        std::function<void()> threadFunc = [func]()
        {
            std::thread thread(func);
            thread.detach();
        };

        return ScheduleAtIntern(time, std::move(threadFunc));
    }

    void ScheduleEvery(std::chrono::system_clock::duration interval, std::function<void()> func)
    {
        std::function<void()> threadFunc = [func]()
        {
            std::thread thread(func);
            thread.detach();
        };

        this->ScheduleEveryIntern(interval, threadFunc);
    }

    ScheduledTaskId ScheduleAt(const csp::common::DateTime& time, std::function<void()> func)
    {
        return ScheduleAt(time.GetTimePoint(), std::move(func));
    }

    void CancelTask(ScheduledTaskId id)
    {
        std::scoped_lock<std::mutex> listLocker(m_listLock);

        auto it = std::remove_if(m_tasks.begin(), m_tasks.end(), [id](const FunctionTimer& func) { return func.Id == id; });

        m_tasks.erase(it, m_tasks.end());
    }

private:
    ScheduledTaskId ScheduleAtIntern(const std::chrono::system_clock::time_point& time, std::function<void()>&& func)
    {
        ScheduledTaskId id = m_idCounter++;

        {
            std::scoped_lock<std::mutex> listLocker(m_listLock);
            m_tasks.push_back(FunctionTimer(std::move(func), time, id));
            m_tasks.sort();
        }

        return id;
    }

    void ScheduleEveryIntern(std::chrono::system_clock::duration interval, std::function<void()> func)
    {
        std::function<void()> waitFunc = [this, interval, func]()
        {
            func();
            this->ScheduleEveryIntern(interval, func);
        };

        ScheduleAtIntern(std::chrono::system_clock::now() + interval, std::move(waitFunc));
    }

    void ThreadLoop()
    {
        while (!m_shouldExit)
        {
            {
                std::scoped_lock<std::mutex> listLocker(m_listLock);

                auto now = std::chrono::system_clock::now();

                if (m_tasks.size() > 0 && m_tasks.front().Time <= now)
                {
                    FunctionTimer func;

                    {
                        func = m_tasks.front();
                        m_tasks.pop_front();
                    }

                    func.Get();
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

private:
    std::mutex m_listLock;
    std::list<FunctionTimer> m_tasks;
    std::thread* m_thread;
    std::atomic_uint32_t m_idCounter;
    bool m_shouldExit;

    Scheduler& operator=(const Scheduler& rhs) = delete;
    Scheduler(const Scheduler& rhs) = delete;
};

Scheduler* GetScheduler();
void DestroyScheduler();

} // namespace csp
