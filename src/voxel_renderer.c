#include "voxel_renderer.h"
#include "raymath.h"
#include <stdlib.h>
#include <string.h>

//----------------------------------------------------------------------------------
// Local Constants
//----------------------------------------------------------------------------------
#define MAX_VERTICES_PER_CHUNK (CHUNK_SIZE * CHUNK_SIZE * WORLD_HEIGHT * 6 * 4) // 6 faces, 4 vertices each
#define MAX_TRIANGLES_PER_CHUNK (CHUNK_SIZE * CHUNK_SIZE * WORLD_HEIGHT * 6 * 2) // 6 faces, 2 triangles each

// Face normal vectors
static const Vector3 faceNormals[6] = {
    { 0,  0,  1}, // FACE_FRONT
    { 0,  0, -1}, // FACE_BACK
    {-1,  0,  0}, // FACE_LEFT
    { 1,  0,  0}, // FACE_RIGHT
    { 0,  1,  0}, // FACE_TOP
    { 0, -1,  0}  // FACE_BOTTOM
};

// Face offset vectors for neighbor checking
static const Vector3 faceOffsets[6] = {
    { 0,  0,  1}, // FACE_FRONT
    { 0,  0, -1}, // FACE_BACK
    {-1,  0,  0}, // FACE_LEFT
    { 1,  0,  0}, // FACE_RIGHT
    { 0,  1,  0}, // FACE_TOP
    { 0, -1,  0}  // FACE_BOTTOM
};

// Vertex positions for each face (relative to block corner)
static const Vector3 faceVertices[6][4] = {
    // FACE_FRONT (Z+)
    {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}},
    // FACE_BACK (Z-)
    {{1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}},
    // FACE_LEFT (X-)
    {{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}},
    // FACE_RIGHT (X+)
    {{1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1}},
    // FACE_TOP (Y+)
    {{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}},
    // FACE_BOTTOM (Y-)
    {{0, 0, 1}, {0, 0, 0}, {1, 0, 0}, {1, 0, 1}}
};

//----------------------------------------------------------------------------------
// Rendering Functions
//----------------------------------------------------------------------------------
void InitVoxelRenderer(void) {
    // Initialize any renderer-specific data if needed
}

void RenderVoxelWorld(VoxelWorld* world, Camera3D camera) {
    // Update chunk visibility based on frustum culling
    FrustumCullChunks(world, camera);
    
    // Update chunk meshes that need regeneration
    for (int i = 0; i < MAX_CHUNKS; i++) {
        if (world->chunks[i].isLoaded && world->chunks[i].needsRegen) {
            UpdateChunkMesh(&world->chunks[i], world);
        }
    }
    
    // Render visible chunks
    for (int i = 0; i < MAX_CHUNKS; i++) {
        if (world->chunks[i].isLoaded && world->chunks[i].isVisible && world->chunks[i].hasMesh) {
            RenderChunk(&world->chunks[i], camera);
        }
    }
}

void RenderChunk(Chunk* chunk, Camera3D camera) {
    if (!chunk->hasMesh || chunk->vertexCount == 0) return;
    
    Vector3 chunkWorldPos = ChunkToWorld(chunk->position);
    Matrix transform = MatrixTranslate(chunkWorldPos.x, chunkWorldPos.y, chunkWorldPos.z);
    
    // Draw the chunk mesh
    DrawMesh(chunk->mesh, LoadMaterialDefault(), transform);
}

void UpdateChunkMesh(Chunk* chunk, VoxelWorld* world) {
    if (!chunk->needsRegen) return;
    
    // Generate new mesh
    GenerateChunkMesh(chunk, world);
    chunk->needsRegen = false;
}

void UnloadVoxelRenderer(void) {
    // Clean up any renderer resources if needed
}

//----------------------------------------------------------------------------------
// Mesh Generation Functions
//----------------------------------------------------------------------------------
void GenerateChunkMesh(Chunk* chunk, VoxelWorld* world) {
    // Unload existing mesh
    if (chunk->hasMesh) {
        UnloadMesh(chunk->mesh);
        chunk->hasMesh = false;
    }
    
    // Allocate temporary arrays for mesh data
    float* vertices = (float*)malloc(MAX_VERTICES_PER_CHUNK * 3 * sizeof(float));
    float* colors = (float*)malloc(MAX_VERTICES_PER_CHUNK * 4 * sizeof(float));
    unsigned short* indices = (unsigned short*)malloc(MAX_TRIANGLES_PER_CHUNK * 3 * sizeof(unsigned short));
    
    if (!vertices || !colors || !indices) {
        if (vertices) free(vertices);
        if (colors) free(colors);
        if (indices) free(indices);
        return;
    }
    
    int vertexIndex = 0;
    int indexIndex = 0;
    
    // Generate mesh for each block in chunk
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                BlockType block = chunk->blocks[x][y][z];
                
                if (block == BLOCK_AIR) continue;
                
                Vector3 blockPos = {x, y, z};
                
                // Check each face of the block
                for (int face = 0; face < 6; face++) {
                    BlockPos neighborPos = {
                        chunk->position.x * CHUNK_SIZE + x + (int)faceOffsets[face].x,
                        y + (int)faceOffsets[face].y,
                        chunk->position.z * CHUNK_SIZE + z + (int)faceOffsets[face].z
                    };
                    
                    if (ShouldRenderFace(world, neighborPos, face)) {
                        AddFaceToMesh(blockPos, face, block, vertices, colors, &vertexIndex);
                        
                        // Add indices for two triangles
                        unsigned short baseIndex = (vertexIndex - 4);
                        
                        // First triangle
                        indices[indexIndex++] = baseIndex;
                        indices[indexIndex++] = baseIndex + 1;
                        indices[indexIndex++] = baseIndex + 2;
                        
                        // Second triangle
                        indices[indexIndex++] = baseIndex;
                        indices[indexIndex++] = baseIndex + 2;
                        indices[indexIndex++] = baseIndex + 3;
                    }
                }
            }
        }
    }
    
    chunk->vertexCount = vertexIndex;
    chunk->triangleCount = indexIndex / 3;
    
    // Create mesh if we have vertices
    if (vertexIndex > 0) {
        Mesh mesh = { 0 };
        mesh.vertexCount = vertexIndex;
        mesh.triangleCount = chunk->triangleCount;
        
        // Allocate and copy vertex data
        mesh.vertices = (float*)RL_MALLOC(vertexIndex * 3 * sizeof(float));
        mesh.colors = (unsigned char*)RL_MALLOC(vertexIndex * 4 * sizeof(unsigned char));
        mesh.indices = (unsigned short*)RL_MALLOC(indexIndex * sizeof(unsigned short));
        
        memcpy(mesh.vertices, vertices, vertexIndex * 3 * sizeof(float));
        memcpy(mesh.indices, indices, indexIndex * sizeof(unsigned short));
        
        // Convert float colors to unsigned char
        for (int i = 0; i < vertexIndex * 4; i++) {
            mesh.colors[i] = (unsigned char)(colors[i] * 255.0f);
        }
        
        // Upload mesh to GPU
        UploadMesh(&mesh, false);
        
        chunk->mesh = mesh;
        chunk->hasMesh = true;
    }
    
    // Free temporary arrays
    free(vertices);
    free(colors);
    free(indices);
}

