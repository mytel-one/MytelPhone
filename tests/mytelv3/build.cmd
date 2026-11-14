@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul
set "PATH=C:\Qt\6.10.0\msvc2022_64\bin;%PATH%"
set "CMAKEEXE=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "TESTBUILD=%TEMP%\mytel-v3-theme-tests"
"%CMAKEEXE%" -S "%~dp0." -B "%TESTBUILD%" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:\Qt\6.10.0\msvc2022_64
if errorlevel 1 exit /b 1
"%CMAKEEXE%" --build "%TESTBUILD%" --parallel 4
if errorlevel 1 exit /b 1
"%TESTBUILD%\mytel-theme-test.exe"
if errorlevel 1 exit /b 1
"%TESTBUILD%\mytel-login-attempt-test.exe"
if errorlevel 1 exit /b 1
"%TESTBUILD%\mytel-friend-addresses-test.exe"
if errorlevel 1 exit /b 1
"%TESTBUILD%\mytel-friend-removal-test.exe"
if errorlevel 1 exit /b 1
"%TESTBUILD%\mytel-chat-layout-test.exe"
if errorlevel 1 exit /b 1
"%TESTBUILD%\mytel-dial-input-test.exe"
exit /b %ERRORLEVEL%
