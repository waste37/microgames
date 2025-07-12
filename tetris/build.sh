#!/bin/sh

set -e
clang tetris.c -o tetris -lSDL2 -lm -Wall -Wextra
