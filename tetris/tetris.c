/*
 * MicroGames
 * 
 * Started coding on Tue Jul 8 03:46:32 PM CEST 2025
 * 
 * Copyright 2025 Tiuna Pierangelo Angelini <tiuna.angelini@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 */

#include <stdint.h>
#include <stddef.h>
#include <uchar.h>
#include <stdio.h>
#include <SDL2/SDL.h>


typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef char16_t c16;

typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef char byte;
typedef ptrdiff_t isize;
typedef uintptr_t uptr;
typedef intptr_t  iptr;
typedef size_t usize;
typedef int32_t b32;

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 800

b32 keyboard[SDL_NUM_SCANCODES] = {0};

int main(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return EXIT_FAILURE;
    }

    SDL_Window *window = SDL_CreateWindow("Tetris", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) goto cleanup;
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) goto cleanup;

    b32 quit = 0;
    SDL_Event ev;
    while (!quit) {
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_QUIT:    quit = 1;                            break;
                case SDL_KEYDOWN: keyboard[e.key.keysym.scancode] = 1; break;
                case SDL_KEYUP:   keyboard[e.key.keysym.scancode] = 0; break;
                default:          /* DO NOTHING */                     break;
            }
        }
    }

cleanup;
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}
