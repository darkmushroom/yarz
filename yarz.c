#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "yarz.h"
#include "map.h"

const int INITIAL_SCREEN_WIDTH = 640;
const int INITIAL_SCREEN_HEIGHT = 480;
const int TILE_SIZE = 32;

// int percentage, so 50 = 50%
const int CAVE_WALL_PROBABILITY = 45;
const int CAVE_GENERATOR_ITERATIONS = 5;

const int HERO = 0;
const int LEGGY = 1;
const int BOOTS = 2;
const int FLOOR = 0;

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

enum directions {
    EMPTY,
    NORTH_WEST,
    NORTH,
    NORTH_EAST,
    EAST,
    SOUTH_EAST,
    SOUTH,
    SOUTH_WEST,
    WEST
};

// TODO: should seed RNG

int main(int argc, char *args[])
{
    // prepare resources that will live for the entirety of the runtime
    // FIXME: total_entities should be dynamic based on how many possible entities there are
    GameState gameState = { .last_input = NONE, .end_turn = false, .status = INIT, .current_player = 0, .current_turn = 0, .total_entities = 3 };
    Camera camera = { .camera_x = 0, .camera_y = 0, .camera_scale = 0 };
    GameMap gameMap = {.map_height = randomRange(50, 100), .map_width = randomRange(50, 100), .map_array = NULL };
    RenderTarget renderTarget;
    Resources resources;
    SDL_Event e;

    init(&renderTarget, &resources, &gameState, &gameMap, &camera);

    while (gameState.status != EXITING) {
        processInputs(&e, &gameState, &renderTarget, &camera);
        gameUpdate(&gameState, &resources, &gameMap, &renderTarget);
        render(&renderTarget, &camera, &resources, &gameMap, &gameState);
    }

    SDL_FreeSurface(renderTarget.debug_info);
    SDL_FreeSurface(resources.sprites);
    SDL_FreeSurface(resources.terrain);
    SDL_FreeSurface(resources.icons);
    SDL_FreeSurface(resources.level);
    cleanup(renderTarget.window); // screen_surface also gets freed here, see SDL_DestroyWindow
    return EXIT_SUCCESS;
}

void render(RenderTarget *renderTarget, Camera *camera, Resources *resources, GameMap *gameMap, GameState *gamestate) {
    // any time the window is resized we must discard the old surface we got for the window and acquire a new one
    if (renderTarget->resizing == true) {
        SDL_FreeSurface(renderTarget->screen_surface);
        renderTarget->screen_surface = SDL_GetWindowSurface(renderTarget->window);
        SDL_FreeSurface(renderTarget->backdrop);
        renderTarget->backdrop = SDL_CreateRGBSurfaceWithFormat(0, renderTarget->screen_width, renderTarget->screen_height, renderTarget->screen_surface->format->BitsPerPixel, renderTarget->screen_surface->format->format);
        renderTarget->resizing = false;
    }

    SDL_BlitSurface(renderTarget->backdrop, NULL, renderTarget->screen_surface, NULL);

    renderTerrain(resources->terrain, gameMap, resources->level);

    renderDirectionIcon(resources->icons, resources->entity_list, resources->level, gamestate);

    // if (last_input != NONE && end_turn == false) renderDirectionIcon(renderTarget.icons, gameState.entity_list, renderTarget.level);
    place(resources->entity_list[0], resources->level);
    place(resources->entity_list[1], resources->level);
    place(resources->entity_list[2], resources->level);

    SDL_Rect cameraRect = {.x = camera->camera_x, .y = camera->camera_y, .h = renderTarget->screen_height + (camera->camera_scale * (renderTarget->screen_height/100)), .w = renderTarget->screen_width + (camera->camera_scale * (renderTarget->screen_width/100))};
    SDL_Rect projection = {.x = 0, .y = 0, .h = renderTarget->screen_height, .w = renderTarget->screen_width};
    SDL_BlitScaled(resources->level, &cameraRect, renderTarget->screen_surface, &projection);

    if (renderTarget->debug_info_changed) {
        SDL_FreeSurface(renderTarget->debug_info);
        renderTarget->debug_info = updateDebugInfo(resources->game_font, renderTarget, gameMap, camera->camera_scale);
        renderTarget->debug_info_rect->h = renderTarget->debug_info->h;
        renderTarget->debug_info_rect->w = renderTarget->debug_info->w;
        renderTarget->debug_info_changed = false;
    }

    SDL_BlitSurface(renderTarget->debug_info, renderTarget->debug_info_rect, renderTarget->screen_surface, renderTarget->debug_info_rect);

    SDL_UpdateWindowSurface(renderTarget->window);
}

