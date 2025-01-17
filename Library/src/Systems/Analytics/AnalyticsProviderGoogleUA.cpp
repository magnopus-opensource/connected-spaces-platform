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
#include "CSP/Systems/Analytics/AnalyticsProviderGoogleUA.h"

#ifdef CSP_WASM
#include "Web/EmscriptenWebClient/EmscriptenWebClient.h"
#else
#include "Web/POCOWebClient/POCOWebClient.h"
#endif

namespace
{
#ifdef CSP_WASM
class UAEmscriptenWebClient : public csp::web::EmscriptenWebClient
{
public:
    UAEmscriptenWebClient(const csp::web::Port InPort, const csp::web::ETransferProtocol Tp)
        : csp::web::EmscriptenWebClient(InPort, Tp)
    {
    }
};
#else
class UAPOCOWebClient : public csp::web::POCOWebClient
{
public:
    UAPOCOWebClient(const csp::web::Port InPort, const csp::web::ETransferProtocol Tp)
        : csp::web::POCOWebClient(InPort, Tp)
    {
    }
};
#endif

class ResponseReceiver : public csp::web::IHttpResponseHandler
{
public:
    void OnHttpResponse(csp::web::HttpResponse& InResponse) override { }

    bool ShouldDelete() const override { return true; }
};
} // namespace

namespace csp::systems
{
csp::common::String CreateUAEventString(const csp::common::String& ClientId, const csp::common::String& Property, csp::systems::AnalyticsEvent* Event)
{
    static constexpr const char* VersionTag = "v=1";
    static constexpr const char* PropertyTag = "&tid=";
    static constexpr const char* ClientIdTag = "&cid=";
    static constexpr const char* EventTag = "&t=event";
    static constexpr const char* CategoryTag = "&ec=event";
    static constexpr const char* ActionTag = "&ea=";
    static constexpr const char* LabelTag = "&el=";
    static constexpr const char* ValueTag = "&ev=";

    csp::common::String EventString
        = csp::common::String(VersionTag) + PropertyTag + Property + ClientIdTag + ClientId + EventTag + CategoryTag + ActionTag + Event->GetTag();

    const csp::common::Map<csp::common::String, csp::systems::MetricValue>& Params = Event->GetParams();
    auto* Values = Params.Values();

    if (Values->Size() > 2)
    {
        return "";
    }

    // UA only has specific event params so we are limited to 1 string param and 1 int param
    bool HasIntegerParam = false;
    bool HasStringParam = false;

    for (int i = 0; i < Values->Size(); ++i)
    {
        if (Values->operator[](i).GetReplicatedValueType() == csp::multiplayer::ReplicatedValueType::Integer)
        {
            if (HasIntegerParam)
            {
                return "";
            }

            HasIntegerParam = true;
            EventString += ValueTag;
            EventString += std::to_string(Values->operator[](i).GetInt()).c_str();
        }
        else if (Values->operator[](i).GetReplicatedValueType() == csp::multiplayer::ReplicatedValueType::String)
        {
            if (HasStringParam)
            {
                return "";
            }

            HasStringParam = true;
            EventString += LabelTag + Values->operator[](i).GetString();
        }
        else
        {
            return "";
        }
    }

    return EventString;
}

AnalyticsProviderGoogleUA::AnalyticsProviderGoogleUA(const csp::common::String& ClientId, const csp::common::String& PropertyTag)
    : ClientId { ClientId }
    , PropertyTag { PropertyTag }
    , Start { std::chrono::steady_clock::now() }
{
#ifdef CSP_WASM
    WebClient = CSP_NEW UAEmscriptenWebClient(80, csp::web::ETransferProtocol::HTTPS);
#else
    WebClient = CSP_NEW UAPOCOWebClient(80, csp::web::ETransferProtocol::HTTPS);
#endif
}

void AnalyticsProviderGoogleUA::Log(AnalyticsEvent* Event)
{
    csp::common::String EventString = CreateUAEventString(ClientId, PropertyTag, Event);

    auto Current = std::chrono::steady_clock::now();
    uint64_t MS = std::chrono::duration_cast<std::chrono::milliseconds>(Current - Start).count();
    EventString += csp::common::String("&cm1=") + std::to_string(MS).c_str();

    auto* Receiver = CSP_NEW ResponseReceiver;

    csp::web::HttpPayload Payload;
    Payload.SetContent(EventString);

    WebClient->SendRequest(csp::web::ERequestVerb::POST, csp::web::Uri("https://www.google-analytics.com/collect"), Payload, Receiver,
        csp::common::CancellationToken::Dummy());
}
} // namespace csp::systems
