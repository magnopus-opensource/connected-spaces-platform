from dataclasses import dataclass
from typing import List


@dataclass
class DefaultArtifactPathsConfig:
    windows: str
    macosx: str
    ios: str
    visionos: str
    android: str
    android64: str
    wasm: str
    csharp: List[str]


@dataclass
class ArtifactsConfig:
    windows: List[str]
    macosx: List[str]
    ios: List[str]
    visionos: List[str]
    android: List[str]
    android64: List[str]
    wasm: List[str]


@dataclass
class Config:
    default_artifact_paths: DefaultArtifactPathsConfig
    default_output_directory: str
    artifacts: ArtifactsConfig


###############################
#  CONFIGURATION STARTS HERE  #
###############################


config = Config(
    default_artifact_paths=DefaultArtifactPathsConfig(
        windows="Library/Binaries/x64/ReleaseDLL",
        macosx="Library/Binaries/macosx/ReleaseDLL",
        ios="Library/Binaries/ios/ReleaseStatic",
        visionos="Library/Binaries/visionos/ReleaseStatic",
        android="ARMv7/ReleaseDLL Android",
        android64="ARM64/ReleaseDLL Android",
        wasm="Library/Binaries/wasm/Release",
        csharp=["Library/CSharpWrapper/src", "Tools/WrapperGenerator/Output/CSharp"],
    ),
    default_output_directory="package",
    artifacts=ArtifactsConfig(
        windows=[
            "ConnectedSpacesPlatform.dll",
            "ConnectedSpacesPlatform_D.dll"
        ],
        macosx=[
            "libConnectedSpacesPlatform.dylib",
        ],
        ios=[
            "libConnectedSpacesPlatform.a",
            "libConnectedSpacesPlatform_D.zip",
            "libcrypto.a",
            "libssl.a"
        ],
        visionos=[
            "libConnectedSpacesPlatform.a",
            "libcrypto.a",
            "libssl.a"
        ],
        android=[
            "libConnectedSpacesPlatform.so",
            "libConnectedSpacesPlatform_D.so"
        ],
        android64=[
            "libConnectedSpacesPlatform.so",
            "libConnectedSpacesPlatform_D.so"
        ],
        wasm=[
            "ConnectedSpacesPlatform_WASM.js",
            "ConnectedSpacesPlatform_WASM.wasm",
            "ConnectedSpacesPlatform_WASM.worker.js"
        ],
    ),
)
