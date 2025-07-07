/**********************************************************************************************
*
*   Voxel Game - Open World Voxel Game
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
#include <stdio.h>

//----------------------------------------------------------------------------------
// Module Variables Definition (local)
//----------------------------------------------------------------------------------
static int framesCounter = 0;
static int finishScreen = 0;

// Pause menu state
static bool gamePaused = false;
static int pauseMenuSelection = 0;
static int pauseMenuItemCount = 2;

// Voxel game systems
static VoxelWorld world;
static Player player;
static bool gameInitialized = false;

//----------------------------------------------------------------------------------
// Local Functions Declaration
//----------------------------------------------------------------------------------
static void DrawPauseMenu(void);

//----------------------------------------------------------------------------------
// Gameplay Screen Functions Definition
//----------------------------------------------------------------------------------

// Gameplay Screen Initialization logic
void InitGameplayScreen(void)
{
    framesCounter = 0;
    finishScreen = 0;
    
    // Reset pause state
    gamePaused = false;
    pauseMenuSelection = 0;
    
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
    
    // Handle ESC key for pause menu (only when inventory is not open)
    if (IsKeyPressed(KEY_ESCAPE))
    {
        // If inventory is open, close it first
        if (player.inventoryOpen) {
            player.inventoryOpen = false;
            DisableCursor();
        } else {
            // Otherwise toggle pause menu
            gamePaused = !gamePaused;
            pauseMenuSelection = 0; // Reset selection when opening menu
            
            if (gamePaused) {
                EnableCursor(); // Show cursor in pause menu
            } else {
                DisableCursor(); // Hide cursor when resuming game
            }
            PlaySound(fxCoin);
        }
    }
    
    if (gamePaused)
    {
        // Handle pause menu input
        if (IsKeyPressed(KEY_UP) && pauseMenuSelection > 0)
        {
            pauseMenuSelection--;
            PlaySound(fxCoin);
        }
        
        if (IsKeyPressed(KEY_DOWN) && pauseMenuSelection < pauseMenuItemCount - 1)
        {
            pauseMenuSelection++;
            PlaySound(fxCoin);
        }
        
        if (IsKeyPressed(KEY_ENTER))
        {
            switch (pauseMenuSelection)
            {
                case 0: // Resume Game
                    gamePaused = false;
                    DisableCursor();
                    PlaySound(fxCoin);
                    break;
                case 1: // Exit to Menu
                    finishScreen = 1;
                    PlaySound(fxCoin);
                    break;
            }
        }
    }
    else
    {
        // Normal gameplay updates when not paused
        // Update world (chunk loading/unloading)
        UpdateVoxelWorld(&world, player.position);
        
        // Update player (handles input, physics, interaction)
        UpdatePlayer(&player, &world);
        
        // Exit to menu (alternative method - keeping ENTER as backup)
        if (IsKeyPressed(KEY_ENTER) && IsCursorOnScreen() && !player.inventoryOpen)
        {
            finishScreen = 1;
            PlaySound(fxCoin);
        }
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
        if (player.hasTarget && !gamePaused) {
            DrawBlockOutline(player.targetBlock);
        }
    }
    EndMode3D();
    
    // 2D UI rendering
    if (!gamePaused) {
        // Draw inventory UI if inventory is open, otherwise draw normal UI
        if (player.inventoryOpen) {
            DrawInventory(&player);
        } else {
            DrawPlayerUI(&player);
        }
    }
    
    // Debug information (only when not paused)
    if (!gamePaused) {
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

        // Always render block debug info, handle null/air targetBlock
        BlockType targetBlock = GetBlock(&world, player.targetBlock);
        const char* blockName = GetBlockName(targetBlock);
        const char* textureName = GetBlockTextureName(targetBlock, FACE_TOP);

        if (player.hasTarget && targetBlock != BLOCK_AIR) {
            DrawText(TextFormat("Target Block: %s", blockName), 10, 110, 20, YELLOW);
            DrawText(TextFormat("Texture: %s.png", textureName), 10, 130, 20, LIGHTGRAY);
            DrawText(TextFormat("Block Pos: (%d, %d, %d)", 
                     player.targetBlock.x, player.targetBlock.y, player.targetBlock.z), 
                     10, 150, 20, GRAY);
        } else {
            DrawText("Target Block: (none)", 10, 110, 20, DARKGRAY);
            DrawText("Texture: (none)", 10, 130, 20, DARKGRAY);
            DrawText("Block Pos: (-, -, -)", 10, 150, 20, DARKGRAY);
        }
    }
    
    // Controls help (when cursor is visible and game not paused)
    if (!IsCursorHidden() && !gamePaused) {
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
        DrawText("E - Open inventory", 50, 320, 18, WHITE);
        DrawText("ESC - Open pause menu", 50, 340, 18, WHITE);
        DrawText("ENTER - Return to menu", 50, 360, 18, WHITE);
        
        DrawText("Click to start playing!", screenWidth/2 - 120, screenHeight - 50, 20, YELLOW);
    }
    
    // Draw pause menu
    if (gamePaused) {
        DrawPauseMenu();
    }
}

// Draw pause menu
void DrawPauseMenu(void)
{
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    
    // Draw semi-transparent overlay
    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.5f));
    
    // Menu background
    int menuWidth = 400;
    int menuHeight = 500;
    int menuX = screenWidth/2 - menuWidth/2;
    int menuY = screenHeight/2 - menuHeight/2;
    
    DrawRectangle(menuX, menuY, menuWidth, menuHeight, (Color){40, 40, 40, 240});
    DrawRectangleLines(menuX, menuY, menuWidth, menuHeight, WHITE);
    
    // Title
    DrawText("GAME PAUSED", menuX + menuWidth/2 - 90, menuY + 30, 30, WHITE);
    
    // Menu options
    const char* menuItems[] = {
        "Resume Game",
        "Exit to Menu"
    };
    
    int itemStartY = menuY + 100;
    int itemSpacing = 40;
    
    for (int i = 0; i < pauseMenuItemCount; i++) {
        Color textColor = (i == pauseMenuSelection) ? YELLOW : WHITE;
        int textWidth = MeasureText(menuItems[i], 24);
        int textX = menuX + menuWidth/2 - textWidth/2;
        int textY = itemStartY + i * itemSpacing;
        
        // Highlight selected item
        if (i == pauseMenuSelection) {
            DrawRectangle(textX - 10, textY - 5, textWidth + 20, 30, Fade(YELLOW, 0.3f));
        }
        
        DrawText(menuItems[i], textX, textY, 24, textColor);
    }
    
    // Settings preview section
    DrawText("SETTINGS (Preview)", menuX + 20, menuY + 220, 20, GRAY);
    DrawText("Coming Soon:", menuX + 20, menuY + 250, 16, WHITE);
    
    const char* settingsPreview[] = {
        "• Fullscreen Mode",
        "• Render Distance",
        "• Field of View",
        "• Mouse Sensitivity",
        "• Volume Settings",
        "• Graphics Quality",
        "• Vsync",
        "• Chunk Loading Distance",
        "• Show Debug Info"
    };
    
    for (int i = 0; i < 9; i++) {
        DrawText(settingsPreview[i], menuX + 30, menuY + 275 + i * 20, 14, LIGHTGRAY);
    }
    
    // Controls
    DrawText("Use UP/DOWN arrows and ENTER to navigate", menuX + 20, menuY + menuHeight - 40, 16, LIGHTGRAY);
    DrawText("Press ESC to resume game", menuX + 20, menuY + menuHeight - 20, 16, LIGHTGRAY);
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
