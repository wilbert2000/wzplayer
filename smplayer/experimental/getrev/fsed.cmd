:: Fake SED - parses a File line by line and replaces a substring
:: syntax: fsed.cmd OldStr NewStr File>NewFile
::          OldStr [in] - string to be replaced
::          NewStr [in] - string to replace with
::          File   [in] - file to be parsed
::          >NewFile    - output to new file
::
:: Original Source: http://www.dostips.com
::

SETLOCAL ENABLEEXTENSIONS
SETLOCAL DISABLEDELAYEDEXPANSION

if "%~1"=="" findstr "^::" "%~f0"&GOTO:EOF
for /f "tokens=1,* delims=]" %%A in ('"type %3|find /n /v """') do (
    set "line=%%B"
    if defined line (
        call set "line=echo.%%line:%~1=%~2%%"
        for /f "delims=" %%X in ('"echo."%%line%%""') do %%~X
    ) ELSE echo.
)