setlocal
pushd %~dp0

set NASM_PASS=%1
set VS_MS_BUILD_CMD_PATH=%2
set BUIDLD_CONFIGURATION=%3
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
popd
if not %ERRORLEVEL%==0 (
  echo [ERROR]clean boringssl failed
  goto BUILD_ERROR
)

:: setup quiche
pushd %QUICHE_PATH%
if "%BUIDLD_CONFIGURATION%"=="release" (
  set BUIDLD_CONFIGURATION=--release
) else (
  set BUIDLD_CONFIGURATION=
)

:: clean quiche directory
call cargo clean %BUIDLD_CONFIGURATION%
if not %ERRORLEVEL%==0 (
  echo [ERROR]clean quiche failed
  popd
  goto BUILD_ERROR
)

:: build quiche
call cargo build --features ffi %BUIDLD_CONFIGURATION%
if not %ERRORLEVEL%==0 (
  echo [ERROR]build quiche failed
  popd
  goto BUILD_ERROR
)
popd

:: build qwfs
if "%BUIDLD_CONFIGURATION%"=="release" (
  set BUIDLD_CONFIGURATION=Release
) else (
  set BUIDLD_CONFIGURATION=Debug
)
call msbuild.exe /t:rebuild /p:Configuration=%BUIDLD_CONFIGURATION%;Platform="x64" %QWFS_PATH%\qwfs.sln
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
