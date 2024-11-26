# Performance Profiling

The Connected Spaces Platform library has support for low-level performance profiling of the library itself, which can be linked into existing client profiling tools such as Unreal Insights, Optick or Microprofile.

## Internal Profiling Hooks
The internal header `Debug/Logging.h` provides the following macros for instrumenting performance within the library.

```
FOUNDATION_PROFILE_SCOPED()

FOUNDATION_PROFILE_BEGIN(TAG)                       
FOUNDATION_PROFILE_END()

FOUNDATION_PROFILE_BEGIN_FORMAT(FORMAT_STR, ...)
FOUNDATION_PROFILE_SCOPED_FORMAT(FORMAT_STR, ...)   
FOUNDATION_PROFILE_SCOPED_TAG(TAG)

FOUNDATION_PROFILE_EVENT_TAG(TAG)                   
FOUNDATION_PROFILE_EVENT_FORMAT(FORMAT_STR, ...)
```

These macros are compiled out in a Release build, or more specifically when profiling is disabled.

```c++
if !defined(FOUNDATION_PROFILING_ENABLED)
  #if defined(NDEBUG)
    // Disable profiling in release by default (change this to override)
    #define FOUNDATION_PROFILING_ENABLED 0
  #else
    #define FOUNDATION_PROFILING_ENABLED 1
  #endif
#endif
```

Example use for function instrumentation.

```c++
void CSPFoundation::Tick()
{
    FOUNDATION_PROFILE_SCOPED();

    csp::events::Event* TickEvent = csp::events::EventSystem::Get().AllocateEvent(csp::events::FOUNDATION_TICK_EVENT_ID); 
    csp::events::EventSystem::Get().EnqueueEvent(TickEvent);

    csp::events::EventSystem::Get().ProcessEvents();
}
```

## Example - Binding to Unreal Insights

The following snippet shows how an Unreal client can hook into the CSP profiling system. 

In this case the client is calling [Unreal Insights](https://docs.unrealengine.com/5.1/en-US/unreal-insights-in-unreal-engine/) profiling calls, but could equally be calling macros for other systems such as [Optick](https://github.com/bombomby/optick) or [Microprofile](https://github.com/jonasmr/microprofile).

![image info](../../_static/debugging/insights.png)

```c++
void FUnrealModule::RegisterCSPCallbacks()
{
#if !UE_BUILD_SHIPPING
    Trace::ToggleChannel(TEXT("CSPTraceChannel"), true);

    csp::systems::SystemsManager::Get().GetLogSystem()->RegisterEventCallback(
        [this](oly_common::String InMessage)
    {
        TRACE_BOOKMARK(TEXT("%s"), *InMessage);
    });

    csp::systems::SystemsManager::Get().GetLogSystem()->RegisterBeginMarkerCallback(
        [this](oly_common::String InMessage)
    {
        bool bChannelEnabled = UE_TRACE_CHANNELEXPR_IS_ENABLED(CSPTraceChannel);
        if (bChannelEnabled)
        {
            unsigned int EventTag = FCpuProfilerTrace::OutputEventType(InMessage.c_str());
            FCpuProfilerTrace::OutputBeginEvent(EventTag);
        }
    });

    csp::systems::SystemsManager::Get().GetLogSystem()->RegisterEndMarkerCallback(
        [this](void*)
    {
        bool bChannelEnabled = UE_TRACE_CHANNELEXPR_IS_ENABLED(CSPTraceChannel);
        if (bChannelEnabled)
        {
            FCpuProfilerTrace::OutputEndEvent();
        }
    });
#endif
}