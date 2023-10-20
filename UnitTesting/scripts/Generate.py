#!/usr/bin/env python3

from GenerateExports import main as GenerateExports
from GenerateTests import main as GenerateTests


def main():
    GenerateExports()
    GenerateTests()


if __name__ == "__main__":
    main()
