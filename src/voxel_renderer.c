#include "voxel_renderer.h"
#include "raymath.h"
#include "rlgl.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/*
---------------------------------------------------------------------------------
Voxel Renderer

Chunk-based voxel world rendering with dual-pass transparency.
Each chunk generates two separate meshes: one for opaque blocks and one for transparent
blocks (e.g., glass, leaves, water). During rendering, opaque meshes are drawn first
(front-to-back) with depth writing enabled, followed by transparent meshes (back-to-front)
with depth masking disabled to ensure correct alpha blending.

Block textures are packed into a single atlas for efficient GPU usage. Texture coordinates
for each block face are precomputed and stored in the texture manager. Face culling and
neighbor checks are used to avoid drawing hidden faces, improving performance.

The renderer integrates with the world/chunk system and player camera. It exposes functions
to update chunk meshes when blocks change, and to render visible chunks based on camera
frustum culling. All block face geometry, normals, and UVs are generated procedurally.

---------------------------------------------------------------------------------
*/



//----------------------------------------------------------------------------------
// Local Constants
//----------------------------------------------------------------------------------
#define MAX_VERTICES_PER_CHUNK (CHUNK_SIZE * CHUNK_SIZE * WORLD_HEIGHT * 6 * 4) // 6 faces, 4 vertices each
#define MAX_TRIANGLES_PER_CHUNK (CHUNK_SIZE * CHUNK_SIZE * WORLD_HEIGHT * 6 * 2) // 6 faces, 2 triangles each

//----------------------------------------------------------------------------------
// Global Variables
//----------------------------------------------------------------------------------
static TextureManager textureManager = {0};

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

// Vertex positions for each face (relative to block corner) - Fixed winding order
static const Vector3 faceVertices[6][4] = {
    // FACE_FRONT (Z+) - Counter-clockwise from front view
    {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}},
    // FACE_BACK (Z-) - Counter-clockwise from back view  
    {{1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}},
    // FACE_LEFT (X-) - Counter-clockwise from left view
    {{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}},
    // FACE_RIGHT (X+) - Counter-clockwise from right view
    {{1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1}},
    // FACE_TOP (Y+) - Counter-clockwise from top view
    {{0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0}},
    // FACE_BOTTOM (Y-) - Counter-clockwise from bottom view
    {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}
};

// UV coordinates for each face vertex - Fixed to match new winding order
static const Vector2 faceUVs[4] = {
    {0.0f, 1.0f}, // Bottom-left
    {1.0f, 1.0f}, // Bottom-right  
    {1.0f, 0.0f}, // Top-right
    {0.0f, 0.0f}  // Top-left
};

//----------------------------------------------------------------------------------
// Rendering Functions
//----------------------------------------------------------------------------------
void InitVoxelRenderer(void) {
    InitTextureManager();
    LoadBlockTextures();
    
    // Enable depth testing for proper 3D rendering
    // Alpha blending is handled automatically by raylib when textures have alpha
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
    
    // Sort chunks by distance for transparent rendering
    SortChunksByDistance(world, camera.position);
    
    // First pass: Render opaque blocks (front to back for Z-buffer efficiency)
    for (int i = 0; i < MAX_CHUNKS; i++) {
        if (world->chunks[i].isLoaded && world->chunks[i].isVisible && world->chunks[i].hasMesh) {
            if (world->chunks[i].vertexCount > 0) {
                Vector3 chunkWorldPos = ChunkToWorld(world->chunks[i].position);
                Matrix transform = MatrixTranslate(chunkWorldPos.x, chunkWorldPos.y, chunkWorldPos.z);
                DrawMesh(world->chunks[i].mesh, world->chunks[i].material, transform);
            }
        }
    }
    
    // Second pass: Render transparent blocks (back to front for proper alpha blending)
    // Enable alpha blending and disable depth writing for transparent objects
    rlSetBlendMode(BLEND_ALPHA);
    rlSetBlendFactors(RL_SRC_ALPHA, RL_ONE_MINUS_SRC_ALPHA, RL_FUNC_ADD);
    
    for (int i = MAX_CHUNKS - 1; i >= 0; i--) {  // Reverse order for back-to-front
        if (world->chunks[i].isLoaded && world->chunks[i].isVisible && world->chunks[i].hasMesh) {
            if (world->chunks[i].transparentVertexCount > 0) {
                Vector3 chunkWorldPos = ChunkToWorld(world->chunks[i].position);
                Matrix transform = MatrixTranslate(chunkWorldPos.x, chunkWorldPos.y, chunkWorldPos.z);
                
                // Disable depth writing for transparent objects but keep depth testing
                rlDisableDepthMask();
                DrawMesh(world->chunks[i].transparentMesh, world->chunks[i].transparentMaterial, transform);
                rlEnableDepthMask();
            }
        }
    }
    
    // Reset blend mode to normal
    rlSetBlendMode(BLEND_ALPHA);
}

