#include "SDL.h"
#include "SDL_image.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdbool.h>


/* TODO: ideally all of this will be dynamic.
 * Dynamically sized levels and a dynamically resized screen
 */
const int LEVEL_WIDTH = 1920;
const int LEVEL_HEIGHT = 1440;

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int TILE_SIZE = 32;

const int HERO = 0;
const int LEGGY = 1;
const int FLOOR = 0;

struct RenderTarget {
    SDL_Window *window;
    SDL_Surface *screenSurface;
};

struct Critter {
    SDL_Surface *srcSpritemap;
    int spriteID;
    int x;
    int y;
};

enum gameState {
    EXITING,
    INIT,
    NEW_GAME,
    PLAYER_TURN,
    HERO_TURN,
    GAME_OVER
};

enum directions {
    NONE,
    NORTHWEST,
    NORTH,
    NORTHEAST,
    EAST,
    SOUTHEAST,
    SOUTH,
    SOUTHWEST,
    WEST,
    SKIP
};

int lastDirection = NONE;
bool endTurn = false;
int gameState = INIT;
int currentPlayer = 0; // FIXME: this is just a hack to get directional icons working

struct RenderTarget init();
bool initSDL2(SDL_Window *);
SDL_Surface* loadSpritemap(const char *, SDL_PixelFormat *);
SDL_Window* initWindow(SDL_Window *);
bool initImageEngine(SDL_Window *);
void generateTerrain(SDL_Surface *, SDL_Surface *);
void placeTile(SDL_Surface *, int, int, int, int, SDL_Surface *);
void place(struct Critter, SDL_Surface *);
void processInputs(SDL_Event *);
void gameLogic(struct Critter *, struct Critter **, int);
void renderDirectionIcon(SDL_Surface *, struct Critter **, SDL_Surface *);
void cleanup(SDL_Window *);

int main(int argc, char *args[])
{
    gameState = INIT;

    struct RenderTarget renderTarget = init();
    if (renderTarget.screenSurface == NULL || renderTarget.window == NULL) return EXIT_FAILURE;

    // TODO: may want to remap alpha color to something other than black
    // FIXME: doesn't check to make sure files are there
    SDL_Surface *spritemap = loadSpritemap("assets/yarz-sprites.png", renderTarget.screenSurface->format);
    SDL_Surface *terrainmap = loadSpritemap("assets/yarz-terrain.png", renderTarget.screenSurface->format);
    SDL_Surface *iconmap = loadSpritemap("assets/yarz-icons.png", renderTarget.screenSurface->format);

    struct Critter hero = {.srcSpritemap = spritemap, .spriteID = HERO, .x = 0, .y = 0 };
    struct Critter leggy = {.srcSpritemap = spritemap, .spriteID = LEGGY, .x = 128, .y = 128 };
    struct Critter leggy2 = {.srcSpritemap = spritemap, .spriteID = LEGGY, .x = 256, .y = 256 }; // FIXME: lazy enemy copy

    struct Critter *enemyList[] = {&leggy, &leggy2};

    SDL_Event e;
    gameState = PLAYER_TURN;
    while (gameState != EXITING) {
        processInputs(&e);
        gameLogic(&hero, enemyList, 2);

        generateTerrain(terrainmap, renderTarget.screenSurface);
        if (lastDirection != NONE && endTurn == false) renderDirectionIcon(iconmap, enemyList, renderTarget.screenSurface);
        place(hero, renderTarget.screenSurface);
        place(leggy, renderTarget.screenSurface);
        place(leggy2, renderTarget.screenSurface); // FIXME: lazy enemy copy
        SDL_UpdateWindowSurface(renderTarget.window);
    }


    SDL_FreeSurface(spritemap);
    SDL_FreeSurface(terrainmap);
    cleanup(renderTarget.window); // screenSurface also gets freed here, see SDL_DestroyWindow
    return EXIT_SUCCESS;
}

