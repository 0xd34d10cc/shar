@echo off
rem Run this script from root of repo to setup build dir for VS

if not exist .git (
  echo Run from root of repo
  exit 1
)

if not exist build (
  mkdir build
)

rem Debug
if not exist build\debug (
  mkdir build\debug
)

pushd build\debug
conan install ..\.. --build=missing -s build_type=Debug -s compiler.runtime=MDd
popd

rem Release
if not exist build\release (
  mkdir build\release
)

pushd build\release
conan install ..\.. --build=missing -s build_type=Release
popd

rem Profile (release with debug info)
rem if not exist build\profile (
rem  mkdir build\profile
rem )

rem pushd build\profile
rem conan install ..\.. --build=missing -s build_type=RelWithDebInfo
rem popd
