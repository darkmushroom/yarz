#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

/* TODO: ideally all of this will be dynamic.
 * Dynamically sized levels and a dynamically resized screen
 */
int SCREEN_WIDTH = 640;
int SCREEN_HEIGHT = 480;
const int TILE_SIZE = 32;

const int LEVEL_WIDTH = 60;
const int LEVEL_HEIGHT = 45;

const int CAVE_WALL_PROBABILITY = 45; // int percentage, so 50 = 50%
const int CAVE_GENERATOR_ITERATIONS = 5;

const int HERO = 0;
const int LEGGY = 1;
const int FLOOR = 0;

struct RenderTarget {
    SDL_Window *window;
    SDL_Surface *screenSurface;
    SDL_Surface *level;
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
int currentPlayer = 0;
int turnOrder[10];
int currentTurn = 0;
int cameraX = 0;
int cameraY = 0;
int cameraScale = 0;
bool resizing = false;
bool debugInfoChanged = false;

struct RenderTarget init();
SDL_Surface* loadSpritemap(const char *, SDL_PixelFormat *);
void generateTerrain(int**);
void renderTerrain(SDL_Surface *, int**, SDL_Surface *);
void placeTile(SDL_Surface *, int, int, int, int, SDL_Surface *);
void place(struct Critter, SDL_Surface *);
void processInputs(SDL_Event *);
void gameLogic(struct Critter **, int);
int randomRange(int, int);
void shuffleTurnOrder(int);
void renderDirectionIcon(SDL_Surface *, struct Critter *[], SDL_Surface *);
SDL_Surface* updateDebugInfo(TTF_Font *);
void asciiOutputMap(int**);
void cleanup(SDL_Window *);

int main(int argc, char *args[])
{
    gameState = INIT;

    struct RenderTarget renderTarget = init();
    if (renderTarget.level == NULL ||
        renderTarget.screenSurface == NULL ||
        renderTarget.window == NULL) return EXIT_FAILURE;

    // TODO: may want to remap alpha color to something other than black
    // FIXME: doesn't check to make sure files are there
    SDL_Surface *spritemap = loadSpritemap("assets/yarz-sprites.png", renderTarget.screenSurface->format);
    SDL_Surface *terrainmap = loadSpritemap("assets/yarz-terrain.png", renderTarget.screenSurface->format);
    SDL_Surface *iconmap = loadSpritemap("assets/yarz-icons.png", renderTarget.screenSurface->format);
    TTF_Font *major_mono = TTF_OpenFont("assets/MajorMonoDisplay-Regular.ttf", 24);

    struct Critter hero = {.srcSpritemap = spritemap, .spriteID = HERO, .x = 0, .y = 0 };
    struct Critter leggy = {.srcSpritemap = spritemap, .spriteID = LEGGY, .x = 128, .y = 128 };
    struct Critter leggy2 = {.srcSpritemap = spritemap, .spriteID = LEGGY, .x = 256, .y = 256 }; // FIXME: lazy enemy copy

    struct Critter *entityList[] = {&hero, &leggy, &leggy2};

    // FIXME: malloc should check for NULL, as unlikely it is our host cannot afford a few MB
    int **map;
    map = (int**)malloc(sizeof(int*) * LEVEL_WIDTH);
    for (int i = 0; i < LEVEL_WIDTH; i++) {
        map[i] = (int*)malloc(sizeof(int) * LEVEL_HEIGHT);
    }

    for (int i = 0; i < LEVEL_WIDTH; i++) {
        for (int j = 0; j < LEVEL_HEIGHT; j++) {
                map[i][j] = 1;
        }
    }

    shuffleTurnOrder(3);
    generateTerrain(map);


    SDL_Event e;
    gameState = PLAYER_TURN;
    SDL_Surface *debugInfo = updateDebugInfo(major_mono);
    SDL_Rect debugTextRect = {.h = debugInfo->h, .w = debugInfo->w, .x = 0, .y = 0};
    while (gameState != EXITING) {
        processInputs(&e);

        // any time the window is resized we must discard the old surface we got for the window and acquire a new one
        if (resizing == true) {
            SDL_FreeSurface(renderTarget.screenSurface);
            renderTarget.screenSurface = SDL_GetWindowSurface(renderTarget.window);
            resizing = false;
        }

        gameLogic(entityList, 3);

        renderTerrain(terrainmap, map, renderTarget.level);


        if (lastDirection != NONE && endTurn == false) renderDirectionIcon(iconmap, entityList, renderTarget.level);
        place(hero, renderTarget.level);
        place(leggy, renderTarget.level);
        place(leggy2, renderTarget.level); // FIXME: lazy enemy copy

        SDL_Rect camera = {.x = cameraX, .y = cameraY, .h = SCREEN_HEIGHT + (cameraScale * (SCREEN_HEIGHT/100)), .w = SCREEN_WIDTH + (cameraScale * (SCREEN_WIDTH/100))};
        SDL_Rect projection = {.x = 0, .y = 0, .h = SCREEN_HEIGHT, .w = SCREEN_WIDTH};
        SDL_BlitScaled(renderTarget.level, &camera, renderTarget.screenSurface, &projection);

        if (debugInfoChanged) {
            SDL_FreeSurface(debugInfo);
            debugInfo = updateDebugInfo(major_mono);
            debugTextRect.h = debugInfo->h;
            debugTextRect.w = debugInfo->w;
            debugInfoChanged = false;
        }

        SDL_BlitSurface(debugInfo, &debugTextRect, renderTarget.screenSurface, &debugTextRect);

        SDL_UpdateWindowSurface(renderTarget.window);
    }

    SDL_FreeSurface(debugInfo);
    SDL_FreeSurface(spritemap);
    SDL_FreeSurface(terrainmap);
    SDL_FreeSurface(iconmap);
    SDL_FreeSurface(renderTarget.level);
    cleanup(renderTarget.window); // screenSurface also gets freed here, see SDL_DestroyWindow
    return EXIT_SUCCESS;
}

SDL_Surface *updateDebugInfo(TTF_Font *font) {
    char debugCameraText[100];
    snprintf(debugCameraText, 100,"  scale value: %d\n scaled width: %d\nscaled height: %d", cameraScale, SCREEN_WIDTH + (cameraScale * (SCREEN_WIDTH/100)), SCREEN_HEIGHT + (cameraScale * (SCREEN_HEIGHT/100)));
    SDL_Color yellow = {.r = 227, .g = 227, .b = 18};
    return TTF_RenderUTF8_Solid_Wrapped(font, debugCameraText, yellow, 0);
}

void renderDirectionIcon(SDL_Surface *iconmap, struct Critter *entityList[], SDL_Surface *dst) {
    switch (lastDirection) {
        case SOUTHWEST:
        placeTile(iconmap, 0, SOUTHWEST, entityList[currentPlayer]->x - TILE_SIZE, entityList[currentPlayer]->y + TILE_SIZE, dst);
        break;

        case SOUTH:
        placeTile(iconmap, 0, SOUTH, entityList[currentPlayer]->x, entityList[currentPlayer]->y + TILE_SIZE, dst);
        break;

        case SOUTHEAST:
        placeTile(iconmap, 0, SOUTHEAST, entityList[currentPlayer]->x + TILE_SIZE, entityList[currentPlayer]->y + TILE_SIZE, dst);
        break;

        case EAST:
        placeTile(iconmap, 0, EAST, entityList[currentPlayer]->x + TILE_SIZE, entityList[currentPlayer]->y, dst);
        break;

        case NORTHEAST:
        placeTile(iconmap, 0, NORTHEAST, entityList[currentPlayer]->x + TILE_SIZE, entityList[currentPlayer]->y - TILE_SIZE, dst);
        break;

        case NORTH:
        placeTile(iconmap, 0, NORTH, entityList[currentPlayer]->x, entityList[currentPlayer]->y - TILE_SIZE, dst);
        break;

        case NORTHWEST:
        placeTile(iconmap, 0, NORTHWEST, entityList[currentPlayer]->x - TILE_SIZE, entityList[currentPlayer]->y - TILE_SIZE, dst);
        break;

        case WEST:
        placeTile(iconmap, 0, WEST, entityList[currentPlayer]->x - TILE_SIZE, entityList[currentPlayer]->y, dst);
        break;
    }

    return;
}

void gameLogic(struct Critter *entityList[], int totalEntities) {

    if (gameState == EXITING) {
        return;
    }

    if (turnOrder[currentTurn] == -1) {
        currentTurn = 0;
        shuffleTurnOrder(3); // FIXME: should not be hardcoded
    }

    currentPlayer = turnOrder[currentTurn] - 1;

    if (entityList[currentPlayer]->spriteID == HERO) {
        gameState = HERO_TURN;
    }
    else {
        gameState = PLAYER_TURN;
    }


    if (gameState == HERO_TURN) {
        entityList[currentPlayer]->x += TILE_SIZE;
        currentTurn++;
    }

    if (endTurn == true){
        switch (lastDirection) {
            case SOUTHWEST:
            entityList[currentPlayer]->x -= TILE_SIZE;
            entityList[currentPlayer]->y += TILE_SIZE;
            break;

            case SOUTH:
            entityList[currentPlayer]->y += TILE_SIZE;
            break;

            case SOUTHEAST:
            entityList[currentPlayer]->x += TILE_SIZE;
            entityList[currentPlayer]->y += TILE_SIZE;
            break;

            case EAST:
            entityList[currentPlayer]->x += TILE_SIZE;
            break;

            case NORTHEAST:
            entityList[currentPlayer]->x += TILE_SIZE;
            entityList[currentPlayer]->y -= TILE_SIZE;
            break;

            case NORTH:
            entityList[currentPlayer]->y -= TILE_SIZE;
            break;

            case NORTHWEST:
            entityList[currentPlayer]->x -= TILE_SIZE;
            entityList[currentPlayer]->y -= TILE_SIZE;
            break;

            case WEST:
            entityList[currentPlayer]->x -= TILE_SIZE;
            break;
        }
        currentTurn++;
        lastDirection = NONE;
        endTurn = false;
    }

    return;
}

void processInputs(SDL_Event *e) {
    while (SDL_PollEvent(e)) {
        if (e->type == SDL_QUIT) {
            gameState = EXITING;
            return;
        }
        if (e->type == SDL_WINDOWEVENT){
            if (e->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                SCREEN_WIDTH = e->window.data1;
                SCREEN_HEIGHT = e->window.data2;
                printf("new screen width: %d\n", SCREEN_WIDTH);
                printf("new screen height: %d\n", SCREEN_HEIGHT);
                debugInfoChanged = true;
                resizing = true;
            }
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

                case SDLK_w:
                cameraY -= 5;
                break;

                case SDLK_a:
                cameraX -= 5;
                break;

                case SDLK_s:
                cameraY += 5;
                break;

                case SDLK_d:
                cameraX +=5;
                break;

                case SDLK_EQUALS:
                cameraScale += 1;
                debugInfoChanged = true;
                break;

                case SDLK_MINUS:
                cameraScale -= 1;
                debugInfoChanged = true;
                break;

                case SDLK_0:
                cameraScale = 0;
                debugInfoChanged = true;
                break;
            }
        }
    }

