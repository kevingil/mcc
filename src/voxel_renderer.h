#ifndef VOXEL_RENDERER_H
#define VOXEL_RENDERER_H

#include "voxel_types.h"
#include "voxel_world.h"

#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------------
// Rendering Functions
//----------------------------------------------------------------------------------
void InitVoxelRenderer(void);
void RenderVoxelWorld(VoxelWorld* world, Camera3D camera);
void RenderChunk(Chunk* chunk, Camera3D camera);
void UpdateChunkMesh(Chunk* chunk, VoxelWorld* world);
void UnloadVoxelRenderer(void);

// Mesh generation
void GenerateChunkMesh(Chunk* chunk, VoxelWorld* world);
void AddFaceToMesh(Vector3 position, int faceIndex, BlockType block, 
                   float* vertices, float* colors, int* vertexIndex);
bool ShouldRenderFace(VoxelWorld* world, BlockPos position, int faceIndex);

// Culling and optimization
bool IsChunkInFrustum(Chunk* chunk, Camera3D camera);
void FrustumCullChunks(VoxelWorld* world, Camera3D camera);
void SortChunksByDistance(VoxelWorld* world, Vector3 playerPosition);

// Face indices for cube faces
#define FACE_FRONT  0
#define FACE_BACK   1
#define FACE_LEFT   2
#define FACE_RIGHT  3
#define FACE_TOP    4
#define FACE_BOTTOM 5

#ifdef __cplusplus
}
#endif

#endif // VOXEL_RENDERER_H 
