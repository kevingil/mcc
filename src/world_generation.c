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

// Simple noise hash function
static int hash2D(int x, int y) {
    int h = x * 374761393 + y * 668265263;
    h = (h ^ (h >> 13)) * 1274126177;
    return h ^ (h >> 16);
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
    
    return WATER_LEVEL + height;
}

bool ShouldPlaceTree(int x, int z) {
    // Use noise to determine tree placement
    float treeNoise = PerlinNoise2D(x * 0.1f, z * 0.1f);
    return (treeNoise > 0.7f && (hash2D(x, z) % 100) < (TREE_FREQUENCY * 100));
}

void PlaceTree(Chunk* chunk, int localX, int baseY, int localZ) {
    if (localX < 0 || localX >= CHUNK_SIZE || localZ < 0 || localZ >= CHUNK_SIZE) return;
    if (baseY < 0 || baseY >= WORLD_HEIGHT - 6) return;
    
    // Tree trunk (4-6 blocks high)
    int trunkHeight = 4 + (hash2D(localX, localZ) % 3);
    for (int y = 0; y < trunkHeight && (baseY + y) < WORLD_HEIGHT; y++) {
        chunk->blocks[localX][baseY + y][localZ] = BLOCK_WOOD;
    }
    
    // Tree leaves (simple sphere)
    int leafY = baseY + trunkHeight;
    for (int dx = -2; dx <= 2; dx++) {
        for (int dy = -1; dy <= 2; dy++) {
            for (int dz = -2; dz <= 2; dz++) {
                int x = localX + dx;
                int y = leafY + dy;
                int z = localZ + dz;
                
                if (x >= 0 && x < CHUNK_SIZE && z >= 0 && z < CHUNK_SIZE && 
                    y >= 0 && y < WORLD_HEIGHT) {
                    
                    // Only place leaves in a roughly spherical shape
                    float dist = sqrtf(dx*dx + dy*dy + dz*dz);
                    if (dist < 2.5f && chunk->blocks[x][y][z] == BLOCK_AIR) {
                        chunk->blocks[x][y][z] = BLOCK_LEAVES;
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
                    // Dirt layer
                    chunk->blocks[x][y][z] = BLOCK_DIRT;
                } else {
                    // Top layer - grass or dirt based on height
                    if (height > WATER_LEVEL) {
                        chunk->blocks[x][y][z] = BLOCK_GRASS;
                    } else {
                        chunk->blocks[x][y][z] = BLOCK_DIRT;
                    }
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
