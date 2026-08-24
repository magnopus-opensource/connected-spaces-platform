"""
CSP SDK dependency deployer.

This deploys the third-party dependencies required for csp to be linked
without requiring Conan in the consumer project.

This can be invoked using the following args at the conan install step:
--deployer=csp_sdk_deployer --deployer-folder=<output-directory>

This deployer performs the following actions:
- Copies static libraries
- Copies dependency license and notices

The output structure of the deployer is:

    output-dir/
        lib/
            dependency libraries
        licenses/
            package/
                licence and notice files
"""

from __future__ import annotations

import shutil
from pathlib import Path
from typing import Any
import filecmp

STATIC_LIBRARY_SUFFIXES = {
    ".lib",
    ".a",
}

# License files can use various conventions, so ensure we don't miss them!
LICENCE_FILE_PREFIXES = (
    "license",
    "licence",
    "copying",
    "copyright",
    "notice",
    "authors",
)

# Copy a file into the destination directory without overwriting
# a different file with the same name.
# If a file with the same name already exists and has identical contents,
# the existing file is reused. If the contents differ, a RuntimeError is raised.
def _copy_file_unique(source: Path, destination: Path) -> Path:
    destination.mkdir(parents=True, exist_ok=True)

    output = destination / source.name

    if output.exists():
        if not filecmp.cmp(output, source, shallow=False):
            raise RuntimeError(
                f"filename collision:\n"
                f"  Existing: {output}\n"
                f"  New:      {source}"
            )

        return output

    shutil.copy2(source, output)
    return output

def deploy(graph: Any, output_folder: str, **kwargs: Any) -> None:
    root_conanfile = graph.root.conanfile
    output_root = Path(output_folder).resolve()

    library_output = output_root / "lib"
    licence_output = output_root / "licenses"

    deployed_libraries: list[str] = []

    # Iterate third-party dependencies
    for dependency in root_conanfile.dependencies.host.values():
        package_name = dependency.ref.name
        package_folder_value = dependency.package_folder

        if not package_folder_value:
            continue

        package_folder = Path(package_folder_value)

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
                deployed = _copy_file_unique(
                    source,
                    library_output,
                )

                deployed_libraries.append(deployed.name)

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

   