#ifndef VOXEL_WORLD_H
#define VOXEL_WORLD_H

#include "voxel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------------
// World Management Structure
//----------------------------------------------------------------------------------
typedef struct {
    Chunk chunks[MAX_CHUNKS];
    int chunkCount;
    Vector3 playerPosition;
} VoxelWorld;

//----------------------------------------------------------------------------------
// World Management Functions
//----------------------------------------------------------------------------------
void InitVoxelWorld(VoxelWorld* world);
void UpdateVoxelWorld(VoxelWorld* world, Vector3 playerPosition);
void UnloadVoxelWorld(VoxelWorld* world);

// Chunk management
Chunk* GetChunk(VoxelWorld* world, ChunkPos position);
Chunk* LoadChunk(VoxelWorld* world, ChunkPos position);
void UnloadChunk(VoxelWorld* world, int index);
void UnloadDistantChunks(VoxelWorld* world, Vector3 playerPosition);

// Block operations
BlockType GetBlock(VoxelWorld* world, BlockPos position);
void SetBlock(VoxelWorld* world, BlockPos position, BlockType block);
bool IsValidBlockPosition(BlockPos position);

// Chunk loading
void LoadChunksAroundPlayer(VoxelWorld* world, Vector3 playerPosition);
bool IsChunkInRange(ChunkPos chunkPos, Vector3 playerPosition, float range);

#ifdef __cplusplus
}
#endif

#endif // VOXEL_WORLD_H 
