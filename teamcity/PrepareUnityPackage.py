#!/usr/bin/env python3

import argparse
import os
import shutil
import uuid

import git

from pathlib import Path

from jinja2 import Environment, FileSystemLoader

from Config import config

def get_git_root():
    repo = git.Repo(os.path.abspath(os.path.dirname(__file__)), search_parent_directories=True)
    root = repo.working_dir

    return root

def generate_guid():
    return uuid.uuid4().hex
    
def add_args(arg_parser):
    arg_parser.add_argument(
        "--output-directory",
        help="Enter the path relative to the git root for the libraries to be copied to.",
        default=f"{config.default_output_directory}/unity",
    )
    arg_parser.add_argument(
        "--name",
        help="Enter the name of the Connected Spaces Platform package.",
        default="connected-spaces-platform.unity.core",
    )
    arg_parser.add_argument(
        "--display-name",
        help="Enter the display name of the Connected Spaces Platform package.",
        default="connected-spaces-platform.unity.core",
    )
    arg_parser.add_argument(
        "--assembly-name",
        help="Enter the name of the Connected Spaces Platform assembly.",
        default="ConnectedSpacesPlatform.Unity.Core",
    )
    arg_parser.add_argument(
        "--assembly-namespace",
        help="Enter the root namespace of the Connected Spaces Platform assembly.",
        default="Csp",
    )
    arg_parser.add_argument(
        "--version",
        help="Enter the version of the Connected Spaces Platform, Semantic Versioning Only.",
        default="0.0.0",
    )

    arg_group = arg_parser.add_argument_group("artifacts")
    arg_group.add_argument(
        "--windows-artifact-root",
        help="Enter the path relative to the git root for Windows artifacts.",
        default=config.default_artifact_paths.windows,
    )
    arg_group.add_argument(
        "--macosx-artifact-root",
        help="Enter the path relative to the git root for macOS artifacts.",
        default=config.default_artifact_paths.macosx,
    )
    arg_group.add_argument(
        "--ios-artifact-root",
        help="Enter the path relative to the git root for iOS artifacts.",
        default=config.default_artifact_paths.ios,
    )
    arg_group.add_argument(
        "--visionos-artifact-root",
        help="Enter the path relative to the git root for VisionOS artifacts.",
        default=config.default_artifact_paths.visionos,
    )
    arg_group.add_argument(
        "--android-artifact-root",
        help="Enter the path relative to the git root for 32-bit Android artifacts.",
        default=config.default_artifact_paths.android,
    )
    arg_group.add_argument(
        "--android64-artifact-root",
        help="Enter the path relative to the git root for 64-bit Android artifacts.",
        default=config.default_artifact_paths.android64,
    )
    arg_group.add_argument(
        "--csharp-artifact-paths",
        nargs="+",
        help="Enter the relative paths from root/teamcity for the C# wrapper source.",
        default=config.default_artifact_paths.csharp,
    )

    arg_parser.add_argument(
        "--platforms",
        nargs="+",
        choices=["windows", "macosx", "ios", "visionos", "android", "android64"],
        help="Enter the platforms to build the package for.",
        default=None,
    )
    arg_parser.add_argument(
        "--unity-version",
        help="Enter the version of Unity supported, e.g. 2021.1.",
        default="2020.1",
    )
    arg_parser.add_argument(
        "--license",
        help="Enter the license required for the Connected Spaces Platform package.",
        default="Apache-2.0",
    )
    arg_parser.add_argument(
        "--description",
        help="Enter the description for the Connected Spaces Platform package.",
        default="This package provides the binaries required to interface with the Connected Spaces Platform API.",
    )
    arg_parser.add_argument(
        "--repository",
        help="This is the location of the Git repository.",
        default="https://github.com/magnopus-opensource/connected-spaces-platform",
    )
    
