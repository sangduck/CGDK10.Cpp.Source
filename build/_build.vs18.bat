@ECHO ----------------------------------------------------------------------
@ECHO                            CGDK10 for C++
@ECHO                           [Network Projects]
@ECHO:
@ECHO                             Core Projects
@ECHO:
@ECHO                    build for Visual Studio 2026       
@ECHO ----------------------------------------------------------------------
@ECHO @ start Compile

@ECHO OFF
set nowpath=%cd%
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
cd /D "%nowpath%"

REM -- VCPKG 복원
setlocal
set "VCPKG_EXE="

REM 1) VCPKG_ROOT 환경변수 경로 우선 검사
if defined VCPKG_ROOT (
  if exist "%VCPKG_ROOT%\vcpkg.exe" set "VCPKG_EXE=%VCPKG_ROOT%\vcpkg.exe"
)

REM 2) PATH에서 vcpkg.exe 검색 (where 사용)
if not defined VCPKG_EXE (
  where vcpkg.exe >nul 2>&1
  if %ERRORLEVEL%==0 (
    for /f "delims=" %%P in ('where vcpkg.exe 2^>nul') do (
      set "VCPKG_EXE=%%P"
      goto :found_vcpkg
    )
  )
)

:found_vcpkg
if defined VCPKG_EXE (
  echo [vcpkg] Running "%VCPKG_EXE%" integrate install
  "%VCPKG_EXE%" integrate install
) else (
  echo [vcpkg] vcpkg.exe not found. Skipping integrate step.
)


REM -- NuGet 복원: nuget.exe가 있으면 __nuget.exe restore__ 사용, 없으면 __msbuild -t:Restore__로 복원 시도 ---
set "NUGET_EXE="

REM NUGET_HOME 환경변수 우선 검사
if defined NUGET_HOME (
  if exist "%NUGET_HOME%\nuget.exe" set "NUGET_EXE=%NUGET_HOME%\nuget.exe"
)

REM PATH에서 nuget.exe 검색
if not defined NUGET_EXE (
  where nuget.exe >nul 2>&1
  if %ERRORLEVEL%==0 (
    for /f "delims=" %%N in ('where nuget.exe 2^>nul') do (
      set "NUGET_EXE=%%N"
      goto :found_nuget
    )
  )
)

:found_nuget
echo [nuget] Restoring NuGet packages for solutions...
for /R "%~dp0" %%S in (*.vs18.slnx) do (
  findstr /I /L /C:".vcxproj" /C:".csproj" /C:".fsproj" /C:".vbproj" "%%~fS" >nul
  if ERRORLEVEL 1 (
    echo [nuget] Skipping solution without build projects: "%%~fS"
  ) else (
    if defined NUGET_EXE (
      echo [nuget] Running "%NUGET_EXE%" restore "%%~fS"
      "%NUGET_EXE%" restore "%%~fS"
    ) else (
      echo [msbuild] Running msbuild restore "%%~fS"
      msbuild "%%~fS" -t:Restore
    )
    if ERRORLEVEL 1 (
      echo [nuget] Restore failed for "%%~fS"
      exit /b 1
    )
  )
)

endlocal

pushd "%~dp0src"

@ECHO @ Compile %1
REM - in the order of the dependencies (the libraries first). stops at the first failure (exit code 1)
for %%S in (
    system\CGDK10.system.vs18.slnx
    net.socket\CGDK10.net.socket.vs18.slnx
    net.update\CGDK10.net.update.vs18.slnx
    server.system\CGDK10.server.system.vs18.slnx
    server.query\CGDK10.server.query.vs18.slnx
    server.http_api\CGDK10.server.http_api.vs18.slnx
    server.service\CGDK10.server.service.vs18.slnx
    server.web\CGDK10.server.web.vs18.slnx
    server.admin\CGDK10.server.admin.admin_server.vs18.slnx
    server.admin\CGDK10.server.admin.host_agent.service.vs18.slnx
) do (
    if /I "%3"=="rebuild" (
        msbuild %%S -t:Clean -m:1 -nr:false -p:BuildInParallel=false;Configuration=%1;Platform=%2
        if errorlevel 1 goto failed
        msbuild %%S -t:Build -m:1 -nr:false -p:BuildInParallel=false;Configuration=%1;Platform=%2
    ) else (
        msbuild %%S -t:%3 -m:1 -nr:false -p:BuildInParallel=false;Configuration=%1;Platform=%2
    )
    if errorlevel 1 goto failed
)

@ECHO ------------------------------------------
@ECHO @ Complete!!! (for Visual Studio 2026)
@ECHO     %1 (%2)
@ECHO ------------------------------------------

popd
exit /b 0

:failed
@ECHO ------------------------------------------
@ECHO @ FAILED (for Visual Studio 2026)
@ECHO     %1 (%2)
@ECHO ------------------------------------------
popd
exit /b 1
