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
 */

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <uchar.h>
#include <unistd.h>

#include <SDL2/SDL.h>
typedef int8_t i8;
typedef int32_t i32;
typedef uint32_t u32;
typedef float f32;
typedef int32_t b32;

#define MS_PER_FRAME 16.666667
#define HORZ_SPEED   1.7
#define HORZ_INITIAL_DELAY 0.2f
#define HORZ_REPEAT_DELAY 0.05f
#define VERT_SPEED   2.7
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 800
#define SCREEN_MARGIN 50
#define NUMCOLS 10
#define NUMROWS 20
#define MATRIX_ORIGIN_X SCREEN_MARGIN
#define MATRIX_ORIGIN_Y SCREEN_MARGIN
#define MATRIXSIDEPX ((SCREEN_HEIGHT - 2 * SCREEN_MARGIN) / NUMROWS)
#define MATRIX_WIDTH  (NUMCOLS * MATRIXSIDEPX)
#define MATRIX_HEIGHT (NUMROWS * MATRIXSIDEPX)

enum tetromino_type {TETROMINO_I=0,TETROMINO_J,TETROMINO_L,TETROMINO_O,TETROMINO_S,TETROMINO_T,TETROMINO_Z};
typedef struct {
    i8 *off; 
    u32 color;
    i32 grid_x, grid_y;  // Grid coordinates instead of continuous positions
    f32 last_move_time;  // For timing gravity
    i8 type;
    i8 state;
} tetromino;


/* STATIC DATA *********************************/
b32 keyboard[SDL_NUM_SCANCODES] = {0};
u32 grid[NUMCOLS * NUMROWS]     = {0}; // colors stored as 0xrrggbb. 0 means empty cell.
SDL_Rect grid_rects[NUMCOLS * NUMROWS] = {0};
#define OFFTBL(x, y) ((x) + (y) * NUMCOLS)
static i8 offsets_table[7][4][4] = {
    [TETROMINO_I] = {                
        {OFFTBL(0,1),OFFTBL(1,1),OFFTBL(2,1),OFFTBL(3,1)},{OFFTBL(2,0),OFFTBL(2,1),OFFTBL(2,2),OFFTBL(2,3)},
        {OFFTBL(0,2),OFFTBL(1,2),OFFTBL(2,2),OFFTBL(3,2)},{OFFTBL(1,0),OFFTBL(1,1),OFFTBL(1,2),OFFTBL(1,3)},
    }, [TETROMINO_J] = {
        {OFFTBL(0,0),OFFTBL(0,1),OFFTBL(1,1),OFFTBL(2,1)},{OFFTBL(1,0),OFFTBL(2,0),OFFTBL(1,1),OFFTBL(1,2)},
        {OFFTBL(0,1),OFFTBL(1,1),OFFTBL(2,1),OFFTBL(2,2)},{OFFTBL(1,0),OFFTBL(1,1),OFFTBL(0,2),OFFTBL(1,2)},
    }, [TETROMINO_L] = {
        {OFFTBL(2,0),OFFTBL(0,1),OFFTBL(1,1),OFFTBL(2,1)},{OFFTBL(1,0),OFFTBL(1,1),OFFTBL(1,2),OFFTBL(2,2)},
        {OFFTBL(0,1),OFFTBL(1,1),OFFTBL(2,1),OFFTBL(0,2)},{OFFTBL(0,0),OFFTBL(1,0),OFFTBL(1,1),OFFTBL(1,2)},
    }, [TETROMINO_O] = {
        {OFFTBL(0,0),OFFTBL(1,0),OFFTBL(0,1),OFFTBL(1,1)},{OFFTBL(0,0),OFFTBL(1,0),OFFTBL(0,1),OFFTBL(1,1)},
        {OFFTBL(0,0),OFFTBL(1,0),OFFTBL(0,1),OFFTBL(1,1)},{OFFTBL(0,0),OFFTBL(1,0),OFFTBL(0,1),OFFTBL(1,1)},
    }, [TETROMINO_S] = {
        {OFFTBL(1,0),OFFTBL(2,0),OFFTBL(0,1),OFFTBL(1,1)},{OFFTBL(1,0),OFFTBL(1,1),OFFTBL(2,1),OFFTBL(2,2)},
        {OFFTBL(1,1),OFFTBL(2,1),OFFTBL(0,2),OFFTBL(1,2)},{OFFTBL(0,0),OFFTBL(0,1),OFFTBL(1,1),OFFTBL(1,2)},
    }, [TETROMINO_T] = {
        {OFFTBL(1,0),OFFTBL(0,1),OFFTBL(1,1),OFFTBL(2,1)},{OFFTBL(1,0),OFFTBL(1,1),OFFTBL(2,1),OFFTBL(1,2)},
        {OFFTBL(0,1),OFFTBL(1,1),OFFTBL(1,2),OFFTBL(2,1)},{OFFTBL(1,0),OFFTBL(0,1),OFFTBL(1,1),OFFTBL(1,2)},
    }, [TETROMINO_Z] = {
        {OFFTBL(0,0),OFFTBL(1,0),OFFTBL(1,1),OFFTBL(2,1)},{OFFTBL(2,0),OFFTBL(1,1),OFFTBL(2,1),OFFTBL(1,2)},
        {OFFTBL(0,1),OFFTBL(1,1),OFFTBL(1,2),OFFTBL(2,2)},{OFFTBL(1,0),OFFTBL(0,1),OFFTBL(1,1),OFFTBL(0,2)},
    }
};
#undef OFFTBL
static f32 gravity_delays[] = { 0.5f, 0.45f, 0.4f, 0.3f, 0.2f };
struct {
    SDL_Rect rows[NUMROWS + 1];
    SDL_Rect cols[NUMCOLS + 1];
} gridlines;

