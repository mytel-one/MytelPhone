@echo off
setlocal
set "TOASTMODE=%~1"
if "%TOASTMODE%"=="" set "TOASTMODE=production"
if not "%TOASTMODE%"=="legacy" if not "%TOASTMODE%"=="production" exit /b 2
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set "QTCORE=C:\Qt\6.10.0\msvc2022_64"
set "PATH=%QTCORE%\bin;%PATH%"
set "TOASTTESTBUILD=%TEMP%\mytel-v310-toast-xml-tests"
if not exist "%TOASTTESTBUILD%" mkdir "%TOASTTESTBUILD%"
set "TOASTDEFINE="
if "%TOASTMODE%"=="legacy" (
    python "%~dp0extract_legacy_toast_xml.py" --output "%TOASTTESTBUILD%\legacy_toast_xml_production.inc"
    if errorlevel 1 exit /b 1
    set "TOASTDEFINE=/DMYTEL_LEGACY_TOAST_XML"
)
cl.exe /nologo /EHsc /std:c++17 /permissive- /Zc:__cplusplus /utf-8 /MD /W4 %TOASTDEFINE% /I"%QTCORE%\include" /I"%QTCORE%\include\QtCore" /I"%~dp0..\..\Linphone" /I"%TOASTTESTBUILD%" /Fo"%TOASTTESTBUILD%\toast-xml-%TOASTMODE%.obj" /Fe"%TOASTTESTBUILD%\toast-xml-%TOASTMODE%.exe" "%~dp0toast_xml_test.cpp" /link /LIBPATH:"%QTCORE%\lib" Qt6Core.lib
if errorlevel 1 exit /b 1
"%TOASTTESTBUILD%\toast-xml-%TOASTMODE%.exe"
exit /b %ERRORLEVEL%