void AddFaceToMesh(Vector3 position, int faceIndex, BlockType block, 
                   float* vertices, float* colors, int* vertexIndex) {
    Color blockColor = GetBlockColor(block);
    float r = blockColor.r / 255.0f;
    float g = blockColor.g / 255.0f;
    float b = blockColor.b / 255.0f;
    float a = blockColor.a / 255.0f;
    
    // Add lighting based on face orientation
    float lighting = 1.0f;
    switch (faceIndex) {
        case FACE_TOP: lighting = 1.0f; break;      // Brightest
        case FACE_FRONT:
        case FACE_BACK: lighting = 0.8f; break;     // Medium
        case FACE_LEFT:
        case FACE_RIGHT: lighting = 0.6f; break;    // Darker
        case FACE_BOTTOM: lighting = 0.4f; break;   // Darkest
    }
    
    r *= lighting;
    g *= lighting;
    b *= lighting;
    
    // Add vertices for this face
    for (int i = 0; i < 4; i++) {
        Vector3 vertex = Vector3Add(position, faceVertices[faceIndex][i]);
        
        // Vertex position
        vertices[(*vertexIndex) * 3 + 0] = vertex.x;
        vertices[(*vertexIndex) * 3 + 1] = vertex.y;
        vertices[(*vertexIndex) * 3 + 2] = vertex.z;
        
        // Vertex color
        colors[(*vertexIndex) * 4 + 0] = r;
        colors[(*vertexIndex) * 4 + 1] = g;
        colors[(*vertexIndex) * 4 + 2] = b;
        colors[(*vertexIndex) * 4 + 3] = a;
        
        (*vertexIndex)++;
    }
}

bool ShouldRenderFace(VoxelWorld* world, BlockPos position, int faceIndex) {
    BlockType neighborBlock = GetBlock(world, position);
    
    // Render face if neighbor is air or transparent
    return IsBlockTransparent(neighborBlock);
}

//----------------------------------------------------------------------------------
// Culling and Optimization Functions
//----------------------------------------------------------------------------------
bool IsChunkInFrustum(Chunk* chunk, Camera3D camera) {
    // Simple distance-based culling for now
    Vector3 chunkWorldPos = ChunkToWorld(chunk->position);
    Vector3 chunkCenter = Vector3Add(chunkWorldPos, (Vector3){CHUNK_SIZE/2, WORLD_HEIGHT/2, CHUNK_SIZE/2});
    
    float distance = Vector3Distance(camera.position, chunkCenter);
    float maxDistance = RENDER_DISTANCE * CHUNK_SIZE;
    
    return distance <= maxDistance;
}

void FrustumCullChunks(VoxelWorld* world, Camera3D camera) {
    for (int i = 0; i < MAX_CHUNKS; i++) {
        if (world->chunks[i].isLoaded) {
            world->chunks[i].isVisible = IsChunkInFrustum(&world->chunks[i], camera);
        }
    }
}

void SortChunksByDistance(VoxelWorld* world, Vector3 playerPosition) {
    // Simple bubble sort by distance (can be optimized)
    for (int i = 0; i < MAX_CHUNKS - 1; i++) {
        for (int j = 0; j < MAX_CHUNKS - i - 1; j++) {
            if (!world->chunks[j].isLoaded) continue;
            if (!world->chunks[j + 1].isLoaded) continue;
            
            Vector3 pos1 = ChunkToWorld(world->chunks[j].position);
            Vector3 pos2 = ChunkToWorld(world->chunks[j + 1].position);
            
            float dist1 = Distance2D(playerPosition, pos1);
            float dist2 = Distance2D(playerPosition, pos2);
            
            if (dist1 > dist2) {
                // Swap chunks
                Chunk temp = world->chunks[j];
                world->chunks[j] = world->chunks[j + 1];
                world->chunks[j + 1] = temp;
            }
        }
    }
} 
