import argparse
import os
import shutil
import subprocess


def main():
    # Ensure our current working directory is always this script's parent directory
    os.chdir(os.path.abspath(os.path.dirname(__file__)))

    current_directory = os.path.dirname(os.path.realpath(__file__))
    project_path = os.path.join(current_directory, "javascript")

    print("Creating `CSP for Web` dummy package...")

    dummy_package_path = os.path.join(
        project_path, "node_modules", "@magnopus-opensource", "connected-spaces-platform.dummy"
    )
    dummy_package_bin_path = os.path.join(dummy_package_path, "Debug")

    # Remove existing CSP dummy package
    shutil.rmtree(dummy_package_path, ignore_errors=True)

    # Recreate dummy package folder structure
    os.makedirs(dummy_package_bin_path)

    # Create dummy package json
    with open(os.path.join(dummy_package_path, "package.json"), "w") as f:
        f.write(
            """
{
  "name": "connected-spaces-platform.dummy",
  "displayName": "connected-spaces-platform.dummy",
  "version": "1.3.37",
  "description": "Exposes dummy code for unit-testing Connected Spaces Platform for Web",
  "license": "Apache-2.0",
  "dependencies": {
  },
  "main": "./connectedspacesplatform.js",
  "types": "./connectedspacesplatform.d.ts",
  "type": "module"
}
        """
        )

    # Copy CSP WASM bin and JS wrapper code into dummy package
    shutil.copy("../Binaries/wasm/Debug/ConnectedSpacesPlatform_WASM.js", dummy_package_bin_path)
    shutil.copy("../Binaries/wasm/Debug/ConnectedSpacesPlatform_WASM.wasm", dummy_package_bin_path)
    shutil.copy("../Binaries/wasm/Debug/ConnectedSpacesPlatform_WASM.worker.js", dummy_package_bin_path)

    shutil.copy("../../Tools/WrapperGenerator/Output/TypeScript/connectedspacesplatform.js", dummy_package_path)
    shutil.copy("../../Tools/WrapperGenerator/Output/TypeScript/connectedspacesplatform.js.map", dummy_package_path)
    shutil.copy("../../Tools/WrapperGenerator/Output/TypeScript/connectedspacesplatform.d.ts", dummy_package_path)
    shutil.copy("../../Tools/WrapperGenerator/Output/TypeScript/connectedspacesplatform.ts", dummy_package_path)

    parser = argparse.ArgumentParser()
    parser.add_argument("--test_output_path", default=os.path.join(current_directory, "node_test_results.xml"))
    args = parser.parse_args()

    print(f"Running Javascript tests and outputting results to { args.test_output_path }...")
    subprocess.run(["node", "javascript/index.js", "--test_output_path", args.test_output_path], shell=False)
    print("Done!")


if __name__ == "__main__":
    main()
