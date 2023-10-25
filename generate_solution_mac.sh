#!/bin/bash
# *** Command Line Arguments *** 
# DLLOnly - Generates a solution with only DLL-related build configurations

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
XCODE_VERSION=$(softwareupdate --history | awk '/Command Line Tools for Xcode/ {print $6}' | tail -1)
cd "${DIR}"

git config core.hooksPath .githooks
git config commit.template .githooks/commit-template.txt

if [ ! -f ./modules/premake/README.md ]
then
	git submodule update --recursive
fi


if [ ! -f /modules/premake/bin/release/premake5 ]
then
	if [ $((${XCODE_VERSION//.})) > 142 ]; then
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

./modules/premake/bin/release/premake5 xcode4 "$@"
./modules/premake/bin/release/premake5 --os=macosx vs2019 "$@"