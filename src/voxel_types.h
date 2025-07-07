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
// Texture Management
//----------------------------------------------------------------------------------
#define MAX_BLOCK_TEXTURES 512
#define TEXTURE_ATLAS_SIZE 1024
#define TEXTURE_SIZE 16  // Each texture is 16x16 pixels

typedef struct {
    Texture2D atlas;
    float texCoords[MAX_BLOCK_TEXTURES][4]; // UV coordinates: x, y, width, height
    int textureCount;
    char textureNames[MAX_BLOCK_TEXTURES][64];
} TextureManager;

// Face types for texture mapping
typedef enum {
    FACE_TYPE_ALL = 0,      // Same texture for all faces
    FACE_TYPE_TOP,          // Top face
    FACE_TYPE_SIDE,         // Side faces
    FACE_TYPE_BOTTOM,       // Bottom face
    FACE_TYPE_FRONT,        // Front face (for directional blocks)
    FACE_TYPE_BACK,         // Back face
    FACE_TYPE_LEFT,         // Left face
    FACE_TYPE_RIGHT,        // Right face
    FACE_TYPE_COUNT
} FaceTextureType;

//----------------------------------------------------------------------------------
// Block Types
//----------------------------------------------------------------------------------
typedef enum {
    BLOCK_AIR = 0,
    
    // Basic terrain blocks
    BLOCK_GRASS,
    BLOCK_DIRT,
    BLOCK_STONE,
    BLOCK_COBBLESTONE,
    BLOCK_BEDROCK,
    BLOCK_SAND,
    BLOCK_GRAVEL,
    BLOCK_WATER,
    
    // Wood blocks
    BLOCK_OAK_LOG,
    BLOCK_OAK_PLANKS,
    BLOCK_OAK_LEAVES,
    BLOCK_BIRCH_LOG,
    BLOCK_BIRCH_PLANKS,
    BLOCK_BIRCH_LEAVES,
    BLOCK_ACACIA_LOG,
    BLOCK_ACACIA_PLANKS,
    BLOCK_ACACIA_LEAVES,
    BLOCK_DARK_OAK_LOG,
    BLOCK_DARK_OAK_PLANKS,
    BLOCK_DARK_OAK_LEAVES,
    
    // Stone variants
    BLOCK_STONE_BRICKS,
    BLOCK_MOSSY_STONE_BRICKS,
    BLOCK_CRACKED_STONE_BRICKS,
    BLOCK_MOSSY_COBBLESTONE,
    BLOCK_SMOOTH_STONE,
    BLOCK_ANDESITE,
    BLOCK_GRANITE,
    BLOCK_DIORITE,
    
    // Sandstone
    BLOCK_SANDSTONE,
    BLOCK_CHISELED_SANDSTONE,
    BLOCK_CUT_SANDSTONE,
    BLOCK_RED_SAND,
    BLOCK_RED_SANDSTONE,
    
    // Ores
    BLOCK_COAL_ORE,
    BLOCK_IRON_ORE,
    BLOCK_GOLD_ORE,
    BLOCK_DIAMOND_ORE,
    BLOCK_REDSTONE_ORE,
    BLOCK_EMERALD_ORE,
    BLOCK_LAPIS_ORE,
    
    // Metal blocks
    BLOCK_IRON_BLOCK,
    BLOCK_GOLD_BLOCK,
    BLOCK_DIAMOND_BLOCK,
    BLOCK_EMERALD_BLOCK,
    BLOCK_REDSTONE_BLOCK,
    BLOCK_LAPIS_BLOCK,
    BLOCK_COAL_BLOCK,
    
    // Wool blocks
    BLOCK_WHITE_WOOL,
    BLOCK_ORANGE_WOOL,
    BLOCK_MAGENTA_WOOL,
    BLOCK_LIGHT_BLUE_WOOL,
    BLOCK_YELLOW_WOOL,
    BLOCK_LIME_WOOL,
    BLOCK_PINK_WOOL,
    BLOCK_GRAY_WOOL,
    BLOCK_LIGHT_GRAY_WOOL,
    BLOCK_CYAN_WOOL,
    BLOCK_PURPLE_WOOL,
    BLOCK_BLUE_WOOL,
    BLOCK_BROWN_WOOL,
    BLOCK_GREEN_WOOL,
    BLOCK_RED_WOOL,
    BLOCK_BLACK_WOOL,
    
    // Concrete
    BLOCK_WHITE_CONCRETE,
    BLOCK_ORANGE_CONCRETE,
    BLOCK_MAGENTA_CONCRETE,
    BLOCK_LIGHT_BLUE_CONCRETE,
    BLOCK_YELLOW_CONCRETE,
    BLOCK_LIME_CONCRETE,
    BLOCK_PINK_CONCRETE,
    BLOCK_GRAY_CONCRETE,
    BLOCK_LIGHT_GRAY_CONCRETE,
    BLOCK_CYAN_CONCRETE,
    BLOCK_PURPLE_CONCRETE,
    BLOCK_BLUE_CONCRETE,
    BLOCK_BROWN_CONCRETE,
    BLOCK_GREEN_CONCRETE,
    BLOCK_RED_CONCRETE,
    BLOCK_BLACK_CONCRETE,
    
    // Terracotta
    BLOCK_TERRACOTTA,
    BLOCK_WHITE_TERRACOTTA,
    BLOCK_ORANGE_TERRACOTTA,
    BLOCK_MAGENTA_TERRACOTTA,
    BLOCK_LIGHT_BLUE_TERRACOTTA,
    BLOCK_YELLOW_TERRACOTTA,
    BLOCK_LIME_TERRACOTTA,
    BLOCK_PINK_TERRACOTTA,
    BLOCK_GRAY_TERRACOTTA,
    BLOCK_LIGHT_GRAY_TERRACOTTA,
    BLOCK_CYAN_TERRACOTTA,
    BLOCK_PURPLE_TERRACOTTA,
    BLOCK_BLUE_TERRACOTTA,
    BLOCK_BROWN_TERRACOTTA,
    BLOCK_GREEN_TERRACOTTA,
    BLOCK_RED_TERRACOTTA,
    BLOCK_BLACK_TERRACOTTA,
    
    // Glass
    BLOCK_GLASS,
    BLOCK_WHITE_STAINED_GLASS,
    BLOCK_ORANGE_STAINED_GLASS,
    BLOCK_MAGENTA_STAINED_GLASS,
    BLOCK_LIGHT_BLUE_STAINED_GLASS,
    BLOCK_YELLOW_STAINED_GLASS,
    BLOCK_LIME_STAINED_GLASS,
    BLOCK_PINK_STAINED_GLASS,
    BLOCK_GRAY_STAINED_GLASS,
    BLOCK_LIGHT_GRAY_STAINED_GLASS,
    BLOCK_CYAN_STAINED_GLASS,
    BLOCK_PURPLE_STAINED_GLASS,
    BLOCK_BLUE_STAINED_GLASS,
    BLOCK_BROWN_STAINED_GLASS,
    BLOCK_GREEN_STAINED_GLASS,
    BLOCK_RED_STAINED_GLASS,
    BLOCK_BLACK_STAINED_GLASS,
    
    // Special blocks
    BLOCK_BRICKS,
    BLOCK_BOOKSHELF,
    BLOCK_CRAFTING_TABLE,
    BLOCK_FURNACE,
    BLOCK_CHEST,
    BLOCK_GLOWSTONE,
    BLOCK_OBSIDIAN,
    BLOCK_NETHERRACK,
    BLOCK_SOUL_SAND,
    BLOCK_END_STONE,
    BLOCK_PURPUR_BLOCK,
    BLOCK_PRISMARINE,
    BLOCK_SEA_LANTERN,
    BLOCK_MAGMA_BLOCK,
    BLOCK_BONE_BLOCK,
    BLOCK_QUARTZ_BLOCK,
    BLOCK_CHISELED_QUARTZ_BLOCK,
    BLOCK_QUARTZ_PILLAR,
    BLOCK_PACKED_ICE,
    BLOCK_BLUE_ICE,
    BLOCK_ICE,
    BLOCK_SNOW_BLOCK,
    BLOCK_CLAY,
    BLOCK_HONEYCOMB_BLOCK,
    BLOCK_HAY_BLOCK,
    BLOCK_MELON,
    BLOCK_PUMPKIN,
    BLOCK_JACK_O_LANTERN,
    BLOCK_CACTUS,
    BLOCK_SPONGE,
    BLOCK_WET_SPONGE,
    
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
    Material material;
    bool hasMesh;
    int vertexCount;
    int triangleCount;
} Chunk;

