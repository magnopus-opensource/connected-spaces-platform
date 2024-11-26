# C++

To get started with Connected Spaces Platform using C++, head on over to the [Releases](https://github.com/magnopus-opensource/connected-spaces-platform/releases) and grab the latest available version for your platform. We provide shared binaries for Windows, macOS, and Android, and static libraries for macOS and iOS.

## Adding Connected Spaces Platform to your project

1. Add the Connected Spaces Platform header files to your include paths.
2. Link against `ConnectedSpacesPlatform` (release) or `ConnectedSpacesPlatform_D` (debug).
3. If linking against a shared build of Connected Spaces Platform, define `USING_CSP_DLL` before including any CSP headers. A good place to do this is in your project settings.