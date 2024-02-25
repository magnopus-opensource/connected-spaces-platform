# Built from changelist ID 8b94eb8d4af160ad5a400d3e39e64316ec23f0e0

<br>

## Summary Lists


### 🔥 ❗ Breaking Changes

- Rename `loginwithtoken` *(27016b9)* [**OF-1047**]
- Expose multiplayer errors *(0e1a844)* [**OB-3069**]


### 🍰 🙌 New Features

- Add avatar info settings *(485b064)* [**OF-793**]
- Wrap `[[deprecated]]` in wrappers *(7e8b589)* [**OF-665**]


### 🐛 🔨 Bug Fixes

- Add getuniquestring (#288) *(3272e6a)* [**OF-1066**]
- Update unity examples (#293) *(00a231e)* [**OSB-10**]
- Remove c style casting (#295) *(bb0f889)* [**OF-1196**]
- Fix .clangformat for latest clang ver *(704cb1f)* [**No Ticket**]
- Python preq installation *(40004a7)* [**No Ticket**]
- More interface implementations *(ebe4ab1)* [**No Ticket**]
- Update module dependencies (#315) *(beb8fa8)* [**No Ticket**]



### 💫 💥 Code Refactors

- Update c++ basicframework example to 4.17.1 (#292) *(d098dc3)* [**OF-1205**]
- Update web examples to 4.17.1 (#291) *(6a182cf)* [**OW-1567**]
- Remove generatereadme (#294) *(c5b155f)* [**OF-1189**]
- Optimise header transform *(d152116)* [**OF-1166**]
- Removing `entitysystemversion` (#301) *(60dffa1)* [**OF-955**]
- Component update documentation (#303) *(b6c9055)* [**OF-1185**]
- Implement transform interfaces *(31bf625)* [**OF-1214**]
- Export all `logsystem` functions *(cdaf2cd)* [**OF-1155**]


### 🙈 🙉 🙊 Test Changes

- Coverage of usersystem (#290) *(86aa2b9)* [**OF-1083**]
- Add tests for apiresponsebase (#300) *(320bcb3)* [**OF-1086**]


### 📖 ✍ Documentation Changes

- Documenting our approach to updating the examples (#287) *(49f2614)* [**No Ticket**]



<br>

## Revision Change Details

| Change ID | Author | Description |
| --------- | ------ | ----------- |
| 49f2614 | Sam Birley | [NT-0] doc: Documenting our approach to updating the examples (#287)<br>Updating the readme to call out that the examples are guaranteed to be updated with each major version update of CSP. |
| 3272e6a | Mag-JB | [OF-1066] fix: Add GetUniqueString (#288)<br>&bull; [OF-1066] fix: Add GetUniqueString<br>&bull; [OF-1066] fix: Add alphanumeric func |
| d098dc3 | AdamThorn | [OF-1205] refac: Update C++ BasicFramework example to 4.17.1 (#292)<br>Updated the C++ BasicFramework example to use CSP 4.17.1. |
| 6a182cf | Thomas Greenhalgh | [OW-1567] refac: Update web examples to 4.17.1 (#291) |
| 00a231e | Mag-JB | [OSB-10] fix: update unity examples (#293) |
| c5b155f | Mag-JB | [OF-1189] refac: remove GenerateReadMe (#294) |
| bb0f889 | Mag-JB | [OF-1196] fix: Remove C style casting (#295) |
| 86aa2b9 | MAG-AW | [OF-1083] test: coverage of UserSystem (#290)<br>&bull; [OF-1083] test: coverage of UserSystem<br>&bull; [OF-1083] test: fixed invalid test case<br>&bull; [OF-1083] test: Changed test name |
| 27016b9 | Michael K | [OF-1047] refac!: rename `LoginWithToken`<br>BREAKING CHANGE: `UserSystem::LoginWithToken` has been renamed to<br>`UserSystem::RefreshSession`. Parameter types have not changed. |
| 704cb1f | Michael K | [NT-0] fix: fix .clangformat for latest clang ver |
| 0e1a844 | Michael K | [OB-3069] refac!: Expose multiplayer errors<br>This change addresses a lack of error handling in our multiplayer code.<br>A lot of calls return values were being ignored, which resulted in<br>internal errors that weren't properly handled. It also exposes a way<br>for the user to handle errors.<br>BREAKING CHANGE: This changes a lot of `MultiplayerConnection`<br>endpoints to take an `ErrorCodeCallbackHandler` instead of a<br>`CallbackHandler`. This callback type gets an `ErrorCode` instead of a<br>`bool` passed as the only parameter. This value should be compared to<br>`ErrorCode::None` to ensure the call succeeded. |
| d152116 | Michael K | [OF-1166] refac: optimise header transform<br>This change removes an extra allocation in the code that transforms<br>header keys and values to lower-case in `EmscriptenWebClient`. To<br>support this, a new `String::ToLower` function was added which returns<br>a copy of the string converted to lower-case. |
| 60dffa1 | Sam Birley | [OF-955] refac: removing `EntitySystemVersion` (#301)<br>This API variable still exists from an old versioning pass which has<br>been removed.<br>While we do want to do this versioning style in future,<br>we are removing this from the code base now to prevent<br>confusion down the line. |
| 40004a7 | Sam Birley | [NT-0] fix: python preq installation<br>On windows, the python prerequisites installed during at the start of solution generation now invokes the same python commands as on OSX. |
| 320bcb3 | MAG-AW | [OF-1086] test: Add tests for APIResponseBase (#300) |
| b6c9055 | MAG-AW | [OF-1185] refac: Component Update documentation (#303)<br>&bull; [OF-1185] refac: Component Update documentation<br>- Improved comments for Component Update flow<br>- Removed redundant property info class which is unused.<br>- Removed commented references to it also.<br>&bull; [OF-1185] refac: removed unneeded template |
| 485b064 | Michael K | [OF-793] feat: add Avatar info settings<br>This change adds `SettingsSystem::SetAvatarInfo()` and<br>`SettingsSystem::GetAvatarInfo()` to allow clients to set persistent<br>Avatar values that can be retrieved outside of the Multiplayer system. |
| ebe4ab1 | Michael K | [NT-0] fix: more interface implementations<br>This change implements `ITransformComponent` for `ImageSpaceComponent`<br>and `IPositionComponent` for `AudioSpaceComponent`. |
| 31bf625 | Michael K | [OF-1214] refac: implement transform interfaces |
| 7e8b589 | Michael K | [OF-665] feat: wrap `[[deprecated]]` in wrappers |
| b5fdb28 | Kuang-Hsin Liu | [OPE-1504] Update examples to Unity 2022 LTS<br>&bull; updated Unity example Initialising Foundation to Unity 2022.3.16f1<br>&bull; updated Unity example Basic Framework to Unity 2022.3.16f1<br>&bull; updated Unity example Multiplayer to Unity 2022.3.16f1<br>&bull; updated the Unity version info in README files |
| cdaf2cd | Michael K | [OF-1155] refac: export all `LogSystem` functions<br>NOTE: Tests are not needed for this as we currently already test<br>`LogSystem` in our C++ tests, and wrapper-related tests are handled by<br>our wrapper generator unit tests. |
| beb8fa8 | AdamThorn | [NT-0] fix: Update module dependencies (#315)<br>Updated requirements.txt to include the distutils<br>module dependency. |
| 1da5a66 | AdamThorn | Staging 4.18.0 (#322) (#323)<br>[NT-0] fix: Remove distutils dependency<br>The build agents have been updated to Python 3.12<br>and support distutils has been removed with that release.<br>Updating the script to use shutil.copy_tree instead. |