/*** CODE **************************************/
static void init_cell_rects(void) 
{
    for (i32 i = 0; i < NUMCOLS * NUMROWS; ++i) {
        grid_rects[i] = (SDL_Rect){
            MATRIX_ORIGIN_X + (i % NUMCOLS) * MATRIXSIDEPX, 
            MATRIX_ORIGIN_Y + (i / NUMCOLS) * MATRIXSIDEPX, 
            MATRIXSIDEPX, MATRIXSIDEPX
        };
    }
}

static void init_gridlines(void) 
{
    for (i32 i = 0; i < NUMROWS + 1; ++i) 
        gridlines.rows[i] = (SDL_Rect){MATRIX_ORIGIN_X, MATRIX_ORIGIN_Y+i*MATRIXSIDEPX, MATRIX_ORIGIN_X+MATRIX_WIDTH, MATRIX_ORIGIN_Y+i*MATRIXSIDEPX};
    for (i32 i = 0; i < NUMCOLS + 1; ++i) 
        gridlines.cols[i] = (SDL_Rect){MATRIX_ORIGIN_X+i*MATRIXSIDEPX, MATRIX_ORIGIN_Y, MATRIX_ORIGIN_X+i*MATRIXSIDEPX, MATRIX_ORIGIN_Y+MATRIX_HEIGHT};
}

static void draw_gridlines(SDL_Renderer *renderer) 
{
    SDL_SetRenderDrawColor(renderer, 0x33, 0x44, 0x66, 0xff);
    for (SDL_Rect *p = gridlines.rows; p != gridlines.rows + NUMROWS + 1; ++p) 
        SDL_RenderDrawLine(renderer, p->x, p->y, p->w, p->h);
    for (SDL_Rect *p = gridlines.cols; p != gridlines.cols + NUMCOLS + 1; ++p) 
        SDL_RenderDrawLine(renderer, p->x, p->y, p->w, p->h);
}

