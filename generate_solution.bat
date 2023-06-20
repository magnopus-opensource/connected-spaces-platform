REM *** Command Line Arguments *** 
REM DLLOnly - Generates a solution with only DLL-related build configurations
pip install -r requirements.txt

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

cd tools/wrappergenerator
call npm install

cd ../..

"modules/premake/bin/release/premake5.exe" vs2019 %*
pause