SDL_Surface * updateDebugInfo(TTF_Font *font, RenderTarget *renderTarget, GameMap *gameMap, int camera_scale) {
    char debugCameraText[200];
    snprintf(debugCameraText, 200,"   resolution: %dx%d\n  scale value: %d\n scaled width: %d\nscaled height: %d\n              tiles/ px\n    map width: (%d) %d\n   map height: (%d) %d",
                                     renderTarget->screen_width,
                                     renderTarget->screen_height,
                                     camera_scale,
                                     renderTarget->screen_width + (camera_scale * (renderTarget->screen_width/100)),
                                     renderTarget->screen_height + (camera_scale * (renderTarget->screen_height/100)),
                                     gameMap->map_width,
                                     gameMap->map_width * TILE_SIZE,
                                     gameMap->map_height,
                                     gameMap->map_height * TILE_SIZE);

    SDL_Color yellow = {.r = 227, .g = 227, .b = 18};
    return TTF_RenderUTF8_Solid_Wrapped(font, debugCameraText, yellow, 0);
}


void renderDirectionIcon(SDL_Surface *icons, Critter *entity_list, SDL_Surface *dst, GameState *gameState) {

    switch (gameState->last_input) {
        case DOWN_LEFT:
        placeTile(icons, 0, SOUTH_WEST, entity_list[gameState->current_player].x - TILE_SIZE, entity_list[gameState->current_player].y + TILE_SIZE, dst);
        break;

        case DOWN:
        placeTile(icons, 0, SOUTH, entity_list[gameState->current_player].x, entity_list[gameState->current_player].y + TILE_SIZE, dst);
        break;

        case DOWN_RIGHT:
        placeTile(icons, 0, SOUTH_EAST, entity_list[gameState->current_player].x + TILE_SIZE, entity_list[gameState->current_player].y + TILE_SIZE, dst);
        break;

        case RIGHT:
        placeTile(icons, 0, EAST, entity_list[gameState->current_player].x + TILE_SIZE, entity_list[gameState->current_player].y, dst);
        break;

        case UP_RIGHT:
        placeTile(icons, 0, NORTH_EAST, entity_list[gameState->current_player].x + TILE_SIZE, entity_list[gameState->current_player].y - TILE_SIZE, dst);
        break;

        case UP:
        placeTile(icons, 0, NORTH, entity_list[gameState->current_player].x, entity_list[gameState->current_player].y - TILE_SIZE, dst);
        break;

        case UP_LEFT:
        placeTile(icons, 0, NORTH_WEST, entity_list[gameState->current_player].x - TILE_SIZE, entity_list[gameState->current_player].y - TILE_SIZE, dst);
        break;

        case LEFT:
        placeTile(icons, 0, WEST, entity_list[gameState->current_player].x - TILE_SIZE, entity_list[gameState->current_player].y, dst);
        break;
    }

    return;
}


void gameUpdate(GameState *gameState, Resources *resources, GameMap *gameMap, RenderTarget *renderTarget) {

    if (gameState->status == EXITING) {
        return;
    }

    // TODO: gameStatus NEW_GAME should show some kind of title screen
    if (gameState->status == INIT) {
        gameState->status = NEW_GAME;
    }

    if (gameState->last_input == DEBUG_GENERATE_NEW_MAP) {
        generateMap(gameMap);
        generateTerrain(gameMap);
        SDL_FreeSurface(resources->level);
        resources->level = SDL_CreateRGBSurfaceWithFormat(0, gameMap->map_width * TILE_SIZE, gameMap->map_height * TILE_SIZE, renderTarget->screen_surface->format->BitsPerPixel, renderTarget->screen_surface->format->format);
        renderTarget->debug_info_changed = true;
        gameState->last_input = NONE;
    }

    if (gameState->turn_order[gameState->current_turn] == -1) {
        gameState->current_turn = 0;
        shuffleTurnOrder(&gameState->turn_order, gameState->total_entities); // FIXME: should not be hardcoded
    }

    gameState->current_player = gameState->turn_order[gameState->current_turn] - 1;

    if (resources->entity_list[gameState->current_player].sprite_ID == HERO) {
        gameState->status = HERO_TURN;
    }
    else {
        gameState->status = PLAYER_TURN;
    }

    if (gameState->status == HERO_TURN) {
        resources->entity_list[gameState->current_player].x += TILE_SIZE;
        gameState->current_turn += 1;
    }

    if (gameState->end_turn == true){
        switch (gameState->last_input) {
            case DOWN_LEFT:
            resources->entity_list[gameState->current_player].x -= TILE_SIZE;
            resources->entity_list[gameState->current_player].y += TILE_SIZE;
            break;

            case DOWN:
            resources->entity_list[gameState->current_player].y += TILE_SIZE;
            break;

            case DOWN_RIGHT:
            resources->entity_list[gameState->current_player].x += TILE_SIZE;
            resources->entity_list[gameState->current_player].y += TILE_SIZE;
            break;

            case RIGHT:
            resources->entity_list[gameState->current_player].x += TILE_SIZE;
            break;

            case UP_RIGHT:
            resources->entity_list[gameState->current_player].x += TILE_SIZE;
            resources->entity_list[gameState->current_player].y -= TILE_SIZE;
            break;

            case UP:
            resources->entity_list[gameState->current_player].y -= TILE_SIZE;
            break;

            case UP_LEFT:
            resources->entity_list[gameState->current_player].x -= TILE_SIZE;
            resources->entity_list[gameState->current_player].y -= TILE_SIZE;
            break;

            case LEFT:
            resources->entity_list[gameState->current_player].x -= TILE_SIZE;
            break;
        }
        gameState->current_turn += 1;
        gameState->last_input = NONE;
        gameState->end_turn = false;
    }

    return;
}

