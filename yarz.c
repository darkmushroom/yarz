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
    GameState game_state =
        { .last_input = NONE, .end_turn = false, .status = INIT,
          .current_player = 0, .current_turn = 0, .total_entities = 3 };

    Camera camera = { .x = 0, .y = 0, .scale = 0 };
    GameMap game_map = { .map_array = NULL };
    game_map.height = randomRange(50, 100);
    game_map.width = randomRange(50, 100);
    RenderTarget render_target;
    Resources resources;
    SDL_Event e;

    init(&render_target, &resources, &game_state, &game_map, &camera);

    while (game_state.status != EXITING) {
        processInputs(&e, &game_state, &render_target, &camera);
        gameUpdate(&game_state, &resources, &game_map, &render_target);
        render(&render_target, &camera, &resources, &game_map, &game_state);
    }

    SDL_FreeSurface(render_target.debug_info);
    SDL_FreeSurface(resources.sprites);
    SDL_FreeSurface(resources.terrain);
    SDL_FreeSurface(resources.icons);
    SDL_FreeSurface(resources.level);
    cleanup(render_target.window); // screen_surface also gets freed here, see SDL_DestroyWindow
    return EXIT_SUCCESS;
}

void render(RenderTarget *render_target, Camera *camera, Resources *resources,
            GameMap *game_map, GameState *game_state) {
    // any time the window is resized we must discard the old surface we got for
    // the window and acquire a new one
    if (render_target->resizing == true) {
        SDL_FreeSurface(render_target->screen_surface);
        render_target->screen_surface =
            SDL_GetWindowSurface(render_target->window);

        SDL_FreeSurface(render_target->backdrop);
        render_target->backdrop =
            SDL_CreateRGBSurfaceWithFormat(0, render_target->screen_width,
                render_target->screen_height,
                render_target->screen_surface->format->BitsPerPixel,
                render_target->screen_surface->format->format
            );

        render_target->resizing = false;
    }

    SDL_BlitSurface(render_target->backdrop, NULL,
        render_target->screen_surface, NULL);

    renderTerrain(resources->terrain, game_map, resources->level);

    if (game_state->last_input != NONE && game_state->end_turn == false) {
        renderDirectionIcon(resources->icons, resources->entity_list,
                            resources->level, game_state);
    }

    place(resources->entity_list[0], resources->level);
    place(resources->entity_list[1], resources->level);
    place(resources->entity_list[2], resources->level);

    SDL_Rect cameraRect = {.x = camera->x, .y = camera->y};
    cameraRect.h = render_target->screen_height
                   + (camera->scale * (render_target->screen_height/100));

    cameraRect.w = render_target->screen_width
                   + (camera->scale * (render_target->screen_width/100));

    SDL_Rect projection = {.x = 0, .y = 0, .h = render_target->screen_height,
                                           .w = render_target->screen_width};

    SDL_BlitScaled(resources->level, &cameraRect, render_target->screen_surface,
                   &projection);

    if (render_target->debug_info_changed) {
        SDL_FreeSurface(render_target->debug_info);
        render_target->debug_info = updateDebugInfo(resources->game_font,
                                        render_target, game_map, camera->scale);

        render_target->debug_info_rect->h = render_target->debug_info->h;
        render_target->debug_info_rect->w = render_target->debug_info->w;
        render_target->debug_info_changed = false;
    }

    SDL_BlitSurface(render_target->debug_info, render_target->debug_info_rect,
        render_target->screen_surface, render_target->debug_info_rect);

    SDL_UpdateWindowSurface(render_target->window);
}

SDL_Surface * updateDebugInfo(TTF_Font *font, RenderTarget *render_target,
                              GameMap *game_map, int camera_scale) {
    char debugCameraText[200];
    snprintf(debugCameraText, 200,
        "   resolution: %dx%d\n"
        "  scale value: %d\n"
        " scaled width: %d\n"
        "scaled height: %d\n"
        "              tiles/ px\n"
        "    map width: (%d) %d\n"
        "   map height: (%d) %d",
        render_target->screen_width, render_target->screen_height,
        camera_scale,
        render_target->screen_width
        + (camera_scale * (render_target->screen_width/100)),
        render_target->screen_height
        + (camera_scale * (render_target->screen_height/100)),
        game_map->width, game_map->width * TILE_SIZE,
        game_map->height, game_map->height * TILE_SIZE);

    SDL_Color yellow = {.r = 227, .g = 227, .b = 18};
    return TTF_RenderUTF8_Solid_Wrapped(font, debugCameraText, yellow, 0);
}


