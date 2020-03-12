#!/usr/bin/env bash

set -e

curDir=$(pwd)
buildDir="$curDir/custom"

pushd $buildDir > /dev/null

bin/buildsuper_x64-linux.sh 4coder_kaitsh/4coder_kaitsh.cpp

cp custom_4coder.so ../custom_4coder.so

popd > /dev/null
