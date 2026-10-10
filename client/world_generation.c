#include "world_generation.h"
#include "raymath.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

//----------------------------------------------------------------------------------
// Local Variables
//----------------------------------------------------------------------------------
static bool isInitialized = false;
static unsigned int worldSeed = 0;

// Simple noise hash function. worldSeed changes hills, water, and trees.
static int hash2D(int x, int y) {
    unsigned int h = (unsigned int)x*374761393u + (unsigned int)y*668265263u;
    h ^= worldSeed*2246822519u;
    h = (h ^ (h >> 13))*1274126177u;
    return (int)(h ^ (h >> 16));
}

//----------------------------------------------------------------------------------
// Noise Functions
//----------------------------------------------------------------------------------
float PerlinNoise2D(float x, float y) {
    // Simple implementation of 2D Perlin noise
    int xi = (int)floor(x);
    int yi = (int)floor(y);
    
    float xf = x - xi;
    float yf = y - yi;
    
    // Get values at corners
    float a = hash2D(xi, yi) / (float)INT_MAX;
    float b = hash2D(xi + 1, yi) / (float)INT_MAX;
    float c = hash2D(xi, yi + 1) / (float)INT_MAX;
    float d = hash2D(xi + 1, yi + 1) / (float)INT_MAX;
    
    // Smooth interpolation
    float u = xf * xf * (3.0f - 2.0f * xf);
    float v = yf * yf * (3.0f - 2.0f * yf);
    
    // Bilinear interpolation
    float i1 = a * (1.0f - u) + b * u;
    float i2 = c * (1.0f - u) + d * u;
    
    return i1 * (1.0f - v) + i2 * v;
}

float SimplexNoise2D(float x, float y) {
    // Simplified noise - can be improved with proper simplex implementation
    return (PerlinNoise2D(x, y) + PerlinNoise2D(x * 2.0f, y * 2.0f) * 0.5f + 
            PerlinNoise2D(x * 4.0f, y * 4.0f) * 0.25f) / 1.75f;
}

//----------------------------------------------------------------------------------
// World Generation Functions
//----------------------------------------------------------------------------------
void InitWorldGeneration(void) {
    isInitialized = true;
}

void SetWorldGenerationSeed(unsigned int seed) {
    worldSeed = seed;
}

float GetTerrainHeight(int x, int z) {
    // Generate height using multiple octaves of noise
    float height = 0.0f;
    float amplitude = TERRAIN_HEIGHT;
    float frequency = TERRAIN_SCALE;
    
    // Add multiple octaves for more interesting terrain
    for (int i = 0; i < 4; i++) {
        height += SimplexNoise2D(x * frequency, z * frequency) * amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }
    
    // Low continent noise opens lakes and oceans. High ground stays dry.
    float continent = SimplexNoise2D(x*0.0025f, z*0.0025f);
    if (continent < 0.05f) height -= (0.05f - continent)*48.0f;

    return WATER_LEVEL + height;
}

float GetSurfaceLevel(int x, int z) {
    // Get the terrain height at this position
    float terrainHeight = GetTerrainHeight(x, z);
    int height = (int)terrainHeight;
    
    // Clamp height to world bounds
    if (height < 0) height = 0;
    if (height >= WORLD_HEIGHT) height = WORLD_HEIGHT - 1;
    
    // If terrain is above water level, surface is at terrain height + 1 (on top of grass)
    // If terrain is at/below water level, surface is at water level + 1 (on top of water)
    if (height > WATER_LEVEL) {
        return height + 1.0f; // On top of grass block
    } else {
        return WATER_LEVEL + 1.0f; // On top of water
    }
}

bool FindShoreSpawn(Vector3 *position, float *yaw) {
    int bestDist = 1000000;
    int bestX = 0;
    int bestZ = 0;
    int faceX = 0;
    int faceZ = 1;
    bool found = false;

    if ((position == NULL) || (yaw == NULL)) return false;

    for (int radius = 1; radius <= 80; radius++) {
        for (int x = -radius; x <= radius; x++) {
            for (int z = -radius; z <= radius; z++) {
                int dist = x*x + z*z;
                if ((abs(x) != radius) && (abs(z) != radius)) continue;
                if ((dist >= bestDist) || ((int)GetTerrainHeight(x, z) <= WATER_LEVEL)) continue;

                for (int dir = 0; dir < 4; dir++) {
                    int nx = x + ((dir == 0) ? 1 : (dir == 1) ? -1 : 0);
                    int nz = z + ((dir == 2) ? 1 : (dir == 3) ? -1 : 0);
                    if ((int)GetTerrainHeight(nx, nz) < WATER_LEVEL) {
                        bestDist = dist;
                        bestX = x;
                        bestZ = z;
                        faceX = nx - x;
                        faceZ = nz - z;
                        found = true;
                        break;
                    }
                }
            }
        }
        if (found) break;
    }

    if (!found) return false;

    *position = (Vector3){ bestX + 0.5f, GetSurfaceLevel(bestX, bestZ), bestZ + 0.5f };
    *yaw = atan2f((float)faceX, (float)faceZ);
    return true;
}

