@echo off
setlocal

rem #############################################################
rem # Configuration
rem #############################################################

rem Set the path to the Cppcheck executable.
rem This path can be updated if the installation location changes.
set "CPPCHECK_EXE=C:\Program Files\Cppcheck\cppcheck.exe"

rem Set the command-line arguments for the analysis.
set "CPPCHECK_ARGS=--xml --xml-version=2 --output-file=cppcheck_output.xml --enable=all --suppressions-list=cppcheck_suppressions.txt -I"../../Library/include" -I"../../Library/src" "../../Library""

echo.
echo ================================================================================
echo Starting Cppcheck Static Analysis
echo ================================================================================
echo.

rem Check if the Cppcheck executable exists.
if not exist "%CPPCHECK_EXE%" (
    echo [ERROR] Cppcheck executable not found.
    echo         Please ensure it is installed at:
    echo         "%CPPCHECK_EXE%"
    echo.
    goto :end_script
)

echo [INFO] Cppcheck executable found.
echo [INFO] Running Cppcheck with the following command:
echo        "%CPPCHECK_EXE%" %CPPCHECK_ARGS%
echo.

rem Execute the Cppcheck command.
"%CPPCHECK_EXE%" %CPPCHECK_ARGS%

rem Check the exit code of the last command.
if %ERRORLEVEL% equ 0 (
    echo.
    echo ================================================================================
    echo [SUCCESS] Cppcheck analysis completed successfully.
    echo           Report generated at: cppcheck_output.xml
    echo ================================================================================
) else (
    echo.
    echo ================================================================================
    echo [FAILURE] Cppcheck analysis failed.
    echo           Exit code: %ERRORLEVEL%
    echo           Check the output above for errors.
    echo ================================================================================
)

:end_script
echo.
pause
endlocal