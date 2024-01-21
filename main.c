#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

const int INITIAL_SCREEN_WIDTH = 640;
const int INITIAL_SCREEN_HEIGHT = 480;
const int TILE_SIZE = 32;

const int CAVE_WALL_PROBABILITY = 45; // int percentage, so 50 = 50%
const int CAVE_GENERATOR_ITERATIONS = 5;

const int HERO = 0;
const int LEGGY = 1;
const int BOOTS = 2;
const int FLOOR = 0;

struct Critter {
    SDL_Surface *srcSpritemap;
    int spriteID;
    int x;
    int y;
};

struct RenderTarget {
    SDL_Window *window;
    SDL_Surface *screenSurface;
    int screenWidth;
    int screenHeight;
};

struct Resources {
    SDL_Surface *level;
    SDL_Surface *sprites;
    SDL_Surface *terrain;
    SDL_Surface *icons;
    TTF_Font *gameFont;
    struct Critter *entityList;
};

// map height and width are in tiles. aka 1 = TILE_SIZE pixels
struct GameMap {
    int map_width;
    int map_height;
    int **map_array;
};

struct GameState {
    int lastInput;
    bool endTurn;
    int status;
    int currentPlayer;
    int *turnOrder;
    int currentTurn;
};

struct Camera {
    int cameraX;
    int cameraY;
    int cameraScale;
};

enum gameStatus {
    EXITING,
    INIT,
    NEW_GAME,
    PLAYER_TURN,
    HERO_TURN,
    GAME_OVER
};

enum inputs {
    NONE,
    UP_LEFT,
    UP,
    UP_RIGHT,
    RIGHT,
    DOWN_RIGHT,
    DOWN,
    DOWN_LEFT,
    LEFT,
    SKIP,
    DEBUG_GENERATE_NEW_MAP
};

int init(struct RenderTarget *, struct Resources *, struct GameState *, struct GameMap *);
int generateMap(struct GameMap *);
SDL_Surface* loadSpritemap(const char *, SDL_PixelFormat *);
void generateTerrain(struct GameMap *);
void renderTerrain(SDL_Surface *, struct GameMap *, SDL_Surface *);
void placeTile(SDL_Surface *, int, int, int, int, SDL_Surface *);
void place(struct Critter, SDL_Surface *);
void processInputs(SDL_Event *, struct GameState *, struct RenderTarget *, struct Camera *, bool *, bool *);
void gameUpdate(struct GameState *, struct Resources *, int);
int randomRange(int, int);
void shuffleTurnOrder(int**, int);
//void renderDirectionIcon(SDL_Surface *, struct Critter *[], SDL_Surface *);
SDL_Surface* updateDebugInfo(TTF_Font *, struct RenderTarget *, struct GameMap *, int);
void asciiOutputMap(struct GameMap *);
void cleanup(SDL_Window *);

// TODO: should seed RNG

