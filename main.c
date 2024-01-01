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

bool initSDL2(SDL_Window *);
SDL_Surface* loadSpritemap(const char *, SDL_PixelFormat *);
SDL_Window* initWindow(SDL_Window *);
bool initImageEngine(SDL_Window *);
void cleanup(SDL_Window *);

int main(int argc, char *args[])
{
    SDL_Window *window = NULL;
    SDL_Surface *screenSurface = NULL;

    if (initSDL2(window) == false) return 0;              //if SDL doesn't start, bail
    if ((window = initWindow(window)) == NULL) return 0;  //if we can't create a window, bail
    if (initImageEngine(window) == false) return 0;       //if we can't load PNG, bail

    screenSurface = SDL_GetWindowSurface(window);

    // pass in screen format to correctly optimize spritemap
    SDL_Surface *spritemap = loadSpritemap("assets/yarz-sprites.png", screenSurface->format);

    SDL_Rect hero;
    hero.h = TILE_SIZE;
    hero.w = TILE_SIZE;
    hero.x = 0;
    hero.y = 0;

    SDL_Rect enemy;
    enemy.h = TILE_SIZE;
    enemy.w = TILE_SIZE;
    enemy.x = 0;
    enemy.y = 32;

    SDL_Rect heroLocation;
    heroLocation.w = 0;
    heroLocation.h = 0;
    heroLocation.x = 320;
    heroLocation.y = 240;

    SDL_Rect enemyLocation;
    enemyLocation.w = 0;
    enemyLocation.h = 0;
    enemyLocation.x = 0;
    enemyLocation.y = 0;

    SDL_BlitSurface(spritemap, &hero, screenSurface, &heroLocation);
    SDL_BlitSurface(spritemap, &enemy, screenSurface, &enemyLocation);
    
    printf(SDL_GetError());
    SDL_UpdateWindowSurface(window);

    SDL_Event e; bool quit = false; while( quit == false ){ while( SDL_PollEvent( &e ) ){ if( e.type == SDL_QUIT ) quit = true; } }

    SDL_FreeSurface(spritemap);
    cleanup(window);
    return 0;
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
