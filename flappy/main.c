#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <math.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

typedef uint8_t u8;
typedef int32_t i32;
typedef uint32_t u32;
typedef int32_t b32;
typedef float f32;
typedef char byte;

#define SCREEN_WIDTH     800
#define SCREEN_HEIGHT    800
#define MS_PER_FRAME     16.666667
#define BACKGROUND_SPEED 2
#define JUMP_VELOCITY    15
#define GRAVITY          1
#define TERMINAL_SPEED   15

enum sprite_type { SPRITE_SKY = 0, SPRITE_BIRD, SPRITE_PIPE, SPRITE_PLAY, SPRITE_RESTART, SPRITE_GAMEOVER, SPRITE_COUNT };
static struct {
    SDL_Texture *texture;
    i32 width; i32 height;
} sprites[SPRITE_COUNT] = {0};

static struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font* font;
    b32 keyboard[SDL_NUM_SCANCODES];
    i32 mouse_x, mouse_y;
    b32 mouse_down, mouse_clicked;
    enum { GAME_STATE_MENU, GAME_STATE_PLAYING, GAME_STATE_GAME_OVER } state;
    i32 score;
    FILE *savefile;
    i32 highscore;
} game = {0};

static struct {
    f32 gap;
    f32 distance;
    i32 current;
    i32 to_pass;
    b32 visible[5];
} spawner = {0};

static struct {
    SDL_Rect top, bottom;
    f32 velocity;
} pipes[5];

static struct {
    SDL_Rect aabb;
    f32 velocity;
    b32 jumped;
} bird;

static struct {
    SDL_Rect r1;
    SDL_Rect r2;
} background;

SDL_Rect button_box = {SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT * 2 / 3 - 50, 100, 100}; 
SDL_Rect gameover_box = {SCREEN_WIDTH / 2 - 300, SCREEN_HEIGHT / 2 - 222, 600, 444}; 

static b32 AABBcollide(SDL_Rect a, SDL_Rect b) 
{
    if (a.x + a.w < b.x || a.x > b.x + b.w) return 0;
    if (a.y + a.h < b.y || a.y > b.y + b.h) return 0;
    return 1;
}

