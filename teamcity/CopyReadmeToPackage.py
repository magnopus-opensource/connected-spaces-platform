#!/usr/bin/env python3

import argparse
import os
import shutil
import sys

import git

from Config import config

def main():

    try:
        # Ensure our current working directory is the git repository root
        repo = git.Repo(__file__, search_parent_directories=True)
        root_dir = repo.working_dir
        os.chdir(root_dir)
    except git.InvalidGitRepositoryError:
        print(f"Error: Could not find a Git repository root.", file=sys.stderr)
        sys.exit(1)

    # Define commandline arguments
    arg_parser = argparse.ArgumentParser(description="Copy README.md from .github directory to the output location.")
    arg_parser.add_argument(
        "--output-directory",
        help="Enter the path relative to the git root for the README to be copied to.",
        default=f"{config.default_output_directory}",
    )

    args = arg_parser.parse_args()

    # Define source and destination paths
    source_readme_path = os.path.join(root_dir, ".github", "README.md")
    destination_directory = os.path.join(root_dir, args.output_directory)
    destination_readme_path = os.path.join(destination_directory, "README.md")
    
    # Check if the source README.md exists
    if not os.path.exists(source_readme_path):
        print(f"Error: Source README.md not found at '{source_readme_path}'. Please ensure it exists.", file=sys.stderr)
        sys.exit(1)

    # Clean the output directory if it exists
    if os.path.exists(destination_directory):
        shutil.rmtree(destination_directory)
    
    # Create the output directory
    os.makedirs(destination_directory, exist_ok=True)

    # Copy the README.md from .github to the output directory
    print(f"Copying '{source_readme_path}' to '{destination_readme_path}'")
    shutil.copy2(source_readme_path, destination_readme_path)

    print(f"Successfully copied README.md to '{destination_directory}'")


if __name__ == "__main__":
    main()