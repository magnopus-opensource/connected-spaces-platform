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
    inline Queue(size_t maxSize = -2147483647 - 1)
        : m_maxSize(maxSize)
        , m_end(false) {};

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

    size_t GetSize() { return m_internalQueue.size(); }

private:
    std::queue<T> m_internalQueue;
    std::mutex m_mutex;
    std::condition_variable m_empty;
    std::condition_variable m_full;
    const size_t m_maxSize;
    std::atomic<bool> m_end;
};

template <typename T> void Queue<T>::Enqueue(T&& t)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_internalQueue.size() == m_maxSize && !m_end)
    {
        m_full.wait(lock);
    }
    assert(!m_end);
    m_internalQueue.push(std::move(t));
    m_empty.notify_one();
}

template <typename T> void Queue<T>::Enqueue(const T& t)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_internalQueue.size() == m_maxSize && !m_end)
    {
        m_full.wait(lock);
    }
    assert(!m_end);
    m_internalQueue.push(std::move(t));
    m_empty.notify_one();
}

template <typename T> std::optional<T> Queue<T>::Dequeue()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_internalQueue.empty() && !m_end)
    {
        m_empty.wait(lock);
    }

    if (m_internalQueue.empty())
    {
        return {};
    }
    T out = std::move(m_internalQueue.front());
    m_internalQueue.pop();
    m_full.notify_one();
    return out;
}

template <typename T> void Queue<T>::Close()
{
    m_end = true;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_empty.notify_one();
    m_full.notify_one();
}

template <typename T> bool Queue<T>::IsEmpty() { return m_internalQueue.empty(); }
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
