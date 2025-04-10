read -r emsdk_version < emsdk_version.txt
export emsdk_version=$emsdk_version;

cd ../..

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

docker run -w /src -v `pwd`:/src --rm emscripten/emsdk:$emsdk_version emmake make -j 8 config="$1"_wasm
if [ $? -ne 0 ]
  then
    cd Tools/Emscripten
    echo ERROR: Build failed. Have you started the docker engine?
	exit 1
fi

python3 Tools/WrapperGenerator/WrapperGenerator.py --generate_typescript
if [ $? -ne 0 ]
  then
    cd Tools/Emscripten
    echo ERROR: Generate TypeScript failed.
	exit 1
fi

cp -r Library/Binaries/wasm/"$1" Tools/WrapperGenerator/Output/TypeScript/connected-spaces-platform.web/
if [ $? -ne 0 ]
  then
    cd Tools/Emscripten
    echo ERROR: Copy failed.
	exit 1
  else
    cd Tools/Emscripten
    echo Success! Build located in: ..\WrapperGenerator\Output\TypeScript\connected-spaces-platform.web
    exit 0
fi


