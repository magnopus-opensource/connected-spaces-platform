#include "CSP/Multiplayer/SequenceHierarchy.h"

#include "CSP/Systems/Sequence/Sequence.h"
#include "Multiplayer/MultiplayerConstants.h"

#include <string>

namespace csp::multiplayer
{

const SequenceHierarchy& csp::multiplayer::SequenceHierarchyResult::GetSequenceHierarchy() const
{
	return SequenceHierarchy;
}

void SequenceHierarchyResult::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
}

common::String CreateSequenceKey(common::Optional<uint64_t> ParentId, const common::String& SpaceId)
{
	common::String Key = csp::multiplayer::GetSequenceHierarchyName() + ":" + SpaceId;

	if (ParentId.HasValue())
	{
		Key += ":" + systems::SequenceIdPrefix + std::to_string(*ParentId).c_str();
	}

	return Key;
}

void SequenceToSequenceHierarchy(const systems::Sequence& Sequence, SequenceHierarchy& Hierarchy)
{
	if (Sequence.MetaData.HasKey("ParentId"))
	{
		Hierarchy.ParentId = std::stoull(Sequence.MetaData["ParentId"].c_str());
	}

	Hierarchy.Ids = common::Array<uint64_t>(Sequence.Items.Size());

	// Convert string ids to uint64_ts
	for (size_t i = 0; i < Hierarchy.Ids.Size(); ++i)
	{
		Hierarchy.Ids[i] = std::stoull(Sequence.Items[i].c_str());
	}
}
bool SequenceHierarchy::HasParent() const
{
	return ParentId != 0;
}

const csp::common::Array<SequenceHierarchy>& SequenceHierarchyCollectionResult::GetSequenceHierarchyCollection() const
{
	return SequenceHierarchyCollection;
}

void SequenceHierarchyCollectionResult ::OnResponse(const csp::services::ApiResponseBase* ApiResponse)
{
}
} // namespace csp::multiplayer
