/**********************************************************************************************
*
*   Voxel Game - Minecraft-like Gameplay Screen
*
*   Open world voxel game with first-person movement, block interaction, and infinite terrain
*
*   Features:
*   - First-person camera with WASD movement and mouse look
*   - Voxel world with chunk-based loading and generation
*   - Block placement and destruction with left/right mouse clicks
*   - Infinite terrain generation using noise
*   - Basic block types: grass, dirt, stone, wood, leaves, water
*   - Hotbar inventory system
*   - Collision detection and physics
*
**********************************************************************************************/

#include "raylib.h"
#include "screens.h"
#include "voxel_types.h"
#include "voxel_world.h"
#include "voxel_renderer.h"
#include "world_generation.h"
#include "player.h"

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static int finishScreen = 0;

// Voxel game systems
static VoxelWorld world;
static Player player;
static bool gameInitialized = false;

//----------------------------------------------------------------------------------
// Gameplay Screen Functions Definition
//----------------------------------------------------------------------------------

// Gameplay Screen Initialization logic
void InitGameplayScreen(void)
{
    framesCounter = 0;
    finishScreen = 0;
    
    if (!gameInitialized) {
        // Initialize voxel world
        InitVoxelWorld(&world);
        
        // Initialize player at a good starting position
        Vector3 startPosition = {0, 100, 0}; // Start safely above max terrain height (62+32=94)
        InitPlayer(&player, startPosition);
        
        // Load initial chunks near spawn BEFORE player physics start
        // otherwise player will fall through the world forever
        LoadChunksAroundPlayer(&world, startPosition);
        
        // Initialize renderer
        InitVoxelRenderer();
        
        gameInitialized = true;
    }
}

// Gameplay Screen Update logic
void UpdateGameplayScreen(void)
{
    framesCounter++;
    
    // Update world (chunk loading/unloading)
    UpdateVoxelWorld(&world, player.position);
    
    // Update player (handles input, physics, interaction)
    UpdatePlayer(&player, &world);
    
    // Exit to menu
    if (IsKeyPressed(KEY_ENTER) && IsCursorOnScreen())
    {
        finishScreen = 1;
        PlaySound(fxCoin);
    }
}

// Gameplay Screen Draw logic
void DrawGameplayScreen(void)
{
    // Clear background with sky color
    ClearBackground((Color){135, 206, 235, 255}); // Sky blue
    
    // 3D rendering
    BeginMode3D(player.camera);
    {
        // Render the voxel world
        RenderVoxelWorld(&world, player.camera);
        
        // Draw block outline for targeted block
        if (player.hasTarget) {
            DrawBlockOutline(player.targetBlock);
        }
    }
    EndMode3D();
    
    // 2D UI rendering
    DrawPlayerUI(&player);
    
    // Debug information
    DrawFPS(10, 10);
    
    // Position info
    DrawText(TextFormat("Position: (%.1f, %.1f, %.1f)", 
             player.position.x, player.position.y, player.position.z), 
             10, 30, 20, WHITE);
    
    // Chunk info
    ChunkPos playerChunk = WorldToChunk(player.position);
    DrawText(TextFormat("Chunk: (%d, %d) | Loaded Chunks: %d", 
             playerChunk.x, playerChunk.z, world.chunkCount), 
             10, 50, 20, WHITE);
    
    // Debug: Check if current chunk is loaded
    Chunk* currentChunk = GetChunk(&world, playerChunk);
    Color chunkStatusColor = currentChunk ? GREEN : RED;
    DrawText(TextFormat("Current Chunk: %s", currentChunk ? "LOADED" : "NOT LOADED"), 
             10, 70, 20, chunkStatusColor);
    
    // Debug: Check ground block
    BlockPos groundPos = {(int)player.position.x, (int)(player.position.y - 1), (int)player.position.z};
    BlockType groundBlock = GetBlock(&world, groundPos);
    DrawText(TextFormat("Ground Block: %d (%s)", groundBlock, 
             groundBlock == BLOCK_AIR ? "AIR" : "SOLID"), 
             10, 90, 20, groundBlock == BLOCK_AIR ? RED : GREEN);
    
    // Controls help (when cursor is visible)
    if (!IsCursorHidden()) {
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        
        DrawText("VOXEL WORLD GAME", screenWidth/2 - 150, 100, 30, WHITE);
        DrawText("CONTROLS:", 50, 150, 20, YELLOW);
        DrawText("WASD - Move", 50, 180, 18, WHITE);
        DrawText("Mouse - Look around", 50, 200, 18, WHITE);
        DrawText("SPACE - Jump", 50, 220, 18, WHITE);
        DrawText("LEFT SHIFT - Run", 50, 240, 18, WHITE);
        DrawText("LEFT CLICK - Break block", 50, 260, 18, WHITE);
        DrawText("RIGHT CLICK - Place block", 50, 280, 18, WHITE);
        DrawText("1-9 - Select block type", 50, 300, 18, WHITE);
        DrawText("ESC - Toggle cursor lock", 50, 320, 18, WHITE);
        DrawText("ENTER - Return to menu", 50, 340, 18, WHITE);
        
        DrawText("Click to start playing!", screenWidth/2 - 120, screenHeight - 50, 20, YELLOW);
    }
}

// Gameplay Screen Unload logic
void UnloadGameplayScreen(void)
{
    if (gameInitialized) {
        UnloadVoxelWorld(&world);
        UnloadVoxelRenderer();
        gameInitialized = false;
    }
}

// Gameplay Screen should finish?
int FinishGameplayScreen(void)
{
    return finishScreen;
}
