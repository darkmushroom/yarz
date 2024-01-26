#ifndef __YARZ_H__
#define __YARZ_H__

#include "SDL.h"

struct Critter {
    SDL_Surface *source_sprite_map;
    int sprite_ID;
    int x;
    int y;
};

struct RenderTarget {
    SDL_Window *window;
    SDL_Surface *screen_surface;
    int screen_width;
    int screen_height;
    bool resizing;
    SDL_Surface *backdrop;
    SDL_Surface *debug_info;
    SDL_Rect *debug_info_rect;
    bool debug_info_changed;
};

struct Resources {
    SDL_Surface *level;
    SDL_Surface *sprites;
    SDL_Surface *terrain;
    SDL_Surface *icons;
    TTF_Font *game_font;
    struct Critter *entity_list;
};

// map height and width are in tiles. aka 1 = TILE_SIZE pixels
struct GameMap {
    int map_width;
    int map_height;
    int **map_array;
};

struct GameState {
    int last_input;
    bool end_turn;
    int status;
    int current_player;
    int *turn_order;
    int current_turn;
    int total_entities; // intentionally 1-based index
};

struct Camera {
    int camera_x;
    int camera_y;
    int camera_scale;
};

int init(struct RenderTarget *, struct Resources *, struct GameState *, 
         struct GameMap *, struct Camera *);
int generateMap(struct GameMap *);
SDL_Surface* loadSpritemap(const char *, SDL_PixelFormat *);
void generateTerrain(struct GameMap *);
void renderTerrain(SDL_Surface *, struct GameMap *, SDL_Surface *);
void placeTile(SDL_Surface *, int, int, int, int, SDL_Surface *);
void place(struct Critter, SDL_Surface *);
void processInputs(SDL_Event *, struct GameState *, struct RenderTarget *,
                   struct Camera *);
void gameUpdate(struct GameState *, struct Resources *, struct GameMap *,
                struct RenderTarget *);
void render(struct RenderTarget *, struct Camera *, struct Resources *,
            struct GameMap *, struct GameState *);
int randomRange(int, int);
void shuffleTurnOrder(int**, int);
void renderDirectionIcon(SDL_Surface *, struct Critter *, SDL_Surface *,
                         struct GameState *);
SDL_Surface* updateDebugInfo(TTF_Font *, struct RenderTarget *,
                             struct GameMap *, int);
void asciiOutputMap(struct GameMap *);
void cleanup(SDL_Window *);

#endif /* __YARZ_H__ */