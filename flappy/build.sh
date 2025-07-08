#!/bin/sh
set -e
clang main.c -o flappy -lSDL2 -lSDL2_image -lm -lSDL2_ttf -O3 -ffast-math
