@echo off
REM ===========================================================================
REM  Build chrome_enroll.dll with MSVC.
REM  Usage:   build.bat            (builds x64 -> build\x64)
REM           build.bat x86        (builds x86 -> build\x86)
REM ===========================================================================
setlocal

set "ARCH=%1"
if "%ARCH%"=="" set "ARCH=x64"

set "VSBT=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools"
if /I "%ARCH%"=="x64" (
    set "VCVARS=%VSBT%\VC\Auxiliary\Build\vcvars64.bat"
) else (
    set "VCVARS=%VSBT%\VC\Auxiliary\Build\vcvars32.bat"
)

call "%VCVARS%"
if errorlevel 1 (echo [ERROR] Could not initialise MSVC environment & exit /b 1)

set "OUT=%~dp0build\%ARCH%"
if not exist "%OUT%" mkdir "%OUT%"

echo.
echo === Compiling libche.dll ^(%ARCH%^) ===
cl /nologo /LD /O2 /W4 /EHsc /MT /DUNICODE /D_UNICODE ^
   /Fe:"%OUT%\libche.dll" ^
   /Fo:"%OUT%\\" ^
   "%~dp0chrome_enroll.cpp" ^
   /link /DEF:"%~dp0chrome_enroll.def" ^
   /IMPLIB:"%OUT%\libche.lib"

if errorlevel 1 (echo [ERROR] Build failed & exit /b 1)

echo.
echo === Build OK : "%OUT%\libche.dll" ===
dir "%OUT%"
endlocal
