#include "world_generation.h"
#include "biomes.h"
#include "biome_houses.h"
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
    BiomeSetSeed(seed);
}

static int TourLocalX(int x) {
    int local = x % BIOME_STRIDE;
    if (local < 0) local += BIOME_STRIDE;
    return local;
}

static float LerpKnot(float c, float c0, float c1, float h0, float h1)
{
    float t = (c - c0)/(c1 - c0);
    return h0 + (h1 - h0)*t;
}

static float ContinentalBase(float c)
{
    float abyss = (float)(WATER_LEVEL - 20);
    float deep = (float)(WATER_LEVEL - 18);
    float shelf = (float)(WATER_LEVEL - 8);
    float beachLow = (float)(WATER_LEVEL + 1);
    float beachHigh = (float)(WATER_LEVEL + 2);
    float inland = 0.0f;

    if (c < -1.0f) c = -1.0f;
    if (c > 1.0f) c = 1.0f;
    inland = (float)(WATER_LEVEL + 6) + c*8.0f;
    if (c < -0.55f) return LerpKnot(c, -1.0f, -0.55f, abyss, deep);
    if (c < -0.42f) return LerpKnot(c, -0.55f, -0.42f, deep, shelf);
    if (c < -0.30f) return shelf;
    if (c < -0.26f) return LerpKnot(c, -0.30f, -0.26f, shelf, beachLow);
    if (c < -0.12f) return LerpKnot(c, -0.26f, -0.12f, beachLow, beachHigh);
    if (c < 0.05f) return LerpKnot(c, -0.12f, 0.05f, beachHigh, (float)(WATER_LEVEL + 6) + 0.05f*8.0f);
    return inland;
}

static float Relief(float continentalness, float erosion)
{
    float fade = 0.0f;
    float hill = 0.0f;

    if (erosion >= 0.20f) return 0.0f;
    if (continentalness <= -0.30f) return 0.0f;
    fade = (continentalness - (-0.30f))/0.22f;
    if (fade > 1.0f) fade = 1.0f;
    hill = (0.20f - erosion)/(0.20f - (-1.0f));
    if (hill < 0.0f) hill = 0.0f;
    if (hill > 1.0f) hill = 1.0f;
    return hill*hill*30.0f*fade;
}

static int SnowLineFor(float temperature)
{
    float t = (temperature + 1.0f)*0.5f;
    float coldLine = (float)(WATER_LEVEL + 14);
    float hotLine = (float)(WATER_LEVEL + 44);
    float line = 0.0f;

    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    line = coldLine + t*(hotLine - coldLine);
    return (int)line;
}

