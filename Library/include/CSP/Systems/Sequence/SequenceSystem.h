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


#include "CSP/CSPCommon.h"
#include "CSP/Common/String.h"
#include "CSP/Systems/Sequence/Sequence.h"
#include "CSP/Systems/SystemBase.h"

namespace csp::web
{

class WebClient;

} // namespace csp::web


namespace csp::memory
{

CSP_START_IGNORE
template <typename T> void Delete(T* Ptr);
CSP_END_IGNORE

} // namespace csp::memory

namespace csp::systems
{
/// @ingroup Sequence System
/// @brief Public facing system that allows the management of groupings of items in a space.
class CSP_API SequenceSystem : public SystemBase
{
	CSP_START_IGNORE
	/** @cond DO_NOT_DOCUMENT */
	friend class SystemsManager;
	friend void csp::memory::Delete<SequenceSystem>(SequenceSystem* Ptr);
	/** @endcond */
	CSP_END_IGNORE
public:
	/// @brief
	/// @param SequenceKey csp::common::String : The unique grouping name. The suggested convention is: Type:[Id]
	/// @param ReferenceType csp::common::String : The type of reference (GroupId, SpaceId etc.)
	/// @param ReferenceId csp::common::String : The id of the reference
	/// @param Items csp::common::Array<csp::common::String> : An ordered array of members
	/// @param Callback SequenceResultCallback : callback to call when a response is received
	void CreateSequence(const csp::common::String& SequenceKey,
						const csp::common::String& ReferenceType,
						const csp::common::String& ReferenceId,
						const csp::common::Array<csp::common::String>& Items,
						SequenceResultCallback Callback);

	/// @brief
	/// @param SequenceKey csp::common::String : The unique grouping name. The suggested convention is: Type:[Id]
	/// @param ReferenceType csp::common::String : The type of reference (GroupId, SpaceId etc.)
	/// @param ReferenceId csp::common::String : The id of the reference
	/// @param Items csp::common::Array<csp::common::String> : An ordered array of members
	/// @param Callback SequenceResultCallback : callback to call when a response is received
	void UpdateSequence(const csp::common::String& SequenceKey,
						const csp::common::String& ReferenceType,
						const csp::common::String& ReferenceId,
						const csp::common::Array<csp::common::String>& Items,
						SequenceResultCallback Callback);

	/// @brief Renames a given sequence. This call will fail if the user isn't a creator of the space.
	/// @param OldSequenceKey csp::common::String : The current sequence key name
	/// @param NewSequenceKey csp::common::String : The new sequence key name
	/// @param Callback SequenceResultCallback : callback to call when a response is received
	void RenameSequence(const csp::common::String& OldSequenceKey, const csp::common::String& NewSequenceKey, SequenceResultCallback Callback);

	/// @brief Finds sequences based on the given criteria
	/// @param SequenceKeys csp::common::Array<csp::common::String> : An array of sequence keys to search for
	/// @param SequenceKeys csp::common::Optional<csp::common::String> : An optional regex string for searching keys
	/// @param ReferenceType csp::common::String : The type of reference (GroupId, SpaceId etc.). Must be used with ReferenceIds
	/// @param ReferenceIds csp::common::Optional<csp::common::Array<csp::common::String>> : The ids of the reference. Must be used with ReferenceType
	/// @param Callback SequencesResultCallback : callback to call when a response is received
	void GetSequencesByCriteria(const csp::common::Optional<csp::common::Array<csp::common::String>>& SequenceKeys,
								const csp::common::Optional<csp::common::String>& KeyRegex,
								const csp::common::Optional<csp::common::String>& ReferenceType,
								const csp::common::Optional<csp::common::Array<csp::common::String>>& ReferenceIds,
								SequencesResultCallback Callback);

	/// @brief Gets a sequence by it's key
	/// @param SequenceKey csp::common::String : The unique grouping name
	/// @param Callback SequenceResultCallback : callback to call when a response is received
	void GetSequence(const csp::common::String& SequenceKey, SequenceResultCallback Callback);

	/// @brief Deletes the given sequences. This call will fail if the user isn't a creator of the space
	/// @param SequenceKeys csp::common::Array<csp::common::String> : An array of sequence keys to delete
	/// @param Callback NullResultCallback : callback to call when a response is received
	void DeleteSequences(const csp::common::Array<csp::common::String>& SequenceKeys, NullResultCallback Callback);

private:
	SequenceSystem(); // This constructor is only provided to appease the wrapper generator and should not be used
	SequenceSystem(csp::web::WebClient* InWebClient);
	~SequenceSystem();

	csp::services::ApiBase* SequenceAPI;
};
} // namespace csp::systems