void renderDirectionIcon(SDL_Surface *icons, Critter *entity_list,
    SDL_Surface *destination, GameState *game_state) {

    switch (game_state->last_input) {
        case DOWN_LEFT:
        placeTile(icons, 0, SOUTH_WEST,
            entity_list[game_state->current_player].x - TILE_SIZE,
            entity_list[game_state->current_player].y + TILE_SIZE, destination);
        break;

        case DOWN:
        placeTile(icons, 0, SOUTH,
            entity_list[game_state->current_player].x,
            entity_list[game_state->current_player].y + TILE_SIZE, destination);
        break;

        case DOWN_RIGHT:
        placeTile(icons, 0, SOUTH_EAST,
            entity_list[game_state->current_player].x + TILE_SIZE,
            entity_list[game_state->current_player].y + TILE_SIZE, destination);
        break;

        case RIGHT:
        placeTile(icons, 0, EAST,
            entity_list[game_state->current_player].x + TILE_SIZE,
            entity_list[game_state->current_player].y, destination);
        break;

        case UP_RIGHT:
        placeTile(icons, 0, NORTH_EAST,
            entity_list[game_state->current_player].x + TILE_SIZE,
            entity_list[game_state->current_player].y - TILE_SIZE, destination);
        break;

        case UP:
        placeTile(icons, 0, NORTH,
            entity_list[game_state->current_player].x,
            entity_list[game_state->current_player].y - TILE_SIZE, destination);
        break;

        case UP_LEFT:
        placeTile(icons, 0, NORTH_WEST,
            entity_list[game_state->current_player].x - TILE_SIZE,
            entity_list[game_state->current_player].y - TILE_SIZE, destination);
        break;

        case LEFT:
        placeTile(icons, 0, WEST,
            entity_list[game_state->current_player].x - TILE_SIZE,
            entity_list[game_state->current_player].y, destination);
        break;
    }

    return;
}


void gameUpdate(GameState *game_state, Resources *resources, GameMap *game_map,
    RenderTarget *render_target) {

    if (game_state->status == EXITING) {
        return;
    }

    // TODO: gameStatus NEW_GAME should show some kind of title screen
    if (game_state->status == INIT) {
        game_state->status = NEW_GAME;
    }

    if (game_state->last_input == DEBUG_GENERATE_NEW_MAP) {
        generateMap(game_map);
        generateTerrain(game_map);
        SDL_FreeSurface(resources->level);
        resources->level =
            SDL_CreateRGBSurfaceWithFormat(0,
                game_map->width * TILE_SIZE, game_map->height * TILE_SIZE,
                render_target->screen_surface->format->BitsPerPixel,
                render_target->screen_surface->format->format);

        render_target->debug_info_changed = true;
        game_state->last_input = NONE;
    }

    if (game_state->turn_order[game_state->current_turn] == -1) {
        game_state->current_turn = 0;
        shuffleTurnOrder(&game_state->turn_order, game_state->total_entities);
    }

    game_state->current_player =
        game_state->turn_order[game_state->current_turn] - 1;

    if (resources->entity_list[game_state->current_player].sprite_ID == HERO) {
        game_state->status = HERO_TURN;
    }
    else {
        game_state->status = PLAYER_TURN;
    }

    if (game_state->status == HERO_TURN) {
        resources->entity_list[game_state->current_player].x += TILE_SIZE;
        game_state->current_turn += 1;
    }

    if (game_state->end_turn == true){
        switch (game_state->last_input) {
            case DOWN_LEFT:
            resources->entity_list[game_state->current_player].x -= TILE_SIZE;
            resources->entity_list[game_state->current_player].y += TILE_SIZE;
            break;

            case DOWN:
            resources->entity_list[game_state->current_player].y += TILE_SIZE;
            break;

            case DOWN_RIGHT:
            resources->entity_list[game_state->current_player].x += TILE_SIZE;
            resources->entity_list[game_state->current_player].y += TILE_SIZE;
            break;

            case RIGHT:
            resources->entity_list[game_state->current_player].x += TILE_SIZE;
            break;

            case UP_RIGHT:
            resources->entity_list[game_state->current_player].x += TILE_SIZE;
            resources->entity_list[game_state->current_player].y -= TILE_SIZE;
            break;

            case UP:
            resources->entity_list[game_state->current_player].y -= TILE_SIZE;
            break;

            case UP_LEFT:
            resources->entity_list[game_state->current_player].x -= TILE_SIZE;
            resources->entity_list[game_state->current_player].y -= TILE_SIZE;
            break;

            case LEFT:
            resources->entity_list[game_state->current_player].x -= TILE_SIZE;
            break;
        }
        game_state->current_turn += 1;
        game_state->last_input = NONE;
        game_state->end_turn = false;
    }

    return;
}