void processInputs(SDL_Event *e, GameState *gamestate, RenderTarget *renderTarget, Camera *camera) {
    while (SDL_PollEvent(e)) {
        if (e->type == SDL_QUIT) {
            gamestate->status = EXITING;
            return;
        }
        if (e->type == SDL_WINDOWEVENT){
            if (e->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                renderTarget->screen_width = e->window.data1;
                renderTarget->screen_height = e->window.data2;
                printf("new screen width: %d\n", renderTarget->screen_width);
                printf("new screen height: %d\n", renderTarget->screen_height);
                renderTarget->debug_info_changed = true;
                renderTarget->resizing = true;
            }
        }

        if (e->type == SDL_KEYDOWN) {
            switch (e->key.keysym.sym) {
                case SDLK_KP_1:
                gamestate->last_input = DOWN_LEFT;
                break;

                case SDLK_KP_2:
                gamestate->last_input = DOWN;
                break;

                case SDLK_KP_3:
                gamestate->last_input = DOWN_RIGHT;
                break;

                case SDLK_KP_4:
                gamestate->last_input = LEFT;
                break;

                case SDLK_KP_5:
                gamestate->last_input = SKIP;
                gamestate->end_turn = true;
                break;

                case SDLK_KP_6:
                gamestate->last_input = RIGHT;
                break;

                case SDLK_KP_7:
                gamestate->last_input = UP_LEFT;
                break;

                case SDLK_KP_8:
                gamestate->last_input = UP;
                break;

                case SDLK_KP_9:
                gamestate->last_input = UP_RIGHT;
                break;

                case SDLK_KP_ENTER:
                gamestate->end_turn = true;
                break;

                case SDLK_n:
                gamestate->last_input = DEBUG_GENERATE_NEW_MAP;
                break;

                // TODO: change these keys to fully qualified camera events (zoom in, zoom out etc)
                case SDLK_w:
                camera->camera_y -= 5;
                break;

                case SDLK_a:
                camera->camera_x -= 5;
                break;

                case SDLK_s:
                camera->camera_y += 5;
                break;

                case SDLK_d:
                camera->camera_x +=5;
                break;

                case SDLK_EQUALS:
                camera->camera_scale += 1;
                renderTarget->debug_info_changed = true;
                break;

                case SDLK_MINUS:
                camera->camera_scale -= 1;
                renderTarget->debug_info_changed = true;
                break;

                case SDLK_0:
                camera->camera_scale = 0;
                renderTarget->debug_info_changed = true;
                break;
            }
        }
    }

    return;
}

void renderTerrain(SDL_Surface *terrainmap, GameMap *gameMap, SDL_Surface *dst) {
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
                placeTile(terrainmap, FLOOR, 0, i * TILE_SIZE, j * TILE_SIZE, dst);
            }
            if ((gameMap->map_array)[i][j] == 1) {
                placeTile(terrainmap, CAVE, 0, i * TILE_SIZE, j * TILE_SIZE, dst);
            }
        }
    }
}

void place(Critter sprite, SDL_Surface *dst) {
    SDL_Rect srcRect = { .h = TILE_SIZE, .w = TILE_SIZE, .x = 0, .y = sprite.sprite_ID * TILE_SIZE };
    SDL_Rect dstRect = { .h = 0, .w = 0, .x = sprite.x, .y = sprite.y };

    SDL_BlitSurface(sprite.source_sprite_map, &srcRect, dst, &dstRect);

}

void placeTile(SDL_Surface *src, int sprite, int offset, int x, int y, SDL_Surface *dst) {

    SDL_Rect srcRect = {.h = TILE_SIZE, .w = TILE_SIZE, .x = offset * TILE_SIZE, .y = sprite * TILE_SIZE};
    SDL_Rect dstRect = {.h = 0, .w = 0, .x = x, .y = y};

    SDL_BlitSurface(src, &srcRect, dst, &dstRect);
    return;
}

