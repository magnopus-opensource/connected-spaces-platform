#pragma once

#include "MCSTypes.h"

#include "CSP/Multiplayer/MultiplayerHubMethods.h"
#include "Multiplayer/SignalR/ISignalRConnection.h"

#include <vector>

namespace csp::common
{
class LogSystem;
}

namespace csp::multiplayer::mcs
{
struct PageScopedObjectsResult
{
    // Objects retrieved in this page.
    std::vector<ObjectMessage> Objects;
    // The total amount of objects that are available.
    uint32_t ObjectTotalCount;
};

class Multiplayer
{
public:
    Multiplayer(ISignalRConnection& Connection, csp::common::LogSystem& LogSystem);

    // todo: remove signalr value
    async::task<std::tuple<signalr::value, std::exception_ptr>> SendObjectMessage(const ObjectMessage& Object);

    async::task<std::exception_ptr> SendObjectPatches(const std::vector<ObjectPatch>& Patches);

    async::task<std::tuple<std::vector<uint64_t>, std::exception_ptr>> GenerateObjectIds(int32_t Count);

    void PageScopedObjects(bool ExcludeClientOwned, bool IncludeClientOwnedPersistentObjects, int Skip, int Limit,
        std::function<void(PageScopedObjectsResult&&, const std::exception_ptr&)> Callback);

    void BindOnObjectMessage(std::function<void(ObjectMessage&&)> Handler);
    void BindOnObjectPatch(std::function<void(ObjectPatch&&)> Handler);

    void BindOnRequestToSendObject(std::function<void(uint64_t)> Handler);

private:
    ISignalRConnection& Connection;
    MultiplayerHubMethodMap HubMethods;

    csp::common::LogSystem& LogSystem;
};
}