void processInputs(SDL_Event *e, GameState *game_state,
                   RenderTarget *render_target, Camera *camera) {
    while (SDL_PollEvent(e)) {
        if (e->type == SDL_QUIT) {
            game_state->status = EXITING;
            return;
        }
        if (e->type == SDL_WINDOWEVENT){
            if (e->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                render_target->screen_width = e->window.data1;
                render_target->screen_height = e->window.data2;
                printf("new screen width: %d\n", render_target->screen_width);
                printf("new screen height: %d\n", render_target->screen_height);
                render_target->debug_info_changed = true;
                render_target->resizing = true;
            }
        }

        if (e->type == SDL_KEYDOWN) {
            switch (e->key.keysym.sym) {
                case SDLK_KP_1:
                game_state->last_input = DOWN_LEFT;
                break;

                case SDLK_KP_2:
                game_state->last_input = DOWN;
                break;

                case SDLK_KP_3:
                game_state->last_input = DOWN_RIGHT;
                break;

                case SDLK_KP_4:
                game_state->last_input = LEFT;
                break;

                case SDLK_KP_5:
                game_state->last_input = SKIP;
                game_state->end_turn = true;
                break;

                case SDLK_KP_6:
                game_state->last_input = RIGHT;
                break;

                case SDLK_KP_7:
                game_state->last_input = UP_LEFT;
                break;

                case SDLK_KP_8:
                game_state->last_input = UP;
                break;

                case SDLK_KP_9:
                game_state->last_input = UP_RIGHT;
                break;

                case SDLK_KP_ENTER:
                game_state->end_turn = true;
                break;

                case SDLK_n:
                game_state->last_input = DEBUG_GENERATE_NEW_MAP;
                break;

                // TODO: change these keys to fully qualified camera events (zoom in, zoom out etc)
                case SDLK_w:
                camera->y -= 5;
                break;

                case SDLK_a:
                camera->x -= 5;
                break;

                case SDLK_s:
                camera->y += 5;
                break;

                case SDLK_d:
                camera->x +=5;
                break;

                case SDLK_EQUALS:
                camera->scale += 1;
                render_target->debug_info_changed = true;
                break;

                case SDLK_MINUS:
                camera->scale -= 1;
                render_target->debug_info_changed = true;
                break;

                case SDLK_0:
                camera->scale = 0;
                render_target->debug_info_changed = true;
                break;
            }
        }
    }

    return;
}

void renderTerrain(SDL_Surface *terrain_map, GameMap *game_map,
                   SDL_Surface *destination) {
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
    for (int i = 0; i < game_map->width; i++) {
        for (int j = 0; j < game_map->height; j++) {
            if ((game_map->map_array)[i][j] == 0) {
                placeTile(terrain_map, FLOOR, 0,
                          i * TILE_SIZE, j * TILE_SIZE, destination);
            }
            if ((game_map->map_array)[i][j] == 1) {
                placeTile(terrain_map, CAVE, 0,
                          i * TILE_SIZE, j * TILE_SIZE, destination);
            }
        }
    }
}

void place(Critter sprite, SDL_Surface *destination) {
    SDL_Rect source_rect = { .h = TILE_SIZE, .w = TILE_SIZE, .x = 0,
                             .y = sprite.sprite_ID * TILE_SIZE };
    SDL_Rect destination_rect = { .h = 0, .w = 0,
                                  .x = sprite.x, .y = sprite.y };

    SDL_BlitSurface(sprite.source_sprite_map, &source_rect,
                    destination, &destination_rect);

}

