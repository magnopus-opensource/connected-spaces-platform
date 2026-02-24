# Changelog

All notable changes to this project will be documented in this file. For compiled binaries, deployment packages, and version-specific artifacts, please visit our [GitHub Releases](https://github.com/magnopus-opensource/connected-spaces-platform/releases).

## [6.28.0]

### 🍰 🙌 New Features

- [OF-1818] feat: Update AsyncCompletedEventCallback format by MAG-AdamThorn
  The structure of the AsyncCallCompletedEvent has been updated by the backend services to include additional properties. `ReferenceId` and `ReferenceType` are being replaced by a new `References` Map, and new `Status` and `StatusReason` properties have been added. This change is being made to enable the backend services to communicate more information about the async call. For example in the case of `DuplicateSpaceAsync`, the new References map will contain both the Id of the old space as well as the newly duplicated one. This change is currently behind a backend feature flag. As this will be a breaking change we have added these new properties to the event, but will temporarily be keeping the old ones. We are populating both the old and new properties when we deserialise the SignalR event values, which means that Clients will continue to be able to consume the event as before. Once this CSP change has been adopted by clients we will remove the old properties and transition logic.

## [6.27.0]

### 🙈 🙉 🙊 Test Changes

- [NT-0] chore: Remove analytics flush log by MAG-ElliotMorris
  A verbose log was added to the AnalyticsSystem::FlushAnalyticsEventsQueue method for instances where the method was called when the queue was empty. However, this method is called on Tick once a given period of time has elapsed since the queue was last sent, which resulted in log spamming. The log has been removed.

## [6.26.0]

### 🐛 🔨 Bug Fixes

- [NT-0] fix: Fixed crash when loading older materials by MAG-mv
  We weren't checking if a property existed before deserializing which was causing a crash with older materials.

- [OB-4151] fix: Address AnalyticsSystem hang after flush of empty queue by MAG-AdamThorn
  Ensure that if the `AnalyticsSystem::FlushAnalyticsEventsQueue()` method is called with an empty queue, that the method fires the passed callback with a valid response.

## [6.25.0]

### 🍰 🙌 New Features

- [OPE-3119] feat: Add equality and inequality operators to several types, in support of explicitly instantiating template container exports by MAG-ElliotMorris.
  This has been done because of a single sticky Map<T, Array<U>> type, which obviously needs equality for Array<U>, which means
  all U's need equality. All equality operations are standard memberwise == comparison, of the like a c++20 =default would generate.
  The following types have been given new operators:
    - MessageInfo
    - ComponentUpdateInfo
    - Asset
    - AssetCollection
    - CurrencyInfo
    - ProductMediaInfo
    - VariantOptionInfo
    - ProductVariantInfo
    - ProductInfo
    - CartLine
    - ShopifyStoreInfo
    - TicketedEvent
    - HotspotGroup
    - Scope
    - FeatureLimitInfo
    - UserTierInfo
    - FeatureQuotaInfo
    - Sequence
    - VersionMetadata
    - ServiceStatus
    - ServicesDeploymentStatus
    - Site
    - BasicSpace
    - Space
    - UserRoleInfo
    - InviteUserRoleInfo
    - OlyAnchorPosition
    - OlyRotation
    - Anchor
    - AnchorResolution
    - PointOfInterest
    - BasicProfile
    - Profile
    - Array<T>

## [6.24.0]

### 🍰 🙌 New Features

- [NT-0] feat: Add isStereoFlipped property to Video component and texture material by MAG-JamesEdgeworth
  This property allows users to specify whether the left and right eye videos should be flipped when rendering stereoscopic video. This is useful for supporting videos that may have been encoded with different stereo formats, ensuring correct playback in the appropriate format.

### 💫 💥 Code Refactors

- [NT-0] refac: Moved enum StereoVideoType to SharedEnums StereoVideoType by MAG-JamesEdgeworth
  This change moves the StereoVideoType enum from the VideoComponent to the SharedEnums header, making it more accessible for use across different components and systems that may need to reference stereo video types.
  
### 🔥 ❗Breaking Changes

- [OF-1821] feat!: Pass `LocomotionModel` on init via `CreateAvatar` by mag-lt
  When creating an Avatar, a `csp::multiplayer::LocomotionModel` must now be passed. This is a breaking change.
  To obtain the same behaviour as before, `csp::multiplayer::LocomotionModel::Grounded` should be provided.

## [6.23.0]

### 🐛 🔨 Bug Fixes

- [OB-5015] fix: Leader election fixes by MAG-mv
  This addresses both OB-5015 and OB-5019. OB-5019 - We were always passing the local client id when registering the default scope, so local client always assumed it was the leader. OB-5015 - We were not deregistering the remote script event when leader election was disabled, which was causing the event to attempt to access a dangling realtimeengine pointer.

## [6.22.0]

### 🔥 ❗ Breaking Changes

- [OB-4350] fix!: Improved log output for unmodifiable entities by MAG-mav
  This improves the clarity of logs that are output when an unmodifiable entity is attempted to be modified.
  SpaceEntity::IsModifiable now returns an enum, specifying the reason.
  Also adds IRealtimeEngine::IsEntityModifiable and derived functions in all realtime engine implementations.
  Also adds SpaceEntity::IsModifiableWithReason which acts as a wrapper around the above functions.
  
### 💫 💥 Code Refactors

- [NT-0] refac: Remove use of metadata for storing Hotspot Sequence keys by MAG-AdamThorn
  When creating a new HotspotGroup, the name property has the sequence type ("Hotspots") and SpaceId appended. The original name was also being stored in the sequence metadata. This was causing issues with renaming Hotspot sequences or when retrieving on Space entry. This change removes use of metadata. As part of this change `StartsWith`, `EndsWith` and `SubString` String utility methods have also be added.

## [6.21.0]

### 🍰 🙌 New Features

- [NT-0] feat: Add `SetUserTier` method to `QuotaSystem` by MAG-ElliotMorris
  This method provides a means of changing a users tier (basic, pro, enterprise, etc)
  through the api. This method will only succeed when logged in as an admin user.
  This method is mostly for internal testing, but there is no harm allowing public use.
  
  This method provides a means of changing a users tier (basic, pro, enterprise, etc) through the api. This method will only succeed when logged in as an admin user. This method is mostly for internal testing, but there is no harm allowing public use.

### 🔨 🔨 Chore

- [OPE-3056] chore: Add exported symbols, equality operators, and hash implementation
  for types required for the common modules in the C# SWIG wrapper. This will be a 
  common theme going forward.
    - Exported Map<String,ReplicatedValue>, and a gamut of Optional<T> arithmetic type instantiations
    - Add `iterator` and `reverse_iterator` typedefs and implementation to `List`
    - Add `std::hash` specializations for `String`, `Array<T>`, `List<T>`, `Map<T>`, `ReplicatedValue`, `ApplicationSettings` and `SettingsCollection`.

### 🐛 🔨 Bug Fixes

- [OB-5015] fix: Fixed crash with server-side leader election after exiting space by MAG-MV
  Internal CSP event was not being deregistered when exiting a space, sometimes causing a crash.

- [OB-5019] fix: Fixed server-side leader election leader change when a new client enters space by MAG-mv
  An incorrect client id was being used internally, causing the new client to always think it is leader.

## [6.20.0]

### 🔥 ❗ Breaking Changes

- [OB-4154] fix!: Improve how Hotspot sequence event data is handled by MAG-AdamThorn
  The existing implementation of `HotspotSequenceChangedNetworkEventData` was unnecessarily complex and confusing to developers. This event type has now been removed and new `SpaceId` and `SequenceType` properties have been added to the `SequenceChangedNetworkEventData`.
  This is a breaking change:
  * `SequenceChangedNetworkEventData` no longer has a `HotspotData` member.
  * `SequenceChangedNetworkEventData` has two new members, `SpaceId` (String) and `SequenceType` (ESequenceType enum).
  * When renaming a Hotspot Sequence, access to the old and new names has been updated as follows:
      * SequenceChangedNetworkEventData.HotspotData.Name > SequenceChangedNetworkEventData.Key
	  * SequenceChangedNetworkEventData.HotspotData.NewName > SequenceChangedNetworkEventData.NewKey

## [6.19.0]

### 🍰 🙌 New Features

- [NT-0] feat: Exposed equality and inequality operators for `Vector2`, `Vector3`, `Vector4` and `Map` by MAG-ElliotMorris.
  Also added hash functions in the `std` namespace for the Vector types.
  
### 🐛 🔨 Bug Fixes

- [OB-4123] fix: Fix some text rendering as black/unreadable in generated docs.

## [6.17.0]

### 🐛 🔨 Bug Fixes

- [NT-0] fix: Address rare race condition in legacy leadership election by MAG-ElliotMorris
  It was possible to trigger a memory race using `EnableLeaderElection` or `DisableLeaderElection` after having returned from `EnterSpace` in the OnlineRealtimeEngine.
  This is due to the leader election being performed in the all entities retreived function, which can come back asyncronously to enable asset streaming, giving a chance for threading syncronisation error.
  Solved with a mutex for now, although the best solve is to move leader election elsewhere.

- [OW-2312] fix: Add select_account for authorise URL prompt parameter.
  Google made changes to the way the oauth URL works, so we need to set prompt to select_account rather than none to restore the functionality. This change updates that parameter in the GetThirdPartyProviderAuthoriseURL function.
- [OPE-2982] fix: Unity Android debug builds crash on login. Caused by Android not being able to find the CSP library file.

## [6.15.0]

### 🔥 ❗Breaking Changes

- [OB-4579] fix!: Issues around renaming hotspot groups by MAG-ChristopherAtkinson
  Rename event has been deprecated and removed at the services level. This is a breaking change due to the removal of the csp::common::ESequenceUpdateType::Rename.

### 🍰 🙌 New Features

- [OF-1758] feat: replace assertion in token expiration with meaningful log to notify users by MAG-ChristopherAtkinson
  RefreshIfExpired invokes a fatal log message in place of the existing assert as asserting on a refresh token failure is too aggressive.
  
### 🙈 🙉 🙊 Test Changes

- [NT-0] test: Await exiting spaces in certain realtime engine tests by MAG-ElliotMorris.
  Correct usage of the realtime engine requires that the exit space call returns fully before deleting the memory, these tests did not do that.

## [6.14.0]

### 🔥 ❗Breaking Changes

- [OF-1806] refac!: Add a new URL property and deprecate others in the AvatarSpaceComponent in https://github.com/magnopus-opensource/connected-spaces-platform/pull/872
  Simplify the setting and getting of the mesh URL for the avatar component. This is a breaking change due to the removal of the getters and setters for CustomAvatarUrl and AvatarMeshIndex.

## [6.13.0]

### 🔥 ❗Breaking Changes

- [NT-0] feat!: expose refresh token expiry length by MAG-ChristopherAtkinson in https://github.com/magnopus-opensource/connected-spaces-platform/pull/863
  Expose RefreshTokenExpiryLength to the token options to support clients configuring custom durations.

### 🔨 🔨 Chore

- [NT-0] chore: Change 'Dirty components map already contains key' log to very verbose by MAG-ThomasGreenhalgh in https://github.com/magnopus-opensource/connected-spaces-platform/pull/874
  Changed the log level of the _'Dirty components map already contains key'_ message emitted from `SpaceEntityStatePatcher::SetDirtyComponent` to `VeryVerbose`.

## [6.12.0]

### 🍰 🙌 New Features

- [NT-0] feat: Add non-exported iterators to Map by MAG-ElliotMorris in https://github.com/magnopus-opensource/connected-spaces-platform/pull/859
  Add non-exported iterators in support of the SWIG generated interop API work.

### 🐛 🔨 Bug Fixes

- [OB-4761] fix: unreal crashes on oko asset drag and drop without internet connection by MAG-ChristopherAtkinson in https://github.com/magnopus-opensource/connected-spaces-platform/pull/860
  Ensures the correct return path when attempting to create an entity in OnlineRealtimeEngine.

### 🙈 🙉 🙊 Test Changes

- [NT-0] test: Update QuotaSystem test by MAG-AdamThorn in https://github.com/magnopus-opensource/connected-spaces-platform/pull/862
  Updated the QuotaSystem `GetTierFeaturesQuota` test to account for the addition of the new `GoogleGenAI` Quota tier feature in MCS.

## [6.11.0]

### 🍰 🙌 New Features

- [OF-1807] feat: support text asset type to support ai chatbot by MAG-ChristopherAtkinson in https://github.com/magnopus-opensource/connected-spaces-platform/pull/853
  Introduce a new TEXT EAssetType to support storage of prompt and guardrail assets for the AI chatbot component.
- [OF-1784] feat: Multiplayer System by MAG-mv in https://github.com/magnopus-opensource/connected-spaces-platform/pull/842
  Part 1 of the server-side leader election work. Addition of a new Multiplayer System which exposes the necessary functionality for leader election from the mcs MultiplayerServices api.

### 🐛 🔨 Bug Fixes

- [NT-0] fix: List::Insert erasing 0th element by MAG-ElliotMorris in https://github.com/magnopus-opensource/connected-spaces-platform/pull/857
  Inserting elements into a List was always adding them as the 0th index rather than at the position specified.
- [OB-4723] fix: exceptions in continuations return incorrect result by MAG-ChristopherAtkinson in https://github.com/magnopus-opensource/connected-spaces-platform/pull/846
  In the continuation flow, we would return an invalid result and therefore lose the associated result data. This change ensures the result data from the exception is preserved.
- [NT-0] fix: Use const string ref for return type by MAG-AlessioRegalbuto in https://github.com/magnopus-opensource/connected-spaces-platform/pull/856
  As part of ongoing Unity interop API work, we updated the 'MimeTypeHelper' to return a const ref string for the mime type to ensure correct SWIG code generation.
- [OF-1524] fix: Wasm callbacks are now automatically forwarded to the main thread by MAG-mv in https://github.com/magnopus-opensource/connected-spaces-platform/pull/806
  This fixes crashes with the log callback if UserSystem::Login fails, and also the MultiplayerConnection::ConnectionInterrupted callback on wasm builds.
- [OB-4961] fix: Some spaces could not find the default scope. by MAG-mv in https://github.com/magnopus-opensource/connected-spaces-platform/pull/886
  Fixes issue with EnterSpace with older spaces using an unexpected format for the scope name.

### 🔨 🔨 Chore

- [NT-0] chore: Finish NO_EXPORT heirarchy by MAG-ElliotMorris in https://github.com/magnopus-opensource/connected-spaces-platform/pull/848
  Adds NO_EXPORT blocks to symbols that derive from, or are dependent on, base constructs that also have NO_EXPORT. Done in service of the Unity Interop API SWIG initiative.

## Historical releases

In November 2025 we migrated to continuous nightly deployments. Please see the [releases](https://github.com/magnopus-opensource/connected-spaces-platform/releases) section of the `connected-spaces-platform` repo for changelogs associated with earlier releases.

## Changelog categories

Changelog categories are listed below. Please use the appropriate unicode symbol and heading for your change.

🔥 ❗Breaking Changes

🍰 🙌 New Features

🐛 🔨 Bug Fixes

💫 💥 Code Refactors

🙈 🙉 🙊 Test Changes

📔 📔 Documentation

🔨 🔨 Chore

🔩 🔧 Ci