int main(int argc, char *args[])
{
    struct GameState gameState = { .lastInput = NONE, .endTurn = false, .status = INIT, .currentPlayer = 0, .currentTurn = 0 };
    struct Camera camera = { .cameraX = 0, .cameraY = 0, .cameraScale = 0 };
    struct GameMap gameMap = {.map_height = randomRange(50, 100), .map_width = randomRange(50, 100), .map_array = NULL };
    bool resizing = false;
    bool debugInfoChanged = false;
    struct RenderTarget renderTarget;
    struct Resources resources;
    SDL_Event e;

    generateMap(&gameMap);
    generateTerrain(&gameMap);
    init(&renderTarget, &resources, &gameState, &gameMap);

    shuffleTurnOrder(&gameState.turnOrder, 3);

    gameState.status = PLAYER_TURN;
    SDL_Surface *debugInfo = updateDebugInfo(resources.gameFont, &renderTarget, &gameMap, camera.cameraScale);
    SDL_Rect debugTextRect = {.h = debugInfo->h, .w = debugInfo->w, .x = 0, .y = 0};

    SDL_Surface *backdrop = SDL_CreateRGBSurfaceWithFormat(0, renderTarget.screenWidth, renderTarget.screenHeight, renderTarget.screenSurface->format->BitsPerPixel, renderTarget.screenSurface->format->format);
    SDL_FillRect(backdrop, NULL, 0);
    while (gameState.status != EXITING) {
        processInputs(&e, &gameState, &renderTarget, &camera, &debugInfoChanged, &resizing);

        // any time the window is resized we must discard the old surface we got for the window and acquire a new one
        if (resizing == true) {
            SDL_FreeSurface(renderTarget.screenSurface);
            renderTarget.screenSurface = SDL_GetWindowSurface(renderTarget.window);
            SDL_FreeSurface(backdrop);
            backdrop = SDL_CreateRGBSurfaceWithFormat(0, renderTarget.screenWidth, renderTarget.screenHeight, renderTarget.screenSurface->format->BitsPerPixel, renderTarget.screenSurface->format->format);
            resizing = false;
        }

        if (gameState.lastInput == DEBUG_GENERATE_NEW_MAP) {
            generateMap(&gameMap);
            generateTerrain(&gameMap);
            SDL_FreeSurface(resources.level);
            resources.level = SDL_CreateRGBSurfaceWithFormat(0, gameMap.map_width * TILE_SIZE, gameMap.map_height * TILE_SIZE, renderTarget.screenSurface->format->BitsPerPixel, renderTarget.screenSurface->format->format);
            debugInfoChanged = true;
            gameState.lastInput = NONE;
        }

        gameUpdate(&gameState, &resources, 3);

        SDL_BlitSurface(backdrop, NULL, renderTarget.screenSurface, NULL);

        renderTerrain(resources.terrain, &gameMap, resources.level);


        // if (lastInput != NONE && endTurn == false) renderDirectionIcon(renderTarget.icons, gameState.entityList, renderTarget.level);
        place(resources.entityList[0], resources.level);
        place(resources.entityList[1], resources.level);
        place(resources.entityList[2], resources.level);

        SDL_Rect cameraRect = {.x = camera.cameraX , .y = camera.cameraY, .h = renderTarget.screenHeight + (camera.cameraScale * (renderTarget.screenHeight/100)), .w = renderTarget.screenWidth + (camera.cameraScale * (renderTarget.screenWidth/100))};
        SDL_Rect projection = {.x = 0, .y = 0, .h = renderTarget.screenHeight, .w = renderTarget.screenWidth};
        SDL_BlitScaled(resources.level, &cameraRect, renderTarget.screenSurface, &projection);

        if (debugInfoChanged) {
            SDL_FreeSurface(debugInfo);
            debugInfo = updateDebugInfo(resources.gameFont, &renderTarget, &gameMap, camera.cameraScale);
            debugTextRect.h = debugInfo->h;
            debugTextRect.w = debugInfo->w;
            debugInfoChanged = false;
        }

        SDL_BlitSurface(debugInfo, &debugTextRect, renderTarget.screenSurface, &debugTextRect);

        SDL_UpdateWindowSurface(renderTarget.window);
    }

    SDL_FreeSurface(debugInfo);
    SDL_FreeSurface(resources.sprites);
    SDL_FreeSurface(resources.terrain);
    SDL_FreeSurface(resources.icons);
    SDL_FreeSurface(resources.level);
    cleanup(renderTarget.window); // screenSurface also gets freed here, see SDL_DestroyWindow
    return EXIT_SUCCESS;
}

SDL_Surface * updateDebugInfo(TTF_Font *font, struct RenderTarget *renderTarget, struct GameMap *gameMap, int cameraScale) {
    char debugCameraText[200];
    snprintf(debugCameraText, 200,"   resolution: %dx%d\n  scale value: %d\n scaled width: %d\nscaled height: %d\n              tiles/ px\n    map width: (%d) %d\n   map height: (%d) %d",
                                     renderTarget->screenWidth,
                                     renderTarget->screenHeight,
                                     cameraScale,
                                     renderTarget->screenWidth + (cameraScale * (renderTarget->screenWidth/100)),
                                     renderTarget->screenHeight + (cameraScale * (renderTarget->screenHeight/100)),
                                     gameMap->map_width,
                                     gameMap->map_width * TILE_SIZE,
                                     gameMap->map_height,
                                     gameMap->map_height * TILE_SIZE);

    SDL_Color yellow = {.r = 227, .g = 227, .b = 18};
    return TTF_RenderUTF8_Solid_Wrapped(font, debugCameraText, yellow, 0);
}

/* FIXME: can't decide if I want to translate inputs to directions
void renderDirectionIcon(SDL_Surface *iconmap, struct Critter *entityList[], SDL_Surface *dst) {

    switch (lastInput) {
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
*/