    return;
}


// TODO: Generate more than one kind of terrain
// "simple" cave generation based on a naive implementation of the following:
// https://www.roguebasin.com/index.php?title=Cellular_Automata_Method_for_Generating_Random_Cave-Like_Levels
void generateTerrain(int **map) {

    enum {
        FLOOR,
        WALL
    };

    // first, randomize map area
    // TODO: made this map area smaller by 1 because my ass is lazy..
    // ..I mean to avoid out-of-bounds errors during generation
    for (int i = 1; i < LEVEL_WIDTH - 1; i++) {
        for (int j = 1; j < LEVEL_HEIGHT - 1; j++) {
            int roll = randomRange(0, 100);
            if (roll < CAVE_WALL_PROBABILITY) {
                map[i][j] = 1;
            }
            else {
                map[i][j] = 0;
            }
        }
    }

    // The cave generator works by walking through the map several times.
    // For each location on the map, it analyzes the 8 adjacent squares.
    // if those squares contain a wall, we add 1 to our count
    for (int step = 0; step < CAVE_GENERATOR_ITERATIONS; step++) {
        for (int i = 1; i < LEVEL_WIDTH - 1; i++) {
            for (int j = 1; j < LEVEL_HEIGHT - 1; j++) {
                int wall_count = 0;
                for (int k = 0; k < 3; k++) {
                    for (int l = 0; l < 3; l++) {
                        if (map[i + k - 1][j + l - 1] == WALL && !(k == 1 && l == 1)) wall_count++;
                    }
                }

                // if our tile is a flimsy unsupported wall has less than four walls around it, we collapse it into a floor tile
                // if instead our tile is a floor encroached by more than 4 walls, we fill this tile with a wall
                if (map[i][j] == WALL && wall_count < 4) map[i][j] = FLOOR;
                if (map[i][j] == FLOOR && wall_count > 4) map[i][j] = WALL;
            }
        }
    }

    asciiOutputMap(map);

    return;
}