void RenderChunk(Chunk* chunk, Camera3D camera) {
    // This function is now handled by RenderVoxelWorld for proper depth sorting
    // Keeping for compatibility but not used in the new rendering pipeline
    if (!chunk->hasMesh) return;
    
    Vector3 chunkWorldPos = ChunkToWorld(chunk->position);
    Matrix transform = MatrixTranslate(chunkWorldPos.x, chunkWorldPos.y, chunkWorldPos.z);
    
    // Draw opaque mesh
    if (chunk->vertexCount > 0) {
        DrawMesh(chunk->mesh, chunk->material, transform);
    }
    
    // Draw transparent mesh with alpha blending
    if (chunk->transparentVertexCount > 0) {
        rlSetBlendMode(BLEND_ALPHA);
        DrawMesh(chunk->transparentMesh, chunk->transparentMaterial, transform);
        rlSetBlendMode(BLEND_ALPHA);
    }
}

void UpdateChunkMesh(Chunk* chunk, VoxelWorld* world) {
    if (!chunk->needsRegen) return;
    
    // Generate new mesh
    GenerateChunkMesh(chunk, world);
    chunk->needsRegen = false;
}

void UnloadVoxelRenderer(void) {
    UnloadTextureManager();
}

//----------------------------------------------------------------------------------
// Mesh Generation Functions
//----------------------------------------------------------------------------------
void GenerateChunkMesh(Chunk* chunk, VoxelWorld* world) {
    // Free existing mesh if it exists
    if (chunk->hasMesh) {
        UnloadMesh(chunk->mesh);
        UnloadMaterial(chunk->material);
        chunk->hasMesh = false;
    }
    
    // Create separate arrays for opaque and transparent blocks
    float* opaqueVertices = (float*)malloc(MAX_VERTICES_PER_CHUNK * 3 * sizeof(float));
    float* opaqueTexCoords = (float*)malloc(MAX_VERTICES_PER_CHUNK * 2 * sizeof(float));
    unsigned short* opaqueIndices = (unsigned short*)malloc(MAX_TRIANGLES_PER_CHUNK * 3 * sizeof(unsigned short));
    
    float* transparentVertices = (float*)malloc(MAX_VERTICES_PER_CHUNK * 3 * sizeof(float));
    float* transparentTexCoords = (float*)malloc(MAX_VERTICES_PER_CHUNK * 2 * sizeof(float));
    unsigned short* transparentIndices = (unsigned short*)malloc(MAX_TRIANGLES_PER_CHUNK * 3 * sizeof(unsigned short));
    
    int opaqueVertexIndex = 0;
    int opaqueIndexIndex = 0;
    int transparentVertexIndex = 0;
    int transparentIndexIndex = 0;
    
    // Generate faces for each block
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                BlockType block = chunk->blocks[x][y][z];
                
                if (block == BLOCK_AIR) continue;
                
                Vector3 blockPos = {x, y, z};
                bool isTransparent = BlockNeedsAlphaBlending(block);
                
                // Choose the appropriate arrays based on block transparency
                float* vertices = isTransparent ? transparentVertices : opaqueVertices;
                float* texCoords = isTransparent ? transparentTexCoords : opaqueTexCoords;
                unsigned short* indices = isTransparent ? transparentIndices : opaqueIndices;
                int* vertexIndex = isTransparent ? &transparentVertexIndex : &opaqueVertexIndex;
                int* indexIndex = isTransparent ? &transparentIndexIndex : &opaqueIndexIndex;
                
                // Check each face of the block
                for (int face = 0; face < 6; face++) {
                    BlockPos neighborPos = {
                        chunk->position.x * CHUNK_SIZE + x + (int)faceOffsets[face].x,
                        y + (int)faceOffsets[face].y,
                        chunk->position.z * CHUNK_SIZE + z + (int)faceOffsets[face].z
                    };
                    
                    if (ShouldRenderFace(world, neighborPos, face)) {
                        AddFaceToMesh(blockPos, face, block, vertices, texCoords, vertexIndex);
                        
                        // Add indices for two triangles (fixed winding order)
                        unsigned short baseIndex = (*vertexIndex - 4);
                        
                        // First triangle (counter-clockwise)
                        indices[(*indexIndex)++] = baseIndex;
                        indices[(*indexIndex)++] = baseIndex + 1;
                        indices[(*indexIndex)++] = baseIndex + 2;
                        
                        // Second triangle (counter-clockwise)
                        indices[(*indexIndex)++] = baseIndex;
                        indices[(*indexIndex)++] = baseIndex + 2;
                        indices[(*indexIndex)++] = baseIndex + 3;
                    }
                }
            }
        }
    }
    
    // Create opaque mesh
    if (opaqueVertexIndex > 0) {
        Mesh opaqueMesh = { 0 };
        opaqueMesh.vertexCount = opaqueVertexIndex;
        opaqueMesh.triangleCount = opaqueIndexIndex / 3;
        
        // Allocate and copy vertex data
        opaqueMesh.vertices = (float*)RL_MALLOC(opaqueVertexIndex * 3 * sizeof(float));
        opaqueMesh.texcoords = (float*)RL_MALLOC(opaqueVertexIndex * 2 * sizeof(float));
        opaqueMesh.indices = (unsigned short*)RL_MALLOC(opaqueIndexIndex * sizeof(unsigned short));
        
        memcpy(opaqueMesh.vertices, opaqueVertices, opaqueVertexIndex * 3 * sizeof(float));
        memcpy(opaqueMesh.texcoords, opaqueTexCoords, opaqueVertexIndex * 2 * sizeof(float));
        memcpy(opaqueMesh.indices, opaqueIndices, opaqueIndexIndex * sizeof(unsigned short));
        
        // Upload mesh to GPU
        UploadMesh(&opaqueMesh, false);
        
        chunk->mesh = opaqueMesh;
        chunk->vertexCount = opaqueVertexIndex;
        chunk->triangleCount = opaqueIndexIndex / 3;
    }
    
    // Create transparent mesh
    if (transparentVertexIndex > 0) {
        Mesh transparentMesh = { 0 };
        transparentMesh.vertexCount = transparentVertexIndex;
        transparentMesh.triangleCount = transparentIndexIndex / 3;
        
        // Allocate and copy vertex data
        transparentMesh.vertices = (float*)RL_MALLOC(transparentVertexIndex * 3 * sizeof(float));
        transparentMesh.texcoords = (float*)RL_MALLOC(transparentVertexIndex * 2 * sizeof(float));
        transparentMesh.indices = (unsigned short*)RL_MALLOC(transparentIndexIndex * sizeof(unsigned short));
        
        memcpy(transparentMesh.vertices, transparentVertices, transparentVertexIndex * 3 * sizeof(float));
        memcpy(transparentMesh.texcoords, transparentTexCoords, transparentVertexIndex * 2 * sizeof(float));
        memcpy(transparentMesh.indices, transparentIndices, transparentIndexIndex * sizeof(unsigned short));
        
        // Upload mesh to GPU
        UploadMesh(&transparentMesh, false);
        
        chunk->transparentMesh = transparentMesh;
        chunk->transparentVertexCount = transparentVertexIndex;
        chunk->transparentTriangleCount = transparentIndexIndex / 3;
    }
    
    // Create materials for both meshes
    if (opaqueVertexIndex > 0 || transparentVertexIndex > 0) {
        chunk->material = LoadMaterialDefault();
        if (textureManager.atlas.id > 0) {
            SetMaterialTexture(&chunk->material, MATERIAL_MAP_DIFFUSE, textureManager.atlas);
            chunk->material.maps[MATERIAL_MAP_DIFFUSE].color = (Color){255, 255, 255, 255};
        }
        
        // Create transparent material
        chunk->transparentMaterial = LoadMaterialDefault();
        if (textureManager.atlas.id > 0) {
            SetMaterialTexture(&chunk->transparentMaterial, MATERIAL_MAP_DIFFUSE, textureManager.atlas);
            chunk->transparentMaterial.maps[MATERIAL_MAP_DIFFUSE].color = (Color){255, 255, 255, 255};
        }
        
        chunk->hasMesh = true;
    }
    
    // Free temporary arrays
    free(opaqueVertices);
    free(opaqueTexCoords);
    free(opaqueIndices);
    free(transparentVertices);
    free(transparentTexCoords);
    free(transparentIndices);
}

