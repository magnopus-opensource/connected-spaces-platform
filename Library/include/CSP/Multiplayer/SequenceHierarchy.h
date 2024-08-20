#pragma once

#include "CSP/CSPCommon.h"
#include "CSP/Common/Array.h"
#include "CSP/Systems/SystemsResult.h"

namespace csp::systems
{
class Sequence;
}

namespace csp::multiplayer
{

common::String CreateSequenceKey(csp::common::Optional<uint64_t> ParentId, const csp::common::String& SpaceId);

/// @ingroup Space Entity System
class CSP_API SequenceHierarchy
{
public:
	// Entity ids of this sequence
	csp::common::Array<uint64_t> Ids;

	// If false, this is the root hierarchy
	bool HasParent() const;

	// ParentId of the sequence, 0 if this is the root hierarchy
	uint64_t ParentId = 0;
};

void SequenceToSequenceHierarchy(const systems::Sequence& Sequence, SequenceHierarchy& Hierarchy);

/// @ingroup Space Entity System
/// @brief Data class used to contain information when attempting to get a sequence hierarchy
class CSP_API SequenceHierarchyResult : public csp::systems::ResultBase
{
	/** @cond DO_NOT_DOCUMENT */
	friend class SpaceEntitySystem;

	CSP_START_IGNORE
	template <typename T, typename U, typename V, typename W> friend class csp::services::ApiResponseHandler;
	CSP_END_IGNORE
	/** @endcond */

public:
	/// @brief Retreives the SequenceHierarchy from the result.
	const SequenceHierarchy& GetSequenceHierarchy() const;

	CSP_NO_EXPORT SequenceHierarchyResult(csp::systems::EResultCode ResCode, uint16_t HttpResCode)
		: csp::systems::ResultBase(ResCode, HttpResCode) {};

private:
	SequenceHierarchyResult(void*) {};

	void OnResponse(const csp::services::ApiResponseBase* ApiResponse) override;

	SequenceHierarchy SequenceHierarchy;
};

typedef std::function<void(const SequenceHierarchyResult& Result)> SequenceHierarchyResultCallback;

} // namespace csp::multiplayer