void renderDirectionIcon(SDL_Surface *iconmap, struct Critter *enemyList[], SDL_Surface *dst) {
    switch (lastDirection) {
        case SOUTHWEST:
        placeTile(iconmap, 0, SOUTHWEST, enemyList[currentPlayer]->x - TILE_SIZE, enemyList[currentPlayer]->y + TILE_SIZE, dst);
        break;

        case SOUTH:
        placeTile(iconmap, 0, SOUTH, enemyList[currentPlayer]->x, enemyList[currentPlayer]->y + TILE_SIZE, dst);
        break;

        case SOUTHEAST:
        placeTile(iconmap, 0, SOUTHEAST, enemyList[currentPlayer]->x + TILE_SIZE, enemyList[currentPlayer]->y + TILE_SIZE, dst);
        break;

        case EAST:
        placeTile(iconmap, 0, EAST, enemyList[currentPlayer]->x + TILE_SIZE, enemyList[currentPlayer]->y, dst);
        break;

        case NORTHEAST:
        placeTile(iconmap, 0, NORTHEAST, enemyList[currentPlayer]->x + TILE_SIZE, enemyList[currentPlayer]->y - TILE_SIZE, dst);
        break;

        case NORTH:
        placeTile(iconmap, 0, NORTH, enemyList[currentPlayer]->x, enemyList[currentPlayer]->y - TILE_SIZE, dst);
        break;

        case NORTHWEST:
        placeTile(iconmap, 0, NORTHWEST, enemyList[currentPlayer]->x - TILE_SIZE, enemyList[currentPlayer]->y - TILE_SIZE, dst);
        break;

        case WEST:
        placeTile(iconmap, 0, WEST, enemyList[currentPlayer]->x - TILE_SIZE, enemyList[currentPlayer]->y, dst);
        break;
    }

    return;
}

void gameLogic(struct Critter *hero, struct Critter *enemyList[], int totalEntities) {

    if (gameState == HERO_TURN) {
        hero->x += TILE_SIZE;
        gameState = PLAYER_TURN;
    }

    if (endTurn == true){
        switch (lastDirection) {
            case SOUTHWEST:
            enemyList[0]->x -= TILE_SIZE;
            enemyList[0]->y += TILE_SIZE;
            break;

            case SOUTH:
            enemyList[0]->y += TILE_SIZE;
            break;

            case SOUTHEAST:
            enemyList[0]->x += TILE_SIZE;
            enemyList[0]->y += TILE_SIZE;
            break;

            case EAST:
            enemyList[0]->x += TILE_SIZE;
            break;

            case NORTHEAST:
            enemyList[0]->x += TILE_SIZE;
            enemyList[0]->y -= TILE_SIZE;
            break;

            case NORTH:
            enemyList[0]->y -= TILE_SIZE;
            break;

            case NORTHWEST:
            enemyList[0]->x -= TILE_SIZE;
            enemyList[0]->y -= TILE_SIZE;
            break;

            case WEST:
            enemyList[0]->x -= TILE_SIZE;
            break;
        }

        lastDirection = NONE;
        endTurn = false;
        gameState = HERO_TURN;
    }

    return;
}

void processInputs(SDL_Event *e) {
    while (SDL_PollEvent(e)) {
        if (e->type == SDL_QUIT) {
            gameState = EXITING;
            return;
        }
        if (e->type == SDL_KEYDOWN) {
            switch (e->key.keysym.sym) {
                case SDLK_KP_1:
                lastDirection = SOUTHWEST;
                break;

                case SDLK_KP_2:
                lastDirection = SOUTH;
                break;

                case SDLK_KP_3:
                lastDirection = SOUTHEAST;
                break;

                case SDLK_KP_4:
                lastDirection = WEST;
                break;

                case SDLK_KP_5:
                lastDirection = SKIP;
                endTurn = true;
                break;

                case SDLK_KP_6:
                lastDirection = EAST;
                break;

                case SDLK_KP_7:
                lastDirection = NORTHWEST;
                break;

                case SDLK_KP_8:
                lastDirection = NORTH;
                break;

                case SDLK_KP_9:
                lastDirection = NORTHEAST;
                break;

                case SDLK_KP_ENTER:
                endTurn = true;
                break;
            }
        }
    }

    return;
}