bool ShouldPlaceTree(int x, int z) {
    // Use noise to determine tree placement
    float treeNoise = PerlinNoise2D(x * 0.1f, z * 0.1f);
    return (treeNoise > 0.7f) && (((unsigned int)hash2D(x, z)%100u) < (unsigned int)(TREE_FREQUENCY*100.0f));
}

void PlaceTree(Chunk* chunk, int x, int y, int z) {
    int worldX = chunk->position.x*CHUNK_SIZE + x;
    int worldZ = chunk->position.z*CHUNK_SIZE + z;
    int treeHeight = 4 + (int)((unsigned int)hash2D(worldX, worldZ)%3u);
    
    // Place trunk
    for (int i = 0; i < treeHeight; i++) {
        if (y + i < WORLD_HEIGHT) {
            chunk->blocks[x][y + i][z] = BLOCK_OAK_LOG;
        }
    }
    
    // Place leaves (3x3x3 cube at top)
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            for (int dy = 0; dy <= 2; dy++) {
                int leafX = x + dx;
                int leafY = y + treeHeight - 1 + dy;
                int leafZ = z + dz;
                
                // Check bounds
                if (leafX >= 0 && leafX < CHUNK_SIZE && 
                    leafZ >= 0 && leafZ < CHUNK_SIZE &&
                    leafY >= 0 && leafY < WORLD_HEIGHT) {
                    
                    // Don't replace trunk blocks
                    if (chunk->blocks[leafX][leafY][leafZ] != BLOCK_OAK_LOG) {
                        chunk->blocks[leafX][leafY][leafZ] = BLOCK_OAK_LEAVES;
                    }
                }
            }
        }
    }
}

void GenerateChunk(Chunk* chunk) {
    if (!isInitialized) InitWorldGeneration();
    
    // Clear chunk
    memset(chunk->blocks, BLOCK_AIR, sizeof(chunk->blocks));
    
    // Generate terrain for each column in the chunk
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            // Get world coordinates
            int worldX = chunk->position.x * CHUNK_SIZE + x;
            int worldZ = chunk->position.z * CHUNK_SIZE + z;
            
            // Generate terrain height
            float terrainHeight = GetTerrainHeight(worldX, worldZ);
            int height = (int)terrainHeight;
            
            // Clamp height to world bounds
            if (height < 0) height = 0;
            if (height >= WORLD_HEIGHT) height = WORLD_HEIGHT - 1;
            
            // Generate layers
            for (int y = 0; y <= height; y++) {
                if (y < height - 3) {
                    // Stone layer
                    chunk->blocks[x][y][z] = BLOCK_STONE;
                } else if (y < height) {
                    if (height <= WATER_LEVEL + 2) chunk->blocks[x][y][z] = BLOCK_SAND;
                    else chunk->blocks[x][y][z] = BLOCK_DIRT;
                } else {
                    // Beaches and lake beds are sand. Inland tops are grass.
                    if (height <= WATER_LEVEL + 2) chunk->blocks[x][y][z] = BLOCK_SAND;
                    else chunk->blocks[x][y][z] = BLOCK_GRASS;
                }
            }
            
            // Add water
            for (int y = height + 1; y <= WATER_LEVEL; y++) {
                if (y < WORLD_HEIGHT) {
                    chunk->blocks[x][y][z] = BLOCK_WATER;
                }
            }
            
            // Place trees on grass
            if (height > WATER_LEVEL && chunk->blocks[x][height][z] == BLOCK_GRASS) {
                if (ShouldPlaceTree(worldX, worldZ)) {
                    PlaceTree(chunk, x, height + 1, z);
                }
            }
        }
    }
    
    chunk->needsRegen = true;
    chunk->isLoaded = true;
} 
