#ifndef VOXEL_TYPES_H
#define VOXEL_TYPES_H

#include "raylib.h"
#include "raymath.h"

//----------------------------------------------------------------------------------
// Voxel Game Constants
//----------------------------------------------------------------------------------
#define CHUNK_SIZE 16
#define CHUNK_HEIGHT 256
#define WORLD_HEIGHT 128
#define RENDER_DISTANCE 8
#define MAX_CHUNKS 256

// World generation constants
#define TERRAIN_SCALE 0.01f
#define TERRAIN_HEIGHT 32
#define WATER_LEVEL 62
#define TREE_FREQUENCY 0.05f

//----------------------------------------------------------------------------------
// Block Types
//----------------------------------------------------------------------------------
typedef enum {
    BLOCK_AIR = 0,
    BLOCK_GRASS,
    BLOCK_DIRT,
    BLOCK_STONE,
    BLOCK_WOOD,
    BLOCK_LEAVES,
    BLOCK_WATER,
    BLOCK_COUNT
} BlockType;

//----------------------------------------------------------------------------------
// World Coordinates
//----------------------------------------------------------------------------------
typedef struct {
    int x, y, z;
} BlockPos;

typedef struct {
    int x, z;
} ChunkPos;

//----------------------------------------------------------------------------------
// Chunk Data Structure
//----------------------------------------------------------------------------------
typedef struct {
    ChunkPos position;
    BlockType blocks[CHUNK_SIZE][WORLD_HEIGHT][CHUNK_SIZE];
    bool needsRegen;
    bool isLoaded;
    bool isVisible;
    
    // Rendering data
    Mesh mesh;
    bool hasMesh;
    int vertexCount;
    int triangleCount;
} Chunk;

//----------------------------------------------------------------------------------
// Player Data Structure
//----------------------------------------------------------------------------------
typedef struct {
    Camera3D camera;
    Vector3 velocity;
    Vector3 position;
    bool onGround;
    bool inWater;
    
    // Camera rotation (independent of camera target)
    float yaw;    // Horizontal rotation in radians
    float pitch;  // Vertical rotation in radians
    
    // Movement settings
    float walkSpeed;
    float runSpeed;
    float jumpHeight;
    float mouseSensitivity;
    
    // Block interaction
    BlockPos targetBlock;
    bool hasTarget;
    BlockType selectedBlock;
    int hotbarSlot;
    BlockType hotbar[9];
} Player;

//----------------------------------------------------------------------------------
// Utility Functions
//----------------------------------------------------------------------------------
static inline ChunkPos WorldToChunk(Vector3 worldPos)
{
    ChunkPos chunkPos;
    chunkPos.x = (int)floor(worldPos.x / CHUNK_SIZE);
    chunkPos.z = (int)floor(worldPos.z / CHUNK_SIZE);
    return chunkPos;
}

static inline BlockPos WorldToBlock(Vector3 worldPos)
{
    BlockPos blockPos;
    blockPos.x = (int)floor(worldPos.x);
    blockPos.y = (int)floor(worldPos.y);
    blockPos.z = (int)floor(worldPos.z);
    return blockPos;
}

static inline Vector3 ChunkToWorld(ChunkPos chunkPos)
{
    Vector3 worldPos;
    worldPos.x = chunkPos.x * CHUNK_SIZE;
    worldPos.y = 0;
    worldPos.z = chunkPos.z * CHUNK_SIZE;
    return worldPos;
}

static inline bool ChunkPosEqual(ChunkPos a, ChunkPos b)
{
    return (a.x == b.x && a.z == b.z);
}

static inline float Distance2D(Vector3 a, Vector3 b)
{
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    return sqrtf(dx*dx + dz*dz);
}

//----------------------------------------------------------------------------------
// Block Properties
//----------------------------------------------------------------------------------
static inline bool IsBlockSolid(BlockType block)
{
    return (block != BLOCK_AIR && block != BLOCK_WATER);
}

static inline bool IsBlockTransparent(BlockType block)
{
    return (block == BLOCK_AIR || block == BLOCK_WATER);
}

static inline Color GetBlockColor(BlockType block)
{
    switch (block) {
        case BLOCK_GRASS: return GREEN;
        case BLOCK_DIRT: return BROWN;
        case BLOCK_STONE: return GRAY;
        case BLOCK_WOOD: return (Color){139, 69, 19, 255}; // Brown
        case BLOCK_LEAVES: return DARKGREEN;
        case BLOCK_WATER: return BLUE;
        default: return WHITE;
    }
}

#endif // VOXEL_TYPES_H 
