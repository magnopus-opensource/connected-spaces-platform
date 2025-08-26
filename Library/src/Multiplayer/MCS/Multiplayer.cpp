#include "Multiplayer.h"
#include "CSP/Common/Systems/Log/LogSystem.h"

namespace csp::multiplayer::mcs
{
Multiplayer::Multiplayer(ISignalRConnection& Connection, csp::common::LogSystem& LogSystem)
    : Connection { Connection }
    , LogSystem { LogSystem }
{
}

async::task<std::tuple<signalr::value, std::exception_ptr>> Multiplayer::SendObjectMessage(const ObjectMessage& Object)
{
    SignalRSerializer Serializer;
    Serializer.WriteValue(std::vector { Object });

    auto Event = std::make_shared<async::event_task<std::tuple<signalr::value, std::exception_ptr>>>();
    auto Task = Event->get_task();

    auto Callback = [Event](const signalr::value& Result, const std::exception_ptr& Except) { Event->set(std::make_tuple(Result, Except)); };

    Connection.Invoke(HubMethods.Get(MultiplayerHubMethod::SEND_OBJECT_MESSAGE), Serializer.Get(), Callback);

    return Task;
}

async::task<std::tuple<std::vector<uint64_t>, std::exception_ptr>> Multiplayer::GenerateObjectIds(int32_t Count)
{
    const signalr::value Param(static_cast<uint64_t>(Count));
    const std::vector Arr { Param };
    const signalr::value Params(Arr);

    auto Event = std::make_shared<async::event_task<std::tuple<std::vector<uint64_t>, std::exception_ptr>>>();
    auto Task = Event->get_task();

    auto Callback = [Event](const signalr::value& Result, const std::exception_ptr& Except)
    {
        std::vector<uint64_t> Ids;
        SignalRDeserializer Deserializer { Result };
        Deserializer.ReadValue(Ids);

        Event->set(std::make_tuple(Ids, Except));
    };

    Connection.Invoke(HubMethods.Get(MultiplayerHubMethod::GENERATE_OBJECT_IDS), Params, Callback);
}

async::task<std::exception_ptr> Multiplayer::SendObjectPatches(const std::vector<ObjectPatch>& Patches)
{
    SignalRSerializer Serializer;
    Serializer.WriteValue(std::vector { Patches });

    auto Event = std::make_shared<async::event_task<std::exception_ptr>>();
    auto Task = Event->get_task();

    auto Callback = [Event](const signalr::value& /*Result*/, const std::exception_ptr& Except) { Event->set(Except); };

    Connection.Invoke(HubMethods.Get(MultiplayerHubMethod::SEND_OBJECT_PATCHES), Serializer.Get(), Callback);

    return Task;
}

void Multiplayer::PageScopedObjects(bool ExcludeClientOwned, bool IncludeClientOwnedPersistentObjects, int Skip, int Limit,
    std::function<void(PageScopedObjectsResult&&, const std::exception_ptr&)> Callback)
{
    std::vector<signalr::value> ParamsVec;
    ParamsVec.push_back(signalr::value(ExcludeClientOwned));
    ParamsVec.push_back(signalr::value(IncludeClientOwnedPersistentObjects));
    ParamsVec.push_back(signalr::value((uint64_t)Skip));
    ParamsVec.push_back(signalr::value((uint64_t)Limit));
    const auto Params = signalr::value(std::move(ParamsVec));

    auto LocalCallback = [Callback](const signalr::value& Result, const std::exception_ptr& Except)
    {
        PageScopedObjectsResult ObjectsResult;

        if (Except == nullptr)
        {
            const auto& Results = Result.as_array();
            const auto& Items = Results[0].as_array();

            ObjectsResult.Objects.resize(Items.size());
            ObjectsResult.ObjectTotalCount = static_cast<uint32_t>(Results[1].as_uinteger());

            for (size_t i = 0; i < Items.size(); ++i)
            {
                const signalr::value& EntityMessage = Items[i];
                SignalRDeserializer Deserializer { EntityMessage.as_array()[0] };

                ObjectMessage Object;
                Deserializer.ReadValue(ObjectsResult.Objects[i]);
            }
        }

        Callback(std::move(ObjectsResult), Except);
    };

    Connection.Invoke(HubMethods.Get(MultiplayerHubMethod::PAGE_SCOPED_OBJECTS), Params, LocalCallback);
}

void Multiplayer::BindOnObjectMessage(std::function<void(ObjectMessage&&)> Handler)
{
    Connection.On(
        HubMethods.Get(MultiplayerHubMethod::ON_OBJECT_MESSAGE),
        [Handler](const signalr::value& Params)
        {
            SignalRDeserializer Deserializer { Params.as_array()[0] };

            mcs::ObjectMessage Patch;
            Deserializer.ReadValue(Patch);

            Handler(std::move(Patch));
        },
        LogSystem);
}

void Multiplayer::BindOnObjectPatch(std::function<void(mcs::ObjectPatch&&)> Handler)
{
    Connection.On(
        HubMethods.Get(MultiplayerHubMethod::ON_OBJECT_MESSAGE),
        [Handler](const signalr::value& Params)
        {
            SignalRDeserializer Deserializer { Params.as_array()[0] };

            mcs::ObjectPatch Patch;
            Deserializer.ReadValue(Patch);

            Handler(std::move(Patch));
        },
        LogSystem);
}

void Multiplayer::BindOnRequestToSendObject(std::function<void(uint64_t)> Handler)
{
    Connection.On(
        HubMethods.Get(MultiplayerHubMethod::ON_REQUEST_TO_SEND_OBJECT),
        [Handler](const signalr::value& Params)
        {
            SignalRDeserializer Deserializer { Params.as_array()[0] };
            uint64_t ObjectId = 0;
            Deserializer.ReadValue(ObjectId);

            Handler(ObjectId);
        },
        LogSystem);
}
}
