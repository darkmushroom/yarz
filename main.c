#include "SDL.h"
#include "SDL_image.h"
#include <stdio.h>
#include <stdbool.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

void cleanup(SDL_Window *);

int main(int argc, char *args[])
{
    SDL_Window *window = NULL;
    SDL_Surface *screenSurface = NULL;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL not initialized! SDL_Error: %s\n", SDL_GetError());
        cleanup(window);
    }

    window = SDL_CreateWindow("yarz", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        cleanup(window);
    }

    /*
    * I plan to only load PNG images. However, the image engine can return 0 or more available image formats.
    * This mask ensures at least one of those formats is PNG.
    * 
    * For example, let's say we ask for PNG to be loaded but only JPG and TIFF are available
    * JPG is 1, TIFF is 4, so IMG_Init will return 5 (0b00000101)
    * 0b00000101 - imgFlags
    * 0b00000010 - png format
    * ========== AND
    * 0b00000000
    * 
    * We invert this result so if no image formats are available (0) or none of them are suitable, 
    * the if statement runs our error code. Otherwise we fall through to loading our assets
    */
    int imgFlags = IMG_Init(IMG_INIT_PNG);

    if (!(imgFlags & IMG_INIT_PNG)) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
        cleanup(window);
    }

    // holy shit. Everything is initialized. Let's /do/ something with it
    screenSurface = SDL_GetWindowSurface(window);

    // FIXME: don't assume spriteMap loaded correctly
    SDL_Surface* spriteMap = IMG_Load("assets/yarz-sprites.png");
    SDL_BlitSurface(spriteMap, NULL, screenSurface, NULL);
    printf(SDL_GetError());
    SDL_UpdateWindowSurface(window);

    SDL_Event e; bool quit = false; while( quit == false ){ while( SDL_PollEvent( &e ) ){ if( e.type == SDL_QUIT ) quit = true; } }

    SDL_FreeSurface(spriteMap);
    cleanup(window);
    return 0;
}

void cleanup (struct SDL_Window *window)
{
    IMG_Quit();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return;
}
