#ifndef WORLD_GENERATION_H
#define WORLD_GENERATION_H

#include "voxel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------------
// World Generation Functions
//----------------------------------------------------------------------------------
void InitWorldGeneration(void);
void GenerateChunk(Chunk* chunk);
float GetTerrainHeight(int x, int z);
bool ShouldPlaceTree(int x, int z);
void PlaceTree(Chunk* chunk, int x, int y, int z);

// Noise functions
float PerlinNoise2D(float x, float y);
float SimplexNoise2D(float x, float y);

#ifdef __cplusplus
}
#endif

#endif // WORLD_GENERATION_H 
