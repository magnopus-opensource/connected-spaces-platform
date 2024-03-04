#!/bin/bash
# *** Command Line Arguments *** 
# DLLOnly - Generates a solution with only DLL-related build configurations
python3 -m pip install -r teamcity/requirements.txt

git config core.hooksPath .githooks
git config commit.template .githooks/commit-template.txt

XCODE_VERSION=$(softwareupdate --history | awk '/Command Line Tools for Xcode/ {print $6}' | tail -1)

if [ ! -f /modules/premake/README.md ]
then
	git submodule update --recursive
fi


if [ ! -f /modules/premake/bin/release/premake5 ]
then
	MIN_XCODE_VERSION="14.2"
    if [ "$(printf '%s\n' "$MIN_XCODE_VERSION" "$XCODE_VERSION" | sort -V | head -n1)" = "$MIN_XCODE_VERSION" ]; then
		mkdir -p ./modules/premake/bin/release/
		cd ./modules/premake/bin/release/
		curl -O -L https://github.com/premake/premake-core/releases/download/v5.0.0-beta2/premake-5.0.0-beta2-macosx.tar.gz
		tar -xzvf premake-5.0.0-beta2-macosx.tar.gz
		rm premake-5.0.0-beta2-macosx.tar.gz
		cd ../../../..
	else
		cd ./modules/premake/
		make -f Bootstrap.mak macosx
		cd ../..
	fi
fi

# Premake does not yet support VisionOS. So we build for 'iOS' with a visionos argument. We then respond to that where necessary in the scripts.
./modules/premake/bin/release/premake5 --os=ios --visionos xcode4 "$@"
