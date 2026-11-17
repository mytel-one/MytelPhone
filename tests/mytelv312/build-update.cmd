@echo off
setlocal
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set "CMAKEEXE=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "TESTBUILD=D:\MytelPhone-build\v312-brand-20261117\windows-update-tests"
"%CMAKEEXE%" -S "%~dp0." -B "%TESTBUILD%" -G Ninja -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1
"%CMAKEEXE%" --build "%TESTBUILD%" --target mytel-update-disabled-test --parallel 1
if errorlevel 1 exit /b 1
"%TESTBUILD%\mytel-update-disabled-test.exe"
exit /b %ERRORLEVEL%
