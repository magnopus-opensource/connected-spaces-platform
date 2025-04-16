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

    FunctionTimer(std::function<void()>&& InFunc, const std::chrono::system_clock::time_point& InTime, ScheduledTaskId InId)
        : Func(InFunc)
        , Time(InTime)
        , Id(InId)
    {
    }

    bool operator<(const FunctionTimer& Rhs) const { return Time > Rhs.Time; }

    bool operator==(const FunctionTimer& Rhs) const { return Id == Rhs.Id; }

    void Get() { Func(); }

    void operator()() { Func(); }

    uint32_t GetId() const { return Id; }
};

class Scheduler
{
public:
    Scheduler()
        : Thread(nullptr)
        , IdCounter(1)
        , ShouldExit(false)
    {
    }

    ~Scheduler() { Shutdown(); }

    void Initialise()
    {
        assert(Thread == nullptr);
        ShouldExit = false;
        Thread = new std::thread([this]() { ThreadLoop(); });
    }

    void Shutdown()
    {
        if (Thread != nullptr)
        {
            ShouldExit = true;
            Thread->join();
            delete (Thread);
            Thread = nullptr;
        }
    }

    ScheduledTaskId ScheduleAt(const std::chrono::system_clock::time_point& Time, std::function<void()> Func)
    {
        std::function<void()> threadFunc = [Func]()
        {
            std::thread Thread(Func);
            Thread.detach();
        };

        return ScheduleAtIntern(Time, std::move(threadFunc));
    }

    void ScheduleEvery(std::chrono::system_clock::duration Interval, std::function<void()> Func)
    {
        std::function<void()> threadFunc = [Func]()
        {
            std::thread Thread(Func);
            Thread.detach();
        };

        this->ScheduleEveryIntern(Interval, threadFunc);
    }

    ScheduledTaskId ScheduleAt(const csp::common::DateTime& Time, std::function<void()> Func)
    {
        return ScheduleAt(Time.GetTimePoint(), std::move(Func));
    }

    void CancelTask(ScheduledTaskId Id)
    {
        std::scoped_lock<std::mutex> ListLocker(ListLock);

        auto it = std::remove_if(Tasks.begin(), Tasks.end(), [Id](const FunctionTimer& Func) { return Func.Id == Id; });

        Tasks.erase(it, Tasks.end());
    }

private:
    ScheduledTaskId ScheduleAtIntern(const std::chrono::system_clock::time_point& Time, std::function<void()>&& Func)
    {
        ScheduledTaskId Id = IdCounter++;

        {
            std::scoped_lock<std::mutex> ListLocker(ListLock);
            Tasks.push_back(FunctionTimer(std::move(Func), Time, Id));
            Tasks.sort();
        }

        return Id;
    }

    void ScheduleEveryIntern(std::chrono::system_clock::duration Interval, std::function<void()> Func)
    {
        std::function<void()> waitFunc = [this, Interval, Func]()
        {
            Func();
            this->ScheduleEveryIntern(Interval, Func);
        };

        ScheduleAtIntern(std::chrono::system_clock::now() + Interval, std::move(waitFunc));
    }

    void ThreadLoop()
    {
        while (!ShouldExit)
        {
            {
                std::scoped_lock<std::mutex> ListLocker(ListLock);

                auto now = std::chrono::system_clock::now();

                if (Tasks.size() > 0 && Tasks.front().Time <= now)
                {
                    FunctionTimer Func;

                    {
                        Func = Tasks.front();
                        Tasks.pop_front();
                    }

                    Func.Get();
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

private:
    std::mutex ListLock;
    std::list<FunctionTimer> Tasks;
    std::thread* Thread;
    std::atomic_uint32_t IdCounter;
    bool ShouldExit;

    Scheduler& operator=(const Scheduler& Rhs) = delete;
    Scheduler(const Scheduler& Rhs) = delete;
};

Scheduler* GetScheduler();
void DestroyScheduler();

} // namespace csp
