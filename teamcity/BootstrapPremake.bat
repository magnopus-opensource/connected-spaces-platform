@ECHO OFF
REM The first parameter is expected to be the VS version (e.g., '2022')
SET vsversion=%1

CALL "C:\Program Files\Microsoft Visual Studio\%vsversion%\Professional\VC\Auxiliary\Build\vcvars64.bat" && nmake MSDEV="vs%vsversion%" -f Bootstrap.mak windows-msbuild