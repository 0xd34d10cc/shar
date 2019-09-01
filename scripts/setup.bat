@echo off
rem Run this script from root of repo to setup build dir for VS

if not exist .git (
  echo Run from root of repo
  exit 1
)

conan remote add --force d34dpkgs https://api.bintray.com/conan/0xd34d10cc/d34dpkgs
conan remote add --force bincrafters https://api.bintray.com/conan/bincrafters/public-conan
conan remote add --force conan-transit https://api.bintray.com/conan/conan/conan-transit

if not exist build (
  mkdir build
)

rem Debug
if not exist build\debug (
  mkdir build\debug
)

pushd build\debug
conan install ..\.. --build=missing -s build_type=Debug
popd

rem Release
if not exist build\release (
  mkdir build\release
)

pushd build\release
conan install ..\.. --build=missing -s build_type=Release
popd

rem Profile (release with debug info)
if not exist build\profile (
  mkdir build\profile
)

pushd build\profile
conan install ..\.. --build=missing -s build_type=RelWithDebInfo
popd
