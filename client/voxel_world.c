#include "voxel_world.h"
#include "world_generation.h"
#include "world_save.h"
#include "water.h"
#include "raymath.h"
#include <string.h>
#include <stdlib.h>

//----------------------------------------------------------------------------------
// World Management Functions
//----------------------------------------------------------------------------------
int GameRenderDistance(void)
{
    static int distance = -1;
    const char *env = NULL;

    if (distance >= 0) return distance;
    distance = RENDER_DISTANCE;
    env = getenv("MCC_RENDER_DISTANCE");
    if (env != NULL)
    {
        int value = atoi(env);

        if ((value >= 1) && (value <= RENDER_DISTANCE)) distance = value;
    }
    return distance;
}

void InitVoxelWorld(VoxelWorld* world) {
    world->chunkCount = 0;
    world->playerPosition = (Vector3){0, 70, 0};
    
    // Initialize all chunks
    for (int i = 0; i < MAX_CHUNKS; i++) {
        world->chunks[i].isLoaded = false;
        world->chunks[i].hasMesh = false;
        world->chunks[i].needsRegen = false;
        world->chunks[i].isVisible = false;
        world->chunks[i].modified = false;
        world->chunks[i].position = (ChunkPos){0, 0};
        world->chunks[i].vertexCount = 0;
        world->chunks[i].triangleCount = 0;
        world->chunks[i].transparentVertexCount = 0;
        world->chunks[i].transparentTriangleCount = 0;
        memset(world->chunks[i].blocks, BLOCK_AIR, sizeof(world->chunks[i].blocks));
    }
    
    InitWorldGeneration();
}

void UpdateVoxelWorld(VoxelWorld* world, Vector3 playerPosition) {
    world->playerPosition = playerPosition;
    UpdateWater(world);
    
    // Load chunks around player
    LoadChunksAroundPlayer(world, playerPosition);
    
    // Unload distant chunks
    UnloadDistantChunks(world, playerPosition);
}

void UnloadVoxelWorld(VoxelWorld* world) {
    for (int i = 0; i < MAX_CHUNKS; i++) {
        if (world->chunks[i].isLoaded && world->chunks[i].hasMesh) {
            // Unload opaque mesh geometry only
            if (world->chunks[i].vertexCount > 0) {
                UnloadMesh(world->chunks[i].mesh);
            }
            
            // Unload transparent mesh geometry only
            if (world->chunks[i].transparentVertexCount > 0) {
                UnloadMesh(world->chunks[i].transparentMesh);
            }
            
            world->chunks[i].hasMesh = false;
        }
        world->chunks[i].isLoaded = false;
    }
    world->chunkCount = 0;
}

//----------------------------------------------------------------------------------
// Chunk Management Functions
//----------------------------------------------------------------------------------
Chunk* GetChunk(VoxelWorld* world, ChunkPos position) {
    for (int i = 0; i < MAX_CHUNKS; i++) {
        if (world->chunks[i].isLoaded && 
            ChunkPosEqual(world->chunks[i].position, position)) {
            return &world->chunks[i];
        }
    }
    return NULL;
}

Chunk* LoadChunk(VoxelWorld* world, ChunkPos position) {
    // Check if chunk already exists
    Chunk* existing = GetChunk(world, position);
    if (existing) return existing;
    
    // Find empty slot
    for (int i = 0; i < MAX_CHUNKS; i++) {
        if (!world->chunks[i].isLoaded) {
            Chunk* chunk = &world->chunks[i];
            chunk->position = position;
            chunk->isLoaded = true;
            chunk->needsRegen = true;
            chunk->hasMesh = false;
            chunk->isVisible = false;
            chunk->vertexCount = 0;
            chunk->triangleCount = 0;
            chunk->transparentVertexCount = 0;
            chunk->transparentTriangleCount = 0;
            chunk->modified = false;

            if (!WorldSaveLoadChunk(chunk)) GenerateChunk(chunk);
            WaterWakeChunk(world, chunk);
            chunk->modified = false;
            chunk->needsRegen = true;
            chunk->isLoaded = true;

            world->chunkCount++;
            return chunk;
        }
    }
    
    return NULL; // No free slots
}

void UnloadChunk(VoxelWorld* world, int index) {
    if (index < 0 || index >= MAX_CHUNKS) return;
    
    Chunk* chunk = &world->chunks[index];
    if (!chunk->isLoaded) return;

    if (chunk->modified) WorldSaveStoreChunk(chunk);
    
    // Unload mesh and material if they exist
    if (chunk->hasMesh) {
        // Unload opaque mesh geometry only
        if (chunk->vertexCount > 0) {
            UnloadMesh(chunk->mesh);
        }
        
        // Unload transparent mesh geometry only
        if (chunk->transparentVertexCount > 0) {
            UnloadMesh(chunk->transparentMesh);
        }
        
        chunk->hasMesh = false;
    }
    
    chunk->isLoaded = false;
    chunk->modified = false;
    chunk->needsRegen = false;
    chunk->isVisible = false;
    chunk->vertexCount = 0;
    chunk->triangleCount = 0;
    chunk->transparentVertexCount = 0;
    chunk->transparentTriangleCount = 0;
    world->chunkCount--;
}