#define grid2x(pos) (MATRIX_ORIGIN_X + ((pos) % NUMCOLS)*MATRIXSIDEPX)
#define grid2y(pos) ()

static b32 is_position_valid(tetromino t) 
{
    for (i32 i = 0; i < 4; i++) {
        i32 block_x = t.grid_x + (t.off[i] % NUMCOLS);
        i32 block_y = t.grid_y + (t.off[i] / NUMCOLS);
        if (block_x < 0 || block_x >= NUMCOLS) return 0;
        if (block_y >= NUMROWS) return 0;
        if (block_y >= 0 && grid[block_y * NUMCOLS + block_x] != 0) return 0;
    }
    return 1;
}

static tetromino trotate(tetromino t, b32 clockwise) 
{
    t.state = (t.state + 1 + (!clockwise * 2)) % 4;
    t.off = offsets_table[t.type][t.state];
    return t;
}

static tetromino tnext(void) 
{
    static u32 colors[] = {0x00ffff, 0x00ff00, 0xff0000, 0xffff00, 0x0000ff, 0xff00ff, 0xffffff};
    tetromino t = {0};
    t.state = 0;
    t.type = rand() % 7;
    t.last_move_time = 0;
    
    if (t.type == TETROMINO_I) {
        t.grid_x = 3;
        t.grid_y = -1;
    } else if (t.type == TETROMINO_O) {
        t.grid_x = 4;
        t.grid_y = 0;
    } else {
        t.grid_x = 3;
        t.grid_y = 0;
    }
    
    t.off = offsets_table[t.type][0];
    t.color = colors[t.type];
    return t;
}

static void tdraw(SDL_Renderer *renderer, tetromino t) 
{
    SDL_SetRenderDrawColor(renderer, (t.color & 0xff0000) >> 16, (t.color & 0xff00) >> 8, (t.color & 0xff), 0xff);
    SDL_Rect rs[4];
    for (i32 i = 0; i < 4; ++i) {
        i32 block_x = t.grid_x + (t.off[i] % NUMCOLS);
        i32 block_y = t.grid_y + (t.off[i] / NUMCOLS);
        rs[i] = (SDL_Rect){MATRIX_ORIGIN_X+block_x*MATRIXSIDEPX, MATRIX_ORIGIN_Y+block_y*MATRIXSIDEPX, MATRIXSIDEPX, MATRIXSIDEPX};
    }
    SDL_RenderFillRects(renderer, rs, 4);
}