void placeTile(SDL_Surface *src, int sprite, int offset, int x, int y,
               SDL_Surface *destination) {

    SDL_Rect source_rect = {.h = TILE_SIZE, .w = TILE_SIZE,
                            .x = offset * TILE_SIZE, .y = sprite * TILE_SIZE};

    SDL_Rect destination_rect = {.h = 0, .w = 0, .x = x, .y = y};

    SDL_BlitSurface(src, &source_rect, destination, &destination_rect);
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

int init(RenderTarget *render_target, Resources *resources,
         GameState *game_state, GameMap *game_map, Camera *camera) {

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL not initialized! SDL_Error: %s\n", SDL_GetError());
        cleanup(NULL);
        game_state->status = EXITING;
        return -1;
    }

    int imgFlags = IMG_Init(IMG_INIT_PNG);
    if (!(imgFlags & IMG_INIT_PNG)) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n",
               IMG_GetError());

        cleanup(NULL);
        game_state->status = EXITING;
        return -1;
    }

    if (TTF_Init() == -1) {
        printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n",
               TTF_GetError());

        cleanup(NULL);
        game_state->status = EXITING;
        return -1;
    }

    render_target->window =
        SDL_CreateWindow("yarz",
                         SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                         INITIAL_SCREEN_WIDTH, INITIAL_SCREEN_HEIGHT,
                         SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (render_target->window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        cleanup(render_target->window);
        game_state->status = EXITING;
        return -1;
    }

    render_target->screen_surface = SDL_GetWindowSurface(render_target->window);
    if (render_target->screen_surface == NULL) {
        printf("Could not capture screen surface! SDL_Error: %s\n",
               SDL_GetError());
        cleanup(render_target->window);
        game_state->status = EXITING;
        return -1;
    }

    render_target->screen_width = INITIAL_SCREEN_WIDTH;
    render_target->screen_height = INITIAL_SCREEN_HEIGHT;
    render_target->resizing = false;
    render_target->debug_info_changed = false;

    render_target->backdrop =
        SDL_CreateRGBSurfaceWithFormat(0,
            render_target->screen_width, render_target->screen_height,
            render_target->screen_surface->format->BitsPerPixel,
            render_target->screen_surface->format->format);

    SDL_FillRect(render_target->backdrop , NULL, 0);

    SDL_PixelFormat *format = render_target->screen_surface->format;
    resources->sprites = loadSpritemap("assets/yarz-sprites.png", format);
    resources->terrain = loadSpritemap("assets/yarz-terrain.png", format);
    resources->icons = loadSpritemap("assets/yarz-icons.png", format);

    if (resources->sprites == NULL || resources->terrain == NULL
        || resources->icons == NULL) {

        cleanup(render_target->window);
        game_state->status = EXITING;
        return -1;
    }

    resources->game_font =
        TTF_OpenFont("assets/MajorMonoDisplay-Regular.ttf", 14);

    if (resources->game_font == NULL) {
        printf("Could not open font MajorMonoDisplay-Regular! TTF_Error: %s\n",
               TTF_GetError());

        cleanup(render_target->window);
        game_state->status = EXITING;
        return -1;
    }

    generateMap(game_map);
    generateTerrain(game_map);

    // this monster of a declaration basically says 'give me a surface the size of the level in the same format as the screen'
    // the other surfaces are optimized to the screen format on load
    resources->level =
        SDL_CreateRGBSurfaceWithFormat(
            0,
            game_map->width * TILE_SIZE,
            game_map->height * TILE_SIZE,
            render_target->screen_surface->format->BitsPerPixel,
            render_target->screen_surface->format->format);

    if (resources->level == NULL) {
        printf("Could not create level surface! SDL_Error: %s\n",
                                                     SDL_GetError());

        cleanup(render_target->window);
        game_state->status = EXITING;
        return -1;
    }

    // FIXME: this malloc has no destroy! :3
    // FIXME: the value of 10 is hardcoded, we may have more than 10 entities that require turn shuffling
    game_state->turn_order = (int *) malloc(sizeof(int) * 10);
    for (int i = 0; i < 10; i++) {
        game_state->turn_order[i] = -1;
    }

    // TODO: This critter list should be loaded from disk
    Critter tempCritterList[] =
        { { .source_sprite_map = resources->sprites, .sprite_ID = HERO, .x = 0, .y = 0} ,
          { .source_sprite_map = resources->sprites, .sprite_ID = LEGGY, .x = 32, .y = 32},
          { .source_sprite_map = resources->sprites, .sprite_ID = BOOTS, .x = 64, .y = 64} };

    resources->entity_list = (Critter *) malloc(sizeof(tempCritterList));
    memcpy(resources->entity_list, tempCritterList, sizeof(tempCritterList));

    render_target->debug_info =
        updateDebugInfo(resources->game_font, render_target,
                        game_map, camera->scale);

    render_target->debug_info_rect = (SDL_Rect *) malloc(sizeof(SDL_Rect));
    render_target->debug_info_rect->h = render_target->debug_info->h;
    render_target->debug_info_rect->w = render_target->debug_info->w;
    render_target->debug_info_rect->x = 0;
    render_target->debug_info_rect->y = 0;

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
        printf("Unable to optimize image %s! SDL Error: %s\n",
                                        path,       SDL_GetError());

        return NULL;
    }

    SDL_FreeSurface(image);

    // TODO: may want to remap alpha color to something other than black
    int set_color_key_status =
        SDL_SetColorKey(optimizedSurface, SDL_TRUE,
                        SDL_MapRGB(optimizedSurface->format, 0, 0, 0));

    if (set_color_key_status < 0) {
        printf("Unable to set transparent pixel for image %s! SDL Error: %s\n",
                                                         path,  SDL_GetError());

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
