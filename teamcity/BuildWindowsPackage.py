"""
This script packages the required Win64 lib files and include directory.
It is used by TeamCity as part of the CSP Feature publish build config.
"""

import os
import shutil

lib_source_dir = "Library\\Binaries\\x64\\Win64-CSharp"
lib_destination_dir = "Library\\Windows_Package\\lib"
include_source_dir = "Library\\Binaries\\include"
include_destination_dir = "Library\\Windows_Package\\include"

lib_files = ["ConnectedSpacesPlatform.dll", "ConnectedSpacesPlatform.lib", "ConnectedSpacesPlatform_D.dll", "ConnectedSpacesPlatform_D.lib", "ConnectedSpacesPlatform_D.pdb"]

try:
    os.makedirs(lib_destination_dir, exist_ok=True)
except OSError as e:
    print(f"{e}")

try:
    os.makedirs(include_destination_dir, exist_ok=True)
except OSError as e:
    print(f"{e}")


for file in lib_files:    
    shutil.copy2(os.path.join(lib_source_dir, file), os.path.join(lib_destination_dir, file))


shutil.copytree(include_source_dir, include_destination_dir, dirs_exist_ok=True)
      