//----------------------------------------------------------------------------------
// Inventory System
//----------------------------------------------------------------------------------
#define INVENTORY_SIZE 45  // 9x5 grid
#define HOTBAR_SIZE 9
#define INVENTORY_ROWS 5
#define INVENTORY_COLS 9

typedef struct {
    BlockType blocks[INVENTORY_SIZE];
    int quantities[INVENTORY_SIZE];
} Inventory;

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
    
    // Inventory system
    Inventory inventory;
    bool inventoryOpen;
    int inventorySelectedSlot;
    int inventoryScrollOffset;
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
    return (block == BLOCK_AIR || 
            block == BLOCK_WATER || 
            block == BLOCK_GLASS ||
            block == BLOCK_WHITE_STAINED_GLASS ||
            block == BLOCK_ORANGE_STAINED_GLASS ||
            block == BLOCK_MAGENTA_STAINED_GLASS ||
            block == BLOCK_LIGHT_BLUE_STAINED_GLASS ||
            block == BLOCK_YELLOW_STAINED_GLASS ||
            block == BLOCK_LIME_STAINED_GLASS ||
            block == BLOCK_PINK_STAINED_GLASS ||
            block == BLOCK_GRAY_STAINED_GLASS ||
            block == BLOCK_LIGHT_GRAY_STAINED_GLASS ||
            block == BLOCK_CYAN_STAINED_GLASS ||
            block == BLOCK_PURPLE_STAINED_GLASS ||
            block == BLOCK_BLUE_STAINED_GLASS ||
            block == BLOCK_BROWN_STAINED_GLASS ||
            block == BLOCK_GREEN_STAINED_GLASS ||
            block == BLOCK_RED_STAINED_GLASS ||
            block == BLOCK_BLACK_STAINED_GLASS ||
            block == BLOCK_OAK_LEAVES ||
            block == BLOCK_BIRCH_LEAVES ||
            block == BLOCK_ACACIA_LEAVES ||
            block == BLOCK_DARK_OAK_LEAVES ||
            block == BLOCK_ICE);
}

