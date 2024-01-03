#include "SDL.h"
#include "SDL_image.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdbool.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int TILE_SIZE = 32;

const int HERO = 0;
const int ENEMY1 = 1;
const int FLOOR = 0;

bool initSDL2(SDL_Window *);
SDL_Surface* loadSpritemap(const char *, SDL_PixelFormat *);
SDL_Window* initWindow(SDL_Window *);
bool initImageEngine(SDL_Window *);
void cleanup(SDL_Window *);

void place(SDL_Surface *, int, int, int, SDL_Surface *);

int main(int argc, char *args[])
{
    SDL_Window *window = NULL;
    SDL_Surface *screenSurface = NULL;

    if (initSDL2(window) == false) return EXIT_FAILURE;              //if SDL doesn't start, bail
    if ((window = initWindow(window)) == NULL) return EXIT_FAILURE;  //if we can't create a window, bail
    if (initImageEngine(window) == false) return EXIT_FAILURE;       //if we can't load PNG, bail

    screenSurface = SDL_GetWindowSurface(window);

    // pass in screen format to correctly optimize spritemap
    SDL_Surface *spritemap = loadSpritemap("assets/yarz-sprites.png", screenSurface->format);
    SDL_SetColorKey(spritemap, SDL_TRUE, SDL_MapRGB(spritemap->format, 0, 0, 0));
    SDL_Surface *terrainmap = loadSpritemap("assets/yarz-terrain.png", screenSurface->format);

    SDL_Event e;
    int enemyx = 64;
    int enemyy = 64;
    while (true) {
        SDL_PollEvent(&e);
        if (e.type == SDL_QUIT) {
            break;
        }
        if (e.type == SDL_KEYDOWN) {
            switch(e.key.keysym.sym) {
                case SDLK_UP:
                enemyy -= 32;
                break;

                case SDLK_DOWN:
                enemyy += 32;
                break;

                case SDLK_LEFT:
                enemyx -= 32;
                break;

                case SDLK_RIGHT:
                enemyx += 32;
                break;
            }
        }
        for (int i = 0; i < 11; i++) {
            for (int j = 0; j < 11; j++) {
                place(terrainmap, FLOOR, i * 32, j * 32, screenSurface);
            }
        }

        place(spritemap, HERO, 320, 240, screenSurface);
        place(spritemap, ENEMY1, enemyx, enemyy, screenSurface);
        SDL_UpdateWindowSurface(window);
    }

    SDL_FreeSurface(spritemap);
    SDL_FreeSurface(terrainmap);
    cleanup(window); // screenSurface also gets freed here, see SDL_DestroyWindow
    return EXIT_SUCCESS;
}

void place(SDL_Surface *src, int sprite, int x, int y, SDL_Surface *dst) {

    SDL_Rect srcRect = {.h = TILE_SIZE, .w = TILE_SIZE, .x = 0, .y = sprite * 32};
    SDL_Rect dstRect = {.h = 0, .w = 0, .x = x, .y = y};

    SDL_BlitSurface(src, &srcRect, dst, &dstRect);
    return;
}

bool initSDL2(SDL_Window *window) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL not initialized! SDL_Error: %s\n", SDL_GetError());
        cleanup(window);
        return false;
    }

    return true;
}

SDL_Window* initWindow(SDL_Window *window) {
    window = SDL_CreateWindow("yarz", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        cleanup(window);
        return NULL;
    }

    return window;
}

/*
* I plan to only load PNG images. However, the image library can return 0 or more available image formats.
* This mask ensures at least one of those formats is PNG.
*
* For example, let's say we ask for PNG (2) to be loaded but only JPG and TIFF are available
* JPG is 1, TIFF is 4, so IMG_Init will return 5 (0b00000101)
* 0b00000101 - imgFlags
* 0b00000010 - png format
* ========== AND
* 0b00000000
*
* We invert this result so if no image formats are available (0) or none of them are suitable,
* the if statement runs our error code. Otherwise we fall through to loading our assets
*/
bool initImageEngine(SDL_Window *window) {
    int imgFlags = IMG_Init(IMG_INIT_PNG);

    if (!(imgFlags & IMG_INIT_PNG)) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
        cleanup(window);
        return false;
    }

    return true;
}


SDL_Surface* loadSpritemap(const char *path, SDL_PixelFormat *pixelFormat) {
    SDL_Surface* spritemap = IMG_Load(path);
    if (spritemap == NULL) {
        printf("Unable to load image %s! SDL Error: %s\n", path, IMG_GetError());
    }
    SDL_Surface* optimizedSurface = SDL_ConvertSurface(spritemap, pixelFormat, 0);
    if(optimizedSurface == NULL) {
        printf("Unable to optimize image %s! SDL Error: %s\n", path, SDL_GetError());
    }

    SDL_FreeSurface(spritemap);
    return optimizedSurface;
}

void cleanup (struct SDL_Window *window)
{
    IMG_Quit();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return;
}
