"""
CSP SDK dependency deployer.

This stages the third-party dependencies required to distribute CSP as a static sdk
without requiring Conan in the consumer project.

This can be invoked using the following args at the conan install step:
--deployer=csp_sdk_deployer --deployer-folder=<output-directory>

This deployer performs the following actions:
- Copies static libraries
- Copies public dependency includes
- Copies dependency license and notices
- Produces a CMake manifest file that lists all dependencies
- Produces a csp-sdk-dependencies.json that describes which package each library comes from

The output structure of the deployer is:

    output-dir/
        lib/
            dependency libraries
        include/
            package/
                public headers
        licenses/
            package/
                licence and notice files
        cmake/
            CSPThirdPartyDependencies.cmake

The generated CSPThirdPartyDependencies manifest file defines:

    CSP_THIRD_PARTY_DEPENDENCY_LIBRARY_FILENAMES

        A list of library filenames relative to the staged lib directory.

    CSP_THIRD_PARTY_PUBLIC_INCLUDE_DIRECTORIES

        A list of include directories relative to the staged include directory.

The main CSP CMake project reads the generated manifest and installs these files into
the SDK's final lib/third-party, include/third-party, and license directories.
"""

from __future__ import annotations

import json
import shutil
from pathlib import Path
from typing import Any


STATIC_LIBRARY_SUFFIXES = {
    ".lib",
    ".a",
}

# Only dependencies whose headers are required by CSP's installed public headers.
PUBLIC_HEADER_PACKAGES = {
    "fmt",
    "rapidjson",
    "asyncplusplus",
}

# We don't want to include certain packages
# gtest doesn't need to be included, as these are only used for our tests, which are not installed
EXCLUDED_PACKAGES = {
    "gtest",
}

# Ensures a file doesn't exist at the destination before copying
def _copy_file_unique(source: Path, destination: Path) -> Path:
    destination.mkdir(parents=True, exist_ok=True)

    output = destination / source.name

    if output.exists():
        if output.read_bytes() != source.read_bytes():
            raise RuntimeError(
                f"SDK deployment filename collision:\n"
                f"  Existing: {output}\n"
                f"  New:      {source}"
            )

        return output

    shutil.copy2(source, output)
    return output


# Recursively copy directory tree
def _copy_tree(source: Path, destination: Path) -> None:
    if not source.is_dir():
        return

    shutil.copytree(
        source,
        destination,
        dirs_exist_ok=True,
    )

def deploy(graph: Any, output_folder: str, **kwargs: Any) -> None:
    root_conanfile = graph.root.conanfile
    output_root = Path(output_folder).resolve()

    library_output = output_root / "lib"
    include_output = output_root / "include"
    licence_output = output_root / "licenses"

    deployed_libraries: list[str] = []
    deployed_include_directories: list[str] = []
    package_manifest: dict[str, dict[str, list[str]]] = {}

    # Iterate third-party dependencies
    for dependency in root_conanfile.dependencies.host.values():
        package_name = dependency.ref.name
        
        if package_name in EXCLUDED_PACKAGES:
            continue
        
        package_folder_value = dependency.package_folder

        if not package_folder_value:
            continue

        package_folder = Path(package_folder_value)

        # Used to track the static libraries copied for this package.
        # This is used to constuct the package manifest.
        package_libraries: list[str] = []

        # dependency.cpp_info.libdirs contains the library directories
        # Search recursively for static libs
        for library_directory_value in dependency.cpp_info.libdirs:
            library_directory = Path(library_directory_value)

            if not library_directory.is_dir():
                continue

            for source in library_directory.rglob("*"):
                if not source.is_file():
                    continue

                # Ensure file is a .lib or .a
                if source.suffix.lower() not in STATIC_LIBRARY_SUFFIXES:
                    continue

                # Copy into provided deployer-folder
                # Use # _copy_file_unique to throw if we find libs with the same name
                deployed = _copy_file_unique(
                    source,
                    library_output,
                )

                package_libraries.append(deployed.name)
                deployed_libraries.append(deployed.name)

        # Only include headers if they are public CSP dependencies
        if package_name in PUBLIC_HEADER_PACKAGES:
            package_include_output = include_output / package_name

            for include_directory_value in dependency.cpp_info.includedirs:
                include_directory = Path(include_directory_value)

                if include_directory.is_dir():
                    _copy_tree(
                        include_directory,
                        package_include_output,
                    )

            relative_include = package_name
            deployed_include_directories.append(relative_include)

        # License files can use various conventions, so ensure we don't miss them!
        LICENCE_FILE_PREFIXES = (
            "license",
            "licence",
            "copying",
            "copyright",
            "notice",
            "authors",
        )

        # Copy license files
        for licence_file in package_folder.rglob("*"):
            if not licence_file.is_file():
                continue
            
            if not licence_file.name.lower().startswith(LICENCE_FILE_PREFIXES):
                continue
            
            print(f"Copying licence: {licence_file}")

            _copy_file_unique(
                licence_file,
                licence_output / package_name,
            )

        package_manifest[package_name] = {
            "libraries": sorted(set(package_libraries)),
        }

    deployed_libraries = sorted(set(deployed_libraries))
    deployed_include_directories = sorted(
        set(deployed_include_directories)
    )

    manifest = {
        "libraries": deployed_libraries,
        "public_include_directories": deployed_include_directories,
        "packages": package_manifest,
    }

    # Write debug json manifest
    output_root.mkdir(parents=True, exist_ok=True)

    with (output_root / "csp-sdk-dependencies.json").open(
        "w",
        encoding="utf-8",
    ) as manifest_file:
        json.dump(
            manifest,
            manifest_file,
            indent=2,
            sort_keys=True,
        )

    # Write CMake manifest
    cmake_manifest = output_root / "CSPThirdPartyDependencies.cmake"

    with cmake_manifest.open("w", encoding="utf-8") as cmake_file:
        cmake_file.write(
            "# Generated by csp_sdk_deployer.py. Do not edit.\n\n"
        )

        cmake_file.write(
            "set(CSP_THIRD_PARTY_DEPENDENCY_LIBRARY_FILENAMES\n"
        )

        for library in deployed_libraries:
            cmake_file.write(f'    "{library}"\n')

        cmake_file.write(")\n\n")

        cmake_file.write(
            "set(CSP_THIRD_PARTY_PUBLIC_INCLUDE_DIRECTORIES\n"
        )

        for include_directory in deployed_include_directories:
            cmake_file.write(f'    "{include_directory}"\n')

        cmake_file.write(")\n")