void shuffleTurnOrder(int **turn_order, int number_of_entities) {
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
            (*turn_order)[index] = pool[draw];
            pool[draw] = 0;
            index++;
            remaining--;
        }
        if (remaining == 0) {
            doneDrawing = true;
        }
    }
    (*turn_order)[index] = -1; //mark the end of shuffled players

    printf("Turn order is player:");
    for (int i = 0; i < number_of_entities + 1; i++) {
        printf(" %d", (*turn_order)[i]);
        if (i < number_of_entities) printf(",");
        printf(" ");
    }
    printf("\n");

    return;
}

int init(RenderTarget *renderTarget, Resources *resources, GameState *gameState, GameMap *gameMap, Camera *camera) {

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

    renderTarget->screen_surface = SDL_GetWindowSurface(renderTarget->window);
    if (renderTarget->screen_surface == NULL) {
        printf("Could not capture screen surface! SDL_Error: %s\n", SDL_GetError());
        cleanup(renderTarget->window);
        gameState->status = EXITING;
        return -1;
    }

    renderTarget->screen_width = INITIAL_SCREEN_WIDTH;
    renderTarget->screen_height = INITIAL_SCREEN_HEIGHT;
    renderTarget->resizing = false;
    renderTarget->debug_info_changed = false;

    renderTarget->backdrop = SDL_CreateRGBSurfaceWithFormat(0, renderTarget->screen_width, renderTarget->screen_height, renderTarget->screen_surface->format->BitsPerPixel, renderTarget->screen_surface->format->format);
    SDL_FillRect(renderTarget->backdrop , NULL, 0);

    resources->sprites = loadSpritemap("assets/yarz-sprites.png", renderTarget->screen_surface->format);
    resources->terrain = loadSpritemap("assets/yarz-terrain.png", renderTarget->screen_surface->format);
    resources->icons = loadSpritemap("assets/yarz-icons.png", renderTarget->screen_surface->format);

    if (resources->sprites == NULL ||
        resources->terrain == NULL ||
        resources->icons == NULL) {

        cleanup(renderTarget->window);
        gameState->status = EXITING;
        return -1;
    }

    resources->game_font = TTF_OpenFont("assets/MajorMonoDisplay-Regular.ttf", 14);
    if (resources->game_font == NULL) {
        printf("Could not open font MajorMonoDisplay-Regular! TTF_Error: %s\n", TTF_GetError());

        cleanup(renderTarget->window);
        gameState->status = EXITING;
        return -1;
    }

    generateMap(gameMap);
    generateTerrain(gameMap);

    // this monster of a declaration basically says 'give me a surface the size of the level in the same format as the screen'
    // the other surfaces are optimized to the screen format on load
    resources->level = SDL_CreateRGBSurfaceWithFormat(0, gameMap->map_width * TILE_SIZE, gameMap->map_height * TILE_SIZE, renderTarget->screen_surface->format->BitsPerPixel, renderTarget->screen_surface->format->format);
    if (resources->level == NULL) {
        printf("Could not create level surface! SDL_Error: %s\n", SDL_GetError());
        cleanup(renderTarget->window);
        gameState->status = EXITING;
        return -1;
    }

    // FIXME: this malloc has no destroy! :3
    // FIXME: the value of 10 is hardcoded, we may have more than 10 entities that require turn shuffling
    gameState->turn_order = (int *) malloc(sizeof(int) * 10);
    for (int i = 0; i < 10; i++) {
        gameState->turn_order[i] = -1;
    }

    // TODO: This critter list should be loaded from disk
    Critter tempCritterList[] = { { .source_sprite_map = resources->sprites, .sprite_ID = HERO, .x = 0, .y = 0} ,
                                         { .source_sprite_map = resources->sprites, .sprite_ID = LEGGY, .x = 32, .y = 32},
                                         { .source_sprite_map = resources->sprites, .sprite_ID = BOOTS, .x = 64, .y = 64} };

    resources->entity_list = (Critter *) malloc(sizeof(tempCritterList));
    memcpy(resources->entity_list, tempCritterList, sizeof(tempCritterList));

    renderTarget->debug_info = updateDebugInfo(resources->game_font, renderTarget, gameMap, camera->camera_scale);
    renderTarget->debug_info_rect = (SDL_Rect *) malloc(sizeof(SDL_Rect));
    renderTarget->debug_info_rect->h = renderTarget->debug_info->h;
    renderTarget->debug_info_rect->w = renderTarget->debug_info->w;
    renderTarget->debug_info_rect->x = 0;
    renderTarget->debug_info_rect->y = 0;

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

void cleanup(SDL_Window *window)
{
    IMG_Quit();
    TTF_Quit();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return;
}