b32 game_started = 0;
static i32 level = 0;
static tetromino update_game(tetromino t, f32 current_time) 
{
    static b32 rotating = 0;
    static f32 next_horizontal_move_time = 0.0f;
    static i8 last_horizontal_direction = 0;
    // Handle rotation
    if (keyboard[SDL_SCANCODE_D] && !rotating) {
        tetromino rotated = trotate(t, 1);
        if (is_position_valid(rotated)) t = rotated;
        rotating = 1;
    } else if (keyboard[SDL_SCANCODE_A] && !rotating) {
        tetromino rotated = trotate(t, 0);
        if (is_position_valid(rotated)) t = rotated;
        rotating = 1;
    }
    if (rotating && !keyboard[SDL_SCANCODE_A] && !keyboard[SDL_SCANCODE_D]) rotating = 0;
    
    i8 horz_dir = 0;
    if (keyboard[SDL_SCANCODE_LEFT]) horz_dir = -1;
    if (keyboard[SDL_SCANCODE_RIGHT]) horz_dir = 1;
    
    if (horz_dir != 0) {
        if (horz_dir != last_horizontal_direction) {
            last_horizontal_direction = horz_dir;
            next_horizontal_move_time = current_time + HORZ_INITIAL_DELAY;
            t.grid_x += horz_dir;
            if (!is_position_valid(t)) t.grid_x -= horz_dir;
        } else if (current_time >= next_horizontal_move_time) {
            t.grid_x += horz_dir;
            if (!is_position_valid(t)) t.grid_x -= horz_dir;
            next_horizontal_move_time = current_time + HORZ_REPEAT_DELAY;
        }
    } else last_horizontal_direction = 0;
    
    // Handle gravity
    f32 gravity_delay = keyboard[SDL_SCANCODE_DOWN] ? 0.02f : gravity_delays[level];
    if (current_time - t.last_move_time > gravity_delay) {
        t.grid_y++;
        t.last_move_time = current_time;
        
        if (!is_position_valid(t)) {
            t.grid_y--;
            // Lock the piece
            for (i32 i = 0; i < 4; i++) {
                i32 block_x = t.grid_x + (t.off[i] % NUMCOLS);
                i32 block_y = t.grid_y + (t.off[i] / NUMCOLS);
                grid[block_y * NUMCOLS + block_x] = t.color;
            }
            t = tnext();
            if (!is_position_valid(t)) {
                game_started = 0;
                return t;
            }
            t.last_move_time = current_time;
        }
    }

    i32 target = NUMROWS - 1, j;
    for (i32 cursor = NUMROWS - 1; cursor >= 0; --cursor) {
        for (j = 0; j < NUMCOLS; ++j) if (!grid[cursor*NUMCOLS + j]) break;
        if (j != NUMCOLS) {
            if (target != cursor) memcpy(grid + target*NUMCOLS, grid + cursor*NUMCOLS, NUMCOLS*sizeof(u32));
            --target;
        }
    }
    while (target >= 0) memset(grid + target-- * NUMCOLS, 0, NUMCOLS * sizeof(u32));
  
    return t;
}

i32 main(void)
{
    srand(time(NULL));
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return EXIT_FAILURE;
    SDL_Window *window = SDL_CreateWindow("Tetris", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) goto window;
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) goto all;

    init_gridlines();
    init_cell_rects();

    tetromino t = tnext();
    b32 quit = 0;
    SDL_Event ev;
    //f32 dt = 0; /* delta time in seconds */
    //printf("(%f, %f) -> %d\n", t.aabb.x, t.aabb.y, t.pos);
    while (!quit) {
        u32 frame_start = SDL_GetTicks();
        f32 current_time = frame_start / 1000.0f;  // Convert to seconds

        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_QUIT:    quit = 1;                             break;
                case SDL_KEYDOWN: keyboard[ev.key.keysym.scancode] = 1; break;
                case SDL_KEYUP:   keyboard[ev.key.keysym.scancode] = 0; break;
                default:          /* NO-OP */                           break;
            }
        }

        if (keyboard[SDL_SCANCODE_SPACE]) {
            memset(grid, 0, NUMCOLS * NUMROWS * sizeof(u32));
            game_started = 1;
        }
        if (keyboard[SDL_SCANCODE_ESCAPE]) quit = 1;
        if (game_started) t = update_game(t, current_time);
        SDL_SetRenderDrawColor(renderer, 0x2d, 0x15, 0x81, 0xff);
        SDL_RenderClear(renderer);
        tdraw(renderer, t);
        for (i32 i = 0; i < NUMCOLS * NUMROWS; ++i) {
            if (grid[i]) { 
                SDL_SetRenderDrawColor(renderer, (grid[i]&0xff0000)>>16, (grid[i] & 0xff00) >> 8, (grid[i] & 0xff), 0xff);
                SDL_RenderFillRect(renderer, &grid_rects[i]); 
            }
        }

        draw_gridlines(renderer);
        SDL_RenderPresent(renderer);
        u32 elapsed = SDL_GetTicks() - frame_start;
        if (elapsed < MS_PER_FRAME) SDL_Delay(MS_PER_FRAME - elapsed);
    }

all:    if (renderer) SDL_DestroyRenderer(renderer);
window: if (window)   SDL_DestroyWindow(window);
    SDL_Quit();
}
