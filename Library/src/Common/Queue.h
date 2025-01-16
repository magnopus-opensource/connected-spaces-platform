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

#include <assert.h>
#include <atomic>
#include <condition_variable>
#include <optional>
#include <queue>
#include <utility>

namespace csp
{
/* Thread Safe Queue Template, C++17 */
template <typename T> struct Queue
{
    /* Create Queue object. Set maximum size of the queue to max_size. */
    inline Queue(size_t max_size = -2147483647 - 1)
        : MaxSize(max_size)
        , End(false) {};

    /* Enqueue T to the queue. Many threads can push at the same time.
     * If the queue is full, calling thread will be suspended until
     * some other thread Enqueue() data. */
    void Enqueue(const T&);
    void Enqueue(T&&);

    /* Close the queue.
     * Be sure all writing threads done their writes before call this.
     * Push data to closed queue is forbidden. */
    void Close();

    /* Dequeue and return T from the queue. Many threads can pop at the same time.
     * If the queue is empty, calling thread will be suspended.
     * If the queue is empty and closed, nullopt returned. */
    std::optional<T> Dequeue();

    bool IsEmpty();

    size_t GetSize() { return InternalQueue.size(); }

private:
    std::queue<T> InternalQueue;
    std::mutex Mutex;
    std::condition_variable Empty;
    std::condition_variable Full;
    const size_t MaxSize;
    std::atomic<bool> End;
};

template <typename T> void Queue<T>::Enqueue(T&& t)
{
    std::unique_lock<std::mutex> Lock(Mutex);
    while (InternalQueue.size() == MaxSize && !End)
    {
        Full.wait(Lock);
    }
    assert(!End);
    InternalQueue.push(std::move(t));
    Empty.notify_one();
}

template <typename T> void Queue<T>::Enqueue(const T& t)
{
    std::unique_lock<std::mutex> Lock(Mutex);
    while (InternalQueue.size() == MaxSize && !End)
    {
        Full.wait(Lock);
    }
    assert(!End);
    InternalQueue.push(std::move(t));
    Empty.notify_one();
}

template <typename T> std::optional<T> Queue<T>::Dequeue()
{
    std::unique_lock<std::mutex> Lock(Mutex);
    while (InternalQueue.empty() && !End)
    {
        Empty.wait(Lock);
    }

    if (InternalQueue.empty())
    {
        return {};
    }
    T Out = std::move(InternalQueue.front());
    InternalQueue.pop();
    Full.notify_one();
    return Out;
}

template <typename T> void Queue<T>::Close()
{
    End = true;
    std::lock_guard<std::mutex> Lock(Mutex);
    Empty.notify_one();
    Full.notify_one();
}

template <typename T> bool Queue<T>::IsEmpty() { return InternalQueue.empty(); }
} // namespace csp

/* Usage sample:
 #include "Queue.h"

 #include <chrono>
 #include <iostream>
 #include <thread>

                using namespace std;
                csp::Queue<int> Que;
                void Foo()
                {
                                for (int i = 0; i < 4; i++)
                                {
                                                Que.Enqueue(i);
                                                this_thread::sleep_for(chrono::seconds(1));
                                }
                                Que.Close();
                }
                int main()
                {
                                thread SomeThread(Foo);

                                while(Que.IsEmpty() == false)
                                {
                                   auto X = Que.Dequeue();
                                   cout << *X << '\n';
                                }

                                SomeThread.join();
                }
 */
