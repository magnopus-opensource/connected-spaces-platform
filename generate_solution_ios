#!/bin/bash
# *** Command Line Arguments *** 
# DLLOnly - Generates a solution with only DLL-related build configurations
git config core.hooksPath .githooks
git config commit.template .githooks/commit-template.txt

if [ ! -f /modules/premake/README.md ]
then
	git submodule update --recursive
fi


if [ ! -f /modules/premake/bin/release/premake5 ]
then
cd modules/premake/
make -f Bootstrap.mak macosx
cd ../..
fi

./modules/premake/bin/release/premake5 --os=ios xcode4 "$@"