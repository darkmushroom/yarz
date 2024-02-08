#ifndef __MAP_H__
#define __MAP_H__

extern const int INITIAL_SCREEN_WIDTH;
extern const int INITIAL_SCREEN_HEIGHT;
extern const int TILE_SIZE;

extern const int CAVE_WALL_PROBABILITY;
extern const int CAVE_GENERATOR_ITERATIONS;

// map height and width are in tiles. aka 1 = TILE_SIZE pixels
typedef struct GameMap {
    int width;
    int height;
    int **map_array;
} GameMap;

GameMap* initMap(int, int);
GameMap* initRandomSizedMap();
void generateCaveTerrain(GameMap *);
int replaceMap(GameMap **);
void destroyMap(GameMap *);
int randomRange(int, int);
void asciiOutputMap(GameMap *);

#endif /* __MAP_H__ */