void gameUpdate(struct GameState *gameState, struct Resources *resources, int totalEntities) {

    if (gameState->status == EXITING) {
        return;
    }

    // TODO: gameStatus NEW_GAME should show some kind of title screen
    if (gameState->status == INIT) {
        gameState->status = NEW_GAME;
    }

    if (gameState->turnOrder[gameState->currentTurn] == -1) {
        gameState->currentTurn = 0;
        shuffleTurnOrder(&gameState->turnOrder, 3); // FIXME: should not be hardcoded
    }

    gameState->currentPlayer = gameState->turnOrder[gameState->currentTurn] - 1;

    if (resources->entityList[gameState->currentPlayer].spriteID == HERO) {
        gameState->status = HERO_TURN;
    }
    else {
        gameState->status = PLAYER_TURN;
    }

    if (gameState->status == HERO_TURN) {
        resources->entityList[gameState->currentPlayer].x += TILE_SIZE;
        gameState->currentTurn += 1;
    }

    if (gameState->endTurn == true){
        switch (gameState->lastInput) {
            case DOWN_LEFT:
            resources->entityList[gameState->currentPlayer].x -= TILE_SIZE;
            resources->entityList[gameState->currentPlayer].y += TILE_SIZE;
            break;

            case DOWN:
            resources->entityList[gameState->currentPlayer].y += TILE_SIZE;
            break;

            case DOWN_RIGHT:
            resources->entityList[gameState->currentPlayer].x += TILE_SIZE;
            resources->entityList[gameState->currentPlayer].y += TILE_SIZE;
            break;

            case RIGHT:
            resources->entityList[gameState->currentPlayer].x += TILE_SIZE;
            break;

            case UP_RIGHT:
            resources->entityList[gameState->currentPlayer].x += TILE_SIZE;
            resources->entityList[gameState->currentPlayer].y -= TILE_SIZE;
            break;

            case UP:
            resources->entityList[gameState->currentPlayer].y -= TILE_SIZE;
            break;

            case UP_LEFT:
            resources->entityList[gameState->currentPlayer].x -= TILE_SIZE;
            resources->entityList[gameState->currentPlayer].y -= TILE_SIZE;
            break;

            case LEFT:
            resources->entityList[gameState->currentPlayer].x -= TILE_SIZE;
            break;
        }
        gameState->currentTurn += 1;
        gameState->lastInput = NONE;
        gameState->endTurn = false;
    }

    return;
}

void processInputs(SDL_Event *e, struct GameState *gamestate, struct RenderTarget *renderTarget, struct Camera *camera, bool *debugInfoChanged, bool *resizing) {
    while (SDL_PollEvent(e)) {
        if (e->type == SDL_QUIT) {
            gamestate->status = EXITING;
            return;
        }
        if (e->type == SDL_WINDOWEVENT){
            if (e->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                renderTarget->screenWidth = e->window.data1;
                renderTarget->screenHeight = e->window.data2;
                printf("new screen width: %d\n", renderTarget->screenWidth);
                printf("new screen height: %d\n", renderTarget->screenHeight);
                *debugInfoChanged = true;
                *resizing = true;
            }
        }

        if (e->type == SDL_KEYDOWN) {
            switch (e->key.keysym.sym) {
                case SDLK_KP_1:
                gamestate->lastInput = DOWN_LEFT;
                break;

                case SDLK_KP_2:
                gamestate->lastInput = DOWN;
                break;

                case SDLK_KP_3:
                gamestate->lastInput = DOWN_RIGHT;
                break;

                case SDLK_KP_4:
                gamestate->lastInput = LEFT;
                break;

                case SDLK_KP_5:
                gamestate->lastInput = SKIP;
                gamestate->endTurn = true;
                break;

                case SDLK_KP_6:
                gamestate->lastInput = RIGHT;
                break;

                case SDLK_KP_7:
                gamestate->lastInput = UP_LEFT;
                break;

                case SDLK_KP_8:
                gamestate->lastInput = UP;
                break;

                case SDLK_KP_9:
                gamestate->lastInput = UP_RIGHT;
                break;

                case SDLK_KP_ENTER:
                gamestate->endTurn = true;
                break;

                case SDLK_n:
                gamestate->lastInput = DEBUG_GENERATE_NEW_MAP;
                break;

                // TODO: change these keys to fully qualified camera events (zoom in, zoom out etc)
                case SDLK_w:
                camera->cameraY -= 5;
                break;

                case SDLK_a:
                camera->cameraX -= 5;
                break;

                case SDLK_s:
                camera->cameraY += 5;
                break;

                case SDLK_d:
                camera->cameraX +=5;
                break;

                case SDLK_EQUALS:
                camera->cameraScale += 1;
                *debugInfoChanged = true;
                break;

                case SDLK_MINUS:
                camera->cameraScale -= 1;
                *debugInfoChanged = true;
                break;

                case SDLK_0:
                camera->cameraScale = 0;
                *debugInfoChanged = true;
                break;
            }
        }
    }

    return;
}


