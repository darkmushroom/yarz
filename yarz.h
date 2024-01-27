#ifndef __YARZ_H__
#define __YARZ_H__

#include "SDL.h"
#include "map.h"

typedef struct Critter {
    SDL_Surface *source_sprite_map;
    int sprite_ID;
    int x;
    int y;
} Critter;

typedef struct RenderTarget {
    SDL_Window *window;
    SDL_Surface *screen_surface;
    int screen_width;
    int screen_height;
    bool resizing;
    SDL_Surface *backdrop;
    SDL_Surface *debug_info;
    SDL_Rect *debug_info_rect;
    bool debug_info_changed;
} RenderTarget;

typedef struct Resources {
    SDL_Surface *level;
    SDL_Surface *sprites;
    SDL_Surface *terrain;
    SDL_Surface *icons;
    TTF_Font *game_font;
    struct Critter *entity_list;
} Resources;

typedef struct GameState {
    int last_input;
    bool end_turn;
    int status;
    int current_player;
    int *turn_order;
    int current_turn;
    int total_entities; // intentionally 1-based index
} GameState;

typedef struct Camera {
    int camera_x;
    int camera_y;
    int camera_scale;
} Camera;

int init(RenderTarget *, Resources *, GameState *, GameMap *, Camera *);
SDL_Surface* loadSpritemap(const char *, SDL_PixelFormat *);
void renderTerrain(SDL_Surface *, GameMap *, SDL_Surface *);
void placeTile(SDL_Surface *, int, int, int, int, SDL_Surface *);
void place(Critter, SDL_Surface *);
void processInputs(SDL_Event *, GameState *, RenderTarget *, Camera *);
void gameUpdate(GameState *, Resources *, GameMap *, RenderTarget *);
void render(RenderTarget *, Camera *, Resources *, GameMap *, GameState *);
void shuffleTurnOrder(int**, int);
void renderDirectionIcon(SDL_Surface *, Critter *, SDL_Surface *, GameState *);
SDL_Surface* updateDebugInfo(TTF_Font *, RenderTarget *, GameMap *, int);
void cleanup(SDL_Window *);

#endif /* __YARZ_H__ */
