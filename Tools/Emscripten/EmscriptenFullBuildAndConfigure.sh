git config core.hooksPath .githooks
git config commit.template .githooks/commit-template.txt

# Example usage, ./EmscriptenFullBuildAndConfigure debug withNode

if ! test -f "../../modules/premake/README.md"; then
    git submodule update --recursive
fi

if ! test -f "../../modules/premake/bin/release/premake5"; then
  if ! test -f "premake5"; then
	  echo "Premake5 not installed."
	  exit
  fi
fi

read -r emsdk_version < emsdk_version.txt
export emsdk_version=$emsdk_version;
cd ../..
rm Makefile

if [ -z "$1" ]
  then
    echo "No configuration name provided"
		exit
fi

if [ "$1" != "debug" ] && [ "$1" != "release" ]
  then
    echo Unsupported configuration "$1". Supported configurations are "debug" and "release". Please try again.
    exit
fi

WITH_NODE=0
if [ "$2" = "withNode" ]; then
    WITH_NODE=1
fi

if [ "$WITH_NODE" -eq 1 ]; then
    echo "Building with Node.js support"
    "premake5" gmake --generate_wasm --wasm_with_node
else
    echo "Building without Node.js support"
    "premake5" gmake --generate_wasm
fi

python Tools/Emscripten/ReplaceComSpec.py
docker run -w /src -v `pwd`:/src --rm emscripten/emsdk:$emsdk_version emmake make config="$1"_wasm clean
if [ $? -ne 0 ]
  then
    cd Tools/Emscripten
    echo ERROR: Clean failed. Have you started the docker engine?
	exit 1
fi
	
docker run -w /src -v `pwd`:/src --rm emscripten/emsdk:$emsdk_version emmake make -j 8 config="$1"_wasm
if [ $? -ne 0 ]
  then
    cd Tools/Emscripten
    echo ERROR: Build failed. Have you started the docker engine?
	exit 1
fi

python ./teamcity/GenerateReadMeWithLink.py
python ./teamcity/BuildNPMWebPackage.py --npm_publish_flag=False
if [ $? -ne 0 ]
  then
    cd Tools/Emscripten
    echo ERROR: Package generation failed.
	exit 1
  else
    cd Tools/Emscripten
    echo Success! Build located in: ../Library/Binaries/package/wasm/connected-spaces-platform.web
    exit 0
fi
