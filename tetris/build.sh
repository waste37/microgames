#!/bin/sh

set -e
gcc tetris.c -o tetris -lSDL2
./tetris
