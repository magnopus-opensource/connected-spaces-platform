#!/bin/bash
# *** Command Line Arguments *** 
# DLLOnly - Generates a solution with only DLL-related build configurations

python_prefix="Python "
python_version=$(python3 -V 2>&1)

if [[ -z "$python_version" ]]
then
    echo "No Python installation found, please install Python 3.12.+ before proceeding."
    exit
else
    # Remove the 'Python ' prefix from the version.
    version_number=${python_version#"$python_prefix"}
    echo "Python version: $version_number installed."
fi

# Check if the installed Python version is >= 3.12
if ! { echo "$version_number"; echo "3.12.0"; } | sort --version-sort --check
then
    # Check to see if venv exists by searching for config file.
    venv_file="./venv/pyvenv.cfg"
    
    # If no venv config file found create a venv.
    if [ ! -f "$venv_file" ]
    then
        echo "Virtual evironment does not exist, creating now..."
        mkdir ./venv
        python3 -m venv ./venv
    fi
    
    echo "Activating virtual evironment."
    source ./venv/bin/activate
fi

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
    mkdir -p ./modules/premake/bin/release/
    cd ./modules/premake/bin/release/
    curl -O -L https://github.com/premake/premake-core/releases/download/v5.0.0-beta2/premake-5.0.0-beta2-macosx.tar.gz
    tar -xzvf premake-5.0.0-beta2-macosx.tar.gz
    rm premake-5.0.0-beta2-macosx.tar.gz
    cd ../../../..
fi

# Premake does not yet support VisionOS. So we build for 'iOS' with a visionos argument. We then respond to that where necessary in the scripts.
./modules/premake/bin/release/premake5 --os=ios --visionos xcode4 "$@"
