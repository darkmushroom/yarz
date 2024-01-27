#ifndef __MAP_H__
#define __MAP_H__

extern const int INITIAL_SCREEN_WIDTH;
extern const int INITIAL_SCREEN_HEIGHT;
extern const int TILE_SIZE;

extern const int CAVE_WALL_PROBABILITY;
extern const int CAVE_GENERATOR_ITERATIONS;

// map height and width are in tiles. aka 1 = TILE_SIZE pixels
struct GameMap {
    int map_width;
    int map_height;
    int **map_array;
};

int generateMap(struct GameMap *);
void generateTerrain(struct GameMap *);
int randomRange(int, int);
void asciiOutputMap(struct GameMap *);

#endif /* __MAP_H__ */
