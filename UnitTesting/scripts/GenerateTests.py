#!/usr/bin/env python3

import os
import shutil

from copy import deepcopy
from jinja2 import Environment, FileSystemLoader

from Config import config


def main():
    # Ensure our current working directory is always this script's parent directory
    os.chdir(os.path.abspath(os.path.dirname(os.path.dirname(__file__))))

    # Generate Unity (C#) tests

    # Clean previously-generated code
    shutil.rmtree("tests/csharp/Assets/Tests/generated", ignore_errors=True)

    os.mkdir("tests/csharp/Assets/Tests/generated")

    # Initialise Jinja environment
    env = Environment(
        loader=FileSystemLoader("templates/tests/csharp"), extensions=["jinja2_workarounds.MultiLineInclude"]
    )

    # Fix config values for C#
    _config = deepcopy(config)

    for p in _config["primitives"]:
        match p["min"]:
            case "INT_MIN":
                p["min"] = "Int32.MinValue"
            case "LLONG_MIN":
                p["min"] = "Int64.MinValue"
            case "0ULL":
                p["min"] = "0UL"
            case _:
                pass

        match p["max"]:
            case "UINT_MAX":
                p["max"] = "UInt32.MaxValue"
            case "INT_MAX":
                p["max"] = "Int32.MaxValue"
            case "ULLONG_MAX":
                p["max"] = "UInt64.MaxValue"
            case "LLONG_MAX":
                p["max"] = "Int64.MaxValue"
            case _:
                pass

    # Render tests
    unity_tests_template = env.get_template("tests.cs.jinja2")

    with open("tests/csharp/Assets/Tests/generated/tests.cs", "w") as f:
        f.write(unity_tests_template.render(config=_config))

    # Generate Node (JavaScript) tests

    # Clean previously-generated code
    try:
        os.remove("tests/javascript/tests.js")
    except OSError:
        pass

    # Initialise Jinja environment
    env = Environment(
        loader=FileSystemLoader("templates/tests/javascript"), extensions=["jinja2_workarounds.MultiLineInclude"]
    )

    # Fix config values for JS
    _config = deepcopy(config)

    for p in _config["primitives"]:
        match p["min"]:
            case "0U":
                p["min"] = "0"
            case "0ULL":
                p["min"] = "0n"
            case "INT_MIN":
                p["min"] = "dummy.Limits.INT32_MIN"
            case "LLONG_MIN":
                p["min"] = "dummy.Limits.INT64_MIN"
            case _:
                pass

        match p["max"]:
            case "UINT_MAX":
                p["max"] = "dummy.Limits.UINT32_MAX"
            case "INT_MAX":
                p["max"] = "dummy.Limits.INT32_MAX"
            case "ULLONG_MAX":
                p["max"] = "dummy.Limits.UINT64_MAX"
            case "LLONG_MAX":
                p["max"] = "dummy.Limits.INT64_MAX"
            case _:
                pass

        # Remove 'f' suffix
        match p["type"]:
            case "float":
                p["min"] = p["min"][:-1]
                p["max"] = p["max"][:-1]
            case _:
                pass

    # Render tests
    node_tests_template = env.get_template("tests.js.jinja2")

    with open("tests/javascript/tests.js", "w") as f:
        f.write(node_tests_template.render(config=_config))


if __name__ == "__main__":
    main()