void AddFaceToMesh(Vector3 position, int faceIndex, BlockType block, 
                   float* vertices, float* texCoords, int* vertexIndex) {
    // Get texture UV coordinates for this block and face
    float u, v, w, h;
    GetBlockTextureUV(block, faceIndex, &u, &v, &w, &h);
    
    // Add vertices for this face
    for (int i = 0; i < 4; i++) {
        Vector3 vertex = Vector3Add(position, faceVertices[faceIndex][i]);
        
        // Vertex position
        vertices[(*vertexIndex) * 3 + 0] = vertex.x;
        vertices[(*vertexIndex) * 3 + 1] = vertex.y;
        vertices[(*vertexIndex) * 3 + 2] = vertex.z;
        
        // Texture coordinates
        float texU = u + faceUVs[i].x * w;
        float texV = v + faceUVs[i].y * h;
        texCoords[(*vertexIndex) * 2 + 0] = texU;
        texCoords[(*vertexIndex) * 2 + 1] = texV;
        
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

//----------------------------------------------------------------------------------
// Texture Management Functions
//----------------------------------------------------------------------------------
void InitTextureManager(void) {
    textureManager.textureCount = 0;
    textureManager.atlas = (Texture2D){0};
    memset(textureManager.texCoords, 0, sizeof(textureManager.texCoords));
    memset(textureManager.textureNames, 0, sizeof(textureManager.textureNames));
}

void LoadBlockTextures(void) {
    // Load actual texture files from resources directory
    const char* textureNames[] = {
        "grass_block_top", "grass_block_side", "dirt",
        "stone", "cobblestone", "bedrock", "sand", "gravel",
        "oak_log", "oak_log_top", "oak_planks", "oak_leaves",
        "birch_log", "birch_log_top", "birch_planks", "birch_leaves",
        "acacia_log", "acacia_log_top", "acacia_planks", "acacia_leaves",
        "dark_oak_log", "dark_oak_log_top", "dark_oak_planks", "dark_oak_leaves",
        "stone_bricks", "mossy_stone_bricks", "andesite", "granite", "diorite",
        "sandstone", "sandstone_top", "sandstone_bottom",
        "coal_ore", "iron_ore", "gold_ore", "diamond_ore",
        "iron_block", "gold_block", "diamond_block",
        "white_wool", "orange_wool", "blue_wool", "red_wool",
        "glass", "bricks", "bookshelf", "glowstone", "obsidian",
        "netherrack", "end_stone", "quartz_block", "packed_ice"
    };
    
    int textureCount = sizeof(textureNames) / sizeof(textureNames[0]);
    int texturesPerRow = TEXTURE_ATLAS_SIZE / TEXTURE_SIZE;
    
    // Create atlas image with transparent background (RGBA with alpha = 0)
    Image atlasImage = GenImageColor(TEXTURE_ATLAS_SIZE, TEXTURE_ATLAS_SIZE, (Color){0, 0, 0, 0});
    
    int successfulLoads = 0;
    int placeholderCount = 0;
    
    for (int i = 0; i < textureCount && i < MAX_BLOCK_TEXTURES; i++) {
        // Try multiple potential file paths
        char filePath[256];
        Image blockTexture = {0};
        bool loaded = false;
        
        // Try different possible paths
        const char* possiblePaths[] = {
            "src/resources/textures/block/%s.png",
            "resources/textures/block/%s.png", 
            "./src/resources/textures/block/%s.png",
            "./resources/textures/block/%s.png"
        };
        
        for (int pathIdx = 0; pathIdx < 4; pathIdx++) {
            snprintf(filePath, sizeof(filePath), possiblePaths[pathIdx], textureNames[i]);
            
            if (FileExists(filePath)) {
                blockTexture = LoadImage(filePath);
                if (blockTexture.data != NULL) {
                    loaded = true;
                    successfulLoads++;
                    
                    // Ensure image format supports alpha channel
                    if (blockTexture.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) {
                        ImageFormat(&blockTexture, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
                    }
                    break;
                }
            }
        }
        
        if (!loaded) {
            // Create a placeholder colored texture with full alpha
            Color placeholderColors[] = {
                GREEN, DARKGREEN, BROWN, GRAY, DARKGRAY, (Color){64,64,64,255}, 
                BEIGE, (Color){136,136,136,255}, (Color){139,69,19,255}, (Color){162,130,78,255},
                DARKGREEN, (Color){220,220,220,255}, (Color){192,175,121,255}, (Color){128,167,85,255},
                (Color){186,99,64,255}, (Color){168,90,50,255}, (Color){99,128,15,255}, (Color){66,43,20,255},
                (Color){123,123,123,255}, (Color){115,121,105,255}, (Color){132,134,132,255}, (Color){149,103,85,255},
                (Color){188,188,188,255}, (Color){245,238,173,255}, (Color){84,84,84,255}, (Color){135,106,97,255},
                (Color){143,140,125,255}, (Color){92,219,213,255}, (Color){220,220,220,255}, GOLD,
                (Color){93,219,213,255}, WHITE, ORANGE, BLUE, RED, (Color){255,255,255,128},
                (Color){150,97,83,255}, (Color){139,69,19,255}, (Color){255,207,139,255}, (Color){20,18,30,255},
                (Color){97,38,38,255}, (Color){221,223,165,255}, (Color){235,229,222,255}, (Color){160,160,255,255}
            };
            
            Color color = (i < sizeof(placeholderColors) / sizeof(placeholderColors[0])) ? 
                          placeholderColors[i] : WHITE;
            blockTexture = GenImageColor(TEXTURE_SIZE, TEXTURE_SIZE, color);
            
            // Ensure placeholder also has alpha channel
            ImageFormat(&blockTexture, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            placeholderCount++;
        }
        
        // Ensure texture is the correct size
        if (blockTexture.width != TEXTURE_SIZE || blockTexture.height != TEXTURE_SIZE) {
            ImageResize(&blockTexture, TEXTURE_SIZE, TEXTURE_SIZE);
        }
        
        // Calculate position in atlas
        int x = (i % texturesPerRow) * TEXTURE_SIZE;
        int y = (i / texturesPerRow) * TEXTURE_SIZE;
        
        // Copy texture to atlas preserving alpha channel (use BLANK instead of WHITE)
        ImageDraw(&atlasImage, blockTexture, 
                  (Rectangle){0, 0, TEXTURE_SIZE, TEXTURE_SIZE}, 
                  (Rectangle){x, y, TEXTURE_SIZE, TEXTURE_SIZE}, 
                  (Color){255, 255, 255, 255}); // Use full white with full alpha
        
        // Store texture info
        strcpy(textureManager.textureNames[i], textureNames[i]);
        textureManager.texCoords[i][0] = (float)x / TEXTURE_ATLAS_SIZE;  // u
        textureManager.texCoords[i][1] = (float)y / TEXTURE_ATLAS_SIZE;  // v
        textureManager.texCoords[i][2] = (float)TEXTURE_SIZE / TEXTURE_ATLAS_SIZE;  // width
        textureManager.texCoords[i][3] = (float)TEXTURE_SIZE / TEXTURE_ATLAS_SIZE;  // height
        
        textureManager.textureCount++;
        
        // Clean up individual texture
        UnloadImage(blockTexture);
    }
    
    // Create texture from atlas
    textureManager.atlas = LoadTextureFromImage(atlasImage);
    UnloadImage(atlasImage);
    
    // Set texture filter to point (pixelated) for retro look
    SetTextureFilter(textureManager.atlas, TEXTURE_FILTER_POINT);
    
    printf("Block textures loaded: %d successful, %d placeholders\n", successfulLoads, placeholderCount);
}

void UnloadTextureManager(void) {
    if (textureManager.atlas.id > 0) {
        UnloadTexture(textureManager.atlas);
    }
    textureManager = (TextureManager){0};
}

int GetTextureIndex(const char* textureName) {
    for (int i = 0; i < textureManager.textureCount; i++) {
        if (strcmp(textureManager.textureNames[i], textureName) == 0) {
            return i;
        }
    }
    return 0; // Default to first texture if not found
}

void GetBlockTextureUV(BlockType block, int faceIndex, float* u, float* v, float* w, float* h) {
    const char* textureName = "stone"; // Default
    
    // Map block types and faces to texture names
    switch (block) {
        case BLOCK_GRASS:
            if (faceIndex == FACE_TOP) textureName = "grass_block_top";
            else if (faceIndex == FACE_BOTTOM) textureName = "dirt";
            else textureName = "grass_block_side";
            break;
        case BLOCK_DIRT: textureName = "dirt"; break;
        case BLOCK_STONE: textureName = "stone"; break;
        case BLOCK_COBBLESTONE: textureName = "cobblestone"; break;
        case BLOCK_BEDROCK: textureName = "bedrock"; break;
        case BLOCK_SAND: textureName = "sand"; break;
        case BLOCK_GRAVEL: textureName = "gravel"; break;
        case BLOCK_WATER: textureName = "water_still"; break;
        
        // Wood blocks
        case BLOCK_OAK_LOG:
            if (faceIndex == FACE_TOP || faceIndex == FACE_BOTTOM) textureName = "oak_log_top";
            else textureName = "oak_log";
            break;
        case BLOCK_OAK_PLANKS: textureName = "oak_planks"; break;
        case BLOCK_OAK_LEAVES: textureName = "oak_leaves"; break;
        case BLOCK_BIRCH_LOG:
            if (faceIndex == FACE_TOP || faceIndex == FACE_BOTTOM) textureName = "birch_log_top";
            else textureName = "birch_log";
            break;
        case BLOCK_BIRCH_PLANKS: textureName = "birch_planks"; break;
        case BLOCK_BIRCH_LEAVES: textureName = "birch_leaves"; break;
        case BLOCK_ACACIA_LOG:
            if (faceIndex == FACE_TOP || faceIndex == FACE_BOTTOM) textureName = "acacia_log_top";
            else textureName = "acacia_log";
            break;
        case BLOCK_ACACIA_PLANKS: textureName = "acacia_planks"; break;
        case BLOCK_ACACIA_LEAVES: textureName = "acacia_leaves"; break;
        case BLOCK_DARK_OAK_LOG:
            if (faceIndex == FACE_TOP || faceIndex == FACE_BOTTOM) textureName = "dark_oak_log_top";
            else textureName = "dark_oak_log";
            break;
        case BLOCK_DARK_OAK_PLANKS: textureName = "dark_oak_planks"; break;
        case BLOCK_DARK_OAK_LEAVES: textureName = "dark_oak_leaves"; break;
        
        // Stone variants
        case BLOCK_STONE_BRICKS: textureName = "stone_bricks"; break;
        case BLOCK_MOSSY_STONE_BRICKS: textureName = "mossy_stone_bricks"; break;
        case BLOCK_ANDESITE: textureName = "andesite"; break;
        case BLOCK_GRANITE: textureName = "granite"; break;
        case BLOCK_DIORITE: textureName = "diorite"; break;
        case BLOCK_MOSSY_COBBLESTONE: textureName = "mossy_cobblestone"; break;
        case BLOCK_SMOOTH_STONE: textureName = "smooth_stone"; break;
        
        // Sandstone
        case BLOCK_SANDSTONE:
            if (faceIndex == FACE_TOP) textureName = "sandstone_top";
            else if (faceIndex == FACE_BOTTOM) textureName = "sandstone_bottom";
            else textureName = "sandstone";
            break;
        case BLOCK_CHISELED_SANDSTONE: textureName = "chiseled_sandstone"; break;
        case BLOCK_CUT_SANDSTONE: textureName = "cut_sandstone"; break;
        case BLOCK_RED_SAND: textureName = "red_sand"; break;
        case BLOCK_RED_SANDSTONE: textureName = "red_sandstone"; break;
        
        // Ores
        case BLOCK_COAL_ORE: textureName = "coal_ore"; break;
        case BLOCK_IRON_ORE: textureName = "iron_ore"; break;
        case BLOCK_GOLD_ORE: textureName = "gold_ore"; break;
        case BLOCK_DIAMOND_ORE: textureName = "diamond_ore"; break;
        case BLOCK_REDSTONE_ORE: textureName = "redstone_ore"; break;
        case BLOCK_EMERALD_ORE: textureName = "emerald_ore"; break;
        case BLOCK_LAPIS_ORE: textureName = "lapis_ore"; break;
        
        // Metal blocks
        case BLOCK_IRON_BLOCK: textureName = "iron_block"; break;
        case BLOCK_GOLD_BLOCK: textureName = "gold_block"; break;
        case BLOCK_DIAMOND_BLOCK: textureName = "diamond_block"; break;
        case BLOCK_EMERALD_BLOCK: textureName = "emerald_block"; break;
        case BLOCK_REDSTONE_BLOCK: textureName = "redstone_block"; break;
        case BLOCK_LAPIS_BLOCK: textureName = "lapis_block"; break;
        case BLOCK_COAL_BLOCK: textureName = "coal_block"; break;
        
        // Wool blocks
        case BLOCK_WHITE_WOOL: textureName = "white_wool"; break;
        case BLOCK_ORANGE_WOOL: textureName = "orange_wool"; break;
        case BLOCK_MAGENTA_WOOL: textureName = "magenta_wool"; break;
        case BLOCK_LIGHT_BLUE_WOOL: textureName = "light_blue_wool"; break;
        case BLOCK_YELLOW_WOOL: textureName = "yellow_wool"; break;
        case BLOCK_LIME_WOOL: textureName = "lime_wool"; break;
        case BLOCK_PINK_WOOL: textureName = "pink_wool"; break;
        case BLOCK_GRAY_WOOL: textureName = "gray_wool"; break;
        case BLOCK_LIGHT_GRAY_WOOL: textureName = "light_gray_wool"; break;
        case BLOCK_CYAN_WOOL: textureName = "cyan_wool"; break;
        case BLOCK_PURPLE_WOOL: textureName = "purple_wool"; break;
        case BLOCK_BLUE_WOOL: textureName = "blue_wool"; break;
        case BLOCK_BROWN_WOOL: textureName = "brown_wool"; break;
        case BLOCK_GREEN_WOOL: textureName = "green_wool"; break;
        case BLOCK_RED_WOOL: textureName = "red_wool"; break;
        case BLOCK_BLACK_WOOL: textureName = "black_wool"; break;
        
        // Glass
        case BLOCK_GLASS: textureName = "glass"; break;
        case BLOCK_WHITE_STAINED_GLASS: textureName = "white_stained_glass"; break;
        case BLOCK_ORANGE_STAINED_GLASS: textureName = "orange_stained_glass"; break;
        case BLOCK_MAGENTA_STAINED_GLASS: textureName = "magenta_stained_glass"; break;
        case BLOCK_LIGHT_BLUE_STAINED_GLASS: textureName = "light_blue_stained_glass"; break;
        case BLOCK_YELLOW_STAINED_GLASS: textureName = "yellow_stained_glass"; break;
        case BLOCK_LIME_STAINED_GLASS: textureName = "lime_stained_glass"; break;
        case BLOCK_PINK_STAINED_GLASS: textureName = "pink_stained_glass"; break;
        case BLOCK_GRAY_STAINED_GLASS: textureName = "gray_stained_glass"; break;
        case BLOCK_LIGHT_GRAY_STAINED_GLASS: textureName = "light_gray_stained_glass"; break;
        case BLOCK_CYAN_STAINED_GLASS: textureName = "cyan_stained_glass"; break;
        case BLOCK_PURPLE_STAINED_GLASS: textureName = "purple_stained_glass"; break;
        case BLOCK_BLUE_STAINED_GLASS: textureName = "blue_stained_glass"; break;
        case BLOCK_BROWN_STAINED_GLASS: textureName = "brown_stained_glass"; break;
        case BLOCK_GREEN_STAINED_GLASS: textureName = "green_stained_glass"; break;
        case BLOCK_RED_STAINED_GLASS: textureName = "red_stained_glass"; break;
        case BLOCK_BLACK_STAINED_GLASS: textureName = "black_stained_glass"; break;
        
        // Special blocks
        case BLOCK_BRICKS: textureName = "bricks"; break;
        case BLOCK_BOOKSHELF: 
            if (faceIndex == FACE_TOP || faceIndex == FACE_BOTTOM) textureName = "oak_planks";
            else textureName = "bookshelf";
            break;
        case BLOCK_CRAFTING_TABLE:
            if (faceIndex == FACE_TOP) textureName = "crafting_table_top";
            else if (faceIndex == FACE_BOTTOM) textureName = "oak_planks";
            else textureName = "crafting_table_side";
            break;
        case BLOCK_FURNACE: textureName = "furnace_side"; break;
        case BLOCK_CHEST: textureName = "chest"; break;
        case BLOCK_GLOWSTONE: textureName = "glowstone"; break;
        case BLOCK_OBSIDIAN: textureName = "obsidian"; break;
        case BLOCK_NETHERRACK: textureName = "netherrack"; break;
        case BLOCK_SOUL_SAND: textureName = "soul_sand"; break;
        case BLOCK_END_STONE: textureName = "end_stone"; break;
        case BLOCK_PURPUR_BLOCK: textureName = "purpur_block"; break;
        case BLOCK_QUARTZ_BLOCK: textureName = "quartz_block_side"; break;
        case BLOCK_PACKED_ICE: textureName = "packed_ice"; break;
        case BLOCK_BLUE_ICE: textureName = "blue_ice"; break;
        case BLOCK_ICE: textureName = "ice"; break;
        case BLOCK_SNOW_BLOCK: textureName = "snow"; break;
        case BLOCK_CACTUS:
            if (faceIndex == FACE_TOP) textureName = "cactus_top";
            else if (faceIndex == FACE_BOTTOM) textureName = "cactus_bottom";
            else textureName = "cactus_side";
            break;
        case BLOCK_PUMPKIN: textureName = "pumpkin_side"; break;
        case BLOCK_JACK_O_LANTERN: 
            if (faceIndex == FACE_FRONT) textureName = "jack_o_lantern";
            else textureName = "pumpkin_side";
            break;
        case BLOCK_MELON: textureName = "melon_side"; break;
        case BLOCK_HAY_BLOCK:
            if (faceIndex == FACE_TOP || faceIndex == FACE_BOTTOM) textureName = "hay_block_top";
            else textureName = "hay_block_side";
            break;
        
        default: textureName = "stone"; break;
    }
    
    int textureIndex = GetTextureIndex(textureName);
    *u = textureManager.texCoords[textureIndex][0];
    *v = textureManager.texCoords[textureIndex][1];
    *w = textureManager.texCoords[textureIndex][2];
    *h = textureManager.texCoords[textureIndex][3];
}

bool BlockNeedsAlphaBlending(BlockType block) {
    switch (block) {
        case BLOCK_GLASS:
        case BLOCK_WHITE_STAINED_GLASS:
        case BLOCK_ORANGE_STAINED_GLASS:
        case BLOCK_MAGENTA_STAINED_GLASS:
        case BLOCK_LIGHT_BLUE_STAINED_GLASS:
        case BLOCK_YELLOW_STAINED_GLASS:
        case BLOCK_LIME_STAINED_GLASS:
        case BLOCK_PINK_STAINED_GLASS:
        case BLOCK_GRAY_STAINED_GLASS:
        case BLOCK_LIGHT_GRAY_STAINED_GLASS:
        case BLOCK_CYAN_STAINED_GLASS:
        case BLOCK_PURPLE_STAINED_GLASS:
        case BLOCK_BLUE_STAINED_GLASS:
        case BLOCK_BROWN_STAINED_GLASS:
        case BLOCK_GREEN_STAINED_GLASS:
        case BLOCK_RED_STAINED_GLASS:
        case BLOCK_BLACK_STAINED_GLASS:
        case BLOCK_OAK_LEAVES:
        case BLOCK_BIRCH_LEAVES:
        case BLOCK_ACACIA_LEAVES:
        case BLOCK_DARK_OAK_LEAVES:
        case BLOCK_ICE:
        case BLOCK_WATER:
            return true;
        default:
            return false;
    }
}

const char* GetBlockTextureName(BlockType block, int faceIndex) {
    // Map block types and faces to texture names (same logic as GetBlockTextureUV)
    switch (block) {
        case BLOCK_GRASS:
            if (faceIndex == FACE_TOP) return "grass_block_top";
            else if (faceIndex == FACE_BOTTOM) return "dirt";
            else return "grass_block_side";
        case BLOCK_DIRT: return "dirt";
        case BLOCK_STONE: return "stone";
        case BLOCK_COBBLESTONE: return "cobblestone";
        case BLOCK_BEDROCK: return "bedrock";
        case BLOCK_SAND: return "sand";
        case BLOCK_GRAVEL: return "gravel";
        case BLOCK_WATER: return "water_still";
        
        // Wood blocks
        case BLOCK_OAK_LOG:
            if (faceIndex == FACE_TOP || faceIndex == FACE_BOTTOM) return "oak_log_top";
            else return "oak_log";
        case BLOCK_OAK_PLANKS: return "oak_planks";
        case BLOCK_OAK_LEAVES: return "oak_leaves";
        case BLOCK_BIRCH_LOG:
            if (faceIndex == FACE_TOP || faceIndex == FACE_BOTTOM) return "birch_log_top";
            else return "birch_log";
        case BLOCK_BIRCH_PLANKS: return "birch_planks";
        case BLOCK_BIRCH_LEAVES: return "birch_leaves";
        case BLOCK_ACACIA_LOG:
            if (faceIndex == FACE_TOP || faceIndex == FACE_BOTTOM) return "acacia_log_top";
            else return "acacia_log";
        case BLOCK_ACACIA_PLANKS: return "acacia_planks";
        case BLOCK_ACACIA_LEAVES: return "acacia_leaves";
        case BLOCK_DARK_OAK_LOG:
            if (faceIndex == FACE_TOP || faceIndex == FACE_BOTTOM) return "dark_oak_log_top";
            else return "dark_oak_log";
        case BLOCK_DARK_OAK_PLANKS: return "dark_oak_planks";
        case BLOCK_DARK_OAK_LEAVES: return "dark_oak_leaves";
        
        // Stone variants
        case BLOCK_STONE_BRICKS: return "stone_bricks";
        case BLOCK_MOSSY_STONE_BRICKS: return "mossy_stone_bricks";
        case BLOCK_ANDESITE: return "andesite";
        case BLOCK_GRANITE: return "granite";
        case BLOCK_DIORITE: return "diorite";
        case BLOCK_MOSSY_COBBLESTONE: return "mossy_cobblestone";
        case BLOCK_SMOOTH_STONE: return "smooth_stone";
        
        // Sandstone
        case BLOCK_SANDSTONE:
            if (faceIndex == FACE_TOP) return "sandstone_top";
            else if (faceIndex == FACE_BOTTOM) return "sandstone_bottom";
            else return "sandstone";
        case BLOCK_CHISELED_SANDSTONE: return "chiseled_sandstone";
        case BLOCK_CUT_SANDSTONE: return "cut_sandstone";
        case BLOCK_RED_SAND: return "red_sand";
        case BLOCK_RED_SANDSTONE: return "red_sandstone";
        
        // Ores
        case BLOCK_COAL_ORE: return "coal_ore";
        case BLOCK_IRON_ORE: return "iron_ore";
        case BLOCK_GOLD_ORE: return "gold_ore";
        case BLOCK_DIAMOND_ORE: return "diamond_ore";
        case BLOCK_REDSTONE_ORE: return "redstone_ore";
        case BLOCK_EMERALD_ORE: return "emerald_ore";
        case BLOCK_LAPIS_ORE: return "lapis_ore";
        
        // Metal blocks
        case BLOCK_IRON_BLOCK: return "iron_block";
        case BLOCK_GOLD_BLOCK: return "gold_block";
        case BLOCK_DIAMOND_BLOCK: return "diamond_block";
        case BLOCK_EMERALD_BLOCK: return "emerald_block";
        case BLOCK_REDSTONE_BLOCK: return "redstone_block";
        case BLOCK_LAPIS_BLOCK: return "lapis_block";
        case BLOCK_COAL_BLOCK: return "coal_block";
        
        // Wool blocks
        case BLOCK_WHITE_WOOL: return "white_wool";
        case BLOCK_ORANGE_WOOL: return "orange_wool";
        case BLOCK_MAGENTA_WOOL: return "magenta_wool";
        case BLOCK_LIGHT_BLUE_WOOL: return "light_blue_wool";
        case BLOCK_YELLOW_WOOL: return "yellow_wool";
        case BLOCK_LIME_WOOL: return "lime_wool";
        case BLOCK_PINK_WOOL: return "pink_wool";
        case BLOCK_GRAY_WOOL: return "gray_wool";
        case BLOCK_LIGHT_GRAY_WOOL: return "light_gray_wool";
        case BLOCK_CYAN_WOOL: return "cyan_wool";
        case BLOCK_PURPLE_WOOL: return "purple_wool";
        case BLOCK_BLUE_WOOL: return "blue_wool";
        case BLOCK_BROWN_WOOL: return "brown_wool";
        case BLOCK_GREEN_WOOL: return "green_wool";
        case BLOCK_RED_WOOL: return "red_wool";
        case BLOCK_BLACK_WOOL: return "black_wool";
        
        // Glass
        case BLOCK_GLASS: return "glass";
        case BLOCK_WHITE_STAINED_GLASS: return "white_stained_glass";
        case BLOCK_ORANGE_STAINED_GLASS: return "orange_stained_glass";
        case BLOCK_MAGENTA_STAINED_GLASS: return "magenta_stained_glass";
        case BLOCK_LIGHT_BLUE_STAINED_GLASS: return "light_blue_stained_glass";
        case BLOCK_YELLOW_STAINED_GLASS: return "yellow_stained_glass";
        case BLOCK_LIME_STAINED_GLASS: return "lime_stained_glass";
        case BLOCK_PINK_STAINED_GLASS: return "pink_stained_glass";
        case BLOCK_GRAY_STAINED_GLASS: return "gray_stained_glass";
        case BLOCK_LIGHT_GRAY_STAINED_GLASS: return "light_gray_stained_glass";
        case BLOCK_CYAN_STAINED_GLASS: return "cyan_stained_glass";
        case BLOCK_PURPLE_STAINED_GLASS: return "purple_stained_glass";
        case BLOCK_BLUE_STAINED_GLASS: return "blue_stained_glass";
        case BLOCK_BROWN_STAINED_GLASS: return "brown_stained_glass";
        case BLOCK_GREEN_STAINED_GLASS: return "green_stained_glass";
        case BLOCK_RED_STAINED_GLASS: return "red_stained_glass";
        case BLOCK_BLACK_STAINED_GLASS: return "black_stained_glass";
        
        // Special blocks
        case BLOCK_BRICKS: return "bricks";
        case BLOCK_BOOKSHELF: 
            if (faceIndex == FACE_TOP || faceIndex == FACE_BOTTOM) return "oak_planks";
            else return "bookshelf";
        case BLOCK_CRAFTING_TABLE:
            if (faceIndex == FACE_TOP) return "crafting_table_top";
            else if (faceIndex == FACE_BOTTOM) return "oak_planks";
            else return "crafting_table_side";
        case BLOCK_FURNACE: return "furnace_side";
        case BLOCK_CHEST: return "chest";
        case BLOCK_GLOWSTONE: return "glowstone";
        case BLOCK_OBSIDIAN: return "obsidian";
        case BLOCK_NETHERRACK: return "netherrack";
        case BLOCK_SOUL_SAND: return "soul_sand";
        case BLOCK_END_STONE: return "end_stone";
        case BLOCK_PURPUR_BLOCK: return "purpur_block";
        case BLOCK_QUARTZ_BLOCK: return "quartz_block_side";
        case BLOCK_PACKED_ICE: return "packed_ice";
        case BLOCK_BLUE_ICE: return "blue_ice";
        case BLOCK_ICE: return "ice";
        case BLOCK_SNOW_BLOCK: return "snow";
        case BLOCK_CACTUS:
            if (faceIndex == FACE_TOP) return "cactus_top";
            else if (faceIndex == FACE_BOTTOM) return "cactus_bottom";
            else return "cactus_side";
        case BLOCK_PUMPKIN: return "pumpkin_side";
        case BLOCK_JACK_O_LANTERN: 
            if (faceIndex == FACE_FRONT) return "jack_o_lantern";
            else return "pumpkin_side";
        case BLOCK_MELON: return "melon_side";
        case BLOCK_HAY_BLOCK:
            if (faceIndex == FACE_TOP || faceIndex == FACE_BOTTOM) return "hay_block_top";
            else return "hay_block_side";
        
        default: return "stone";
    }
} 