// currently just makes a room with four walls
void generateTerrain(SDL_Surface *terrainmap, SDL_Surface *dst) {
    enum tileset {
        FLOOR,
        WALLS
    };
    enum wall {
        NORTH,
        SOUTH,
        EAST,
        WEST
    };
    for (int i = 0; i < SCREEN_WIDTH; i += 32) {
        for (int j = 0; j < SCREEN_HEIGHT; j += 32) {
            placeTile(terrainmap, FLOOR, 0, i, j, dst);
            if (j == 0) { //top of map
                placeTile(terrainmap, WALLS, NORTH, i, j, dst);
            }
            if (i == 0) { //left of map
                placeTile(terrainmap, WALLS, WEST, i, j, dst);
            }
            if (i == SCREEN_WIDTH - 32) { //right of map
                placeTile(terrainmap, WALLS, EAST, i, j, dst);
            }
            if (j == SCREEN_HEIGHT - 32) { //bottom of map
                placeTile(terrainmap, WALLS, SOUTH, i, j, dst);
            }
        }
    }
}

void place(struct Critter sprite, SDL_Surface *dst) {
    SDL_Rect srcRect = { .h = TILE_SIZE, .w = TILE_SIZE, .x = 0, .y = sprite.spriteID * 32 };
    SDL_Rect dstRect = { .h = 0, .w = 0, .x = sprite.x, .y = sprite.y };

    SDL_BlitSurface(sprite.srcSpritemap, &srcRect, dst, &dstRect);

}

void placeTile(SDL_Surface *src, int sprite, int offset, int x, int y, SDL_Surface *dst) {

    SDL_Rect srcRect = {.h = TILE_SIZE, .w = TILE_SIZE, .x = offset * 32, .y = sprite * 32};
    SDL_Rect dstRect = {.h = 0, .w = 0, .x = x, .y = y};

    SDL_BlitSurface(src, &srcRect, dst, &dstRect);
    return;
}

struct RenderTarget init() {
    SDL_Window *window = NULL;
    struct RenderTarget tempRenderTarget = { .screenSurface = NULL, .window = NULL };
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL not initialized! SDL_Error: %s\n", SDL_GetError());
        cleanup(window);
        return tempRenderTarget;
    }

    window = SDL_CreateWindow("yarz", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        cleanup(window);
        return tempRenderTarget;
    }

    int imgFlags = IMG_Init(IMG_INIT_PNG);
    if (!(imgFlags & IMG_INIT_PNG)) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
        cleanup(window);
        return tempRenderTarget;
    }

    tempRenderTarget.window = window;
    tempRenderTarget.screenSurface = SDL_GetWindowSurface(window);
    return tempRenderTarget;
}

SDL_Surface* loadSpritemap(const char *path, SDL_PixelFormat *pixelFormat) {
    SDL_Surface* image = IMG_Load(path);
    if (image == NULL) {
        printf("Unable to load image %s! SDL Error: %s\n", path, IMG_GetError());
    }
    SDL_Surface* optimizedSurface = SDL_ConvertSurface(image, pixelFormat, 0);
    if(optimizedSurface == NULL) {
        printf("Unable to optimize image %s! SDL Error: %s\n", path, SDL_GetError());
    }

    SDL_FreeSurface(image);
    SDL_SetColorKey(optimizedSurface, SDL_TRUE, SDL_MapRGB(optimizedSurface->format, 0, 0, 0));
    return optimizedSurface;
}

void cleanup (struct SDL_Window *window)
{
    IMG_Quit();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return;
}
