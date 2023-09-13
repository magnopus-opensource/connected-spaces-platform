rem Generate dummy lib exports and unit tests

python scripts/Generate.py


rem Generate projects

"../modules/premake/bin/release/premake5.exe" vs2019