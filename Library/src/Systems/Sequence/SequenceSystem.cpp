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

#include "CSP/Systems/Sequence/SequenceSystem.h"

#include "CallHelpers.h"
#include "Common/Convert.h"
#include "Services/AggregationService/Api.h"
#include "Systems/ResultHelpers.h"

using namespace csp;
using namespace csp::common;

namespace chs = csp::services::generated::aggregationservice;

namespace
{
std::shared_ptr<chs::SequenceDto>
	CreateSequenceDto(const String& SequenceKey, const String& ReferenceType, const String& ReferenceId, const Array<String>& Items)
{
	auto SequenceInfo = std::make_shared<chs::SequenceDto>();
	SequenceInfo->SetKey(SequenceKey);
	SequenceInfo->SetReferenceType(ReferenceType);
	SequenceInfo->SetReferenceId(ReferenceId);
	SequenceInfo->SetItems(Convert(Items));

	return SequenceInfo;
}

} // namespace

namespace csp::systems
{
void SequenceSystem::CreateSequence(
	const String& SequenceKey, const String& ReferenceType, const String& ReferenceId, const Array<String>& Items, SequenceResultCallback Callback)
{
	const auto SequenceInfo = CreateSequenceDto(SequenceKey, ReferenceType, ReferenceId, Items);

	csp::services::ResponseHandlerPtr ResponseHandler
		= SequenceAPI->CreateHandler<SequenceResultCallback, SequenceResult, void, chs::SequenceDto>(Callback, nullptr);

	static_cast<chs::SequenceApi*>(SequenceAPI)
		->apiV1SequencesPut(SequenceInfo,			   // Dto
							ResponseHandler,		   // ResponseHandler
							CancellationToken::Dummy() // CancellationToken
		);
}

void SequenceSystem::UpdateSequence(
	const String& SequenceKey, const String& ReferenceType, const String& ReferenceId, const Array<String>& Items, SequenceResultCallback Callback)
{
	CreateSequence(SequenceKey, ReferenceType, ReferenceId, Items, Callback);
}

void SequenceSystem::RenameSequence(const String& OldSequenceKey, const String& NewSequenceKey, SequenceResultCallback Callback)
{
	csp::services::ResponseHandlerPtr ResponseHandler
		= SequenceAPI->CreateHandler<SequenceResultCallback, SequenceResult, void, chs::SequenceDto>(Callback, nullptr);

	static_cast<chs::SequenceApi*>(SequenceAPI)
		->apiV1SequencesKeysOldKeyKeyPut(OldSequenceKey,			// OldKey
										 NewSequenceKey,			// NewKey
										 ResponseHandler,			// ResponseHandler
										 CancellationToken::Dummy() // CancellationToken
		);
}

void SequenceSystem::GetSequencesByCriteria(const Optional<Array<String>>& InSequenceKeys,
											const Optional<String>& InKeyRegex,
											const Optional<String>& InReferenceType,
											const Optional<Array<String>>& InReferenceIds,
											SequencesResultCallback Callback)
{
	if (InReferenceType.HasValue() && InReferenceIds.HasValue() == false || InReferenceIds.HasValue() && InReferenceType.HasValue() == false)
	{
		CSP_LOG_ERROR_MSG("InReferenceType and InReferenceIds need to be used together");
		INVOKE_IF_NOT_NULL(Callback, MakeInvalid<SequencesResult>());
		return;
	}

	std::optional<std::vector<String>> SequenceKeys = Convert(InSequenceKeys);
	std::optional<String> KeyRegex					= Convert(InKeyRegex);
	std::optional<String> ReferenceType				= Convert(InReferenceType);
	std::optional<std::vector<String>> ReferenceIds = Convert(InReferenceIds);

	csp::services::ResponseHandlerPtr ResponseHandler
		= SequenceAPI->CreateHandler<SequencesResultCallback, SequencesResult, void, csp::services::DtoArray<chs::SequenceDto>>(Callback, nullptr);

	static_cast<chs::SequenceApi*>(SequenceAPI)
		->apiV1SequencesGet(SequenceKeys,			   // Keys
							KeyRegex,				   // Regex
							ReferenceType,			   // ReferenceType
							ReferenceIds,			   // ReferenceIds
							std::nullopt,			   // Skip
							std::nullopt,			   // Limit
							ResponseHandler,		   // ResponseHandler
							CancellationToken::Dummy() // CancellationToken
		);
}

void SequenceSystem::GetSequence(const String& SequenceKey, SequenceResultCallback Callback)
{
	csp::services::ResponseHandlerPtr ResponseHandler
		= SequenceAPI->CreateHandler<SequenceResultCallback, SequenceResult, void, chs::SequenceDto>(Callback, nullptr);

	static_cast<chs::SequenceApi*>(SequenceAPI)
		->apiV1SequencesKeysKeyGet(SequenceKey,				  // Key
								   ResponseHandler,			  // ResponseHandler
								   CancellationToken::Dummy() // CancellationToken
		);
}

void SequenceSystem::DeleteSequences(const Array<String>& InSequenceKeys, NullResultCallback Callback)
{
	csp::services::ResponseHandlerPtr ResponseHandler
		= SequenceAPI->CreateHandler<NullResultCallback, NullResult, void, csp::services::NullDto>(Callback,
																								   nullptr,
																								   csp::web::EResponseCodes::ResponseNoContent);

	std::vector<String> SequenceKeys = Convert(InSequenceKeys);
	static_cast<chs::SequenceApi*>(SequenceAPI)
		->apiV1SequencesKeysDelete(SequenceKeys,			  // Keys
								   ResponseHandler,			  // ResponseHandler
								   CancellationToken::Dummy() // CancellationToken)
		);
}

SequenceSystem::SequenceSystem() : SystemBase(), SequenceAPI(nullptr)
{
}

SequenceSystem::SequenceSystem(web::WebClient* InWebClient) : SystemBase(InWebClient)
{
	SequenceAPI = CSP_NEW chs::SequenceApi(InWebClient);
}

SequenceSystem::~SequenceSystem()
{
	CSP_DELETE(SequenceAPI);
}

} // namespace csp::systems
