@echo off
rem Run this script from root of repo to setup build dir for VS

if not exist build (
  mkdir build
)

if not exist build\debug (
  mkdir build\debug
)

if not exist build\debug\conanbuildinfo.cmake (
  pushd build\debug
  conan install ..\.. --build=missing -s build_type=Debug
  popd
) else (
  echo Debug: conan build info already configured
)

if not exist build\release (
  mkdir build\release
)

if not exist build\release\conanbuildinfo.cmake (
  pushd build\release
  conan install ..\.. --build=missing -s build_type=Release
  popd
) else (
  echo Release: conan build info already configured
)