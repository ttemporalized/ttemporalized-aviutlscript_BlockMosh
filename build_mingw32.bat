@echo off
setlocal

where i686-w64-mingw32-gcc >nul 2>nul
if errorlevel 1 (
  echo i686-w64-mingw32-gcc was not found in PATH.
  exit /b 1
)

i686-w64-mingw32-dlltool -d lua51.def -l liblua51.a
if errorlevel 1 exit /b 1

if "%LUA51_INCLUDE%"=="" (
  echo Set LUA51_INCLUDE to the folder containing lua.h and lauxlib.h.
  exit /b 1
)

i686-w64-mingw32-gcc -std=c99 -O3 -shared -I"%LUA51_INCLUDE%" block_mosh_core.c liblua51.a -o block_mosh_core.dll
if errorlevel 1 exit /b 1

echo Built block_mosh_core.dll
