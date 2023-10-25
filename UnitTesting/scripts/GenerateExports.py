#!/usr/bin/env python3

import os
import shutil
import subprocess

from Config import config

from jinja2 import Environment, FileSystemLoader


def main():
    # Ensure our current working directory is always this script's parent directory
    os.chdir(os.path.abspath(os.path.dirname(os.path.dirname(__file__))))

    # Clean previously-generated code
    shutil.rmtree("src", ignore_errors=True)
    shutil.rmtree("include", ignore_errors=True)

    os.mkdir("src")
    os.mkdir("include")

    # Initialise Jinja environment
    env = Environment(loader=FileSystemLoader("templates/exports"), extensions=["jinja2_workarounds.MultiLineInclude"])

    # Render exports
    header_template = env.get_template("exports.h.jinja2")
    source_template = env.get_template("exports.cpp.jinja2")

    with open("include/generated_exports.h", "w") as f:
        f.write(header_template.render(config=config))

    with open("src/generated_exports.cpp", "w") as f:
        f.write(source_template.render(config=config))

    subprocess.run(f'"clang-format" -i "include/generated_exports.h"', shell=True)
    subprocess.run(f'"clang-format" -i "src/generated_exports.cpp"', shell=True)


if __name__ == "__main__":
    main()
