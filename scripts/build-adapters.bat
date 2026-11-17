@echo off
setlocal
cd /d "%~dp0.."
if not exist build\hip-tmp mkdir build\hip-tmp
set "TEMP=%CD%\build\hip-tmp"
set "TMP=%TEMP%"
set "TMPDIR=%TEMP%"
if not defined MI50_ROCM_DEVEL set "MI50_ROCM_DEVEL=%CD%\build\rocm-venv\Lib\site-packages\_rocm_sdk_devel"
if not exist build\adapters mkdir build\adapters
"%MI50_ROCM_DEVEL%\lib\llvm\bin\clang++.exe" -std=c++17 -O2 -shared ^
  -D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH -D_CRT_SECURE_NO_DEPRECATE ^
  scripts\hip57_shim.cpp -o build\adapters\amdhip64_7.dll -ldxgi -lole32 -ld3d11 -ladvapi32
if errorlevel 1 exit /b 1
cl /nologo /O2 /EHsc /LD scripts\hipblas57_adapter.cpp /link /OUT:build\adapters\hipblas.dll
exit /b %errorlevel%
