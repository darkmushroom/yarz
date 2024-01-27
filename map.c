#include "map.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

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

// TODO: Generate more than one kind of terrain
// FIXME: cave terrain generation has a strange bottom-right cutoff issue. I assume this is because we are evaluating the map and editing values in the same step.
// Maybe double-buffer it? (analyze map, writing changes to buffer, replace map with buffer, repeat)
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

// completely stolen from https://stackoverflow.com/a/18386648, ty vitim.us!
// https://stackoverflow.com/users/938822/vitim-us
int randomRange(int min, int max) {
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
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
