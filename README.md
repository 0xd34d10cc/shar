# shar

Modern opensource P2P  streaming.

## Installation

```cmd
git clone
conan remote add —force d34dpkgs https://api.bintray.com/conan/0xd34d10cc/d34dpkgs 
conan remote add —force bincrafters https://api.bintray.com/conan/bincrafters/public-conan
conan remote add —force p1q https://api.bintray.com/conan/p1q/ffmpeg
mkdir build 
cd build
conan install --build=missing ..
cmake ..
go build
```

## Usage

```
Build solution
Replace config.json from conf to build\bin
Execute server
Execute receiver
Execute sender
```