// TODO: Generate more than one kind of terrain
// "simple" cave generation based on a naive implementation of the following:
// https://www.roguebasin.com/index.php?title=Cellular_Automata_Method_for_Generating_Random_Cave-Like_Levels
void generateTerrain(struct GameMap *gameMap) {

    enum {
        FLOOR,
        WALL
    };

    // first, randomize map area
    // TODO: made this map area smaller by 1 because my ass is lazy..
    // ..I mean to avoid out-of-bounds errors during generation
    for (int i = 1; i < gameMap->map_width - 1; i++) {
        for (int j = 1; j < gameMap->map_height - 1; j++) {
            int roll = randomRange(0, 100);
            if (roll < CAVE_WALL_PROBABILITY) {
                (gameMap->map_array)[i][j] = 1;
            }
            else {
                (gameMap->map_array)[i][j] = 0;
            }
        }
    }

    // The cave generator works by walking through the map several times.
    // For each location on the map, it analyzes the 8 adjacent squares.
    // if those squares contain a wall, we add 1 to our count
    for (int step = 0; step < CAVE_GENERATOR_ITERATIONS; step++) {
        for (int i = 1; i < gameMap->map_width - 1; i++) {
            for (int j = 1; j < gameMap->map_height - 1; j++) {
                int wall_count = 0;
                for (int k = 0; k < 3; k++) {
                    for (int l = 0; l < 3; l++) {
                        if ((gameMap->map_array)[i + k - 1][j + l - 1] == WALL && !(k == 1 && l == 1)) wall_count++;
                    }
                }

                // if our tile is a flimsy unsupported wall has less than four walls around it, we collapse it into a floor tile
                // if instead our tile is a floor encroached by more than 4 walls, we fill this tile with a wall
                if ((gameMap->map_array)[i][j] == WALL && wall_count < 4) (gameMap->map_array)[i][j] = FLOOR;
                if ((gameMap->map_array)[i][j] == FLOOR && wall_count > 4) (gameMap->map_array)[i][j] = WALL;
            }
        }
    }

    return;
}

// strictly for debugging purposes
void asciiOutputMap(struct GameMap *gameMap) {

    for (int i = 0; i < gameMap->map_width; i++) {
        for (int j = 0; j < gameMap->map_height; j++) {
            printf("%d", gameMap->map_array[i][j]);
        }
        printf("\n");
    }
}

