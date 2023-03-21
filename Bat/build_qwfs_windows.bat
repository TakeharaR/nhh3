@echo off
setlocal
pushd %~dp0

set BUIDLD_CONFIGURATION=%1
set QUICHE_PATH=..\External\quiche
set BOLINGSSL_PATH=%QUICHE_PATH%\quiche\deps\boringssl
set QWFS_PATH=..\qwfs

:: clean BoringSSL directory
pushd %BOLINGSSL_PATH%
call git clean -fx
popd
if not %ERRORLEVEL%==0 (
  echo [ERROR]clean boringssl failed
  goto BUILD_ERROR
)

:: setup quiche
:: clean quiche local change
pushd %QUICHE_PATH%
call git checkout .
popd

:: patch quiche
xcopy /Y /e .\patch\quiche %QUICHE_PATH%\quiche

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
call cargo build --features ffi %BUIDLD_CONFIGURATION_QUICHE%
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
call msbuild.exe /t:rebuild /p:Configuration=%BUIDLD_CONFIGURATION_QWFS%;Platform="x64" %QWFS_PATH%\qwfs.sln
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
