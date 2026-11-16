@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul
set "SDKROOT=%~dp0..\..\..\win\sdk\linphone-sdk\win64"
set "PATH=C:\Qt\6.10.0\msvc2022_64\bin;%SDKROOT%\bin;%PATH%"
set "CMAKEEXE=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "TESTBUILD=%TEMP%\mytel-v3-theme-tests"
"%CMAKEEXE%" -S "%~dp0..\mytelv3" -B "%TESTBUILD%" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:\Qt\6.10.0\msvc2022_64 -DMYTEL_TEST_SDK_ROOT="%SDKROOT%"
if errorlevel 1 exit /b 1
"%CMAKEEXE%" --build "%TESTBUILD%" --target mytel-login-auth-test --parallel 1
if errorlevel 1 exit /b 1
pushd "%SDKROOT%"
"%TESTBUILD%\mytel-login-auth-test.exe"
set "TESTEXIT=%ERRORLEVEL%"
popd
exit /b %TESTEXIT%