void renderTerrain(SDL_Surface *terrainmap, struct GameMap *gameMap, SDL_Surface *dst) {
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
    for (int i = 0; i < gameMap->map_width; i++) {
        for (int j = 0; j < gameMap->map_height; j++) {
            if ((gameMap->map_array)[i][j] == 0) {
                placeTile(terrainmap, FLOOR, 0, i * 32, j * 32, dst);
            }
            if ((gameMap->map_array)[i][j] == 1) {
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

void shuffleTurnOrder(int **turnOrder, int number_of_entities) {
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
            (*turnOrder)[index] = pool[draw];
            pool[draw] = 0;
            index++;
            remaining--;
        }
        if (remaining == 0) {
            doneDrawing = true;
        }
    }
    (*turnOrder)[index] = -1; //mark the end of shuffled players

    printf("Turn order is player:");
    for (int i = 0; i < number_of_entities + 1; i++) {
        printf(" %d", (*turnOrder)[i]);
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

int init(struct RenderTarget *renderTarget, struct Resources *resources, struct GameState *gameState, struct GameMap *gameMap) {

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL not initialized! SDL_Error: %s\n", SDL_GetError());
        cleanup(NULL);
        gameState->status = EXITING;
        return -1;
    }

    int imgFlags = IMG_Init(IMG_INIT_PNG);
    if (!(imgFlags & IMG_INIT_PNG)) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
        cleanup(NULL);
        gameState->status = EXITING;
        return -1;
    }

    if (TTF_Init() == -1) {
        printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
        cleanup(NULL);
        gameState->status = EXITING;
        return -1;
    }

    renderTarget->window = SDL_CreateWindow("yarz", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, INITIAL_SCREEN_WIDTH, INITIAL_SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (renderTarget->window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        cleanup(renderTarget->window);
        gameState->status = EXITING;
        return -1;
    }

    renderTarget->screenSurface = SDL_GetWindowSurface(renderTarget->window);
    if (renderTarget->screenSurface == NULL) {
        printf("Could not capture screen surface! SDL_Error: %s\n", SDL_GetError());
        cleanup(renderTarget->window);
        gameState->status = EXITING;
        return -1;
    }

    renderTarget->screenWidth = INITIAL_SCREEN_WIDTH;
    renderTarget->screenHeight = INITIAL_SCREEN_HEIGHT;

    resources->sprites = loadSpritemap("assets/yarz-sprites.png", renderTarget->screenSurface->format);
    resources->terrain = loadSpritemap("assets/yarz-terrain.png", renderTarget->screenSurface->format);
    resources->icons = loadSpritemap("assets/yarz-icons.png", renderTarget->screenSurface->format);

    if (resources->sprites == NULL ||
        resources->terrain == NULL ||
        resources->icons == NULL) {

        cleanup(renderTarget->window);
        gameState->status = EXITING;
        return -1;
    }

    resources->gameFont = TTF_OpenFont("assets/MajorMonoDisplay-Regular.ttf", 14);
    if (resources->gameFont == NULL) {
        printf("Could not open font MajorMonoDisplay-Regular! TTF_Error: %s\n", TTF_GetError());

        cleanup(renderTarget->window);
        gameState->status = EXITING;
        return -1;
    }

    // this monster of a declaration basically says 'give me a surface the size of the level in the same format as the screen'
    // the other surfaces are optimized to the screen format on load
    resources->level = SDL_CreateRGBSurfaceWithFormat(0, gameMap->map_width * TILE_SIZE, gameMap->map_height * TILE_SIZE, renderTarget->screenSurface->format->BitsPerPixel, renderTarget->screenSurface->format->format);
    if (resources->level == NULL) {
        printf("Could not create level surface! SDL_Error: %s\n", SDL_GetError());
        cleanup(renderTarget->window);
        gameState->status = EXITING;
        return -1;
    }

    // FIXME: this malloc has no destroy! :3
    gameState->turnOrder = (int *) malloc(sizeof(int) * 10);
    for (int i = 0; i < 10; i++) {
        gameState->turnOrder[i] = -1;
    }

    struct Critter tempCritterList[] = { { .srcSpritemap = resources->sprites, .spriteID = HERO, .x = 0, .y = 0} ,
                                         { .srcSpritemap = resources->sprites, .spriteID = LEGGY, .x = 32, .y = 32},
                                         { .srcSpritemap = resources->sprites, .spriteID = BOOTS, .x = 64, .y = 64} };

    resources->entityList = (struct Critter *) malloc(sizeof(tempCritterList));
    memcpy(resources->entityList, tempCritterList, sizeof(tempCritterList));

    return 0;
}

int generateMap(struct GameMap *gameMap) {

    //free up the old map
    if (gameMap->map_array != NULL) {
        for (int i = 0; i < gameMap->map_width; i++) {
            free((gameMap->map_array)[i]);
        }
        free(gameMap->map_array);
    }

    // randomize map size
    gameMap->map_width = randomRange(50, 100);
    gameMap->map_height = randomRange(50, 100);

    // allocating all the space for our map
    gameMap->map_array = (int**) malloc(sizeof(int*) * gameMap->map_width);
    for (int i = 0; i < gameMap->map_width; i++) {
        (gameMap->map_array)[i] = (int *)malloc(sizeof(int) * gameMap->map_height);
    }

    // init everything to '1'
    for (int i = 0; i < gameMap->map_width; i++) {
        for (int j = 0; j < gameMap->map_height; j++) {
            (gameMap->map_array)[i][j] = 1;
        }
    }
    return 0;
}

SDL_Surface* loadSpritemap(const char *path, SDL_PixelFormat *pixelFormat) {
    SDL_Surface* image = IMG_Load(path);
    if (image == NULL) {
        printf("Unable to load image %s! SDL Error: %s\n", path, IMG_GetError());
        return NULL;
    }
    SDL_Surface* optimizedSurface = SDL_ConvertSurface(image, pixelFormat, 0);
    if(optimizedSurface == NULL) {
        printf("Unable to optimize image %s! SDL Error: %s\n", path, SDL_GetError());
        return NULL;
    }

    SDL_FreeSurface(image);

    // TODO: may want to remap alpha color to something other than black
    if (SDL_SetColorKey(optimizedSurface, SDL_TRUE, SDL_MapRGB(optimizedSurface->format, 0, 0, 0)) < 0) {
        printf("Unable to set transparent pixel for image %s! SDL Error: %s\n", path, SDL_GetError());
        return NULL;
    }

    return optimizedSurface;
}

void cleanup(struct SDL_Window *window)
{
    IMG_Quit();
    TTF_Quit();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return;
}
