git config core.hooksPath .githooks
git config commit.template .githooks/commit-template.txt

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

"premake5" gmake2 --generate_wasm

python Tools/Emscripten/ReplaceComSpec.py
docker run -w /src -v `pwd`:/src --rm emscripten/emsdk:$emsdk_version emmake make config="$1"_wasm clean
docker run -w /src -v `pwd`:/src --rm emscripten/emsdk:$emsdk_version emmake make -j 8 config="$1"_wasm
cd Tools/Emscripten