def prepare_package(args):
    print ('Preparing package within: ' + args.output_directory)
    
    # Template directory is relative to this script
    template_directory = f"{os.path.dirname(os.path.realpath(__file__))}/templates/unity"
    
    # Ensure our current working directory is always the git repository root
    root_dir = get_git_root()
    os.chdir(root_dir)
    
    print ('Root directory: ' + root_dir)

    # Clean previously-generated package
    shutil.rmtree(args.output_directory, ignore_errors=True)

    os.makedirs(args.output_directory)

    # Initialise Jinja environment
    env = Environment(loader=FileSystemLoader(template_directory), extensions=["jinja2_workarounds.MultiLineInclude"])

    minimal_meta_template = env.get_template("minimal.meta.jinja2")
    source_meta_template = env.get_template("source.cs.meta.jinja2")
    asmdef_template = env.get_template("asmdef.json.jinja2")
    plugin_meta_template = env.get_template("plugin.meta.jinja2")
    package_json_template = env.get_template("package.json.jinja2")

    # Copy C# source code and generate meta files
    source_output_directory = f"{args.output_directory}/Source"
    os.mkdir(source_output_directory)

    with open(f"{source_output_directory}.meta", "w") as f:
        f.write(minimal_meta_template.render(guid=generate_guid(), importer_type="Default"))

    for path in args.csharp_artifact_paths:
        shutil.copytree(path, source_output_directory, dirs_exist_ok=True)

    for path, dirs, files in os.walk(source_output_directory):
        for dir in dirs:
            with open(f"{path}/{dir}.meta", "w") as f:
                f.write(minimal_meta_template.render(guid=generate_guid(), importer_type="Default"))

        for file in files:
            with open(f"{path}/{file}.meta", "w") as f:
                f.write(source_meta_template.render(guid=generate_guid()))

    # Create C# assembly definition file and meta file
    with open(f"{source_output_directory}/{args.assembly_name}.asmdef", "w") as f:
        f.write(asmdef_template.render(assembly_name=args.assembly_name, root_namespace=args.assembly_namespace))

    with open(f"{source_output_directory}/{args.assembly_name}.asmdef.meta", "w") as f:
        f.write(
            # GUID needs to be const here to prevent references to the C# assembly from being removed when the package is updated
            minimal_meta_template.render(guid="1678634e69642a55e208df4d4d895a52", importer_type="AssemblyDefinition")
        )

    # Copy artifacts and generate meta files
    if args.platforms:
        for platform in args.platforms:
            artifact_directory = getattr(args, f"{platform}_artifact_root")
            platform_output_directory = f"{args.output_directory}/{platform}"
            os.mkdir(platform_output_directory)

            with open(f"{platform_output_directory}.meta", "w") as f:
                f.write(minimal_meta_template.render(guid=generate_guid(), importer_type="Default"))

            for artifact in getattr(config.artifacts, platform):
                shutil.copy(f"{artifact_directory}/{artifact}", platform_output_directory)

                with open(f"{platform_output_directory}/{artifact}.meta", "w") as f:
                    f.write(
                        plugin_meta_template.render(guid=generate_guid(), importer_type="Plugin", platform=platform)
                    )

    # Copy custom build processor files and generate meta files
    buildproc_directory = f"{args.output_directory}/Editor"
    os.mkdir(buildproc_directory)

    with open(f"{buildproc_directory}.meta", "w") as f:
        f.write(minimal_meta_template.render(guid=generate_guid(), importer_type="Default"))

    shutil.copy(f"{template_directory}/NativePluginBuildProcessor.asmdef", buildproc_directory)

    with open(f"{buildproc_directory}/NativePluginBuildProcessor.asmdef.meta", "w") as f:
        f.write(
            # GUID is const here to preserve references
            minimal_meta_template.render(guid="1678634e69642a55e208df4d4d895a53", importer_type="AssemblyDefinition")
        )

    shutil.copy(f"{template_directory}/NativePluginBuildProcessor.cs", buildproc_directory)

    with open(f"{buildproc_directory}/NativePluginBuildProcessor.cs.meta", "w") as f:
        f.write(source_meta_template.render(guid=generate_guid()))

    # Create package.json and meta file
    with open(f"{args.output_directory}/package.json", "w") as f:
        f.write(package_json_template.render(vars(args)))

    with open(f"{args.output_directory}/package.json.meta", "w") as f:
        f.write(minimal_meta_template.render(guid=generate_guid(), importer_type="TextScript"))

    # Copy readme and create meta file
    if (os.path.exists(f"{config.default_output_directory}/README.md")):
        shutil.copy(f"{config.default_output_directory}/README.md", args.output_directory)
    
        with open(f"{args.output_directory}/README.md.meta", "w") as f:
            f.write(minimal_meta_template.render(guid=generate_guid(), importer_type="Default"))

def main():
    # Template directory is relative to this script
    template_directory = f"{os.path.dirname(os.path.realpath(__file__))}/templates/unity"
  
    # Define commandline arguments
    arg_parser = argparse.ArgumentParser(description="Prepare the CSP for Unity package.")
    add_args(arg_parser)

    args = arg_parser.parse_args()
    prepare_package(args)

if __name__ == "__main__":
    main()
