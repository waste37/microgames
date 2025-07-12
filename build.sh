#!/bin/sh
set -e

pushd flappy
./build.sh
popd
pushd tetris
./build.sh
popd
