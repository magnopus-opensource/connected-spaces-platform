#pragma once

#include "CSP/Common/String.h"
#include "CSP/Common/Vector.h"

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

    bool Mount(const std::string& EntityId, const std::string& TreeJson);
    bool Unmount(const std::string& EntityId);

    std::string DrainPendingUpdatesJson();
    std::string GetDrawablesJson(const std::string& EntityId) const;

private:
    struct Impl;
    std::unique_ptr<Impl> Pimpl;
};

} // namespace csp::systems
