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
static Material globalOpaqueMaterial = {0};
static Material globalTransparentMaterial = {0};
static bool materialsInitialized = false;

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
void InitGlobalMaterials(void) {
    if (materialsInitialized) {
        // Clean up existing materials
        UnloadMaterial(globalOpaqueMaterial);
        UnloadMaterial(globalTransparentMaterial);
    }
    
    // Create opaque material
    globalOpaqueMaterial = LoadMaterialDefault();
    if (textureManager.atlas.id > 0) {
        SetMaterialTexture(&globalOpaqueMaterial, MATERIAL_MAP_DIFFUSE, textureManager.atlas);
        globalOpaqueMaterial.maps[MATERIAL_MAP_DIFFUSE].color = (Color){255, 255, 255, 255};
    }
    
    // Create transparent material
    globalTransparentMaterial = LoadMaterialDefault();
    if (textureManager.atlas.id > 0) {
        SetMaterialTexture(&globalTransparentMaterial, MATERIAL_MAP_DIFFUSE, textureManager.atlas);
        globalTransparentMaterial.maps[MATERIAL_MAP_DIFFUSE].color = (Color){255, 255, 255, 255};
    }
    
    materialsInitialized = true;
}

void InitVoxelRenderer(void) {
    InitTextureManager();
    LoadBlockTextures();
    InitGlobalMaterials();
    
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
    // Clean up global materials
    if (materialsInitialized) {
        UnloadMaterial(globalOpaqueMaterial);
        UnloadMaterial(globalTransparentMaterial);
        materialsInitialized = false;
    }
    
    UnloadTextureManager();
}

