import argparse
import os
import os.path
import shutil
import subprocess
import sys


def get_editor_path_cachefile_path():
    filename = "editor_filepath.cfg"

    if sys.platform == "win32":
        return f"{ os.getenv('LOCALAPPDATA') }/Magnopus/CSPForUnityUnitTests/{ filename }"

    if sys.platform == "darwin":
        return f"~/Library/Application Support/MagnopusCSPForUnityUnitTests/{ filename }"

    if sys.platform == "linux":
        return f"~/.local/share/MagnopusCSPForUnityUnitTests/{ filename }"


def get_unity_hub_path():
    if sys.platform == "win32":
        return "C:\\Program Files\\Unity Hub\\Unity Hub.exe"


def find_unity_installation():
    print("Finding compatible Unity installation...")
    result = subprocess.run(
        [get_unity_hub_path(), "--", "--headless", "editors", "-i"], shell=True, capture_output=True, text=True
    )
    installed_editors = reversed(result.stdout.strip().splitlines())

    for e in installed_editors:
        if "2020.3." in e or "2021.3" in e:
            version, path = e.split(",")
            version = version.strip()
            path = path.strip()[len("installed at ") :]

            print(f"Found { version } at '{ path }'")

            cachefile_path = get_editor_path_cachefile_path()
            cachefile_dir = os.path.dirname(cachefile_path)

            if not os.path.exists(cachefile_dir):
                os.makedirs(cachefile_dir)

            file = open(cachefile_path, "w")
            file.write(path)
            file.close()

            return

    print("Could not find compatible Unity installation! Please install any version of Unity 2020.3.")
    sys.exit(1)


def get_unity_installation():
    cachefile_path = get_editor_path_cachefile_path()

    if not os.path.exists(cachefile_path):
        find_unity_installation()

    file = open(cachefile_path, "r")
    editor_path = file.read()
    file.close()

    if not os.path.exists(editor_path):
        print("Unable to locate previously detected Unity installtion!")
        find_unity_installation()

    file = open(cachefile_path, "r")
    editor_path = file.read()
    file.close()

    return editor_path


def main():
    current_directory = os.path.dirname(os.path.realpath(__file__))
    project_path = os.path.join(current_directory, "csharp")

    print("Creating `CSP for Unity` dummy package...")

    dummy_package_path = os.path.join(project_path, "Packages", "connected-spaces-platform.dummy")
    dummy_package_source_path = os.path.join(dummy_package_path, "Source")
    dummy_package_generated_source_path = os.path.join(dummy_package_source_path, "generated")

    # Remove existing CSP dummy package
    shutil.rmtree(dummy_package_path, ignore_errors=True)

    # Recreate dummy package folder structure
    os.makedirs(dummy_package_generated_source_path)

    # Create dummy package json

    with open(os.path.join(dummy_package_path, "package.json"), "w") as f:
        f.write(
            """
{
  "name": "connected-spaces-platform.dummy",
  "displayName": "connected-spaces-platform.dummy",
  "unity": "2020.3",
  "version": "1.3.37",
  "description": "Exposes dummy code for unit-testing CSP for Unity",
  "license": "Apache-2.0",
  "dependencies": {
  }
}
        """
        )

    with open(os.path.join(dummy_package_source_path, "connected-spaces-platform.asmdef"), "w") as f:
        f.write(
            """
{
  "name": "ConnectedSpacesPlatform",
  "rootNamespace": "ConnectedSpacesPlatform",
  "references": [],
  "includePlatforms": [],
  "excludePlatforms": [],
  "allowUnsafeCode": true,
  "overrideReferences": false,
  "precompiledReferences": [],
  "autoReferenced": true,
  "defineConstraints": [],
  "versionDefines": [],
  "noEngineReferences": false
}

        """
        )

    # Copy CSP DLL and C# wrapper code into dummy package
    shutil.copy("../Binaries/x64/Debug/ConnectedSpacesPlatform_D.dll", dummy_package_path)

    csharp_src_path = "../../Library/CSharpWrapper/src/"
    files = os.listdir(csharp_src_path)

    for f in files:
        shutil.copy(os.path.join(csharp_src_path, f), dummy_package_source_path)

    csharp_src_path = "../../Tools/WrapperGenerator/Output/CSharp/"
    files = os.listdir(csharp_src_path)

    for f in files:
        shutil.copy(os.path.join(csharp_src_path, f), dummy_package_generated_source_path)

    parser = argparse.ArgumentParser()
    parser.add_argument("--test_output_path", default=os.path.join(current_directory, "unity_test_results.xml"))
    args = parser.parse_args()

    editor_path = get_unity_installation()

    print(f"Running .Net tests and outputting results to { args.test_output_path }...")
    subprocess.run(
        [
            editor_path,
            "-runTests",
            "-batchmode",
            "-projectPath",
            project_path,
            "-testResults",
            args.test_output_path,
            "-testPlatform",
            "PlayMode",
            "-nographics",
        ],
        shell=True,
    )
    print("Done!")


if __name__ == "__main__":
    main()
