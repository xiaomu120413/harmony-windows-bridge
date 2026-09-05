@echo off
setlocal
set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR:~0,-1%"
set "HVIGORW_CMD="
set "IDE_HOME="
set "JAVA_BIN_DIR="
set "BUILD_TARGET=%~1"
if not defined BUILD_TARGET set "BUILD_TARGET=app"

if /i not "%BUILD_TARGET%"=="app" if /i not "%BUILD_TARGET%"=="tablet" if /i not "%BUILD_TARGET%"=="2in1" (
  echo Unknown build target: %BUILD_TARGET%. Expected app, tablet, or 2in1.
  endlocal
  exit /b 2
)

if not defined IDE_HOME if defined DEVECOSTUDIO_HOME (
  set "IDE_HOME=%DEVECOSTUDIO_HOME%"
)

if not defined IDE_HOME if exist "C:\Program Files\Huawei\DevEco Studio\tools\hvigor\bin\hvigorw.bat" (
  set "IDE_HOME=C:\Program Files\Huawei\DevEco Studio"
)

if defined IDE_HOME if exist "%IDE_HOME%\jbr\bin\java.exe" (
  set "JAVA_BIN_DIR=%IDE_HOME%\jbr\bin"
) else if defined JAVA_HOME if exist "%JAVA_HOME%\bin\java.exe" (
  set "JAVA_BIN_DIR=%JAVA_HOME%\bin"
)

if not defined HVIGORW_CMD if defined IDE_HOME if exist "%IDE_HOME%\tools\hvigor\bin\hvigorw.bat" (
  set "HVIGORW_CMD=%IDE_HOME%\tools\hvigor\bin\hvigorw.bat"
)

if defined JAVA_BIN_DIR (
  set "PATH=%JAVA_BIN_DIR%;C:\Windows\System32;C:\Windows;%PATH%"
)

cd /d "%PROJECT_DIR%"

if not defined HVIGORW_CMD (
  echo hvigorw not found. Set DEVECOSTUDIO_HOME or install DevEco Studio.
  endlocal
  exit /b 1
)

if exist "%PROJECT_DIR%\..\out\ohos-arm64\runtime-libs\libfreerdp-client3.so" (
  powershell -NoProfile -ExecutionPolicy Bypass -File "%PROJECT_DIR%\..\scripts\windows\sync-freerdp-runtime.ps1"
  if errorlevel 1 (
    echo sync-freerdp-runtime failed with exit code %ERRORLEVEL%.
    endlocal
    exit /b %ERRORLEVEL%
  )
)

if /i not "%BUILD_TARGET%"=="tablet" (
  powershell -NoProfile -ExecutionPolicy Bypass -File "%PROJECT_DIR%\..\scripts\windows\package-xrdp-hnp.ps1"
  if errorlevel 1 (
    echo package-xrdp-hnp failed with exit code %ERRORLEVEL%.
    endlocal
    exit /b %ERRORLEVEL%
  )
)

if /i "%BUILD_TARGET%"=="app" (
  call "%HVIGORW_CMD%" --no-daemon --no-parallel assembleApp -p product=default -p buildMode=debug
) else if /i "%BUILD_TARGET%"=="tablet" (
  call "%HVIGORW_CMD%" --no-daemon --no-parallel assembleHap --mode module -p product=default -p buildMode=debug -p module=entry_tablet@default
) else (
  call "%HVIGORW_CMD%" --no-daemon --no-parallel assembleHap --mode module -p product=default -p buildMode=debug -p module=entry@default
)
if errorlevel 1 (
  echo hvigor %BUILD_TARGET% build failed with exit code %ERRORLEVEL%.
  endlocal
  exit /b %ERRORLEVEL%
)

if /i "%BUILD_TARGET%"=="tablet" (
  for %%I in ("entry_tablet\build\default\outputs\default\entry_tablet-default-signed.hap") do echo TABLET HAP %%~fI ^| size=%%~zI ^| time=%%~tI
  endlocal
  exit /b 0
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%PROJECT_DIR%\..\scripts\windows\repack-hap-with-hnp.ps1"
if errorlevel 1 (
  echo repack-hap-with-hnp failed with exit code %ERRORLEVEL%.
  endlocal
  exit /b %ERRORLEVEL%
)

if /i "%BUILD_TARGET%"=="2in1" (
  for %%I in ("entry\build\default\outputs\default\entry-default-signed.hap") do echo 2IN1 HAP %%~fI ^| size=%%~zI ^| time=%%~tI
  endlocal
  exit /b 0
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%PROJECT_DIR%\..\scripts\windows\package-multidevice-app.ps1"
if errorlevel 1 (
  echo package-multidevice-app failed with exit code %ERRORLEVEL%.
  endlocal
  exit /b %ERRORLEVEL%
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%PROJECT_DIR%\..\..\tools\verify_multidevice_app.ps1"
if errorlevel 1 (
  echo verify_multidevice_app failed with exit code %ERRORLEVEL%.
  endlocal
  exit /b %ERRORLEVEL%
)

if not exist "build\outputs\default\app-default-signed.app" (
  echo Build output not found: build\outputs\default\app-default-signed.app
  endlocal
  exit /b 1
)

for %%I in ("build\outputs\default\app-default-signed.app") do echo APP %%~fI ^| size=%%~zI ^| time=%%~tI
endlocal
