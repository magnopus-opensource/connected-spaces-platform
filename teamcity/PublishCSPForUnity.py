#!/usr/bin/env python3

import argparse
import os
import subprocess

import PrepareUnityPackage

def main():
    # Define commandline arguments
    arg_parser = argparse.ArgumentParser(description="Build and publish the CSP for Unity NPM package.")

    arg_parser.add_argument(
        "--registry",
        help="This is the upstream location of the package.",
        default="https://npm.pkg.github.io/@magnopus-opensource",
    )
    arg_parser.add_argument(
        "--release-mode",
        help="NPM release command, pack and publish are available.",
        default="pack",
    )
    arg_parser.add_argument(
        "--npm_publish_flag",
        help="Whether the package should be published to NPM. Default == True",
        default="True")

    PrepareUnityPackage.add_args(arg_parser)

    args = arg_parser.parse_args()
    
    PrepareUnityPackage.prepare_package(args)

    # Create and publish NPM package
    root_dir = PrepareUnityPackage.get_git_root()
    os.chdir(args.output_directory)
    if eval(args.npm_publish_flag):
        subprocess.call(
            f'npm {args.release_mode} --userconfig="{root_dir}/unity.npmrc" --"@magnopus-opensource:registry={args.registry}"',
            shell=True,
        )


if __name__ == "__main__":
    main()
