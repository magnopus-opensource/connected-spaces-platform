#!/usr/bin/env python3

import argparse
import os
import re
import shutil

import git

from Config import config


def main():
    # Ensure our current working directory is always the git repository root
    repo = git.Repo(os.path.abspath(os.path.dirname(__file__)), search_parent_directories=True)
    root_dir = repo.working_dir
    os.chdir(root_dir)

    # Define commandline arguments
    arg_parser = argparse.ArgumentParser(description="Generate a README.md file for CSP packages.")
    arg_parser.add_argument(
        "--output-directory",
        help="Enter the path relative to the git root for the README to be copied to.",
        default=f"{config.default_output_directory}",
    )

    args = arg_parser.parse_args()

    # Clean previously-generated README
    shutil.rmtree(args.output_directory, ignore_errors=True)

    os.makedirs(args.output_directory)
	
    start_commit = ""

    # Get changelist
    if len(repo.tags) > 0:
        # Get commit ID of latest tag
        latest_tag = repo.git.describe(tags=True, abbrev=0)
        tag_commit_id = repo.git.rev_list(latest_tag, n=1)
        start_commit = tag_commit_id
    else:
        first_commit = list(repo.iter_commits())[-1]
        start_commit = first_commit.hexsha
	
    # Render README
    with open(f"{args.output_directory}/README.md", "w", encoding="utf-8") as f:
        f.write("## Built from changelist ID: " + start_commit)
        f.write("\n")
        f.write("The Release Notes for this package can be found [here](https://github.com/magnopus-opensource/connected-spaces-platform/releases).")


if __name__ == "__main__":
    main()
