
REM Install typescript runner, test framework, etc. All at pinned versions from yarn.lock
CALL yarn install --immutable

REM Copy CSP WASM to node-modules (Assumes already built)
CALL robocopy ..\Library\Binaries\package\wasm\connected-spaces-platform.web node_modules\connected-spaces-platform.web /E

REM Robocopy uses bitmask style exit codes, 8+ is an actual error
IF ERRORLEVEL 8 (
    ECHO Failed to copy connected-spaces-platform.web. Have you built the package?
    EXIT /B %ERRORLEVEL%
)

REM Run tests (via uvu)
CALL yarn test