REM *** Command Line Arguments *** 
REM DLLOnly - Generates a solution with only DLL-related build configurations
py -m pip install -r teamcity/requirements.txt

if "%~1"=="-ci" (goto :NoGitConfig) else goto :GitConfig

:GitConfig
echo "Configuring Git"
git config core.hooksPath .githooks
git config commit.template .githooks/commit-template.txt

:NoGitConfig

IF NOT EXIST "modules/premake/README.md" (
	git submodule init
	git submodule update --recursive
)

IF NOT EXIST "modules/premake/bin/release/premake5.exe" (
	cd modules/premake
	call Bootstrap.bat
	cd ../..
)

IF NOT EXIST "modules/googletest/build/ALL_BUILD.vcxproj" (
	cd modules/googletest
	mkdir build
	cd build
	call cmake ..
	call msbuild ALL_BUILD.vcxproj /property:Configuration=Release
	cd ../../..
)

REM Tiny process library used in tests project (statically linked) to invoke multiplayer test runner.
REM We install to thirdparty, i'm not sure this is the best idea, but i'm not sure why modules and thirdparty are split.
IF NOT EXIST "ThirdParty/tiny-process-library/install/lib/tiny-process-library.lib" (
	cd Thirdparty/tiny-process-library
	mkdir build
	call cmake -S . -B build -G "Visual Studio 17 2022" -DCMAKE_INSTALL_PREFIX=install
	call cmake --build build --config Release
	call cmake --install build --config Release
	cd ../..
)

cd tools/wrappergenerator
call npm install
cd ../..

"modules/premake/bin/release/premake5.exe" vs2022 %*
pause