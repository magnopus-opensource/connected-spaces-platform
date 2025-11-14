# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### 🍰 🙌 New Features

- [OF-1807] feat: support text asset type to support ai chatbot by MAG-ChristopherAtkinson in https://github.com/magnopus-opensource/connected-spaces-platform/pull/853
  Introduce a new TEXT EAssetType to support storage of prompt and guardrail assets for the AI chatbot component.

### 🐛 🔨 Bug Fixes

- [NT-0] fix: List::Insert erasing 0th element by MAG-ElliotMorris in https://github.com/magnopus-opensource/connected-spaces-platform/pull/850
  Inserting elements into a List was always adding them as the 0th index rather than at the position specified.
- [OB-4723] fix: exceptions in continuations return incorrect result by MAG-ChristopherAtkinson in https://github.com/magnopus-opensource/connected-spaces-platform/pull/846
  In the continuation flow, we would return an invalid result and therefore lose the associated result data. This change ensures the result data from the exception is preserved.
- [NT-0] fix: Use const string ref for return type by MAG-AlessioRegalbuto in https://github.com/magnopus-opensource/connected-spaces-platform/pull/856
  As part of ongoing Unity interop API work, we updated the 'MimeTypeHelper' to return a const ref string for the mime type to ensure correct SWIG code generation.

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
