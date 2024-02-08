#include "map.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

// creates a new map of random size initialized to all 1s
GameMap* initMap(int x_in_tiles, int y_in_tiles) {
    GameMap *game_map = (GameMap *)malloc(sizeof(GameMap));
    game_map->width = x_in_tiles;
    game_map->height = y_in_tiles;

    // allocating all the space for our map
    game_map->map_array = (int**) malloc(sizeof(int*) * game_map->width);
    for (int i = 0; i < game_map->width; i++) {
        (game_map->map_array)[i] = (int *)malloc(sizeof(int) * game_map->height);
    }

    // init everything to '1'
    for (int i = 0; i < game_map->width; i++) {
        for (int j = 0; j < game_map->height; j++) {
            (game_map->map_array)[i][j] = 1;
        }
    }

    return game_map;
}

// convenience function, inits a map between 50x50 and 100x100 tiles
GameMap* initRandomSizedMap() {
    int width = randomRange(50, 100);
    int height = randomRange(50, 100);
    return(initMap(width, height));
}

// TODO: Generate more than one kind of terrain
// FIXME: cave terrain generation has a strange bottom-right cutoff issue. I assume this is because we are evaluating the map and editing values in the same step.
// Maybe double-buffer it? (analyze map, writing changes to buffer, replace map with buffer, repeat)
// "simple" cave generation based on a naive implementation of the following:
// https://www.roguebasin.com/index.php?title=Cellular_Automata_Method_for_Generating_Random_Cave-Like_Levels
void generateCaveTerrain(GameMap *game_map) {

    enum {
        FLOOR,
        WALL
    };

    // first, randomize map area
    // TODO: made this map area smaller by 1 because my ass is lazy..
    // ..I mean to avoid out-of-bounds errors during generation
    for (int i = 1; i < game_map->width - 1; i++) {
        for (int j = 1; j < game_map->height - 1; j++) {
            int roll = randomRange(0, 100);
            if (roll < CAVE_WALL_PROBABILITY) {
                (game_map->map_array)[i][j] = 1;
            }
            else {
                (game_map->map_array)[i][j] = 0;
            }
        }
    }

    // The cave generator works by walking through the map several times.
    // For each location on the map, it analyzes the 8 adjacent squares.
    // if those squares contain a wall, we add 1 to our count
    for (int step = 0; step < CAVE_GENERATOR_ITERATIONS; step++) {
        for (int i = 1; i < game_map->width - 1; i++) {
            for (int j = 1; j < game_map->height - 1; j++) {
                int wall_count = 0;
                for (int k = 0; k < 3; k++) {
                    for (int l = 0; l < 3; l++) {
                        if ((game_map->map_array)[i + k - 1][j + l - 1] == WALL
                            && !(k == 1 && l == 1))
                        {
                            wall_count++;
                        }
                    }
                }

                // if our tile is a flimsy unsupported wall has less than four walls around it, we collapse it into a floor tile
                // if instead our tile is a floor encroached by more than 4 walls, we fill this tile with a wall
                if ((game_map->map_array)[i][j] == WALL && wall_count < 4){
                    (game_map->map_array)[i][j] = FLOOR;
                }
                if ((game_map->map_array)[i][j] == FLOOR && wall_count > 4) {
                    (game_map->map_array)[i][j] = WALL;
                }
            }
        }
    }

    return;
}

int replaceMap(GameMap **game_map) {

    //free up the old map
    destroyMap(*game_map);
    *game_map = initRandomSizedMap();
    generateCaveTerrain(*game_map);
    return EXIT_SUCCESS;
}

void destroyMap(GameMap *game_map) {

    //free up the old map_array
    if (game_map->map_array != NULL) {
        for (int i = 0; i < game_map->width; i++) {
            free((game_map->map_array)[i]);
            (game_map->map_array)[i] = NULL;
        }
        free(game_map->map_array);
        game_map->map_array = NULL;
    }

    //and finally clear the struct
    free(game_map);
    game_map = NULL;
}

// completely stolen from https://stackoverflow.com/a/18386648, ty vitim.us!
// https://stackoverflow.com/users/938822/vitim-us
int randomRange(int min, int max) {
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

// strictly for debugging purposes
void asciiOutputMap(GameMap *game_map) {

    for (int i = 0; i < game_map->width; i++) {
        for (int j = 0; j < game_map->height; j++) {
            printf("%d", game_map->map_array[i][j]);
        }
        printf("\n");
    }
}
