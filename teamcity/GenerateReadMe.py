#!/usr/bin/env python3

import argparse
import os
import re
import shutil
import sys

import git

from jinja2 import Environment, FileSystemLoader

from Config import config


def main():
    # Template directory is relative to this script
    template_directory = f"{os.path.dirname(os.path.realpath(__file__))}/templates/readme"
    
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

    # Initialise Jinja environment
    env = Environment(loader=FileSystemLoader(template_directory), extensions=["jinja2_workarounds.MultiLineInclude"])

    readme_template = env.get_template("readme.md.jinja2")

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
    
    commits = list(repo.iter_commits(f"{start_commit}..", reverse=True))    # Reverse to get oldest-first order
    commits = list(filter(lambda x: len(x.parents) < 2, commits))           # Ignore merge commits

    # Filter changes
    breaking_changes = []
    new_features = []
    bug_fixes = []
    style_changes = []
    refactors = []
    test_changes = []
    doc_changes = []
    misc_changes = []

    for commit in commits:
        index = commit.message.find('\n')

        if index > -1:
            full_title = commit.message[:index]

        description = commit.message[index + 1:]
        match = re.match(r'\[(?P<jira_id>[a-zA-Z]+\-[0-9]+)\] (?P<tag>[a-zA-Z]+)(?P<breaking_change>[\!]*)\: (?P<title>.*)', full_title)

        if match is None:
            match = re.match(r'(?P<jira_id>[a-zA-Z]+\-[0-9]+) (?P<title>.*)', full_title)
            if match is None:
                print("Invalid commit title format. Skipping...", file=sys.stderr)
                print(">>", full_title)
                continue
            
            jira_id = match.group("jira_id")
            tag = "misc"
            title = match.group("title")
            is_breaking_change = False
        else:
            jira_id = match.group("jira_id")
            tag = match.group("tag")
            title = match.group("title")
            is_breaking_change = len(match.group("breaking_change")) > 0

        if jira_id.startswith("NT-"):
            jira_id = "No Ticket"

        commit_details = {
            "jira_id": jira_id,
            "commit_hash": commit.hexsha,
            "title": title,
            "description": description
        }

        if is_breaking_change:
            breaking_changes.append(commit_details)
        else:
            if tag == "feat":
                new_features.append(commit_details)
            elif tag == "fix":
                bug_fixes.append(commit_details)
            elif tag == "style":
                style_changes.append(commit_details)
            elif tag == "refac":
                refactors.append(commit_details)
            elif tag == "test":
                test_changes.append(commit_details)
            elif tag == "doc":
                doc_changes.append(commit_details)
            else:
                misc_changes.append(commit_details)

    # Render README
    with open(f"{args.output_directory}/README.md", "w", encoding="utf-8") as f:
        f.write(
            readme_template.render(
                changelist_id=start_commit,
                commits=commits,
                breaking_changes=breaking_changes,
                new_features=new_features,
                bug_fixes=bug_fixes,
                style_changes=style_changes,
                refactors=refactors,
                test_changes=test_changes,
                doc_changes=doc_changes,
                misc_changes=misc_changes
            )
        )


if __name__ == "__main__":
    main()
