SET "SHARED_TARGET=%CD%\shared"
SET "NODE_MODULES_TARGET=%CD%\node_modules"

REM Junction node_modules so web server can serve them
IF NOT EXIST ".\html_tests\node_modules" (
    mklink /J ".\html_tests\node_modules" "%NODE_MODULES_TARGET%"
)

REM Junction code that's shared between typescript and html runtimes
IF NOT EXIST ".\html_tests\shared" (
    mklink /J ".\html_tests\shared" "%SHARED_TARGET%"
)

REM Caddy is the web server we use to serve the html files
CALL choco install caddy --version=2.10.0 -y --source=https://community.chocolatey.org/api/v2/

REM Kill any caddy instances that may be open
taskkill /IM caddy.exe /F

REM Start Caddy to serve the HTML tests directory on localhost
echo Starting Caddy server on http://localhost:8888
start /B caddy run

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