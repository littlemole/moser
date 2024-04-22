#!/bin/bash


BIN_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
SRC_DIR=$( dirname $BIN_DIR )

pushd $SRC_DIR/

#clean
rm -rf ./build

#config
cmake --preset gcc-release
#build
cmake --build --preset gcc-release
#(local) install
DESTDIR=./_install cmake --build --target install --preset gcc-release
# package
cpack --config build/CPackConfig.cmake  -G DEB

echo "package created"
# print sha256 hash
ls _packages/moser_1.0_amd64.deb
echo "md5"
cat _packages/moser_1.0_amd64.deb | md5sum
echo "sha256"
cat _packages/moser_1.0_amd64.deb | sha256sum

popd +0