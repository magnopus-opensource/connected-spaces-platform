rem Generate dummy lib exports and unit tests

python scripts/Generate.py


rem Generate projects

del Makefile
del UnitTestingBinary.make
del mimalloc.make
"../modules/premake/bin/release/premake5.exe" gmake2 --generate_wasm