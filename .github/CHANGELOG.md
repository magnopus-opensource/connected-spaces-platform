# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### 🍰 🙌 New Features

- [NT-0] feat: Add non-exported iterators to Map by MAG-ElliotMorris in https://github.com/magnopus-opensource/connected-spaces-platform/pull/859
  Add non-exported iterators in support of the SWIG generated interop API work.

### 🐛 🔨 Bug Fixes

- [OB-4761] fix: unreal crashes on oko asset drag and drop without internet connection by MAG-ChristopherAtkinson in https://github.com/magnopus-opensource/connected-spaces-platform/pull/860
  Ensures the correct return path when attempting to create an entity in OnlineRealtimeEngine.

### 🙈 🙉 🙊 Test Changes

- [NT-0] test: Update QuotaSystem test by MAG-AdamThorn in https://github.com/magnopus-opensource/connected-spaces-platform/pull/862
  Updated the QuotaSystem `GetTierFeaturesQuota` test to account for the addition of the new `GoogleGenAI` Quota tier feature in MCS.

## [6.11.0] - 2025-11-18_11-14-15

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