//----------------------------------------------------------------------------------
// Mesh Generation Functions
//----------------------------------------------------------------------------------
void GenerateChunkMesh(Chunk* chunk, VoxelWorld* world) {
    // Free existing mesh geometry if it exists, but KEEP materials
    if (chunk->hasMesh) {
        // Unload opaque mesh geometry only
        if (chunk->vertexCount > 0) {
            UnloadMesh(chunk->mesh);
        }
        
        // Unload transparent mesh geometry only  
        if (chunk->transparentVertexCount > 0) {
            UnloadMesh(chunk->transparentMesh);
        }
        
        // Reset counts but keep materials
        chunk->vertexCount = 0;
        chunk->triangleCount = 0;
        chunk->transparentVertexCount = 0;
        chunk->transparentTriangleCount = 0;
    }
    
    // Validate texture atlas before proceeding
    if (!ValidateTextureManager()) {
        printf("Error: Cannot generate chunk mesh without valid texture manager\n");
        return;
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
                    
                    if (ShouldRenderFace(world, neighborPos, face, block)) {
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
    
    // Assign global materials to the chunk
    if (opaqueVertexIndex > 0) {
        chunk->material = globalOpaqueMaterial;
    }
    
    if (transparentVertexIndex > 0) {
        chunk->transparentMaterial = globalTransparentMaterial;
    }
    
    // Set hasMesh flag only if we actually created something
    if (opaqueVertexIndex > 0 || transparentVertexIndex > 0) {
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
        Vector3 vertex = faceVertices[faceIndex][i];
        if (IsWaterBlock(block)) vertex.y *= WaterVisualHeight(block);
        vertex = Vector3Add(position, vertex);
        
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

bool ShouldRenderFace(VoxelWorld* world, BlockPos position, int faceIndex, BlockType current) {
    BlockType neighborBlock = GetBlock(world, position);

    if (IsWaterBlock(current)) {
        if (IsWaterBlock(neighborBlock)) {
            if ((faceIndex == FACE_TOP) || (faceIndex == FACE_BOTTOM)) return false;
            return WaterVisualHeight(current) > WaterVisualHeight(neighborBlock) + 0.01f;
        }
        if (IsBlockSolid(neighborBlock)) return false;
        return true;
    }
    
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
    float maxDistance = GameRenderDistance() * CHUNK_SIZE;
    
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

// Every block face resolves to one texture name. Meshing, item icons, and the
// debug readout all read this table, so a block looks the same everywhere.
// Directional blocks have no facing yet; their front is the north (-Z) face.
static const char *const dyeNames[16] = {
    "white", "orange", "magenta", "light_blue", "yellow", "lime", "pink", "gray",
    "light_gray", "cyan", "purple", "blue", "brown", "green", "red", "black"
};

// Flat item sprites the inventory draws. They share the block atlas.
static const char *const itemTextureNames[] = {
    "bucket", "water_bucket", "sign", "redstone", "compass_00", "diamond_pickaxe", "paper", "lava_bucket", "apple"
};

static int faceTextures[BLOCK_COUNT][6] = { 0 };

static const char *DyedTexture(int family, int color)
{
    static const char *const suffixes[4] = { "wool", "concrete", "terracotta", "stained_glass" };
    static char names[4][16][40] = { 0 };

    if (names[family][color][0] == '\0') snprintf(names[family][color], sizeof(names[family][color]), "%s_%s", dyeNames[color], suffixes[family]);
    return names[family][color];
}

static const char *BlockFaceTexture(BlockType block, int faceIndex)
{
    bool vertical = (faceIndex == FACE_TOP) || (faceIndex == FACE_BOTTOM);

    if ((block >= BLOCK_WHITE_WOOL) && (block <= BLOCK_BLACK_WOOL)) return DyedTexture(0, block - BLOCK_WHITE_WOOL);
    if ((block >= BLOCK_WHITE_CONCRETE) && (block <= BLOCK_BLACK_CONCRETE)) return DyedTexture(1, block - BLOCK_WHITE_CONCRETE);
    if ((block >= BLOCK_WHITE_TERRACOTTA) && (block <= BLOCK_BLACK_TERRACOTTA)) return DyedTexture(2, block - BLOCK_WHITE_TERRACOTTA);
    if ((block >= BLOCK_WHITE_STAINED_GLASS) && (block <= BLOCK_BLACK_STAINED_GLASS)) return DyedTexture(3, block - BLOCK_WHITE_STAINED_GLASS);
    if (IsWaterBlock(block)) return (block == BLOCK_WATER)? "water_still" : "water_flow";

    switch (block)
    {
        case BLOCK_GRASS: return (faceIndex == FACE_TOP)? "grass_block_top" : ((faceIndex == FACE_BOTTOM)? "dirt" : "grass_block_side");
        case BLOCK_DIRT: return "dirt";
        case BLOCK_STONE: return "stone";
        case BLOCK_COBBLESTONE: return "cobblestone";
        case BLOCK_BEDROCK: return "bedrock";
        case BLOCK_SAND: return "sand";
        case BLOCK_GRAVEL: return "gravel";
        case BLOCK_BUCKET: return "bucket";
        case BLOCK_WATER_BUCKET: return "water_bucket";
        case BLOCK_OAK_LOG: return vertical? "oak_log_top" : "oak_log";
        case BLOCK_OAK_PLANKS: return "oak_planks";
        case BLOCK_OAK_LEAVES: return "oak_leaves";
        case BLOCK_BIRCH_LOG: return vertical? "birch_log_top" : "birch_log";
        case BLOCK_BIRCH_PLANKS: return "birch_planks";
        case BLOCK_BIRCH_LEAVES: return "birch_leaves";
        case BLOCK_ACACIA_LOG: return vertical? "acacia_log_top" : "acacia_log";
        case BLOCK_ACACIA_PLANKS: return "acacia_planks";
        case BLOCK_ACACIA_LEAVES: return "acacia_leaves";
        case BLOCK_DARK_OAK_LOG: return vertical? "dark_oak_log_top" : "dark_oak_log";
        case BLOCK_DARK_OAK_PLANKS: return "dark_oak_planks";
        case BLOCK_DARK_OAK_LEAVES: return "dark_oak_leaves";
        case BLOCK_STONE_BRICKS: return "stone_bricks";
        case BLOCK_MOSSY_STONE_BRICKS: return "mossy_stone_bricks";
        case BLOCK_CRACKED_STONE_BRICKS: return "cracked_stone_bricks";
        case BLOCK_MOSSY_COBBLESTONE: return "mossy_cobblestone";
        case BLOCK_SMOOTH_STONE: return "stone_slab_top";
        case BLOCK_ANDESITE: return "andesite";
        case BLOCK_GRANITE: return "granite";
        case BLOCK_DIORITE: return "diorite";
        case BLOCK_SANDSTONE: return (faceIndex == FACE_TOP)? "sandstone_top" : ((faceIndex == FACE_BOTTOM)? "sandstone_bottom" : "sandstone");
        case BLOCK_CHISELED_SANDSTONE: return vertical? "sandstone_top" : "chiseled_sandstone";
        case BLOCK_CUT_SANDSTONE: return vertical? "sandstone_top" : "cut_sandstone";
        case BLOCK_RED_SAND: return "red_sand";
        case BLOCK_RED_SANDSTONE: return (faceIndex == FACE_TOP)? "red_sandstone_top" : ((faceIndex == FACE_BOTTOM)? "red_sandstone_bottom" : "red_sandstone");
        case BLOCK_COAL_ORE: return "coal_ore";
        case BLOCK_IRON_ORE: return "iron_ore";
        case BLOCK_GOLD_ORE: return "gold_ore";
        case BLOCK_DIAMOND_ORE: return "diamond_ore";
        case BLOCK_REDSTONE_ORE: return "redstone_ore";
        case BLOCK_EMERALD_ORE: return "emerald_ore";
        case BLOCK_LAPIS_ORE: return "lapis_ore";
        case BLOCK_IRON_BLOCK: return "iron_block";
        case BLOCK_GOLD_BLOCK: return "gold_block";
        case BLOCK_DIAMOND_BLOCK: return "diamond_block";
        case BLOCK_EMERALD_BLOCK: return "emerald_block";
        case BLOCK_REDSTONE_BLOCK: return "redstone_block";
        case BLOCK_LAPIS_BLOCK: return "lapis_block";
        case BLOCK_COAL_BLOCK: return "coal_block";
        case BLOCK_TERRACOTTA: return "terracotta";
        case BLOCK_GLASS: return "glass";
        case BLOCK_BRICKS: return "bricks";
        case BLOCK_BOOKSHELF: return vertical? "oak_planks" : "bookshelf";
        case BLOCK_CRAFTING_TABLE:
        {
            if (faceIndex == FACE_TOP) return "crafting_table_top";
            if (faceIndex == FACE_BOTTOM) return "oak_planks";
            return ((faceIndex == FACE_BACK) || (faceIndex == FACE_LEFT))? "crafting_table_front" : "crafting_table_side";
        }
        case BLOCK_FURNACE: return vertical? "furnace_top" : ((faceIndex == FACE_BACK)? "furnace_front" : "furnace_side");
        case BLOCK_CHEST: return vertical? "chest_top" : ((faceIndex == FACE_BACK)? "chest_front" : "chest_side");
        case BLOCK_GLOWSTONE: return "glowstone";
        case BLOCK_OBSIDIAN: return "obsidian";
        case BLOCK_NETHERRACK: return "netherrack";
        case BLOCK_SOUL_SAND: return "soul_sand";
        case BLOCK_END_STONE: return "end_stone";
        case BLOCK_PURPUR_BLOCK: return "purpur_block";
        case BLOCK_PRISMARINE: return "prismarine";
        case BLOCK_SEA_LANTERN: return "sea_lantern";
        case BLOCK_MAGMA_BLOCK: return "magma";
        case BLOCK_BONE_BLOCK: return vertical? "bone_block_top" : "bone_block_side";
        case BLOCK_QUARTZ_BLOCK: return (faceIndex == FACE_TOP)? "quartz_block_top" : ((faceIndex == FACE_BOTTOM)? "quartz_block_bottom" : "quartz_block_side");
        case BLOCK_CHISELED_QUARTZ_BLOCK: return vertical? "chiseled_quartz_block_top" : "chiseled_quartz_block";
        case BLOCK_QUARTZ_PILLAR: return vertical? "quartz_pillar_top" : "quartz_pillar";
        case BLOCK_PACKED_ICE: return "packed_ice";
        case BLOCK_BLUE_ICE: return "blue_ice";
        case BLOCK_ICE: return "ice";
        case BLOCK_SNOW_BLOCK: return "snow";
        case BLOCK_CLAY: return "clay";
        case BLOCK_HONEYCOMB_BLOCK: return "honeycomb_block";
        case BLOCK_HAY_BLOCK: return vertical? "hay_block_top" : "hay_block_side";
        case BLOCK_MELON: return vertical? "melon_top" : "melon_side";
        case BLOCK_PUMPKIN: return vertical? "pumpkin_top" : "pumpkin_side";
        case BLOCK_JACK_O_LANTERN: return vertical? "pumpkin_top" : ((faceIndex == FACE_BACK)? "jack_o_lantern" : "pumpkin_side");
        case BLOCK_CACTUS: return (faceIndex == FACE_TOP)? "cactus_top" : ((faceIndex == FACE_BOTTOM)? "cactus_bottom" : "cactus_side");
        case BLOCK_SPONGE: return "sponge";
        case BLOCK_WET_SPONGE: return "wet_sponge";
        default: return "stone";
    }
}

static void AddTextureName(const char **names, int *count, const char *name)
{
    for (int i = 0; i < *count; i++)
    {
        if (strcmp(names[i], name) == 0) return;
    }
    if (*count < MAX_BLOCK_TEXTURES) names[(*count)++] = name;
}

static Image LoadTextureFile(const char *pattern, const char *name)
{
    static const char *const roots[] = { "resources/textures", "client/resources/textures" };
    char path[256] = { 0 };
    Image image = { 0 };

    for (int i = 0; i < 2; i++)
    {
        snprintf(path, sizeof(path), pattern, roots[i], name);
        if (!FileExists(path)) continue;
        image = LoadImage(path);
        if (image.data != NULL) break;
    }
    if (image.data == NULL) return image;
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    // Animated strips keep the first frame.
    if ((image.width > 0) && (image.height > image.width*2)) ImageCrop(&image, (Rectangle){ 0, 0, (float)image.width, (float)image.width });
    return image;
}

// Magenta and black quarters, so a missing file stands out instead of
// silently borrowing another block's texture.
static Image MissingTile(void)
{
    Image image = GenImageColor(TEXTURE_SIZE, TEXTURE_SIZE, BLACK);
    Color magenta = { 248, 0, 248, 255 };

    ImageDrawRectangle(&image, 0, 0, TEXTURE_SIZE/2, TEXTURE_SIZE/2, magenta);
    ImageDrawRectangle(&image, TEXTURE_SIZE/2, TEXTURE_SIZE/2, TEXTURE_SIZE/2, TEXTURE_SIZE/2, magenta);
    return image;
}

// The chest is a 14-pixel box with a lid and a latch. As a full cube it uses
// the lid top, and lid-over-base strips for the sides; the front gets the latch.
static Image ChestTile(Image chest, const char *name)
{
    Image tile = { 0 };

    if (strcmp(name, "chest_top") == 0) tile = ImageFromImage(chest, (Rectangle){ 14, 0, 14, 14 });
    else
    {
        bool front = (strcmp(name, "chest_front") == 0);
        float u = front? 14.0f : 0.0f;

        tile = GenImageColor(14, 15, BLANK);
        ImageDraw(&tile, chest, (Rectangle){ u, 14, 14, 5 }, (Rectangle){ 0, 0, 14, 5 }, WHITE);
        ImageDraw(&tile, chest, (Rectangle){ u, 33, 14, 10 }, (Rectangle){ 0, 5, 14, 10 }, WHITE);
        if (front) ImageDraw(&tile, chest, (Rectangle){ 1, 1, 2, 4 }, (Rectangle){ 6, 3, 2, 4 }, WHITE);
    }
    ImageResizeNN(&tile, TEXTURE_SIZE, TEXTURE_SIZE);
    return tile;
}

// Rows of offset wax cells with clipped corners, lit from the top left.
static Image HoneycombTile(void)
{
    Image image = GenImageColor(TEXTURE_SIZE, TEXTURE_SIZE, BLANK);
    Color *pixels = (Color *)image.data;

    for (int y = 0; y < TEXTURE_SIZE; y++)
    {
        for (int x = 0; x < TEXTURE_SIZE; x++)
        {
            int band = y/4;
            int cellY = y%4;
            int cellX = (x + ((band%2 == 1)? 4 : 0))%8;
            bool wall = (cellY == 0) || (cellX == 0) || ((cellY == 1) && ((cellX == 1) || (cellX == 7)));
            Color color = { 229, 148, 29, 255 };

            if (wall) color = (Color){ 168, 96, 18, 255 };
            else if ((cellY == 1) || (cellX == 1)) color = (Color){ 248, 190, 66, 255 };
            else if ((cellY == 3) || (cellX == 7)) color = (Color){ 206, 124, 22, 255 };
            pixels[y*TEXTURE_SIZE + x] = color;
        }
    }
    return image;
}

void LoadBlockTextures(void) {
    const char *names[MAX_BLOCK_TEXTURES] = { 0 };
    int nameCount = 0;
    int texturesPerRow = TEXTURE_ATLAS_SIZE/TEXTURE_SIZE;
    Image atlasImage = GenImageColor(TEXTURE_ATLAS_SIZE, TEXTURE_ATLAS_SIZE, BLANK);
    Image chest = LoadTextureFile("%s/entity/chest/%s.png", "normal");
    int successfulLoads = 0;
    int placeholderCount = 0;

    AddTextureName(names, &nameCount, "missing");
    for (int block = 1; block < BLOCK_COUNT; block++)
    {
        for (int face = 0; face < 6; face++) AddTextureName(names, &nameCount, BlockFaceTexture((BlockType)block, face));
    }
    for (int i = 0; i < (int)(sizeof(itemTextureNames)/sizeof(itemTextureNames[0])); i++) AddTextureName(names, &nameCount, itemTextureNames[i]);

    for (int i = 0; i < nameCount; i++)
    {
        Image tile = { 0 };
        int x = (i%texturesPerRow)*TEXTURE_SIZE;
        int y = (i/texturesPerRow)*TEXTURE_SIZE;

        if (strcmp(names[i], "missing") == 0) tile = MissingTile();
        else if (strncmp(names[i], "chest_", 6) == 0)
        {
            if (chest.data != NULL) tile = ChestTile(chest, names[i]);
        }
        else if (strcmp(names[i], "honeycomb_block") == 0) tile = HoneycombTile();
        else
        {
            tile = LoadTextureFile("%s/block/%s.png", names[i]);
            if (tile.data == NULL) tile = LoadTextureFile("%s/item/%s.png", names[i]);
        }

        if (tile.data == NULL)
        {
            tile = MissingTile();
            placeholderCount++;
        }
        else if (i > 0) successfulLoads++;
        if ((tile.width != TEXTURE_SIZE) || (tile.height != TEXTURE_SIZE)) ImageResize(&tile, TEXTURE_SIZE, TEXTURE_SIZE);

        ImageDraw(&atlasImage, tile, (Rectangle){ 0, 0, TEXTURE_SIZE, TEXTURE_SIZE },
            (Rectangle){ (float)x, (float)y, TEXTURE_SIZE, TEXTURE_SIZE }, WHITE);
        snprintf(textureManager.textureNames[i], sizeof(textureManager.textureNames[i]), "%s", names[i]);
        textureManager.texCoords[i][0] = (float)x/TEXTURE_ATLAS_SIZE;
        textureManager.texCoords[i][1] = (float)y/TEXTURE_ATLAS_SIZE;
        textureManager.texCoords[i][2] = (float)TEXTURE_SIZE/TEXTURE_ATLAS_SIZE;
        textureManager.texCoords[i][3] = (float)TEXTURE_SIZE/TEXTURE_ATLAS_SIZE;
        textureManager.textureCount++;
        UnloadImage(tile);
    }
    if (chest.data != NULL) UnloadImage(chest);

    for (int block = 0; block < BLOCK_COUNT; block++)
    {
        for (int face = 0; face < 6; face++)
        {
            int index = GetTextureIndex(BlockFaceTexture((BlockType)block, face));

            faceTextures[block][face] = (index < 0)? 0 : index;
        }
    }

    textureManager.atlas = LoadTextureFromImage(atlasImage);
    UnloadImage(atlasImage);
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
    return -1;
}

bool GetNamedTextureUV(const char *name, float *u, float *v, float *w, float *h)
{
    int index = ((name != NULL)? GetTextureIndex(name) : -1);

    if (index < 0) return false;
    *u = textureManager.texCoords[index][0];
    *v = textureManager.texCoords[index][1];
    *w = textureManager.texCoords[index][2];
    *h = textureManager.texCoords[index][3];
    return true;
}

Texture2D GetTextureAtlas(void) {
    return textureManager.atlas;
}

bool ValidateTextureManager(void) {
    // Check if texture atlas is valid
    if (textureManager.atlas.id == 0 || textureManager.textureCount == 0) {
        // Unload existing resources
        if (textureManager.atlas.id > 0) {
            UnloadTexture(textureManager.atlas);
        }
        
        // Reinitialize texture manager
        InitTextureManager();
        LoadBlockTextures();
        
        // Check if reload was successful
        if (textureManager.atlas.id > 0 && textureManager.textureCount > 0) {
            // Reinitialize global materials with new texture atlas
            InitGlobalMaterials();
            return true;
        } else {
            printf("Error: Failed to reload texture manager\n");
            return false;
        }
    }
    
    return true; // Already valid
}

void GetBlockTextureUV(BlockType block, int faceIndex, float* u, float* v, float* w, float* h) {
    int index = 0;

    if ((block >= 0) && (block < BLOCK_COUNT) && (faceIndex >= 0) && (faceIndex < 6)) index = faceTextures[block][faceIndex];
    *u = textureManager.texCoords[index][0];
    *v = textureManager.texCoords[index][1];
    *w = textureManager.texCoords[index][2];
    *h = textureManager.texCoords[index][3];
}

bool BlockNeedsAlphaBlending(BlockType block) {
    if (IsWaterBlock(block)) return true;
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
            return true;
        default:
            return false;
    }
}

const char* GetBlockTextureName(BlockType block, int faceIndex) {
    return BlockFaceTexture(block, faceIndex);
}