void UnloadDistantChunks(VoxelWorld* world, Vector3 playerPosition) {
    float maxDistance = GameRenderDistance() * CHUNK_SIZE * 1.5f; // Add some buffer
    
    for (int i = 0; i < MAX_CHUNKS; i++) {
        if (world->chunks[i].isLoaded) {
            Vector3 chunkWorldPos = ChunkToWorld(world->chunks[i].position);
            float distance = Distance2D(playerPosition, chunkWorldPos);
            
            if (distance > maxDistance) {
                UnloadChunk(world, i);
            }
        }
    }
}

//----------------------------------------------------------------------------------
// Block Operations
//----------------------------------------------------------------------------------
BlockType GetBlock(VoxelWorld* world, BlockPos position) {
    if (!IsValidBlockPosition(position)) return BLOCK_AIR;
    
    // Get chunk position
    ChunkPos chunkPos = WorldToChunk((Vector3){position.x, position.y, position.z});
    Chunk* chunk = GetChunk(world, chunkPos);
    
    if (!chunk) return BLOCK_AIR;
    
    // Get local coordinates within chunk
    int localX = position.x - (chunkPos.x * CHUNK_SIZE);
    int localZ = position.z - (chunkPos.z * CHUNK_SIZE);
    
    // Handle negative coordinates properly
    if (localX < 0) localX += CHUNK_SIZE;
    if (localZ < 0) localZ += CHUNK_SIZE;
    
    if (localX < 0 || localX >= CHUNK_SIZE || 
        localZ < 0 || localZ >= CHUNK_SIZE ||
        position.y < 0 || position.y >= WORLD_HEIGHT) {
        return BLOCK_AIR;
    }
    
    return chunk->blocks[localX][position.y][localZ];
}

void SetBlock(VoxelWorld* world, BlockPos position, BlockType block) {
    if (!IsValidBlockPosition(position)) return;
    
    // Get chunk position
    ChunkPos chunkPos = WorldToChunk((Vector3){position.x, position.y, position.z});
    Chunk* chunk = GetChunk(world, chunkPos);
    
    if (!chunk) {
        // Load chunk if needed
        chunk = LoadChunk(world, chunkPos);
        if (!chunk) return;
    }
    
    // Get local coordinates within chunk
    int localX = position.x - (chunkPos.x * CHUNK_SIZE);
    int localZ = position.z - (chunkPos.z * CHUNK_SIZE);
    
    // Handle negative coordinates properly
    if (localX < 0) localX += CHUNK_SIZE;
    if (localZ < 0) localZ += CHUNK_SIZE;
    
    if (localX < 0 || localX >= CHUNK_SIZE || 
        localZ < 0 || localZ >= CHUNK_SIZE ||
        position.y < 0 || position.y >= WORLD_HEIGHT) {
        return;
    }
    
    // Set the block
    chunk->blocks[localX][position.y][localZ] = block;
    chunk->modified = true;
    chunk->needsRegen = true;
    WaterNotify(world, position);
    
    // Mark neighboring chunks for regeneration if block is on edge
    if (localX == 0) {
        ChunkPos leftChunk = {chunkPos.x - 1, chunkPos.z};
        Chunk* leftChunkPtr = GetChunk(world, leftChunk);
        if (leftChunkPtr) leftChunkPtr->needsRegen = true;
    }
    if (localX == CHUNK_SIZE - 1) {
        ChunkPos rightChunk = {chunkPos.x + 1, chunkPos.z};
        Chunk* rightChunkPtr = GetChunk(world, rightChunk);
        if (rightChunkPtr) rightChunkPtr->needsRegen = true;
    }
    if (localZ == 0) {
        ChunkPos frontChunk = {chunkPos.x, chunkPos.z - 1};
        Chunk* frontChunkPtr = GetChunk(world, frontChunk);
        if (frontChunkPtr) frontChunkPtr->needsRegen = true;
    }
    if (localZ == CHUNK_SIZE - 1) {
        ChunkPos backChunk = {chunkPos.x, chunkPos.z + 1};
        Chunk* backChunkPtr = GetChunk(world, backChunk);
        if (backChunkPtr) backChunkPtr->needsRegen = true;
    }
}

bool IsValidBlockPosition(BlockPos position) {
    return (position.y >= 0 && position.y < WORLD_HEIGHT);
}

//----------------------------------------------------------------------------------
// Chunk Loading Functions
//----------------------------------------------------------------------------------
void LoadChunksAroundPlayer(VoxelWorld* world, Vector3 playerPosition) {
    ChunkPos playerChunk = WorldToChunk(playerPosition);
    
    // Load chunks in a square around the player
    int renderDistance = GameRenderDistance();

    for (int x = -renderDistance; x <= renderDistance; x++) {
        for (int z = -renderDistance; z <= renderDistance; z++) {
            ChunkPos chunkPos = {playerChunk.x + x, playerChunk.z + z};
            
            // Check if chunk is in circular range (not square)
            Vector3 chunkWorldPos = ChunkToWorld(chunkPos);
            float distance = Distance2D(playerPosition, chunkWorldPos);
            
            if (distance <= renderDistance * CHUNK_SIZE) {
                // Load chunk if not already loaded
                if (!GetChunk(world, chunkPos)) {
                    LoadChunk(world, chunkPos);
                }
            }
        }
    }
}

bool IsChunkInRange(ChunkPos chunkPos, Vector3 playerPosition, float range) {
    Vector3 chunkWorldPos = ChunkToWorld(chunkPos);
    float distance = Distance2D(playerPosition, chunkWorldPos);
    return distance <= range;
} 
