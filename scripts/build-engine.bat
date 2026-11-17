@echo off
setlocal
cd /d "%~dp0.."
if not exist build\hip-tmp mkdir build\hip-tmp
set "TEMP=%CD%\build\hip-tmp"
set "TMP=%TEMP%"
set "TMPDIR=%TEMP%"
if not defined MI50_ROCM_DEVEL set "MI50_ROCM_DEVEL=%CD%\build\rocm-venv\Lib\site-packages\_rocm_sdk_devel"
set "ROCM_F=%MI50_ROCM_DEVEL:\=/%"
set "HIP_PLATFORM=amd"
set "HIP_PATH=%MI50_ROCM_DEVEL%"
set "ROCM_PATH=%MI50_ROCM_DEVEL%"
set "PATH=%MI50_ROCM_DEVEL%\bin;%MI50_ROCM_DEVEL%\lib\llvm\bin;%PATH%"
if not exist "%MI50_ROCM_DEVEL%\lib\llvm\bin\clang++.exe" (
  echo Install the pinned TheRock toolchain first. See docs\BUILD.md.
  exit /b 1
)
cmake -G Ninja -S vendor/strata -B build/engine -DCMAKE_BUILD_TYPE=Release ^
  -DSTRATA_HIP_GFX906=ON -DSTRATA_ENABLE_HIP=OFF -DSTRATA_ENABLE_CUDA=OFF ^
  -DSTRATA_BUILD_TESTS=OFF -DSTRATA_NATIVE_EXPERTS=ON -DSTRATA_PORTABLE=ON -DSTRATA_MMQ_KQUANTS=ON ^
  -DCMAKE_HIP_ARCHITECTURES=gfx906 ^
  "-DCMAKE_HIP_FLAGS_RELEASE=-O3 -DNDEBUG" ^
  "-DCMAKE_C_COMPILER=%ROCM_F%/lib/llvm/bin/clang.exe" ^
  "-DCMAKE_CXX_COMPILER=%ROCM_F%/lib/llvm/bin/clang++.exe" ^
  "-DCMAKE_HIP_COMPILER=%ROCM_F%/lib/llvm/bin/clang++.exe" ^
  "-DCMAKE_HIP_COMPILER_ROCM_ROOT=%ROCM_F%" "-DCMAKE_PREFIX_PATH=%ROCM_F%" ^
  "-DCMAKE_HIP_FLAGS=--rocm-path=%ROCM_F% --rocm-device-lib-path=%ROCM_F%/lib/llvm/amdgcn/bitcode -mcode-object-version=4" ^
  "-DSTRATA_GGML_DIR=%CD:\=/%/vendor/strata/third_party/llama.cpp"
if errorlevel 1 exit /b 1
cmake --build build/engine --target strata -- -j 2
exit /b %errorlevel%
