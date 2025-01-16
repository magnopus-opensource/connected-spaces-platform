#pragma once

#include "CSP/Services/WebService.h"

#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

/// Wait for a response from an aync event with a timeout
class ResponseWaiter
{
public:
    /// @brief Wait for an event to occur
    /// @param IsDone Functional (function or lambda) that return true when an event occurs
    /// @param TimeOutInSeconds Maximum time to wait in Seconds
    /// @param SleepTimeMs (Optional) Millseconds to sleep for while waiting (default 100 ms)
    /// @return True if event occured or False if timeout period expired
    bool WaitFor(std::function<bool(void)> IsDone, const std::chrono::seconds& TimeOutInSeconds,
        const std::chrono::milliseconds SleepTimeMs = std::chrono::milliseconds(100))
    {
        using clock = std::chrono::system_clock;

        auto Start = clock::now();
        clock::duration Elapsed = clock::now() - Start;

        const auto TimeOut = TimeOutInSeconds;

        while (!IsDone() && (Elapsed < TimeOut))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(SleepTimeMs));
            Elapsed = clock::now() - Start;
        }

        // Returns True if done event occured or False if we timeout
        return IsDone();
    }
};

template <class ResultType> class ServiceResponseReceiver : public ResponseWaiter
{
public:
    ServiceResponseReceiver(csp::services::EResultCode InExpectedResult = csp::services::EResultCode::Success)
        : ExpectedResult(InExpectedResult)
        , ResponseReceived(false)
    {
    }

    void OnResult(const ResultType& InResult)
    {
        if (InResult.GetResultCode() == csp::services::EResultCode::InProgress)
            return;

        ResponseReceived = true;
    }

    bool WaitForResult()
    {
        return WaitFor([this] { return IsResponseReceived(); }, std::chrono::seconds(20));
    }

    bool IsResponseReceived() const { return ResponseReceived; }

private:
    csp::services::EResultCode ExpectedResult;
    std::atomic<bool> ResponseReceived;
};