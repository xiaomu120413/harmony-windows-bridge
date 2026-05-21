@echo off
setlocal
set "SCRIPT_DIR=%~dp0"
set "PROJECT_DIR=%SCRIPT_DIR:~0,-1%"
set "HVIGORW_CMD="
set "IDE_HOME="
set "JAVA_BIN_DIR="

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

if exist "%PROJECT_DIR%\..\out\xrdp-ohos-arm64\sysroot\lib\libxrdpserver.so" (
  powershell -NoProfile -ExecutionPolicy Bypass -File "%PROJECT_DIR%\..\scripts\windows\sync-xrdp-runtime.ps1"
  if errorlevel 1 (
    echo sync-xrdp-runtime failed with exit code %ERRORLEVEL%.
    endlocal
    exit /b %ERRORLEVEL%
  )

  powershell -NoProfile -ExecutionPolicy Bypass -File "%PROJECT_DIR%\..\scripts\windows\package-xrdp-hnp.ps1"
  if errorlevel 1 (
    echo package-xrdp-hnp failed with exit code %ERRORLEVEL%.
    endlocal
    exit /b %ERRORLEVEL%
  )
)

call "%HVIGORW_CMD%" --no-daemon assembleHap --mode module -p product=default -p module=entry@default
if errorlevel 1 (
  echo hvigor assembleHap failed with exit code %ERRORLEVEL%.
  endlocal
  exit /b %ERRORLEVEL%
)

if exist "%PROJECT_DIR%\entry\hnp\arm64-v8a\xrdp.hnp" (
  powershell -NoProfile -ExecutionPolicy Bypass -File "%PROJECT_DIR%\..\scripts\windows\repack-hap-with-hnp.ps1"
  if errorlevel 1 (
    echo repack-hap-with-hnp failed with exit code %ERRORLEVEL%.
    endlocal
    exit /b %ERRORLEVEL%
  )
)

if not exist "entry\build\default\outputs\default\entry-default-signed.hap" (
  echo Build output not found: entry\build\default\outputs\default\entry-default-signed.hap
  endlocal
  exit /b 1
)

for %%I in ("entry\build\default\outputs\default\entry-default-signed.hap") do echo HAP %%~fI ^| size=%%~zI ^| time=%%~tI
endlocal
