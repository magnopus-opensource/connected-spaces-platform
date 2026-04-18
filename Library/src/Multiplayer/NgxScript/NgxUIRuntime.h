#pragma once

#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"

#include "quickjs.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace csp::common
{
class LogSystem;
}

namespace csp::systems
{

class NgxUIRuntime
{
public:
    typedef std::function<csp::common::Vector2(const csp::common::String&, float, const csp::common::String&)> TextMeasureCallback;

    explicit NgxUIRuntime(csp::common::LogSystem& InLogSystem);
    ~NgxUIRuntime();

    void Clear();
    void SetViewportSize(float Width, float Height);
    void SetTextMeasureCallback(TextMeasureCallback InCallback);
    std::string DrainPendingTextMeasureRequestsJson();
    bool SubmitTextMeasureResultsJson(const std::string& ResultsJson);

    // Mount walks a JS tree (the raw object returned by a code component's ui()
    // function), building the internal UINode graph and capturing handler
    // functions into a C++-owned table. No JSON round-trip; no JS-side
    // normalization. The runtime holds a reference to JSValue handlers until
    // the entity is unmounted or Clear() is called.
    bool Mount(const std::string& EntityId, JSContext* Ctx, JSValueConst TreeValue);
    bool Unmount(const std::string& EntityId);

    // Look up a previously-registered handler function for (entityId, handlerId).
    // Returns JS_UNDEFINED if not found. Caller must NOT call JS_FreeValue on
    // the returned value — ownership stays with the runtime.
    JSValueConst GetHandler(const std::string& EntityId, const std::string& HandlerId) const;

    void SetDebugModeEnabled(bool bEnabled);
    bool IsDebugModeEnabled() const;

    std::string DrainPendingUpdatesJson();
    std::string GetDrawablesJson(const std::string& EntityId) const;

private:
    struct Impl;
    std::unique_ptr<Impl> Pimpl;
};

} // namespace csp::systems