float GetTerrainHeight(int x, int z) {
    const Biome *biome = NULL;
    float continentalness = 0.0f;
    float erosion = 0.0f;
    float height = 0.0f;
    float local = 0.0f;
    float mask = 0.0f;
    float target = (float)(WATER_LEVEL - 1);

    if (BiomeTourActive()) {
        int localX = TourLocalX(x);
        int mid = BIOME_STRIDE/2;
        int dx = 0;
        int dz = 0;

        biome = BiomeAt(x, z);
        dx = abs(localX - mid);
        dz = abs(z - mid);
        if (biome->flags & BIOME_VOID) {
            if ((dx <= 2) && (dz <= 2)) return (float)(WATER_LEVEL + 4);
            return 6.0f;
        }
        if (biome->flags & BIOME_ISLAND) {
            if ((dx > 8) || (dz > 8)) return 6.0f;
        }
        return (float)(WATER_LEVEL + biome->lift);
    }

    BiomeClimate(x, z, NULL, NULL, &continentalness, &erosion, NULL);
    local = PerlinNoise2D((float)x*0.022f, (float)z*0.022f)*1.6f;
    local += PerlinNoise2D((float)x*0.061f, (float)z*0.061f)*0.8f;
    height = ContinentalBase(continentalness) + Relief(continentalness, erosion) + local;
    mask = BiomeRiverMask(x, z);
    if ((mask > 0.0f) && (height > target)) height = height + (target - height)*mask;
    return height;
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

static int WaterBeside(int x, int z, int *faceX, int *faceZ)
{
    int dir = 0;

    for (dir = 0; dir < 4; dir++)
    {
        int nx = x + ((dir == 0) ? 1 : (dir == 1) ? -1 : 0);
        int nz = z + ((dir == 2) ? 1 : (dir == 3) ? -1 : 0);

        if ((int)GetTerrainHeight(nx, nz) < WATER_LEVEL)
        {
            *faceX = nx - x;
            *faceZ = nz - z;
            return 1;
        }
    }
    return 0;
}

bool FindShoreSpawn(Vector3 *position, float *yaw) {
    int bestDist = 8000000;
    int bestX = 0;
    int bestZ = 0;
    int faceX = 0;
    int faceZ = 1;
    bool found = false;
    int radius = 0;

    if ((position == NULL) || (yaw == NULL)) return false;

    for (radius = 1; radius <= 160; radius++) {
        int x = 0;
        int z = 0;

        for (x = -radius; x <= radius; x++) {
            for (z = -radius; z <= radius; z++) {
                int dist = x*x + z*z;
                int fx = 0;
                int fz = 0;

                if ((abs(x) != radius) && (abs(z) != radius)) continue;
                if ((dist >= bestDist) || ((int)GetTerrainHeight(x, z) <= WATER_LEVEL)) continue;
                if (!WaterBeside(x, z, &fx, &fz)) continue;
                bestDist = dist;
                bestX = x;
                bestZ = z;
                faceX = fx;
                faceZ = fz;
                found = true;
            }
        }
        if (found) break;
    }

    if (!found)
    {
        int ray = 0;

        for (ray = 0; ray < 24; ray++)
        {
            float ang = (float)ray*(6.2831853f/24.0f);
            int stepX = (int)lroundf(cosf(ang)*2.0f);
            int stepZ = (int)lroundf(sinf(ang)*2.0f);
            int prevLand = ((int)GetTerrainHeight(0, 0) > WATER_LEVEL) ? 1 : 0;
            int prevX = 0;
            int prevZ = 0;
            int along = 0;

            if ((stepX == 0) && (stepZ == 0)) continue;
            for (along = 1; along <= 2200; along++)
            {
                int x = stepX*along;
                int z = stepZ*along;
                int land = ((int)GetTerrainHeight(x, z) > WATER_LEVEL) ? 1 : 0;
                int back = 0;

                if (land == prevLand)
                {
                    prevX = x;
                    prevZ = z;
                    continue;
                }
                for (back = 1; back <= 2; back++)
                {
                    int sx = prevX + ((x - prevX)*back)/2;
                    int sz = prevZ + ((z - prevZ)*back)/2;
                    int fx = 0;
                    int fz = 0;

                    if ((int)GetTerrainHeight(sx, sz) <= WATER_LEVEL) continue;
                    if (!WaterBeside(sx, sz, &fx, &fz)) continue;
                    bestX = sx;
                    bestZ = sz;
                    faceX = fx;
                    faceZ = fz;
                    found = true;
                    break;
                }
                if (found) break;
                prevX = x;
                prevZ = z;
                prevLand = land;
            }
            if (found) break;
        }
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

static void PlaceTreeOf(Chunk* chunk, int x, int y, int z, BlockType log, BlockType leaves, int treeHeight) {
    int i = 0;

    if (treeHeight < 3) treeHeight = 3;
    for (i = 0; i < treeHeight; i++) {
        if (y + i < WORLD_HEIGHT) chunk->blocks[x][y + i][z] = log;
    }

    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            for (int dy = 0; dy <= 2; dy++) {
                int leafX = x + dx;
                int leafY = y + treeHeight - 1 + dy;
                int leafZ = z + dz;

                if (leafX >= 0 && leafX < CHUNK_SIZE &&
                    leafZ >= 0 && leafZ < CHUNK_SIZE &&
                    leafY >= 0 && leafY < WORLD_HEIGHT) {
                    if (chunk->blocks[leafX][leafY][leafZ] != log) {
                        chunk->blocks[leafX][leafY][leafZ] = leaves;
                    }
                }
            }
        }
    }
}

void PlaceTree(Chunk* chunk, int x, int y, int z) {
    int worldX = chunk->position.x*CHUNK_SIZE + x;
    int worldZ = chunk->position.z*CHUNK_SIZE + z;
    int treeHeight = 4 + (int)((unsigned int)hash2D(worldX, worldZ)%3u);

    PlaceTreeOf(chunk, x, y, z, BLOCK_OAK_LOG, BLOCK_OAK_LEAVES, treeHeight);
}

static BlockType BandBlock(int y) {
    BlockType bands[5] = {
        BLOCK_TERRACOTTA,
        BLOCK_ORANGE_TERRACOTTA,
        BLOCK_RED_TERRACOTTA,
        BLOCK_YELLOW_TERRACOTTA,
        BLOCK_BROWN_TERRACOTTA
    };
    int index = y/2;
    if (index < 0) index = -index;
    return bands[index%5];
}

static void PlaceFeature(Chunk *chunk, int x, int y, int z, BlockType block, int height) {
    int i = 0;
    for (i = 0; i < height; i++) {
        if ((y + i >= 0) && (y + i < WORLD_HEIGHT)) chunk->blocks[x][y + i][z] = block;
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
            
            const Biome *biome = BiomeAt(worldX, worldZ);
            unsigned roll = (unsigned int)hash2D(worldX, worldZ);
            BlockType surface = (BlockType)biome->surface;
            BlockType filler = (BlockType)biome->filler;
            BlockType rock = (BlockType)biome->rock;
            float temperature = 0.0f;
            int snowLine = WORLD_HEIGHT;

            if (!BiomeTourActive())
            {
                BiomeClimate(worldX, worldZ, &temperature, NULL, NULL, NULL, NULL);
                snowLine = SnowLineFor(temperature);
                if ((height == WATER_LEVEL) && !(biome->flags & BIOME_OCEAN))
                {
                    if ((surface != BLOCK_SAND) && (surface != BLOCK_STONE) && (surface != BLOCK_SNOW_BLOCK) && (surface != BLOCK_GRAVEL))
                    {
                        surface = BLOCK_SAND;
                    }
                }
            }
            if (biome->flags & BIOME_BANDS) {
                surface = BandBlock(height);
                filler = BandBlock(height - 1);
            }
            if ((!BiomeTourActive()) && (!(biome->flags & BIOME_OCEAN)) && (surface != BLOCK_SNOW_BLOCK) && (height > WATER_LEVEL) && (height > snowLine))
            {
                surface = BLOCK_SNOW_BLOCK;
            }

            for (int y = 0; y <= height; y++) {
                if (y < height - 3) chunk->blocks[x][y][z] = rock;
                else if (y < height) chunk->blocks[x][y][z] = (biome->flags & BIOME_BANDS) ? BandBlock(y) : filler;
                else chunk->blocks[x][y][z] = surface;
            }

            for (int y = height + 1; y <= WATER_LEVEL; y++) {
                if (y < WORLD_HEIGHT) chunk->blocks[x][y][z] = BLOCK_WATER;
            }
            if ((biome->flags & BIOME_ICE) && (height < WATER_LEVEL) && (WATER_LEVEL < WORLD_HEIGHT)) {
                chunk->blocks[x][WATER_LEVEL][z] = BLOCK_ICE;
            }

            if ((biome->log != BLOCK_AIR) && (height > WATER_LEVEL) && ((roll % 1000u) < biome->trees)) {
                int treeHeight = 4 + (int)(roll % 3u);
                if (biome->flags & BIOME_POLE) PlaceFeature(chunk, x, height + 1, z, BLOCK_LIME_CONCRETE, 6);
                else PlaceTreeOf(chunk, x, height + 1, z, (BlockType)biome->log, (BlockType)biome->leaves, treeHeight);
            } else if ((biome->flags & BIOME_CACTUS) && (surface == BLOCK_SAND) && ((roll % 17u) == 0)) {
                PlaceFeature(chunk, x, height + 1, z, BLOCK_CACTUS, 3);
            } else if ((biome->flags & BIOME_SPIKE) && ((roll % 9u) == 0)) {
                BlockType spike = (surface == BLOCK_SNOW_BLOCK || surface == BLOCK_PACKED_ICE) ? BLOCK_PACKED_ICE : BLOCK_GRANITE;
                if (biome->rock == BLOCK_STONE && surface == BLOCK_STONE) spike = BLOCK_ANDESITE;
                PlaceFeature(chunk, x, height + 1, z, spike, 6 + (int)(roll % 6u));
            } else if ((biome->flags & BIOME_MELON) && ((roll % 23u) == 0) && (height + 1 < WORLD_HEIGHT)) {
                chunk->blocks[x][height + 1][z] = BLOCK_MELON;
            } else if ((biome->flags & BIOME_PUMPKIN) && ((roll % 29u) == 0) && (height + 1 < WORLD_HEIGHT)) {
                chunk->blocks[x][height + 1][z] = ((roll % 2u) == 0) ? BLOCK_PUMPKIN : BLOCK_RED_CONCRETE;
            } else if ((biome->flags & BIOME_HAY) && ((roll % 19u) == 0) && (height + 1 < WORLD_HEIGHT)) {
                chunk->blocks[x][height + 1][z] = BLOCK_HAY_BLOCK;
            } else if ((biome->flags & BIOME_GLOW) && ((roll % 11u) == 0) && (height + 1 < WORLD_HEIGHT)) {
                chunk->blocks[x][height + 1][z] = BLOCK_GLOWSTONE;
            } else if ((biome->flags & BIOME_MAGMA) && ((roll % 7u) == 0)) {
                chunk->blocks[x][height][z] = BLOCK_MAGMA_BLOCK;
            } else if ((biome->flags & BIOME_BONE) && ((roll % 8u) == 0)) {
                PlaceFeature(chunk, x, height + 1, z, BLOCK_BONE_BLOCK, 4);
            } else if ((biome->flags & BIOME_PURPUR) && ((roll % 13u) == 0)) {
                PlaceFeature(chunk, x, height + 1, z, BLOCK_PURPUR_BLOCK, 5);
            }

            if (BiomeTourActive() && (TourLocalX(worldX) == 0)) {
                int top = height + 3;
                if (top >= WORLD_HEIGHT) top = WORLD_HEIGHT - 1;
                for (int y = 0; y <= top; y++) chunk->blocks[x][y][z] = BLOCK_BEDROCK;
            }

            if (BiomeTourActive())
            {
                BiomeHousesStamp(chunk, x, z, worldX, worldZ, height);
            }
        }
    }
    
    chunk->needsRegen = true;
    chunk->isLoaded = true;
} 