static inline Color GetBlockColor(BlockType block)
{
    switch (block) {
        // Basic terrain blocks
        case BLOCK_GRASS: return GREEN;
        case BLOCK_DIRT: return BROWN;
        case BLOCK_STONE: return GRAY;
        case BLOCK_COBBLESTONE: return DARKGRAY;
        case BLOCK_BEDROCK: return (Color){64, 64, 64, 255};
        case BLOCK_SAND: return BEIGE;
        case BLOCK_GRAVEL: return (Color){136, 136, 136, 255};
        case BLOCK_WATER: return BLUE;
        
        // Wood blocks
        case BLOCK_OAK_LOG: return (Color){139, 69, 19, 255};
        case BLOCK_OAK_PLANKS: return (Color){162, 130, 78, 255};
        case BLOCK_OAK_LEAVES: return DARKGREEN;
        case BLOCK_BIRCH_LOG: return (Color){220, 220, 220, 255};
        case BLOCK_BIRCH_PLANKS: return (Color){192, 175, 121, 255};
        case BLOCK_BIRCH_LEAVES: return (Color){128, 167, 85, 255};
        case BLOCK_ACACIA_LOG: return (Color){186, 99, 64, 255};
        case BLOCK_ACACIA_PLANKS: return (Color){168, 90, 50, 255};
        case BLOCK_ACACIA_LEAVES: return (Color){99, 128, 15, 255};
        case BLOCK_DARK_OAK_LOG: return (Color){66, 43, 20, 255};
        case BLOCK_DARK_OAK_PLANKS: return (Color){66, 43, 20, 255};
        case BLOCK_DARK_OAK_LEAVES: return (Color){65, 89, 26, 255};
        
        // Stone variants
        case BLOCK_STONE_BRICKS: return (Color){123, 123, 123, 255};
        case BLOCK_MOSSY_STONE_BRICKS: return (Color){115, 121, 105, 255};
        case BLOCK_CRACKED_STONE_BRICKS: return (Color){106, 106, 106, 255};
        case BLOCK_MOSSY_COBBLESTONE: return (Color){122, 126, 122, 255};
        case BLOCK_SMOOTH_STONE: return (Color){158, 158, 158, 255};
        case BLOCK_ANDESITE: return (Color){132, 134, 132, 255};
        case BLOCK_GRANITE: return (Color){149, 103, 85, 255};
        case BLOCK_DIORITE: return (Color){188, 188, 188, 255};
        
        // Sandstone
        case BLOCK_SANDSTONE: return (Color){245, 238, 173, 255};
        case BLOCK_CHISELED_SANDSTONE: return (Color){245, 238, 173, 255};
        case BLOCK_CUT_SANDSTONE: return (Color){245, 238, 173, 255};
        case BLOCK_RED_SAND: return (Color){190, 102, 33, 255};
        case BLOCK_RED_SANDSTONE: return (Color){190, 102, 33, 255};
        
        // Ores
        case BLOCK_COAL_ORE: return (Color){84, 84, 84, 255};
        case BLOCK_IRON_ORE: return (Color){135, 106, 97, 255};
        case BLOCK_GOLD_ORE: return (Color){143, 140, 125, 255};
        case BLOCK_DIAMOND_ORE: return (Color){92, 219, 213, 255};
        case BLOCK_REDSTONE_ORE: return (Color){132, 107, 107, 255};
        case BLOCK_EMERALD_ORE: return (Color){116, 134, 118, 255};
        case BLOCK_LAPIS_ORE: return (Color){102, 112, 134, 255};
        
        // Metal blocks
        case BLOCK_IRON_BLOCK: return (Color){220, 220, 220, 255};
        case BLOCK_GOLD_BLOCK: return GOLD;
        case BLOCK_DIAMOND_BLOCK: return (Color){93, 219, 213, 255};
        case BLOCK_EMERALD_BLOCK: return (Color){80, 218, 109, 255};
        case BLOCK_REDSTONE_BLOCK: return (Color){175, 24, 5, 255};
        case BLOCK_LAPIS_BLOCK: return (Color){31, 64, 182, 255};
        case BLOCK_COAL_BLOCK: return (Color){25, 25, 25, 255};
        
        // Wool blocks
        case BLOCK_WHITE_WOOL: return WHITE;
        case BLOCK_ORANGE_WOOL: return ORANGE;
        case BLOCK_MAGENTA_WOOL: return MAGENTA;
        case BLOCK_LIGHT_BLUE_WOOL: return SKYBLUE;
        case BLOCK_YELLOW_WOOL: return YELLOW;
        case BLOCK_LIME_WOOL: return LIME;
        case BLOCK_PINK_WOOL: return PINK;
        case BLOCK_GRAY_WOOL: return GRAY;
        case BLOCK_LIGHT_GRAY_WOOL: return LIGHTGRAY;
        case BLOCK_CYAN_WOOL: return (Color){21, 137, 145, 255};
        case BLOCK_PURPLE_WOOL: return PURPLE;
        case BLOCK_BLUE_WOOL: return BLUE;
        case BLOCK_BROWN_WOOL: return BROWN;
        case BLOCK_GREEN_WOOL: return GREEN;
        case BLOCK_RED_WOOL: return RED;
        case BLOCK_BLACK_WOOL: return BLACK;
        
        // Concrete
        case BLOCK_WHITE_CONCRETE: return (Color){207, 213, 214, 255};
        case BLOCK_ORANGE_CONCRETE: return (Color){224, 97, 1, 255};
        case BLOCK_MAGENTA_CONCRETE: return (Color){169, 48, 159, 255};
        case BLOCK_LIGHT_BLUE_CONCRETE: return (Color){36, 137, 199, 255};
        case BLOCK_YELLOW_CONCRETE: return (Color){240, 175, 21, 255};
        case BLOCK_LIME_CONCRETE: return (Color){94, 169, 24, 255};
        case BLOCK_PINK_CONCRETE: return (Color){214, 101, 143, 255};
        case BLOCK_GRAY_CONCRETE: return (Color){84, 90, 96, 255};
        case BLOCK_LIGHT_GRAY_CONCRETE: return (Color){125, 125, 115, 255};
        case BLOCK_CYAN_CONCRETE: return (Color){21, 119, 136, 255};
        case BLOCK_PURPLE_CONCRETE: return (Color){100, 32, 156, 255};
        case BLOCK_BLUE_CONCRETE: return (Color){45, 47, 143, 255};
        case BLOCK_BROWN_CONCRETE: return (Color){96, 60, 32, 255};
        case BLOCK_GREEN_CONCRETE: return (Color){73, 91, 36, 255};
        case BLOCK_RED_CONCRETE: return (Color){142, 33, 33, 255};
        case BLOCK_BLACK_CONCRETE: return (Color){8, 10, 15, 255};
        
        // Terracotta
        case BLOCK_TERRACOTTA: return (Color){152, 94, 67, 255};
        case BLOCK_WHITE_TERRACOTTA: return (Color){209, 178, 161, 255};
        case BLOCK_ORANGE_TERRACOTTA: return (Color){161, 83, 37, 255};
        case BLOCK_MAGENTA_TERRACOTTA: return (Color){149, 88, 108, 255};
        case BLOCK_LIGHT_BLUE_TERRACOTTA: return (Color){113, 108, 137, 255};
        case BLOCK_YELLOW_TERRACOTTA: return (Color){186, 133, 36, 255};
        case BLOCK_LIME_TERRACOTTA: return (Color){103, 117, 53, 255};
        case BLOCK_PINK_TERRACOTTA: return (Color){161, 78, 78, 255};
        case BLOCK_GRAY_TERRACOTTA: return (Color){57, 42, 35, 255};
        case BLOCK_LIGHT_GRAY_TERRACOTTA: return (Color){135, 107, 98, 255};
        case BLOCK_CYAN_TERRACOTTA: return (Color){87, 92, 92, 255};
        case BLOCK_PURPLE_TERRACOTTA: return (Color){122, 73, 88, 255};
        case BLOCK_BLUE_TERRACOTTA: return (Color){76, 62, 92, 255};
        case BLOCK_BROWN_TERRACOTTA: return (Color){77, 51, 35, 255};
        case BLOCK_GREEN_TERRACOTTA: return (Color){76, 83, 42, 255};
        case BLOCK_RED_TERRACOTTA: return (Color){143, 61, 46, 255};
        case BLOCK_BLACK_TERRACOTTA: return (Color){37, 22, 16, 255};
        
        // Glass (semi-transparent)
        case BLOCK_GLASS: return (Color){255, 255, 255, 128};
        case BLOCK_WHITE_STAINED_GLASS: return (Color){255, 255, 255, 128};
        case BLOCK_ORANGE_STAINED_GLASS: return (Color){255, 165, 0, 128};
        case BLOCK_MAGENTA_STAINED_GLASS: return (Color){255, 0, 255, 128};
        case BLOCK_LIGHT_BLUE_STAINED_GLASS: return (Color){173, 216, 230, 128};
        case BLOCK_YELLOW_STAINED_GLASS: return (Color){255, 255, 0, 128};
        case BLOCK_LIME_STAINED_GLASS: return (Color){0, 255, 0, 128};
        case BLOCK_PINK_STAINED_GLASS: return (Color){255, 192, 203, 128};
        case BLOCK_GRAY_STAINED_GLASS: return (Color){128, 128, 128, 128};
        case BLOCK_LIGHT_GRAY_STAINED_GLASS: return (Color){211, 211, 211, 128};
        case BLOCK_CYAN_STAINED_GLASS: return (Color){0, 255, 255, 128};
        case BLOCK_PURPLE_STAINED_GLASS: return (Color){128, 0, 128, 128};
        case BLOCK_BLUE_STAINED_GLASS: return (Color){0, 0, 255, 128};
        case BLOCK_BROWN_STAINED_GLASS: return (Color){165, 42, 42, 128};
        case BLOCK_GREEN_STAINED_GLASS: return (Color){0, 128, 0, 128};
        case BLOCK_RED_STAINED_GLASS: return (Color){255, 0, 0, 128};
        case BLOCK_BLACK_STAINED_GLASS: return (Color){0, 0, 0, 128};
        
        // Special blocks
        case BLOCK_BRICKS: return (Color){150, 97, 83, 255};
        case BLOCK_BOOKSHELF: return (Color){139, 69, 19, 255};
        case BLOCK_CRAFTING_TABLE: return (Color){107, 71, 42, 255};
        case BLOCK_FURNACE: return (Color){62, 62, 62, 255};
        case BLOCK_CHEST: return (Color){139, 69, 19, 255};
        case BLOCK_GLOWSTONE: return (Color){255, 207, 139, 255};
        case BLOCK_OBSIDIAN: return (Color){20, 18, 30, 255};
        case BLOCK_NETHERRACK: return (Color){97, 38, 38, 255};
        case BLOCK_SOUL_SAND: return (Color){84, 64, 51, 255};
        case BLOCK_END_STONE: return (Color){221, 223, 165, 255};
        case BLOCK_PURPUR_BLOCK: return (Color){169, 125, 169, 255};
        case BLOCK_PRISMARINE: return (Color){99, 156, 151, 255};
        case BLOCK_SEA_LANTERN: return (Color){172, 199, 190, 255};
        case BLOCK_MAGMA_BLOCK: return (Color){128, 57, 28, 255};
        case BLOCK_BONE_BLOCK: return (Color){229, 225, 207, 255};
        case BLOCK_QUARTZ_BLOCK: return (Color){235, 229, 222, 255};
        case BLOCK_CHISELED_QUARTZ_BLOCK: return (Color){235, 229, 222, 255};
        case BLOCK_QUARTZ_PILLAR: return (Color){235, 229, 222, 255};
        case BLOCK_PACKED_ICE: return (Color){160, 160, 255, 255};
        case BLOCK_BLUE_ICE: return (Color){116, 168, 253, 255};
        case BLOCK_ICE: return (Color){145, 166, 255, 200};
        case BLOCK_SNOW_BLOCK: return (Color){248, 248, 248, 255};
        case BLOCK_CLAY: return (Color){160, 166, 179, 255};
        case BLOCK_HONEYCOMB_BLOCK: return (Color){229, 148, 29, 255};
        case BLOCK_HAY_BLOCK: return (Color){166, 136, 25, 255};
        case BLOCK_MELON: return (Color){113, 169, 59, 255};
        case BLOCK_PUMPKIN: return (Color){192, 118, 21, 255};
        case BLOCK_JACK_O_LANTERN: return (Color){192, 118, 21, 255};
        case BLOCK_CACTUS: return (Color){88, 121, 53, 255};
        case BLOCK_SPONGE: return (Color){193, 193, 57, 255};
        case BLOCK_WET_SPONGE: return (Color){170, 170, 51, 255};
        
        default: return WHITE;
    }
}

#endif // VOXEL_TYPES_H 