// strictly for debugging purposes
void asciiOutputMap(int **map) {

    for (int i = 0; i < LEVEL_WIDTH; i++) {
        for (int j = 0; j < LEVEL_HEIGHT; j++) {
            printf("%d", map[i][j]);
        }
        printf("\n");
    }
}

void renderTerrain(SDL_Surface *terrainmap, int **map, SDL_Surface *dst) {
    enum tileset {
        CAVE,
        FLOOR,
        WALLS
    };
    enum wall {
        NORTH,
        SOUTH,
        EAST,
        WEST
    };
    for (int i = 0; i < LEVEL_WIDTH; i++) {
        for (int j = 0; j < LEVEL_HEIGHT; j++) {
            if (map[i][j] == 0) {
                placeTile(terrainmap, FLOOR, 0, i * 32, j * 32, dst);
            }
            if (map[i][j] == 1) {
                placeTile(terrainmap, CAVE, 0, i * 32, j * 32, dst);
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

void shuffleTurnOrder(int number_of_entities) {
    //set up temporary pool of entities
    int pool[number_of_entities];
    memset(pool, 0, number_of_entities*sizeof(int));

    //seed the pool
    for (int i = 0; i < number_of_entities; i++) {
        pool[i] = i+1;
    }

    bool doneDrawing = false;
    int remaining = number_of_entities;
    int index = 0;
    while (!doneDrawing) {
        int draw = randomRange(0, number_of_entities - 1);
        if (pool[draw] != 0) {
            turnOrder[index] = pool[draw];
            pool[draw] = 0;
            index++;
            remaining--;
        }
        if (remaining == 0) {
            doneDrawing = true;
        }
    }
    turnOrder[index] = -1; //mark the end of shuffled players

    printf("Turn order is player:");
    for (int i = 0; i < number_of_entities + 1; i++) {
        printf(" %d", turnOrder[i]);
        if (i < number_of_entities) printf(",");
        printf(" ");
    }
    printf("\n");

    return;
}

// completely stolen from https://stackoverflow.com/a/18386648, ty vitim.us!
// https://stackoverflow.com/users/938822/vitim-us
int randomRange(int min, int max) {
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
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

struct RenderTarget init() {
    SDL_Window *window = NULL;
    struct RenderTarget tempRenderTarget = { .screenSurface = NULL, .window = NULL };
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL not initialized! SDL_Error: %s\n", SDL_GetError());
        cleanup(window);
        return tempRenderTarget;
    }

    window = SDL_CreateWindow("yarz", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
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

    if (TTF_Init() == -1) {
        printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
        cleanup(window);
        return tempRenderTarget;
    }

    // FIXME: screenSurface creation and level creation do not check for errors
    tempRenderTarget.window = window;
    tempRenderTarget.screenSurface = SDL_GetWindowSurface(window);

    // this monster of a declaration basically says 'give me a surface the size of the level in the same format as the screen'
    // the other surfaces are optimized to the screen format on load
    tempRenderTarget.level = SDL_CreateRGBSurfaceWithFormat(0, LEVEL_WIDTH * TILE_SIZE, LEVEL_HEIGHT * TILE_SIZE, tempRenderTarget.screenSurface->format->BitsPerPixel, tempRenderTarget.screenSurface->format->format);

    return tempRenderTarget;
}

void cleanup(struct SDL_Window *window)
{
    IMG_Quit();
    TTF_Quit();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return;
}
