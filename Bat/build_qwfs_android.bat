setlocal
pushd %~dp0

set NASM_PASS=%1
set VS_MS_BUILD_CMD_PATH=%2
set NINJA_PATH=%3
set BUIDLD_CONFIGURATION=%4
set QUICHE_PATH=..\External\quiche
set BOLINGSSL_PATH=%QUICHE_PATH%\quiche\deps\boringssl
set QWFS_PATH=..\qwfs

set path=%NASM_PASS%;%path%

:: call VsMSBuildCmd.bat
call %VS_MS_BUILD_CMD_PATH%\VsMSBuildCmd.bat
if not %ERRORLEVEL%==0 (
  echo [ERROR]call VsMSBuildCmd.bat failed
  goto BUILD_ERROR
)

:: clean BoringSSL directory
pushd %BOLINGSSL_PATH%
call git clean -fx
if not %ERRORLEVEL%==0 (
  echo [ERROR]clean boringssl failed
  popd
  goto BUILD_ERROR
)

:: build BoringSSL
if "%BUIDLD_CONFIGURATION%"=="release" (
  set BUIDLD_CONFIGURATION_BORING=Release
) else (
  set BUIDLD_CONFIGURATION_BORING=Debug
)
call cmake -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-21 -DCMAKE_TOOLCHAIN_FILE=%ANDROID_NDK_HOME%/build/cmake/android.toolchain.cmake -DCMAKE_MAKE_PROGRAM=%NINJA_PATH% -DANDROID_NATIVE_API_LEVEL=21 -DANDROID_STL=c++_shared -DCMAKE_BUILD_TYPE=%BUIDLD_CONFIGURATION_BORING% -GNinja
if not %ERRORLEVEL%==0 (
  echo [ERROR]build boringssl failed
  popd
  goto BUILD_ERROR
)
call %NINJA_PATH%
popd

:: setup quiche
xcopy /Y quiche_android_Cargo.toml %QUICHE_PATH%\quiche\Cargo.toml
pushd %QUICHE_PATH%
if "%BUIDLD_CONFIGURATION%"=="release" (
  set BUIDLD_CONFIGURATION_QUICHE=--release
) else (
  set BUIDLD_CONFIGURATION_QUICHE=
)

:: clean quiche directory
call cargo clean %BUIDLD_CONFIGURATION_QUICHE%
if not %ERRORLEVEL%==0 (
  echo [ERROR]clean quiche failed
  popd
  goto BUILD_ERROR
)

:: build quiche
set QUICHE_BSSL_PATH=quiche\deps\boringssl
call cargo ndk -t arm64-v8a -p 21 build --features ffi %BUIDLD_CONFIGURATION_QUICHE%
if not %ERRORLEVEL%==0 (
  echo [ERROR]build quiche failed
  popd
  goto BUILD_ERROR
)
popd

:: build qwfs
if "%BUIDLD_CONFIGURATION%"=="release" (
  set BUIDLD_CONFIGURATION_QWFS=Release
) else (
  set BUIDLD_CONFIGURATION_QWFS=Debug
)
call msbuild.exe /t:rebuild /p:Configuration=%BUIDLD_CONFIGURATION_QWFS%;Platform="ARM64" %QWFS_PATH%\qwfs.sln
IF %ERRORLEVEL% == 1 (
  echo [ERROR]build qwfs failed
  goto BUILD_ERROR
)

endlocal
popd
exit /b 0


:BUILD_ERROR
endlocal
popd
exit /b 1