static i32 cleanup(void)
{
    for (i32 i = 0; i < SPRITE_COUNT; ++i) { 
        if (sprites[i].texture) SDL_DestroyTexture(sprites[i].texture); 
    }
    if (game.font) TTF_CloseFont(game.font);
    if (game.renderer) SDL_DestroyRenderer(game.renderer);
    if (game.window) SDL_DestroyWindow(game.window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}

static void set_pipe(i32 pipe, f32 pos_x, f32 pos_y)
{
    pipes[pipe].top = (SDL_Rect){
        pos_x - sprites[SPRITE_PIPE].width / 2.0f,
        pos_y - sprites[SPRITE_PIPE].height - 0.5 * spawner.gap,
        sprites[SPRITE_PIPE].width,
        sprites[SPRITE_PIPE].height
    };
    pipes[pipe].bottom = (SDL_Rect){
        pos_x - sprites[SPRITE_PIPE].width / 2.0f, 
        pos_y + 0.5 * spawner.gap,
        sprites[SPRITE_PIPE].width,
        sprites[SPRITE_PIPE].height
    };
}

static void reset(void)
{
    game.score = 0;
    bird.aabb = (SDL_Rect){
        SCREEN_WIDTH / 3.0f - sprites[SPRITE_BIRD].width / 2, 
        SCREEN_HEIGHT / 2.0f - sprites[SPRITE_BIRD].height / 2,
        sprites[SPRITE_BIRD].width,
        sprites[SPRITE_BIRD].height,
    };
    bird.velocity = 0.0f;
    bird.jumped = 0;
    spawner.gap = 3 * sprites[SPRITE_BIRD].width;
    spawner.distance = 300.0;
    spawner.current = spawner.to_pass = 0;
    for (i32 i = 0;  i < 5; ++i) { spawner.visible[i] = 0; }
    spawner.visible[0] = 1;
    set_pipe(0, SCREEN_WIDTH, SCREEN_HEIGHT / 2.0);
}

static b32 initialize(void)
{
    game.highscore = 0;
    if (!(game.savefile = fopen("savefile", "rb"))) {
        game.highscore = 0;
        game.savefile = fopen("savefile", "wb");
        fwrite(&game.highscore, sizeof(game.highscore), 1, game.savefile);
    } else {
        fread(&game.highscore, sizeof(game.highscore), 1, game.savefile);
    }

    fclose(game.savefile);


    static const char *sprite_paths[SPRITE_COUNT] = { 
        [SPRITE_SKY] = "assets/sky.jpg",
        [SPRITE_BIRD] = "assets/bird.png",
        [SPRITE_PIPE] = "assets/pipe.png",
        [SPRITE_PLAY] = "assets/play.png",
        [SPRITE_RESTART] = "assets/restart.png",
        [SPRITE_GAMEOVER] = "assets/gameover.png",
    };

    srand(time(0));
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 0;
    i32 img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!(IMG_Init(img_flags) & img_flags)) goto sdl;
    if (TTF_Init() < 0) goto img;

    game.window = SDL_CreateWindow("Flappy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!game.window) { goto all; }
    game.renderer = SDL_CreateRenderer(game.window, -1, SDL_RENDERER_ACCELERATED);
    if (!game.renderer) { goto all; }

    for (i32 i = 0; i < SPRITE_COUNT; ++i) {
        SDL_Surface *surface = IMG_Load(sprite_paths[i]);
        if (!surface) { 
            fprintf(stderr, "Failed to load image: %s\n", sprite_paths[i]);
            goto all; 
        }
        sprites[i].texture = SDL_CreateTextureFromSurface(game.renderer, surface);
        if (!sprites[i].texture) goto all;
        sprites[i].width = surface->w;
        sprites[i].height = surface->h;
        SDL_FreeSurface(surface);
    }

    game.font = TTF_OpenFont("assets/Retro Gaming.ttf", 36);
    if (!game.font) { 
        fprintf(stderr, "Failed to load font\n");
        goto all; 
    }
    background.r1 = (SDL_Rect){0, 0, sprites[SPRITE_SKY].width, sprites[SPRITE_SKY].height};
    background.r2 = (SDL_Rect){sprites[SPRITE_SKY].width, 0, sprites[SPRITE_SKY].width, sprites[SPRITE_SKY].height};
    reset();
    game.state = GAME_STATE_MENU;
    return 1;

all: return cleanup();
img: IMG_Quit();
sdl: SDL_Quit();
    return 0;
}

static void draw_sprite(i32 sprite, SDL_Rect rect, f32 angle, i32 flip)
{
    SDL_RenderCopyEx(game.renderer, sprites[sprite].texture, 0, &rect, angle, 0, flip);
}

static void draw_pipes(void) 
{
    for (i32 i = 0; i < 5; ++i) {
        if (spawner.visible[i]) {
            draw_sprite(SPRITE_PIPE, pipes[i].top, 0, SDL_FLIP_VERTICAL);
            draw_sprite(SPRITE_PIPE, pipes[i].bottom, 0, 0);
        }
    }
}

static void do_background(void)
{
    background.r1.x -= BACKGROUND_SPEED;
    background.r2.x -= BACKGROUND_SPEED;

    if (background.r1.x + background.r1.w <= 0) {
        background.r1.x = background.r2.x + background.r2.w;
    }

    if (background.r2.x + background.r2.w <= 0) {
        background.r2.x = background.r1.x + background.r1.w;
    }

    draw_sprite(SPRITE_SKY, background.r1, 0, 0);
    draw_sprite(SPRITE_SKY, background.r2, 0, 0);
}

static void draw_text(const char *text, i32 pos_x, i32 pos_y, f32 scale) 
{
    SDL_Surface* surface = TTF_RenderText_Solid(game.font, text, (SDL_Color){0, 0, 0});
    SDL_Texture* msg = SDL_CreateTextureFromSurface(game.renderer, surface);
    SDL_Rect r = { pos_x - surface->w,  pos_y - surface->h,  scale * (f32)surface->w,  scale * (f32)surface->h };
    SDL_RenderCopy(game.renderer, msg, 0, &r);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(msg);
}

static void update_playing()
{
    for (i32 i = 0; i < 5; ++i) {
        if (spawner.visible[i]) {
            pipes[i].top.x -= 5.0f;
            pipes[i].bottom.x -= 5.0f;
            spawner.visible[i] = pipes[i].top.x + pipes[i].top.w >= 0;
        }
    }

    if (SCREEN_WIDTH - pipes[spawner.current].top.x >= spawner.distance) {
        spawner.current = (spawner.current + 1) % 5;
        spawner.visible[spawner.current] = 1;
        f32 pos_x = SCREEN_WIDTH + sprites[SPRITE_PIPE].width;
        f32 pos_y = fmin(SCREEN_HEIGHT - 200, fmax(200, rand() % SCREEN_HEIGHT));
        set_pipe(spawner.current, pos_x, pos_y);
    }

    if (game.keyboard[SDL_SCANCODE_SPACE] && !bird.jumped) {
        bird.velocity = -JUMP_VELOCITY;
        bird.jumped = 1;
    } else if (!game.keyboard[SDL_SCANCODE_SPACE]) {
        bird.jumped = 0;
    }

    bird.velocity = fmin(bird.velocity + GRAVITY, TERMINAL_SPEED);
    bird.aabb.y += bird.velocity;

    if (bird.aabb.x + bird.aabb.w > pipes[spawner.to_pass].top.x + sprites[SPRITE_PIPE].width) {
        spawner.to_pass = (spawner.to_pass + 1) % 5;
        game.score++;
    }

    if (bird.aabb.y + bird.aabb.h >= SCREEN_HEIGHT || bird.aabb.y <= 0) {
        game.state = GAME_STATE_GAME_OVER;
        return;
    }

    SDL_Rect bird_hitbox = { bird.aabb.x + 20, bird.aabb.y + 5, bird.aabb.w - 20, bird.aabb.h - 10 };
    for (i32 i = 0; i < 5; ++i) {
        if (spawner.visible[i]) {
            if (AABBcollide(bird_hitbox, pipes[i].top) || AABBcollide(bird_hitbox, pipes[i].bottom)) {
                game.state = GAME_STATE_GAME_OVER;
                return;
            }
        }
    }
}

static void update_menu(void)
{
    if (game.keyboard[SDL_SCANCODE_SPACE] || (game.mouse_clicked && AABBcollide((SDL_Rect){game.mouse_x, game.mouse_y, 1, 1}, button_box))) {
        reset();
        game.state = GAME_STATE_PLAYING;
    }
}

int main(void)
{
    if (!initialize()) {
        printf("error: game couldn't start\n");
        return 1;
    }

    SDL_Event e; 
    b32 quit = 0; 
    byte textbuffer[10]; 
    while (!quit) {
        u32 start = SDL_GetTicks();
        game.mouse_clicked = 0;
        while (SDL_PollEvent(&e)) {
            switch(e.type) {
                case SDL_QUIT:            quit = 1; puts("quitting...");                        break;
                case SDL_KEYDOWN:         game.keyboard[e.key.keysym.scancode] = 1;             break;
                case SDL_KEYUP:           game.keyboard[e.key.keysym.scancode] = 0;             break;
                case SDL_MOUSEMOTION:     game.mouse_x = e.motion.x; game.mouse_y = e.motion.y; break;
                case SDL_MOUSEBUTTONDOWN: game.mouse_down = e.button.button == SDL_BUTTON_LEFT; break;
                case SDL_MOUSEBUTTONUP: 
                    if (e.button.button == SDL_BUTTON_LEFT && game.mouse_down) {
                        game.mouse_clicked = 1;
                        game.mouse_down = 0;
                    }
                    break;
                default: break;
            }
        }

        if (game.keyboard[SDL_SCANCODE_ESCAPE]) break;

        SDL_RenderClear(game.renderer);
        do_background();

        switch (game.state) {
            case GAME_STATE_MENU: 
                update_menu(); 
                bird.aabb.y = SCREEN_HEIGHT/2 + 30.0f * sinf(SDL_GetTicks() / 500.0f);
                draw_sprite(SPRITE_BIRD, bird.aabb, 1.5 * bird.velocity, 0);
                draw_sprite(SPRITE_PLAY, button_box, 0, 0);
                break;
            case GAME_STATE_PLAYING: 
                update_playing(); 
                draw_pipes();
                draw_sprite(SPRITE_BIRD, bird.aabb, 1.5 * bird.velocity, 0);
                break;
            case GAME_STATE_GAME_OVER: 
                if (game.score > game.highscore) {
                    game.highscore = game.score;
                    game.savefile = fopen("savefile", "wb");
                    fwrite(&game.highscore, sizeof(game.highscore), 1, game.savefile);
                }

                update_menu(); 
                draw_pipes();
                draw_sprite(SPRITE_BIRD, bird.aabb, 1.5 * bird.velocity, 0);
                draw_sprite(SPRITE_GAMEOVER, gameover_box, 0, 0);
                draw_sprite(SPRITE_RESTART, button_box, 0, 0);
                snprintf(textbuffer, 10, "%d", game.score);
                draw_text(textbuffer, gameover_box.x + gameover_box.w - 80, gameover_box.y + 200 + 3, 1);
                snprintf(textbuffer, 10, "%d", game.highscore);
                draw_text(textbuffer, gameover_box.x + gameover_box.w - 80, gameover_box.y + 250 - 3, 1);
                break;
        }

        snprintf(textbuffer, 10, "%d", game.score);
        draw_text(textbuffer, SCREEN_WIDTH / 2, 100, 2);
        SDL_RenderPresent(game.renderer);
        u32 elapsed = SDL_GetTicks() - start;
        f32 sleep_ms = (MS_PER_FRAME) - elapsed;
        if (sleep_ms > 0) usleep(sleep_ms * 1000);
    }
    return cleanup();
}
