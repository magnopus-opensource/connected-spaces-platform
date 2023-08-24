rem Make sure we can run msbuild

rem Try VS2019 first

if not defined DevEnvDir (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat" amd64
)

rem If VS2019 is not installed, try VS2022

if not defined DevEnvDir (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" amd64
)

rem Build Windows Connected Spaces Platform lib

msbuild WrapperGeneratorUnitTesting.sln